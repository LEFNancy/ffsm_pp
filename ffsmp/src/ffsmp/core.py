"""Port of ModelCoreSpatial: the yearly dynamic of the spatial model (market, forest resources and management
modules) and the initialisation period.

All the indexes are 0-based, as in the C++ code (forest types, diameter classes, products).
"""

import copy
import math
import struct
import sys

from .utils import (msgOut, i2s, d2s, cmin, cmax, cpow, cdiv, vSum, getMaxPos, getMax, getAvg, getSd, getPos,
                    createCombinationsVector, yint, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR, MSG_CRITICAL_ERROR,
                    MSG_NO_MSG, DATA_NOW, DIAM_ALL, INF)
from .modeldata import (getProdData, getForData, setProdData, setForData, assessProdPossibility, getIntSetting,
                        getDoubleSetting, getStringSetting, getBoolSetting, getStringVectorSetting, getRegionIds,
                        getRegion, getL2r, getForTypeIds, getForType, getForTypeParentId, getForTypeChilds,
                        getForTypeChilds_pos, getForTypeParents, getNForTypesChilds, getSiblings,
                        calculateAnnualisedEquivalent, getAvailableDeathTimber, getAllocableProductIdsFromDeathTimber,
                        getMaxYearUsableDeathTimberAll, deathTimberInventory_incrOrAdd, deathTimberInventory_get)
from .gis import (regPixels, getDoubleValue, changeValue, getMultiplier, getSTData, stKey, stValue, getPathMortality,
                  getPastRegArea, layerExist, updateImage)
from .carbon import (initialiseEmissionCounters, initialiseDeathBiomassStocks, initialiseProductsStocks,
                     registerDeathBiomass, registerHarvesting, registerProducts, registerTransports, HWP_eol2energy)
from .market import solveMarket

FLOATMAX = sys.float_info.max


class CoreState:
    def __init__(self):
        self.firstYear = 0
        self.secondYear = 0
        self.thirdYear = 0
        self.WL2 = 0
        self.regIds2 = []
        self.priProducts = []
        self.secProducts = []
        self.allProducts = []
        self.dClasses = []
        self.pDClasses = []
        self.fTypes = []
        self.l2r = []
        self.regType = ""
        self.natRegAllocation = ""
        self.rescaleFrequencies = False
        self.oldVol2AreaMethod = False
        self.forestAreaChangeMethod = ""
        self.ir = 0.0
        self.cumOverHarvesting = 0.0     # a static variable in ModelCoreSpatial::runMarketModule()
        self.original_vols = []          # static in loadExogenousForestLayers()


# convenient handles
def gpd(m, t, r, p, y=DATA_NOW, fd=""):
    return getProdData(m, t, r, p, y, fd)


def gfd(m, t, r, ft, fd, y=DATA_NOW):
    return getForData(m, t, r, ft, fd, y)


def spd(m, v, t, r, p, y=DATA_NOW, allowCreate=False, fd=""):
    setProdData(m, v, t, r, p, y, allowCreate, fd)


def sfd(m, v, t, r, ft, fd, y=DATA_NOW, allowCreate=False):
    setForData(m, v, t, r, ft, fd, y, allowCreate)


def app(m, pr, ft, dc):
    return assessProdPossibility(m.MD, pr, ft, dc)


def appf(m, pr, ft, dc):
    """app() as a double (as when the C++ bool is used in arithmetic)"""
    return 1.0 if assessProdPossibility(m.MD, pr, ft, dc) else 0.0


def toFloat32(x):
    """double -> float -> double, as a C++ float variable"""
    try:
        return struct.unpack("f", struct.pack("f", x))[0]
    except OverflowError:
        return INF if x > 0 else -INF


def regionPixels(m, regId):
    return regPixels(m, getRegion(m.MD, regId))


def runInitPeriod(m):
    from .output import printOutput
    cacheCoreSettings(m)
    cacheDynamicSettings(m)
    initializePixelVolumes(m)
    assignSpMultiplierPropToVols(m)
    initMarketModule(m)
    initialiseDeathTimber(m)
    printOutput(m, True)
    m.SCD.year += 1
    msgOut(MSG_INFO, f"### 2013 {m.SCD.year}  year started..")
    resetPixelValues(m)
    cachePixelExogenousData(m)
    computeInventary(m)
    computeCumulativeData(m)
    initializePixelArea(m)
    runBiologicalModule(m)
    runManagementModule(m)
    updateMapAreas(m)
    updateOtherMapData(m)
    sumRegionalForData(m)
    initialiseCarbonModule(m)
    printOutput(m, True)   # Output::print() has earlyPrint=true as default argument


def runSimulationYear(m):
    from .output import printOutput
    cacheDynamicSettings(m)
    resetPixelValues(m)
    cachePixelExogenousData(m)
    computeInventary(m)
    runMarketModule(m)
    computeCumulativeData(m)
    cachePixelExogenousData(m)
    runBiologicalModule(m)
    runManagementModule(m)
    updateMapAreas(m)
    updateOtherMapData(m)
    sumRegionalForData(m)
    registerCarbonEvents(m)
    computeEconomicBalances(m)
    printOutput(m, False)


def cacheCoreSettings(m):
    msgOut(MSG_INFO, "Cashing initial model settings..")
    c = m.core
    c.firstYear = getIntSetting(m, "initialYear")
    c.secondYear = c.firstYear + 1
    c.thirdYear = c.firstYear + 2
    c.WL2 = getIntSetting(m, "worldCodeLev2")
    c.regIds2 = getRegionIds(m.MD, 2)
    c.priProducts = getStringVectorSetting(m, "priProducts")
    c.secProducts = getStringVectorSetting(m, "secProducts")
    c.allProducts = c.priProducts + c.secProducts
    c.dClasses = getStringVectorSetting(m, "dClasses")
    c.pDClasses = c.dClasses[1:]
    c.fTypes = getForTypeIds(m.MD)
    c.l2r = getL2r(m.MD)


def cacheDynamicSettings(m):
    msgOut(MSG_INFO, "Cashing model settings that may change every year..")
    c = m.core
    c.regType = getStringSetting(m, "regType", DATA_NOW)
    c.natRegAllocation = getStringSetting(m, "natRegAllocation", DATA_NOW)
    c.rescaleFrequencies = getBoolSetting(m, "rescaleFrequencies", DATA_NOW)
    c.oldVol2AreaMethod = getBoolSetting(m, "oldVol2AreaMethod", DATA_NOW)
    c.forestAreaChangeMethod = getStringSetting(m, "forestAreaChangeMethod", DATA_NOW)
    c.ir = getDoubleSetting(m, "ir", DATA_NOW)


# ------------------------------------------------------------------------------
# Market module
# ------------------------------------------------------------------------------

def initMarketModule(m):
    msgOut(MSG_INFO, "Starting market module (init stage)..")
    c = m.core
    sy, fy, WL2 = c.secondYear, c.firstYear, c.WL2
    for r2 in c.regIds2:
        for sp in c.secProducts:
            value = 0.0
            for pp in c.priProducts:
                value += gpd(m, "pl", r2, pp, sy) * gpd(m, "a", r2, pp, sy, sp)
            value += gpd(m, "m", r2, sp, sy) * gpd(m, "pol_trSub", r2, sp, sy)
            spd(m, value, "pl", r2, sp, sy, True)
        for pp in c.priProducts:
            value = 0.0
            for sp in c.secProducts:
                value += gpd(m, "sl", r2, sp, sy) * gpd(m, "a", r2, pp, sy, sp)
            spd(m, value, "dl", r2, pp, sy, True)
            stvalue = gpd(m, "sl", r2, pp, sy) + gpd(m, "sa", r2, pp, sy)
            spd(m, stvalue, "st", r2, pp, sy, True)
            supply_fromForestShare = gpd(m, "supply_fromForestShare", r2, pp, sy)
            spd(m, stvalue * supply_fromForestShare, "st_fromHarv", r2, pp, sy, True)
        for ap in c.allProducts:
            dtvalue = gpd(m, "dl", r2, ap, sy) + gpd(m, "da", r2, ap, sy)
            spd(m, dtvalue, "dt", r2, ap, sy, True)
        for sp in c.secProducts:
            psi = gpd(m, "psi", r2, sp, sy)
            dl = gpd(m, "dl", r2, sp, sy)
            da = gpd(m, "da", r2, sp, sy)
            pl = gpd(m, "pl", r2, sp, sy)
            sa = gpd(m, "sa", r2, sp, sy)
            sl = gpd(m, "sl", r2, sp, sy)
            k1 = gpd(m, "k1", r2, sp, sy)
            pWo = gpd(m, "pl", WL2, sp, sy)
            stvalue = sl + sa
            q1 = cdiv(1, 1 + cpow(cdiv(dl, da), cdiv(1, psi)) * cdiv(pl, pWo))
            p1 = 1 - q1
            dc = cpow(q1 * cpow(da, cdiv(psi - 1, psi)) + p1 * cpow(dl, cdiv(psi - 1, psi)), cdiv(psi, psi - 1))
            pc = cdiv(da, dc) * pWo + cdiv(dl, dc) * pl
            pw = cdiv(dl * pl + da * pWo, dl + da)
            k = k1 * sl
            spd(m, stvalue, "st", r2, sp, sy, True)
            spd(m, q1, "q1", r2, sp, fy, True)
            spd(m, dc, "dc", r2, sp, sy, True)
            spd(m, pc, "pc", r2, sp, sy, True)
            spd(m, pw, "pw", r2, sp, sy, True)
            spd(m, k, "k", r2, sp, sy, True)
        for pp in c.priProducts:
            sl = gpd(m, "sl", r2, pp, sy)
            sa = gpd(m, "sa", r2, pp, sy)
            eta = gpd(m, "eta", r2, pp, sy)
            pl = gpd(m, "pl", r2, pp, sy)
            pWo = gpd(m, "pl", WL2, pp, sy)
            t1 = cdiv(1, 1 + (cpow(cdiv(sl, sa), cdiv(1, eta))) * cdiv(pl, pWo))
            r1 = 1 - t1
            sc = cpow(t1 * cpow(sa, cdiv(eta - 1, eta)) + r1 * cpow(sl, cdiv(eta - 1, eta)), cdiv(eta, eta - 1))
            pc = cdiv(sa, sc) * pWo + cdiv(sl, sc) * pl
            pw = cdiv(sl * pl + sa * pWo, sl + sa)
            spd(m, t1, "t1", r2, pp, fy, True)
            spd(m, sc, "sc", r2, pp, sy, True)
            spd(m, pc, "pc", r2, pp, sy, True)
            spd(m, pw, "pw", r2, pp, sy, True)
    for regs in c.l2r:
        for rFrom in regs:
            for p in c.allProducts:
                for rTo in regs:
                    spd(m, 0, "rt", rFrom, p, sy, True, i2s(rTo))
                    if rFrom == rTo:
                        spd(m, 1, "ct", rFrom, p, fy, True, i2s(rTo))
            for rTo in regs:
                if rFrom == rTo:
                    spd(m, 0, "dist", rFrom, "", fy, True, i2s(rTo))


def runMarketModule(m):
    from .output import printOptLog
    msgOut(MSG_INFO, "Starting market module")
    c = m.core
    MD = m.MD
    thisYear = m.SCD.year
    previousYear = thisYear - 1
    useDeathTimber = getBoolSetting(m, "useDeathTimber", DATA_NOW)
    intRate = getDoubleSetting(m, "ir")

    # *** pre-optimisation yearly operations..
    for r2 in c.regIds2:
        for sp in c.secProducts:
            g1 = gpd(m, "g1", r2, sp, previousYear)
            sigma = gpd(m, "sigma", r2, sp)
            pc_1 = gpd(m, "pc", r2, sp, previousYear)
            dc_1 = gpd(m, "dc", r2, sp, previousYear)
            k_1 = gpd(m, "k", r2, sp, previousYear)
            pol_d = gpd(m, "pol_mktDirInt_d", r2, sp)
            pol_d_1 = gpd(m, "pol_mktDirInt_d", r2, sp, previousYear)
            q1 = gpd(m, "q1", r2, sp)
            pol_mktStr_d = gpd(m, "pol_mktStr_d", r2, sp)
            q1_polCorr = cmin(1.0, (q1 - 0.5) * pol_mktStr_d + 0.5)
            k = (1 + g1) * k_1
            aa = cdiv(cdiv(sigma, sigma + 1) * pc_1 * cpow(dc_1, cdiv(-1, sigma)) * pol_d_1, pol_d)
            gg = dc_1 * cpow(pc_1 * pol_d_1, -sigma)
            spd(m, k, "k", r2, sp)
            spd(m, aa, "aa", r2, sp, DATA_NOW, True)
            spd(m, gg, "gg", r2, sp, DATA_NOW, True)
            spd(m, q1_polCorr, "q1_polCorr", r2, sp, DATA_NOW, True)
        for pp in c.priProducts:
            gamma_incr = gpd(m, "gamma_incr", r2, pp)
            gamma_decr = gpd(m, "gamma_decr", r2, pp)
            gamma_d_incr = gpd(m, "gamma_d_incr", r2, pp)
            gamma_d_decr = gpd(m, "gamma_d_decr", r2, pp)
            sigma = gpd(m, "sigma", r2, pp)
            pc_1 = gpd(m, "pc", r2, pp, previousYear)
            sc_1 = gpd(m, "sc", r2, pp, previousYear)
            inv = gpd(m, "in", r2, pp)
            in_0 = gpd(m, "in", r2, pp, getIntSetting(m, "initialYear"))
            in_1 = gpd(m, "in", r2, pp, previousYear)
            in_d = gpd(m, "in_deathTimber", r2, pp)
            if thisYear == getIntSetting(m, "initialOptYear"):
                in_d_1 = gpd(m, "in_deathTimber", r2, pp, thisYear)
            else:
                in_d_1 = gpd(m, "in_deathTimber", r2, pp, previousYear)
            min_in_d = cmax(0.00001, in_0 * getDoubleSetting(m, "minShareDeathIn"))
            pol_s = gpd(m, "pol_mktDirInt_s", r2, pp)
            pol_s_1 = gpd(m, "pol_mktDirInt_s", r2, pp, previousYear)
            t1 = gpd(m, "t1", r2, pp)
            pol_mktStr_s = gpd(m, "pol_mktStr_s", r2, pp)
            carbonPrice_1 = gpd(m, "carbonPrice", r2, "", previousYear)
            omega = gpd(m, "co2content_products", r2, pp) / 1000
            Pct_1 = carbonPrice_1 * intRate
            t1_polCorr = cmin(1.0, (t1 - 0.5) * pol_mktStr_s + 0.5)
            gamma = gamma_incr if inv > in_1 else gamma_decr
            gamma_d = gamma_d_incr if in_d > in_d_1 else gamma_d_decr
            in_ratio = cdiv(inv, in_1)
            in_d_ratio = cdiv(cmax(min_in_d, in_d), cmax(min_in_d, in_d_1)) if useDeathTimber else 1.0
            ff = sc_1 * cpow((pc_1 - omega * Pct_1) * pol_s_1, -sigma) * cpow(in_ratio, gamma) * cpow(in_d_ratio, gamma_d) * cpow(pol_s, sigma)
            bb = cpow(ff, cdiv(-1.0, sigma)) * cdiv(sigma, sigma + 1.0)
            spd(m, bb, "bb", r2, pp, DATA_NOW, True)
            spd(m, ff, "ff", r2, pp, DATA_NOW, True)
            spd(m, sigma, "sigma", r2, pp, DATA_NOW, True)
            spd(m, t1_polCorr, "t1_polCorr", r2, pp, DATA_NOW, True)

    # *** optimisation..
    solved, iters, objv = solveMarket(m)
    if solved:
        print("\n*** The problem solved year %d in %d iterations!" % (thisYear, iters))
        print("\n*** The final value of the objective function is %e." % objv)
        msgOut(MSG_INFO, f"The problem solved successfully year {thisYear} in {iters} iterations.")
        printOptLog(m, True, iters, objv)
    else:
        print(f"***ERROR: MODEL DIDN'T SOLVE FOR THIS YEAR ({thisYear})")
        printOptLog(m, False, 0, 0.0)
        msgOut(MSG_CRITICAL_ERROR, f"Model DIDN'T SOLVE for this year ({thisYear})")

    # *** post optimisation..
    priPrCombs = createCombinationsVector(len(c.priProducts))
    WL2 = c.WL2
    for regId in c.regIds2:
        REG = getRegion(MD, regId)
        for p in c.allProducts:
            st = gpd(m, "sl", regId, p) + gpd(m, "sa", regId, p)
            dt = gpd(m, "dl", regId, p) + gpd(m, "da", regId, p)
            spd(m, st, "st", regId, p)
            spd(m, st, "st_or", regId, p, DATA_NOW, True)
            spd(m, dt, "dt", regId, p)
        for p in c.secProducts:
            dl = gpd(m, "dl", regId, p)
            pl = gpd(m, "pl", regId, p)
            da = gpd(m, "da", regId, p)
            pworld = gpd(m, "pl", WL2, p)
            dt = gpd(m, "dt", regId, p)
            pw = (dl * pl + da * pworld) / dt if dt != 0 else 0.0
            spd(m, pw, "pw", regId, p)
        for p in c.priProducts:
            sl = gpd(m, "sl", regId, p)
            pl = gpd(m, "pl", regId, p)
            sa = gpd(m, "sa", regId, p)
            pworld = gpd(m, "pl", WL2, p)
            st = gpd(m, "st", regId, p)
            st_fromFor = st * gpd(m, "supply_fromForestShare", regId, p)
            pw = (sl * pl + sa * pworld) / st if st != 0 else 0.0
            spd(m, pw, "pw", regId, p)
            spd(m, st_fromFor, "st_fromFor", regId, p, DATA_NOW, True)
        # correcting st if this is over the in
        for i, comb in enumerate(priPrCombs):
            stMkMod = 0.0
            sumIn = REG.inResByAnyCombination[i]
            for p in comb:
                stMkMod += gpd(m, "st_fromFor", regId, c.priProducts[p])
            if stMkMod > sumIn:
                pInvolved = "".join(c.priProducts[p] + "; " for p in comb)
                inV_over_hV_ratio = sumIn / stMkMod if stMkMod != 0 else 0.0
                c.cumOverHarvesting += (stMkMod - sumIn)
                msgOut(MSG_DEBUG, f"Overharvesting has happened. Year: {thisYear}Region: {regId}Involved products:  {pInvolved}. sumIn: {d2s(sumIn)} stMkMod:{d2s(stMkMod)} cumOverHarvesting: {d2s(c.cumOverHarvesting)}")
                for p in comb:
                    orig = gpd(m, "st_fromFor", regId, c.priProducts[p])
                    spd(m, orig * inV_over_hV_ratio, "st_fromFor", regId, c.priProducts[p])
        total_st = [gpd(m, "st_fromFor", regId, pp) for pp in c.priProducts]
        for pp in c.priProducts:
            gpd(m, "in_deathTimber", regId, pp)
            gpd(m, "in", regId, pp)
        st_fromHarv = allocateHarvesting(m, total_st, regId)
        for i, pp in enumerate(c.priProducts):
            spd(m, st_fromHarv[i], "st_fromHarv", regId, pp, DATA_NOW, True)
    if c.cumOverHarvesting > 0.0:
        msgOut(MSG_DEBUG, f"Overharvesting is present. Year: {thisYear} cumOverHarvesting: {d2s(c.cumOverHarvesting)}")


def allocateHarvesting(m, total_st, regId):
    """Allocate the total supply first to the usable death timber (newer to older years, higher to smaller dc)
    and return the remaining part, that comes from harvesting"""
    c = m.core
    MD = m.MD
    if not getBoolSetting(m, "useDeathTimber", DATA_NOW):
        return total_st
    total_st = list(total_st)
    maxYears = getMaxYearUsableDeathTimberAll(MD)
    currentYear = m.SCD.year
    for y in range(currentYear - 1, currentYear - maxYears - 1, -1):
        for u in range(len(c.dClasses) - 1, -1, -1):
            dc = c.dClasses[u]
            for ft in c.fTypes:
                allocable = getAllocableProductIdsFromDeathTimber(m, regId, ft, dc, y, currentYear - 1)
                key = (y, regId, ft, dc)
                deathTimber = deathTimberInventory_get(MD, key)
                s = 0.0
                for ap in allocable:
                    s += total_st[ap]
                for ap in allocable:
                    share = total_st[ap] / s if s != 0 else 0.0
                    allocated = cmin(total_st[ap], deathTimber * share)
                    deathTimberInventory_incrOrAdd(MD, key, -allocated)
                    total_st[ap] -= allocated
    return total_st


# ------------------------------------------------------------------------------
# Biological (forest resources) module
# ------------------------------------------------------------------------------

def getAvgAgeByDc(m, px, ft, dc):
    if dc == len(m.core.dClasses) - 1:
        return px.cumTp[ft][dc] * 1.2
    return (px.cumTp[ft][dc] + px.cumTp[ft][dc + 1]) / 2


def runBiologicalModule(m):
    msgOut(MSG_INFO, "Starting resource module..")
    c = m.core
    MD = m.MD
    G = m.GIS
    thisYear = m.SCD.year
    useDeathTimber = getBoolSetting(m, "useDeathTimber", DATA_NOW)
    nDc = len(c.dClasses)
    nPri = len(c.priProducts)
    for r2 in c.regIds2:
        regId = r2
        REG = getRegion(MD, r2)
        regPx = regPixels(m, REG)
        shareMortalityUsableTimber = 0.0
        # cache of the regional values used in the pixel loop (pure look-ups)
        st_in = [(gpd(m, "st_fromHarv", r2, pp), gpd(m, "in", r2, pp)) for pp in c.priProducts]
        regForAreas = {}
        appm = [[[appf(m, pp, ft, dc) for dc in c.dClasses] for pp in c.priProducts] for ft in c.fTypes]
        fhf = [[gfd(m, "finalHarvestFlag", regId, ft, dc) for dc in c.dClasses] for ft in c.fTypes]
        smut = [gfd(m, "shareMortalityUsableTimber", r2, ft, "") if useDeathTimber else 0.0 for ft in c.fTypes]
        for px in regPx:
            for j, ft in enumerate(c.fTypes):
                hV_byDiam = []
                productivity_byDc = []
                hV_byDiamAndPrd = []
                hArea_byDc = []
                newVol_byDiam = []
                vMort_byDc = []
                vMortAdd_byDc = []
                areasMovingUp = [0.0] * nDc
                areaFirstProdClass = 0.0
                px_tp = px.tp[j]
                px_vol_l = px.vol_l[j]
                px_area_l = px.area_l[j]
                px_area = px.area[j]
                avalCoef = px.avalCoef[j]

                # A - regeneration
                tp_u0 = yint(px_tp[0])
                if thisYear == c.secondYear:
                    px.initialDc0Area.append(px_area_l[0])
                if c.regType != "fixed" and (thisYear - c.secondYear) >= tp_u0:
                    pastRegArea = getPastRegArea(px, j, thisYear - tp_u0)
                    availableArea = px_area_l[0]
                    vHa = px.vHa[j][1]
                    if c.oldVol2AreaMethod:
                        areaFirstProdClass = pastRegArea
                    else:
                        areaFirstProdClass = cmin(availableArea, pastRegArea)
                    px.vReg.append(areaFirstProdClass * vHa / 1000000.0)
                else:
                    regAreaFt = regForAreas.get(ft)
                    if regAreaFt is None:
                        regAreaFt = regForAreas[ft] = regionLayerSum(m, REG, "forArea_" + ft)
                    pxArea = getDoubleValue(G, px, "forArea_" + ft, True)
                    regRegVolumes = gfd(m, "vReg", r2, ft, "")
                    newVReg = regRegVolumes * pxArea / regAreaFt if regAreaFt != 0 else 0.0
                    px.vReg.append(newVReg)
                    areaFirstProdClass = cdiv(1.0, float(tp_u0)) * px.initialDc0Area[j]

                shareMortalityUsableTimber = smut[j]

                for u in range(nDc):
                    dc = c.dClasses[u]
                    hr = 0.0
                    age = getAvgAgeByDc(m, px, j, u)
                    pastYearVol = px_vol_l[u]
                    hV_byPrd = []
                    hr_byPrd = []
                    for pi in range(nPri):
                        st, inv = st_in[pi]
                        hr_pr = appm[j][pi][u] * st / inv if inv != 0 else 0.0
                        hr_byPrd.append(hr_pr)
                        hr += hr_pr
                    origHr = hr
                    hr = cmin(1.0, hr)
                    for pi in range(nPri):
                        hr_pr = hr_byPrd[pi] * cmin(1.0, 1.0 / origHr) if origHr != 0 else 0.0
                        hV_byPrd.append(hr_pr * pastYearVol * avalCoef)
                    hV = hr * pastYearVol * avalCoef
                    hV_byDiam.append(hV)
                    hV_byDiamAndPrd.append(hV_byPrd)

                    tp = px_tp[u]
                    mort = px.mort[j][u]
                    addMort = px.addMort[j][u]
                    vReg = px.vReg[j]
                    beta = px.beta[j][u]
                    vHa = px.vHa[j][u]
                    finalHarvestFlag = fhf[j][u]
                    vMort = mort * pastYearVol
                    vMortAdd = addMort * pastYearVol
                    vMort_byDc.append(vMort)
                    vMortAdd_byDc.append(vMortAdd)
                    if useDeathTimber:
                        deathTimberInventory_incrOrAdd(MD, (thisYear, r2, ft, dc), (vMort + vMortAdd) * shareMortalityUsableTimber)
                    if u == 0:
                        vol = 0.0
                    elif u == 1:
                        vol = cmax(0.0, (1 - cdiv(1, tp) - mort - addMort)) * pastYearVol + vReg
                    else:
                        inc = 0.0 if u == nDc - 1 else cdiv(1.0, tp)
                        tp_1 = px_tp[u - 1]
                        pastYearVol_1 = px_vol_l[u - 1]
                        vol = cmax(0.0, (1 - inc - mort - addMort) * pastYearVol - hV + cdiv(1, tp_1) * beta * pastYearVol_1)
                    if u != 0:
                        inc = 0.0 if u == nDc - 1 else cdiv(1.0, tp)
                        pastArea = px_area_l[u]
                        areasMovingUp[u] = inc * pastArea
                        if c.oldVol2AreaMethod:
                            hArea_byDc.append(cdiv(finalHarvestFlag * 1000000 * hV, vHa))
                        else:
                            finalHarvestedVolumes = finalHarvestFlag * hV
                            finalHarvestedRate = finalHarvestedVolumes / pastYearVol if pastYearVol != 0 else 0.0
                            hArea_byDc.append(finalHarvestedRate * pastArea)
                        px_area[u] = cmax(0.0, px_area_l[u] - areasMovingUp[u] + areasMovingUp[u - 1] - hArea_byDc[u])
                    else:
                        areasMovingUp[u] = areaFirstProdClass
                        hArea_byDc.append(0.0)
                        px_area[u] = px_area_l[u] - areasMovingUp[u] - hArea_byDc[u]
                    newVol_byDiam.append(vol)
                    productivity = cdiv(1000000.0 * hV_byDiam[u], hArea_byDc[u] * age) if hArea_byDc[u] != 0 else 0.0
                    productivity_byDc.append(productivity)
                px.hVol.append(hV_byDiam)
                px.hVol_byPrd.append(hV_byDiamAndPrd)
                px.hArea.append(hArea_byDc)
                px.vol.append(newVol_byDiam)
                px.vMort.append(vMort_byDc)
                px.vMortAdd.append(vMortAdd_byDc)
                px.hProductivity.append(productivity_byDc)


def regionLayerSum(m, REG, layer):
    """ModelRegion::getValue(layer, OP_SUM)"""
    G = m.GIS
    s = 0.0
    pv = G.pxVector
    for i in REG.myPixels:
        s += getDoubleValue(G, pv[i], layer, True)
    return s


def regionArea(m, REG):
    """ModelRegion::getArea(): whole forest area (from the pixel->area matrix)"""
    s = 0.0
    pv = m.GIS.pxVector
    for i in REG.myPixels:
        s += vSum(pv[i].area)
    return s


# ------------------------------------------------------------------------------
# Management module
# ------------------------------------------------------------------------------

def _expectedPriceParts(curLocPrice, worldCurPrice, worldFutPrice, sl, sa):
    """The two terms of the expected price (ModelCoreSpatial::computeExpectedPrice), whose weighted sum by the
    expectation coefficient gives the expected price"""
    fullExpWPrice = cdiv(curLocPrice * cdiv(worldFutPrice, worldCurPrice) * sl + worldFutPrice * sa, sa + sl)
    curWPrice = cdiv(curLocPrice * sl + worldCurPrice * sa, sl + sa)
    return curWPrice, fullExpWPrice


def _computeExpectedPrice(curLocPrice, worldCurPrice, worldFutPrice, sl, sa, expCoef):
    curWPrice, fullExpWPrice = _expectedPriceParts(curLocPrice, worldCurPrice, worldFutPrice, sl, sa)
    return curWPrice * (1 - expCoef) + fullExpWPrice * expCoef


def getVHaByYear(m, px, ft, year, extraBiomass_ratio, regId):
    """Volume/ha of a forest type `year` years after planting (linear interpolation between the expected dc
    volumes); ft is 0-based"""
    c = m.core
    nDc = len(c.dClasses)
    cumTp = px.cumTp_exp[ft]
    vHa = px.vHa_exp[ft]
    for u in range(nDc):
        cumTP = cumTp[u]
        if not (year >= cumTP):
            if u == 0:
                raise IndexError("getVHaByYear: year before the first diameter class")
            cumTP_plus = cumTp[u]
            cumTP_minus = cumTp[u - 1]
            volHa_plus = vHa[u]
            volHa_minus = vHa[u - 1]
            slope = cdiv(volHa_plus - volHa_minus, cumTP_plus - cumTP_minus)
            return (volHa_minus + (year - cumTP_minus) * slope) * (1 + extraBiomass_ratio)
    mortCoeff = gfd(m, "mort", regId, c.fTypes[ft], c.dClasses[nDc - 1], m.SCD.year)
    cumTP_max = cumTp[nDc - 1]
    time_elapsed = year - cumTP_max
    volHa_max = cumTp[nDc - 1]  # as in the C++ code (it uses cumTp_exp, not vHa_exp)
    return volHa_max * cpow(1 - mortCoeff, time_elapsed) * (1 + extraBiomass_ratio)


def runManagementModule(m):
    msgOut(MSG_INFO, "Starting management module..")
    c = m.core
    MD = m.MD
    G = m.GIS
    fTypes = c.fTypes
    nft = len(fTypes)
    dClasses = c.dClasses
    nDc = len(dClasses)
    priProducts = c.priProducts
    nPri = len(priProducts)
    allFTypes = getForTypeIds(MD, True)
    hAreaByFTypeGroup = {ft: 0.0 for ft in allFTypes}
    thisYear = m.SCD.year
    ir = getDoubleSetting(m, "ir")
    WL2 = c.WL2
    heteroRA = getBoolSetting(m, "heterogeneousRiskAversion", DATA_NOW)
    mpow = math.pow
    parentOf = [getForTypeParentId(MD, ft) for ft in fTypes]
    annDen = {}      # years -> pow(1+ir, years) - 1 (as in calculateAnnualisedEquivalent)

    for r2 in c.regIds2:
        regId = r2
        REG = getRegion(MD, r2)
        regPx = regPixels(m, REG)
        flagHartman = getStringSetting(m, "flagHartman", DATA_NOW, r2)
        pickling = getDoubleSetting(m, "pickling")
        carbonPrice_NOW = gpd(m, "carbonPrice", r2, "", thisYear + 1)
        futureCarbonPrices = []
        if flagHartman != "NONE":
            for y in range(1000):
                futureCarbonPrices.append(gpd(m, "carbonPrice", r2, "", thisYear + y))
        polBal_fiSub = [0.0] * nft
        fArea_reg = regionArea(m, REG)
        fArea_diff = 0.0
        fArea_reldiff = 0.0
        if c.forestAreaChangeMethod == "relative":
            fArea_reldiff = gfd(m, "forestChangeAreaIncrementsRel", r2, "", "", DATA_NOW)
            fArea_diff = fArea_reg * fArea_reldiff
        elif c.forestAreaChangeMethod == "absolute":
            fArea_diff = gfd(m, "forestChangeAreaIncrementsHa", r2, "", "", DATA_NOW)
            fArea_reldiff = cdiv(fArea_diff, fArea_reg)
        regHArea = 0.0

        # management rate: from forData if there, otherwise from the settings
        origErrorLevel = MD.errorLevel
        MD.tempBool = True
        MD.errorLevel = MSG_NO_MSG
        mr = getForData(m, "mr", regId, "", "")
        if not MD.tempBool:
            MD.errorLevel = MSG_CRITICAL_ERROR
            mr = getDoubleSetting(m, "mr", DATA_NOW, regId)
        MD.errorLevel = origErrorLevel

        futureCarbonRef = []
        futureExtraBiomass_ratio = []
        if flagHartman != "NONE":
            for ft in fTypes:
                cr = []
                eb = []
                for y in range(1000):
                    cr.append(gfd(m, "carbonRef", r2, ft, "", thisYear + y))
                    eb.append(gfd(m, "extraBiomass_ratio", regId, ft, "", DATA_NOW))
                futureCarbonRef.append(cr)
                futureExtraBiomass_ratio.append(eb)

        # regional values used in the pixel loop (pure look-ups, cached for performance)
        pinfo = [(gpd(m, "pol_mktDirInt_s", regId, pp), gpd(m, "pl", regId, pp), gpd(m, "pl", WL2, pp),
                  gpd(m, "sl", regId, pp), gpd(m, "sa", regId, pp)) for pp in priProducts]
        fhf = [[gfd(m, "finalHarvestFlag", regId, ft, dc) for dc in dClasses] for ft in fTypes]
        co2inv = [gfd(m, "co2content_inventory", r2, ft, "", DATA_NOW) / 1000 for ft in fTypes]
        appm = [[[appf(m, pp, ft, dc) for dc in dClasses] for pp in priProducts] for ft in fTypes]
        priceParts = {}   # (pp index, future year) -> (curWPrice, fullExpWPrice)
        pricesByYear = {}  # future year -> [(curWPrice, fullExpWPrice) by pp]

        def expPriceParts(pi, fy):
            k = (pi, fy)
            v = priceParts.get(k)
            if v is None:
                pp = priProducts[pi]
                pol_s_fut = gpd(m, "pol_mktDirInt_s", regId, pp, fy)
                worldFut = gpd(m, "pl", WL2, pp, fy)
                pol_s, plv, plw, sl, sa = pinfo[pi]
                pl = plv * pol_s
                worldCurPrice = plw * pol_s * pol_s
                worldFutPrice = worldFut * pol_s_fut
                v = priceParts[k] = _expectedPriceParts(pl, worldCurPrice, worldFutPrice, sl, sa)
            return v
        endoCache = {}

        def endoPrices(pp, fy):
            k = (pp, fy)
            v = endoCache.get(k)
            if v is None:
                v = endoCache[k] = (gpd(m, "pl", regId, pp), gpd(m, "pl", WL2, pp), gpd(m, "pl", WL2, pp, fy),
                                    gpd(m, "sl", regId, pp), gpd(m, "sa", regId, pp))
            return v
        transCost = [[(gfd(m, "invTransCost", regId, ft, ft2, DATA_NOW), gfd(m, "pol_fiSub", regId, ft, ft2, DATA_NOW)) for ft2 in fTypes] for ft in fTypes]

        for px in regPx:
            px.expectedReturns = []
            px.expectedReturnsNotCorrByRa = []
            px.optDc = []
            px.optFtChosen = []
            px.optDcChosen = []
            px.expectedAnnualIncome_carbon = []
            px.expectedAnnualIncome_timber = []
            for k in hAreaByFTypeGroup:
                hAreaByFTypeGroup[k] = 0.0
            totalHarvestedAreaOrig = vSum(px.hArea)
            thisYearRegAreas = [0.0] * nft
            expectedReturns = [0.0] * nft
            rotation_dc = [0] * nft
            deltaAreas = [[0.0] * (nft + 1) for _ in range(nft + 1)]

            fArea_px = vSum(px.area)
            fArea_diff_px = cdiv(fArea_px * fArea_diff, fArea_reg)
            fArea_incr = cmax(0.0, fArea_diff_px)
            fArea_incr_rel = fArea_incr / totalHarvestedAreaOrig if totalHarvestedAreaOrig != 0 else 0.0
            fArea_decr = -cmin(0.0, fArea_diff_px)
            fArea_decr_rel = cmin(1.0, fArea_decr / totalHarvestedAreaOrig) if totalHarvestedAreaOrig != 0 else 0.0
            regHArea += totalHarvestedAreaOrig
            totalHarvestedArea = totalHarvestedAreaOrig * (1 - fArea_decr_rel)

            # A - harvested area by parent ft group
            for j in range(nft):
                parentFt = parentOf[j]
                hAreaThisFt = vSum(px.hArea[j]) * (1 - fArea_decr_rel)
                if parentFt not in hAreaByFTypeGroup:
                    msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                hAreaByFTypeGroup[parentFt] += hAreaThisFt

            # expected volume by year after planting (getVHaByYear), by forest type, computed once per pixel
            vhyCache = {}

            def vHaByYear(j, y):
                lst = vhyCache.get(j)
                if lst is None:
                    lst = vhyCache[j] = []
                while len(lst) <= y:
                    yy = len(lst)
                    lst.append(getVHaByYear(m, px, j, yy, futureExtraBiomass_ratio[j][yy], regId))
                return lst[y]

            def carbonPayments(cache, j, n, ref, co2, carbonPrice_y):
                """The yearly carbon payments carbonPrice*ir*(co2*vHa(y) - ref[y]) for y < n (cached by forest type)"""
                lst = cache.get(j)
                if lst is None:
                    lst = cache[j] = []
                while len(lst) < n:
                    y = len(lst)
                    lst.append(carbonPrice_y * ir * (co2 * vHaByYear(j, y) - ref[y]))
                return lst

            def discountedSum(cp, cumTp_u, inclusive):
                """sum over y < cumTp_u (or <= with inclusive) of cp[y] * pow(1+ir, cumTp_u - y), in order"""
                b = 1 + ir
                s = 0.0
                y = 0
                try:
                    if inclusive:
                        while y <= cumTp_u:
                            s += cp[y] * mpow(b, cumTp_u - y)
                            y += 1
                    else:
                        while y < cumTp_u:
                            s += cp[y] * mpow(b, cumTp_u - y)
                            y += 1
                except (ValueError, OverflowError):      # C pow semantics
                    s = 0.0
                    y = 0
                    while (y <= cumTp_u) if inclusive else (y < cumTp_u):
                        s += cp[y] * cpow(b, cumTp_u - y)
                        y += 1
                return s
            cpExo = {}
            cpEndo = {}

            # B - uncorrected expected returns
            expTypePrices = px.expTypePrices
            for j, ft in enumerate(fTypes):
                co2content_inventory = co2inv[j]
                expReturns = -FLOATMAX
                expReturns_carbon = -FLOATMAX
                expReturns_timber = -FLOATMAX
                optDc = 0
                vHa_exp = px.vHa_exp[j]
                cumTp_exp = px.cumTp_exp[j]
                appm_j = appm[j]
                fhf_j = fhf[j]
                for u in range(nDc):
                    vHa = vHa_exp[u]
                    finalHarvestFlag = fhf_j[u]
                    cumTp_u = cumTp_exp[u]
                    fy = yint(thisYear + cumTp_u)
                    parts = pricesByYear.get(fy)
                    if parts is None:
                        parts = pricesByYear[fy] = [expPriceParts(pi, fy) for pi in range(nPri)]
                    # calculateAnnualisedEquivalent(amount, cumTp_u, ir), with the denominator computed once
                    yC = int(math.ceil(cumTp_u)) if math.isfinite(cumTp_u) else -1
                    if yC > 0:
                        den = annDen.get(yC)
                        if den is None:
                            den = annDen[yC] = cpow(1.0 + ir, yC) - 1.0
                    if flagHartman == "EXO":
                        # sum of the yearly carbon payments (the same for every product)
                        carbonPrice_y = carbonPrice_NOW
                        nY = int(math.ceil(cumTp_u)) if cumTp_u > 0 else 0
                        cp = carbonPayments(cpExo, j, nY, futureCarbonRef[j], co2content_inventory, carbonPrice_y)
                        carbonSum = discountedSum(cp, cumTp_u, False)
                    for pi in range(nPri):
                        curWPrice, fullExpWPrice = parts[pi]
                        pw_exp = curWPrice * (1 - expTypePrices) + fullExpWPrice * expTypePrices
                        raw_amount = finalHarvestFlag * pw_exp * vHa * appm_j[pi][u]
                        if yC > 0:
                            anualised_amount_timber = cdiv(raw_amount * ir, den)
                        else:
                            anualised_amount_timber = raw_amount if yC == 0 else 0.0
                        anualised_amount = anualised_amount_timber
                        raw_amount_carbon = 0.0
                        if flagHartman == "EXO":
                            raw_amount_carbon = carbonSum
                            raw_amount_carbon += pickling * carbonPrice_y * finalHarvestFlag * vHa * appm_j[pi][u]
                        if yC > 0:
                            anualised_amount_carbon = cdiv(raw_amount_carbon * ir, den)
                        else:
                            anualised_amount_carbon = raw_amount_carbon if yC == 0 else 0.0
                        anualised_amount += anualised_amount_carbon
                        if anualised_amount > expReturns:
                            expReturns = anualised_amount
                            optDc = u
                            expReturns_carbon = anualised_amount_carbon
                            expReturns_timber = anualised_amount_timber
                if flagHartman in ("NONE", "EXO"):
                    px.expectedAnnualIncome_timber.append(expReturns_timber)
                    px.expectedAnnualIncome_carbon.append(expReturns_carbon)
                    px.expectedReturnsNotCorrByRa.append(expReturns)
                    px.optDc.append(optDc)
                if heteroRA:
                    ra = getDoubleValue(G, px, "ra")
                    cumMort = 1 - px.cumAlive_exp[j][optDc]
                    origExpReturns = expReturns
                    expReturns = origExpReturns * (1.0 - ra * cumMort)
                if flagHartman in ("NONE", "EXO"):
                    px.expectedReturns.append(expReturns)
                expectedReturns[j] = expReturns
                rotation_dc[j] = optDc

            # Hartman with endogenous (BAU) reference
            if flagHartman == "ENDO":
                ft_opt_index = getMaxPos(expectedReturns)
                ft_opt = fTypes[ft_opt_index]
                dc_opt_index = rotation_dc[ft_opt_index]
                cumTP_opt = px.cumTp_exp[ft_opt_index][dc_opt_index]
                co2content_inventory_opt = gfd(m, "co2content_inventory", r2, ft_opt, "", DATA_NOW) / 1000
                FutureCarbonContent_opt = []
                y = 0
                while y <= cumTP_opt:
                    expectedVHa_opt_y = vHaByYear(ft_opt_index, y)
                    FutureCarbonContent_opt.append(co2content_inventory_opt * expectedVHa_opt_y)
                    y += 1
                while len(FutureCarbonContent_opt) <= 2000:
                    FutureCarbonContent_opt.extend(list(FutureCarbonContent_opt))
                for j, ft in enumerate(fTypes):
                    co2content_inventory = gfd(m, "co2content_inventory", r2, ft, "", DATA_NOW) / 1000
                    expReturns_hartman = -10000000000.0
                    expReturns_timber_hartman = 0.0
                    expReturns_carbon_hartman = 0.0
                    optDc_hartman = -1
                    for u in range(1, nDc):
                        dc = dClasses[u]
                        cumTp_u = px.cumTp_exp[j][u]
                        carbonPrice_y = carbonPrice_NOW
                        nY = int(math.floor(cumTp_u)) + 1 if cumTp_u >= 0 else 0
                        cp = carbonPayments(cpEndo, j, nY, FutureCarbonContent_opt, co2content_inventory, carbonPrice_y)
                        raw_amount_carbon = discountedSum(cp, cumTp_u, True)
                        vHa = px.vHa_exp[j][u]
                        finalHarvestFlag = gfd(m, "finalHarvestFlag", regId, ft, dc)
                        fyE = yint(thisYear + cumTp_u)
                        for pp in priProducts:
                            pl, worldCurPrice, worldFutPrice, sl, sa = endoPrices(pp, fyE)
                            pw_exp = _computeExpectedPrice(pl, worldCurPrice, worldFutPrice, sl, sa, px.expTypePrices)
                            raw_amount = finalHarvestFlag * pw_exp * vHa * appf(m, pp, ft, dc)
                            anualised_amount_timber = calculateAnnualisedEquivalent(raw_amount, cumTp_u, ir)
                            raw_amount_carbon += pickling * carbonPrice_y * finalHarvestFlag * vHa * appf(m, pp, ft, dc)
                            anualised_amount_carbon = calculateAnnualisedEquivalent(raw_amount_carbon, cumTp_u, ir)
                            anualised_amount = anualised_amount_timber + anualised_amount_carbon
                            if anualised_amount >= expReturns_hartman:
                                expReturns_hartman = anualised_amount
                                expReturns_carbon_hartman = anualised_amount_carbon
                                expReturns_timber_hartman = anualised_amount_timber
                                optDc_hartman = u
                    px.expectedAnnualIncome_timber.append(expReturns_timber_hartman)
                    px.expectedAnnualIncome_carbon.append(expReturns_carbon_hartman)
                    px.expectedReturnsNotCorrByRa.append(expReturns_hartman)
                    px.optDc.append(optDc_hartman)
                    if heteroRA:
                        ra = getDoubleValue(G, px, "ra")
                        cumMort = 1 - px.cumAlive_exp[j][optDc_hartman]
                        expReturns_hartman = expReturns_hartman * (1.0 - ra * cumMort)
                    px.expectedReturns.append(expReturns_hartman)
                    expectedReturns[j] = expReturns_hartman
                    rotation_dc[j] = optDc_hartman

            # C, D, E - corrected expected returns and allocation of the managed and unmanaged regeneration areas
            for j, ft in enumerate(fTypes):
                harvestedAreaForThisFT = vSum(px.hArea[j]) * (1 - fArea_decr_rel)
                deltaAreas[j][nft] += vSum(px.hArea[j]) * (fArea_decr_rel)
                corrExpectedReturns = [0.0] * nft
                polBal_fiSub_unitvalue = [0.0] * nft
                for j2 in range(nft):
                    itc, pfs = transCost[j][j2]
                    invTransCost = itc * pfs
                    polBal_fiSub_unitvalue[j2] = itc * (pfs - 1)
                    corrExpectedReturns[j2] = cdiv(expectedReturns[j2], ir) - invTransCost
                optFtChosen = getMaxPos(corrExpectedReturns)
                optFtER = getMaxPos(expectedReturns)
                px.optFtChosen.append(optFtChosen)
                px.optDcChosen.append(px.optDc[optFtChosen])
                thisYearRegAreas[optFtChosen] += harvestedAreaForThisFT * mr
                thisYearRegAreas[optFtER] += fArea_incr * mr / float(nft)
                deltaAreas[j][optFtChosen] += harvestedAreaForThisFT * mr
                deltaAreas[nft][optFtER] += fArea_incr * mr / float(nft)
                polBal_fiSub[optFtChosen] += harvestedAreaForThisFT * mr * polBal_fiSub_unitvalue[optFtChosen]

                if c.natRegAllocation == "pp":
                    parentFt = parentOf[j]
                    freq = gfd(m, "freq_norm", regId, parentFt, "") if c.rescaleFrequencies else gfd(m, "freq", regId, parentFt, "")
                    if parentFt not in hAreaByFTypeGroup:
                        msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                    hAreaThisFtGroup = hAreaByFTypeGroup[parentFt]
                    if hAreaThisFtGroup > 0:
                        hRatio = harvestedAreaForThisFT / hAreaThisFtGroup
                    else:
                        hRatio = cdiv(1.0, getNForTypesChilds(MD, parentFt))
                    thisYearRegAreas[j] += totalHarvestedArea * (1 - mr) * freq * hRatio
                    thisYearRegAreas[j] += fArea_incr * (1 - mr) * freq * hRatio
                    for j2 in range(nft):
                        deltaAreas[j2][j] += vSum(px.hArea[j2]) * (1 - fArea_decr_rel) * (1 - mr) * freq * hRatio
                    deltaAreas[nft][j] += fArea_incr * (1 - mr) * freq * hRatio
                else:
                    mortRatePath = getPathMortality(m, px, ft, "0")
                    if mortRatePath > 0:
                        parentFt = parentOf[j]
                        siblings = getForTypeChilds(MD, parentFt)
                        siblingAreas = []
                        for s in siblings:
                            siblingAreas.append(0.0 if s == ft else vSum(px.area[getPos(s, fTypes)]))
                        areaAllSiblings = vSum(siblingAreas)
                        thisYearRegAreas[j] += harvestedAreaForThisFT * (1 - mr) * (1 - mortRatePath)
                        deltaAreas[j][j] += harvestedAreaForThisFT * (1 - mr) * (1 - mortRatePath)
                        if areaAllSiblings > 0.0:
                            for k, s in enumerate(siblings):
                                v = harvestedAreaForThisFT * (1.0 - mr) * (mortRatePath) * (siblingAreas[k] / areaAllSiblings)
                                thisYearRegAreas[getPos(s, fTypes)] += v
                                deltaAreas[j][getPos(s, fTypes)] += v
                        elif len(siblings) > 1:
                            for s in siblings:
                                if s != ft:
                                    v = harvestedAreaForThisFT * (1.0 - mr) * (mortRatePath) * 1.0 / (toFloat32(float(len(siblings))) - 1.0)
                                    thisYearRegAreas[getPos(s, fTypes)] += v
                                    deltaAreas[j][getPos(s, fTypes)] += v
                    else:
                        thisYearRegAreas[j] += harvestedAreaForThisFT * (1.0 - mr)
                        deltaAreas[j][j] += harvestedAreaForThisFT * (1.0 - mr)
                    totArea = vSum(px.area)
                    newAreaThisFt = fArea_incr * (1 - mr) * vSum(px.area[j]) / totArea if totArea != 0 else 0.0
                    thisYearRegAreas[j] += newAreaThisFt
                    deltaAreas[nft][j] += newAreaThisFt
                    if not (thisYearRegAreas[j] >= 0.0):
                        v = thisYearRegAreas[j]
                        msgOut(MSG_ERROR, f"thisYearRegAreas[j] is not non negative (j: {j}, thisYearRegAreas[j]: {i2s(v) if math.isfinite(v) else v}).")
            for j in range(nft):
                px.area[j][0] += thisYearRegAreas[j]
            if thisYear not in px.regArea:
                px.regArea[thisYear] = thisYearRegAreas
            px.deltaArea = deltaAreas
        if -fArea_diff > regHArea:
            msgOut(MSG_WARNING, f"In region {regId} the exogenous area decrement ({d2s(-fArea_diff)} ha) is higger than the harvesting ({d2s(regHArea)} ha). Ratio forced to 1.")
        for j, ft in enumerate(fTypes):
            sfd(m, polBal_fiSub[j] / 1000000.0, "polBal_fiSub", regId, ft, "", DATA_NOW, True)


# ------------------------------------------------------------------------------
# Initialisation of pixel values
# ------------------------------------------------------------------------------

def initializePixelVolumes(m):
    msgOut(MSG_INFO, "Starting initializing pixel-level values")
    c = m.core
    G = m.GIS
    if not getBoolSetting(m, "usePixelData"):
        return
    for r in c.regIds2:
        reg = getRegion(m.MD, r)
        rpx = regPixels(m, reg)
        for px in rpx:
            px.vol = []
        for ft in c.fTypes:
            regForArea = regionLayerSum(m, reg, "forArea_" + ft)
            regVols = [gfd(m, "vol", r, ft, c.dClasses[z], c.firstYear) if z > 0 else 0.0 for z in range(len(c.dClasses))]
            for px in rpx:
                vol_byu = []
                for z in range(len(c.dClasses)):
                    regVol = regVols[z]
                    pxArea = getDoubleValue(G, px, "forArea_" + ft, True)
                    if pxArea < 0.0:
                        msgOut(MSG_CRITICAL_ERROR, "Error in initializePixelVolumes, negative pxArea!")
                    pxVol = regVol * pxArea / regForArea if regForArea != 0 else 0.0
                    vol_byu.append(pxVol)
                px.vol.append(vol_byu)
    loadExogenousForestLayers(m, "vol")


def assignSpMultiplierPropToVols(m):
    if not getBoolSetting(m, "useSpatialVarPropToVol"):
        return
    c = m.core
    for r in c.regIds2:
        rpx = regionPixels(m, r)
        for f, ft in enumerate(c.fTypes):
            agr = gfd(m, "agr", r, ft, "")
            sStDev = gfd(m, "sStDev", r, ft, "")
            vols = [vSum(px.vol[f]) for px in rpx]
            vol_max = getMax(vols)
            diffVols = [vol_max - v for v in vols]
            diffVols_avg = getAvg(diffVols)
            rescale_ratio_avg = agr / diffVols_avg if diffVols_avg != 0.0 else 1.0
            diffVols_rescaled = [d * rescale_ratio_avg for d in diffVols]
            diffVols_rescaled_avg = getAvg(diffVols_rescaled)
            diffVols_rescaled_sd = getSd(diffVols_rescaled, sample=False)
            rescale_ratio_sd = sStDev / diffVols_rescaled_sd if diffVols_rescaled_sd != 0.0 else 1.0
            for p, px in enumerate(rpx):
                dev = diffVols_rescaled[p] - diffVols_rescaled_avg
                devr = dev * rescale_ratio_sd
                final_value = diffVols_rescaled_avg + devr
                px.spMods[f] = cmin(1.6, cmax(0.4, final_value / agr)) if agr != 0.0 else 1.0


def initialiseCarbonModule(m):
    c = m.core
    initialiseEmissionCounters(m)
    for r in c.regIds2:
        deathBiomass = [gfd(m, "vMort", r, ft, DIAM_ALL, DATA_NOW) + gfd(m, "vMortAdd", r, ft, DIAM_ALL, DATA_NOW) for ft in c.fTypes]
        initialiseDeathBiomassStocks(m, deathBiomass, r)
        qProducts = []
        for p in c.priProducts:
            qProducts.append(gpd(m, "sa", r, p, DATA_NOW))
        for p in c.secProducts:
            qProducts.append(gpd(m, "dl", r, p, DATA_NOW))
        initialiseProductsStocks(m, qProducts, r)


def initialiseDeathTimber(m):
    c = m.core
    currentYear = m.SCD.year
    for y in range(currentYear, currentYear - 30, -1):
        for r in c.regIds2:
            for ft in c.fTypes:
                for dc in c.dClasses:
                    deathTimberInventory_incrOrAdd(m.MD, (y, r, ft, dc), 0.0)


def initializePixelArea(m):
    msgOut(MSG_INFO, "Starting initializing pixel-level area")
    c = m.core
    G = m.GIS
    if not getBoolSetting(m, "usePixelData"):
        return
    for r in c.regIds2:
        reg = getRegion(m.MD, r)
        rpx = regPixels(m, reg)
        rinfo = {}
        for ft in c.fTypes:
            rinfo[ft] = (regionLayerSum(m, reg, "forArea_" + ft) / 10000.0, gfd(m, "vReg", r, ft, ""), gfd(m, "entryVolHa", r, ft, ""))
        for px in rpx:
            for j, ft in enumerate(c.fTypes):
                tempAreas = []
                pxArea = getDoubleValue(G, px, "forArea_" + ft, True) / 10000.0
                for u in range(len(c.dClasses)):
                    if u == 0:
                        regionAreaV, regRegVolumes, entryVolHa = rinfo[ft]
                        newVReg = regRegVolumes * pxArea / regionAreaV if regionAreaV != 0 else 0.0
                        tp_u0 = px.tp[j][0]
                        tempAreas.append(cdiv(newVReg * 1000000.0, entryVolHa) * tp_u0)
                    else:
                        dcVol = px.vol_l[j][u] * 1000000.0
                        dcVHa = px.vHa[j][u]
                        tempAreas.append(dcVol / dcVHa if dcVHa != 0 else 0.0)
                sumTempArea = vSum(tempAreas)
                normCoef = pxArea / sumTempArea if sumTempArea != 0 else 0.0
                px.area_l.append([a * normCoef for a in tempAreas])
            px.area = copy.deepcopy(px.area_l)
    loadExogenousForestLayers(m, "area")


def computeCumulativeData(m):
    msgOut(MSG_INFO, "Starting computing some cumulative values..")
    c = m.core
    MD = m.MD
    thisYear = m.SCD.year
    firstYear = c.firstYear
    dClasses = c.dClasses
    nDc = len(dClasses)
    fTypes = c.fTypes
    usePath = m.cache.usePathogenModule
    for regId in c.regIds2:
        regPx = regionPixels(m, regId)
        # regional data by ft and dc (pure look-ups) cached by (name, ft index, dc index, year); dc index -1: no dc
        dcache = {}

        def rd(name, j, u, y):
            k = (name, j, u, y)
            v = dcache.get(k)
            if v is None:
                v = dcache[k] = gfd(m, name, regId, fTypes[j], "" if u < 0 else dClasses[u], y)
            return v
        # the values of the current and of the first year, and the spatio-temporal layer keys of the
        # additional mortality, by forest type and diameter class (silent look-ups, as in the pixel loop)
        origErrorLevel = MD.errorLevel
        MD.errorLevel = MSG_NO_MSG
        reg = []
        for j, ft in enumerate(fTypes):
            reg.append((
                [rd("tp", j, u, thisYear) for u in range(nDc)], [rd("tp", j, u, firstYear) for u in range(nDc)],
                [rd("mortCoef", j, u, thisYear) for u in range(nDc)], [rd("mortCoef", j, u, firstYear) for u in range(nDc)],
                [rd("betaCoef", j, u, thisYear) for u in range(nDc)], [rd("betaCoef", j, u, firstYear) for u in range(nDc)],
                rd("entryVolHa", j, -1, thisYear), rd("entryVolHa", j, -1, firstYear),
                [rd("additionalMortality_multiplier", j, u, thisYear) for u in range(nDc)],
                [rd("additionalMortality_multiplier", j, u, firstYear) for u in range(nDc)],
                [stKey(m, "additionalMortality", ft, thisYear, dc) for dc in dClasses],
                [stKey(m, "additionalMortality", ft, firstYear, dc) for dc in dClasses]))
        MD.errorLevel = origErrorLevel
        amKeys = {}

        def amKey(j, u, y):
            k = (j, u, y)
            v = amKeys.get(k, False)
            if v is False:
                v = amKeys[k] = stKey(m, "additionalMortality", fTypes[j], y, dClasses[u])
            return v
        for px in regPx:
            px.cumTp = []
            px.cumTp_exp = []
            px.vHa_exp = []
            px.vHa = []
            px.cumAlive = []
            px.cumAlive_exp = []
            expType = px.expType
            for j, ft in enumerate(fTypes):
                (tp_now, tp_t0, mc_now, mc_t0, bc_now, bc_t0, evh_now, evh_t0, amm_now, amm_t0, amk_now, amk_t0) = reg[j]
                mult = {}

                def multiplier(name, y):
                    k = (name, y)
                    v = mult.get(k)
                    if v is None:
                        v = mult[k] = getMultiplier(m, px, name, ft, y)
                    return v
                tp_multiplier_now = multiplier("tp_multiplier", thisYear)
                tp_multiplier_t0 = multiplier("tp_multiplier", firstYear)
                mortCoef_multiplier_now = multiplier("mortCoef_multiplier", thisYear)
                mortCoef_multiplier_t0 = multiplier("mortCoef_multiplier", firstYear)
                betaCoef_multiplier_now = multiplier("betaCoef_multiplier", thisYear)
                betaCoef_multiplier_t0 = multiplier("betaCoef_multiplier", firstYear)
                MD.errorLevel = MSG_NO_MSG
                cumTp_temp = [0.0]
                vHa_temp = [0.0]
                cumAlive_temp = [1.0]
                cumTp_exp_temp = [0.0]
                vHa_exp_temp = [0.0]
                cumAlive_exp_temp = [1.0]
                for u in range(1, nDc):
                    dc = dClasses[u]
                    pathMort_now = getPathMortality(m, px, ft, dc, thisYear) if usePath else 0.0
                    pathMort_t0 = getPathMortality(m, px, ft, dc, firstYear) if usePath else 0.0
                    additionalMortality_now = stValue(m, px, amk_now[u], 0.0) * amm_now[u]
                    additionalMortality_t0 = stValue(m, px, amk_t0[u], 0.0) * amm_t0[u]
                    tp = tp_now[u - 1] * tp_multiplier_now
                    cumTp_u = cumTp_temp[u - 1] + tp
                    if u == 1:
                        vHa_u = evh_now
                        mort = 0.0
                    else:
                        mort = 1 - cpow(1 - cmin(mc_now[u - 1] * mortCoef_multiplier_now + pathMort_now + additionalMortality_now, 1.0), tp)
                        beta = bc_now[u] * betaCoef_multiplier_now
                        vHa_u = vHa_temp[u - 1] * beta * (1 - mort)
                    cumAlive_u = cmax(0.0, cumAlive_temp[u - 1] * (1 - mort))
                    cumAlive_temp.append(cumAlive_u)
                    cumTp_temp.append(cumTp_u)
                    vHa_temp.append(vHa_u)
                    if expType == -1:
                        tp_exp = tp_t0[u - 1] * tp_multiplier_t0
                        cumTp_exp_temp.append(cumTp_exp_temp[u - 1] + tp_exp)
                        if u == 1:
                            vHa_u_exp = evh_t0
                            mort_exp = 0.0
                        else:
                            mort_exp = 1 - cpow(1 - cmin(mc_t0[u - 1] * mortCoef_multiplier_t0 + pathMort_t0 + additionalMortality_t0, 1.0), tp_exp)
                            beta_exp = bc_t0[u] * betaCoef_multiplier_t0
                            vHa_u_exp = vHa_exp_temp[u - 1] * beta_exp * (1 - mort_exp)
                    else:
                        yDyn = yint(thisYear + math.ceil(cumTp_exp_temp[u - 1]))
                        tp_multiplier_dynamic = multiplier("tp_multiplier", yDyn)
                        tp_noExp = tp_now[u - 1] * tp_multiplier_now
                        cumTp_u_noExp = cumTp_exp_temp[u - 1] + tp_noExp
                        tp_fullExp = rd("tp", j, u - 1, yDyn) * tp_multiplier_dynamic
                        cumTp_u_fullExp = cumTp_exp_temp[u - 1] + tp_fullExp
                        cumTp_u_exp = cumTp_u_fullExp * expType + cumTp_u_noExp * (1 - expType)
                        cumTp_exp_temp.append(cumTp_u_exp)
                        if u == 1:
                            vHa_u_noExp = evh_now
                            vHa_u_fullExp = rd("entryVolHa", j, -1, yint(thisYear + math.ceil(cumTp_u)))
                            vHa_u_exp = vHa_u_fullExp * expType + vHa_u_noExp * (1 - expType)
                            mort_exp = 0.0
                        else:
                            mort_noExp = 1 - cpow(1 - cmin(1.0, mc_now[u - 1] * mortCoef_multiplier_now + pathMort_now + additionalMortality_now), tp_noExp)
                            mortCoef_multiplier_dynamic = multiplier("mortCoef_multiplier", yDyn)
                            pathMort_dynamic = getPathMortality(m, px, ft, dc, yDyn) if usePath else 0.0
                            am_mult_dyn = rd("additionalMortality_multiplier", j, u, yDyn)
                            additionalMortality_dynamic = stValue(m, px, amKey(j, u, yDyn), 0.0) * am_mult_dyn
                            mort_fullExp = 1 - cpow(1 - cmin(1.0, rd("mortCoef", j, u - 1, yDyn) * mortCoef_multiplier_dynamic + pathMort_dynamic + additionalMortality_dynamic), tp_fullExp)
                            beta_noExp = bc_now[u] * betaCoef_multiplier_now
                            yB = yint(thisYear + math.ceil(cumTp_u))
                            betaCoef_multiplier_dynamic = multiplier("betaCoef_multiplier", yB)
                            beta_fullExp = rd("betaCoef", j, u, yB) * betaCoef_multiplier_dynamic
                            mort_exp = mort_fullExp * expType + mort_noExp * (1 - expType)
                            beta_exp = beta_fullExp * expType + beta_noExp * (1 - expType)
                            vHa_u_exp = vHa_exp_temp[u - 1] * beta_exp * (1 - mort_exp)
                    vHa_exp_temp.append(vHa_u_exp)
                    cumAlive_exp_temp.append(cmax(0.0, cumAlive_exp_temp[u - 1] * (1 - mort_exp)))
                px.cumTp.append(cumTp_temp)
                px.vHa.append(vHa_temp)
                px.cumAlive.append(cumAlive_temp)
                px.cumTp_exp.append(cumTp_exp_temp)
                px.vHa_exp.append(vHa_exp_temp)
                px.cumAlive_exp.append(cumAlive_exp_temp)
    MD.errorLevel = MSG_ERROR


def resetPixelValues(m):
    msgOut(MSG_INFO, "Starting resetting pixel level values")
    for regId in m.core.regIds2:
        for px in regionPixels(m, regId):
            px.vol_l = px.vol                 # vol_l = vol (and vol is then cleared)
            px.area_l = px.area               # area_l = area
            px.vol = []
            px.area = copy.deepcopy(px.area_l)   # area = area_l (a copy)
            px.hArea = []
            px.deltaArea = []
            px.hVol = []
            px.hVol_byPrd = []
            px.vReg = []
            px.expectedReturns = []
            px.beta = []
            px.mort = []
            px.addMort = []
            px.tp = []
            px.cumTp = []
            px.vHa = []
            px.cumTp_exp = []
            px.vHa_exp = []
            px.cumAlive = []
            px.cumAlive_exp = []
            px.vMort = []
            px.vMortAdd = []


def cachePixelExogenousData(m):
    msgOut(MSG_INFO, "Starting cashing on pixel spatial-level exogenous data")
    c = m.core
    applyAvalCoef = getBoolSetting(m, "applyAvalCoef")
    noValue = getDoubleSetting(m, "noValue")
    for regId in c.regIds2:
        regPx = regionPixels(m, regId)
        rdata = {}
        for j, ft in enumerate(c.fTypes):
            for u, dc in enumerate(c.dClasses):
                rdata[(j, u)] = (gfd(m, "additionalMortality_multiplier", regId, ft, dc, DATA_NOW),
                                 gfd(m, "tp", regId, ft, dc, c.firstYear), gfd(m, "tp", regId, ft, dc, DATA_NOW),
                                 gfd(m, "betaCoef", regId, ft, dc, DATA_NOW) if u > 0 else 0.0,
                                 gfd(m, "mortCoef", regId, ft, dc, DATA_NOW) if u > 0 else 0.0)
        for px in regPx:
            px.tp = []
            px.beta = []
            px.mort = []
            px.addMort = []
            px.avalCoef = []
            for j, ft in enumerate(c.fTypes):
                tp_byu = []
                beta_byu = []
                mort_byu = []
                addMort_byu = []
                tp_multiplier_now = getMultiplier(m, px, "tp_multiplier", ft, DATA_NOW)
                mortCoef_multiplier_now = getMultiplier(m, px, "mortCoef_multiplier", ft, DATA_NOW)
                betaCoef_multiplier_now = getMultiplier(m, px, "betaCoef_multiplier", ft, DATA_NOW)
                avalCoef_ft = getSTData(m, px, "avalCoef", ft, DATA_NOW) if applyAvalCoef else 1.0
                if avalCoef_ft == noValue:
                    msgOut(MSG_CRITICAL_ERROR, "applyAvalCoef has benn asked for, but I can't find the avalCoef in the data!")
                for u, dc in enumerate(c.dClasses):
                    pathMortality = getPathMortality(m, px, ft, dc, DATA_NOW)
                    am_mult, tp_t0, tp_now, betaC, mortC = rdata[(j, u)]
                    additionalMortality = getSTData(m, px, "additionalMortality", ft, DATA_NOW, dc, 0.0) * am_mult
                    if u == 0:
                        tp = tp_t0 * getMultiplier(m, px, "tp_multiplier", ft, c.firstYear)
                    else:
                        tp = tp_now * tp_multiplier_now
                    beta_real = betaC * betaCoef_multiplier_now if u > 0 else 0.0
                    mort_real = cmin(mortC * mortCoef_multiplier_now + pathMortality if u > 0 else 0.0, 1.0)
                    tp_byu.append(tp)
                    beta_byu.append(beta_real)
                    mort_byu.append(mort_real)
                    addMort_byu.append(additionalMortality)
                px.tp.append(tp_byu)
                px.beta.append(beta_byu)
                px.mort.append(mort_byu)
                px.addMort.append(addMort_byu)
                px.avalCoef.append(avalCoef_ft)


def getAvailableAliveTimber(m, primProd, regId):
    """ModelData::getAvailableAliveTimber()"""
    MD = m.MD
    toReturn = 0.0
    regPx = regionPixels(m, regId)
    for i, ft in enumerate(getForTypeIds(MD)):
        for u, dc in enumerate(MD.diamClasses):
            possible = any(assessProdPossibility(MD, pp, ft, dc) for pp in primProd)
            if possible:
                for px in regPx:
                    toReturn += px.vol_l[i][u] * px.avalCoef[i]
    return toReturn


def computeInventary(m):
    msgOut(MSG_INFO, "Starting computing inventary available for this year..")
    c = m.core
    MD = m.MD
    combs = createCombinationsVector(len(c.priProducts))
    currentYear = m.SCD.year
    nFt = len(c.fTypes)
    nDc = len(c.dClasses)
    for r2 in c.regIds2:
        REG = getRegion(MD, r2)
        regPx = regPixels(m, REG)
        in_reg = [0.0] * len(c.priProducts)
        in_deathTimber_reg = [0.0] * len(c.priProducts)
        appv = [[[appf(m, pp, ft, dc) for dc in c.dClasses] for ft in c.fTypes] for pp in c.priProducts]
        for px in regPx:
            for pp in range(len(c.priProducts)):
                inv = 0.0
                a_pp = appv[pp]
                for ft in range(nFt):
                    vl = px.vol_l[ft]
                    ac = px.avalCoef[ft]
                    a_ft = a_pp[ft]
                    for dc in range(nDc):
                        inv += a_ft[dc] * vl[dc] * ac
                in_reg[pp] += inv
        for pi, pp in enumerate(c.priProducts):
            in_deathTimber_reg[pi] += getAvailableDeathTimber(m, [pp], r2, currentYear - 1)
            spd(m, in_reg[pi], "in", r2, pp, DATA_NOW, True)
            spd(m, in_deathTimber_reg[pi], "in_deathTimber", r2, pp, DATA_NOW, True)
        for i, comb in enumerate(combs):
            ids = [c.priProducts[k] for k in comb]
            bound_alive = getAvailableAliveTimber(m, ids, r2)
            bound_deathTimber = getAvailableDeathTimber(m, ids, r2, currentYear - 1)
            REG.inResByAnyCombination[i] = bound_alive + bound_deathTimber


def updateMapAreas(m):
    msgOut(MSG_INFO, "Updating map areas..")
    c = m.core
    G = m.GIS
    if not c.oldVol2AreaMethod:
        if not getBoolSetting(m, "usePixelData"):
            return
        for r in c.regIds2:
            for px in regionPixels(m, r):
                for j, ft in enumerate(c.fTypes):
                    changeValue(G, px, "forArea_" + ft, vSum(px.area[j]) * 10000)
    else:
        currentYear = m.SCD.year
        fTypes = getForTypeIds(m.MD)
        for regId in getRegionIds(m.MD, 2, True):
            rpx = regionPixels(m, regId)
            for j, ft in enumerate(fTypes):
                for px in rpx:
                    oldValue = getDoubleValue(G, px, "forArea_" + ft, True) / 10000
                    hArea = vSum(px.hArea[j])
                    if currentYear not in px.regArea:
                        msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                    regArea = px.regArea[currentYear][j]
                    changeValue(G, px, "forArea_" + ft, (oldValue - hArea + regArea) * 10000)


def updateOtherMapData(m):
    G = m.GIS
    fTypes = getForTypeIds(m.MD)
    for regId in getRegionIds(m.MD, 2, True):
        rpx = regionPixels(m, regId)
        for j, ft in enumerate(fTypes):
            hasVol = layerExist(G, "vol_" + ft)
            hasER = layerExist(G, "expectedReturns_" + ft)
            for px in rpx:
                vol = vSum(px.vol[j])
                expectedReturns = px.expectedReturns[j]
                if hasVol:
                    changeValue(G, px, "vol_" + ft, vol)
                if hasER:
                    changeValue(G, px, "expectedReturns_" + ft, expectedReturns)
    for ft in fTypes:
        updateImage(m, "vol_" + ft)
        updateImage(m, "expectedReturns_" + ft)


def sumRegionalForData(m):
    from .output import printDetailedHV
    msgOut(MSG_INFO, "Summing data pixels->region..")
    c = m.core
    MD = m.MD
    G = m.GIS
    currentYear = m.SCD.year
    hVol_byPrd = {}
    outDetailedHv = getBoolSetting(m, "outDetailedHv", DATA_NOW)
    nPri = len(c.priProducts)
    for regId in c.regIds2:
        regPx = regionPixels(m, regId)
        regIdS = i2s(regId)
        for j, ft in enumerate(c.fTypes):
            regArea = 0.0
            sumAreaByFt = 0.0
            vReg = 0.0
            sumSumHProductivity = 0.0
            sumHArea = 0.0
            for u, dc in enumerate(c.dClasses):
                vol = 0.0
                hV = 0.0
                hArea = 0.0
                vMort = 0.0
                vMortAdd = 0.0
                sumHProductivity = 0.0
                keys = [(regIdS, ft, dc, pr) for pr in c.priProducts] if outDetailedHv else None
                for px in regPx:
                    vol += px.vol[j][u]
                    hV += px.hVol[j][u]
                    hArea += px.hArea[j][u]
                    vMort += px.vMort[j][u]
                    vMortAdd += px.vMortAdd[j][u]
                    sumHProductivity += (px.hProductivity[j][u] * px.hArea[j][u])
                    if outDetailedHv:
                        hvp = px.hVol_byPrd[j][u]
                        for pp in range(nPri):
                            k = keys[pp]
                            hVol_byPrd[k] = hVol_byPrd.get(k, 0.0) + hvp[pp]
                if u > 0:
                    sfd(m, vol, "vol", regId, ft, dc, DATA_NOW)
                    sfd(m, hV, "hV", regId, ft, dc, DATA_NOW, True)
                    hProductivityByDc = sumHProductivity / hArea if hArea != 0 else 0.0
                    sumSumHProductivity += (hProductivityByDc * hArea)
                    sumHArea += hArea
                    sfd(m, hProductivityByDc, "hProductivityByDc", regId, ft, dc, DATA_NOW, True)
                    sfd(m, hArea, "harvestedArea", regId, ft, dc, DATA_NOW, True)
                    sfd(m, vMort, "vMort", regId, ft, dc, DATA_NOW, True)
                    sfd(m, vMortAdd, "vMortAdd", regId, ft, dc, DATA_NOW, True)
                    vol_1 = gfd(m, "vol", regId, ft, dc, currentYear - 1)
                    if vol_1 != 0:
                        sfd(m, hV / vol_1, "hr", regId, ft, dc, DATA_NOW, True)
                    else:
                        sfd(m, 0.0, "hr", regId, ft, dc, DATA_NOW, True)
            hProductivity = sumSumHProductivity / sumHArea if sumHArea != 0 else 0.0
            sfd(m, hProductivity, "hProductivity", regId, ft, "", DATA_NOW, True)
            for px in regPx:
                vReg += px.vReg[j]
                if currentYear not in px.regArea:
                    msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                regArea += px.regArea[currentYear][j]
                sumAreaByFt += getDoubleValue(G, px, "forArea_" + ft, True) / 10000
                if not (sumAreaByFt >= 0.0):
                    msgOut(MSG_CRITICAL_ERROR, "sumAreaByFt is not non negative.")
            sfd(m, vReg, "vReg", regId, ft, "", DATA_NOW, True)
            sfd(m, regArea, "regArea", regId, ft, "", DATA_NOW, True)
            sfd(m, sumAreaByFt, "forArea", regId, ft, "", DATA_NOW, True)
    printDetailedHV(m, hVol_byPrd)

    parentFtypes = getForTypeParents(MD)
    for regId in c.regIds2:
        regPx = regionPixels(m, regId)
        totRegionForArea = 0.0
        totSumExpRet = 0.0
        totSumExpRet_byFTParent = [0.0] * len(parentFtypes)
        totSumExpRet_byFTypes = [0.0] * len(c.fTypes)
        childIds = [getForTypeChilds(MD, p) for p in parentFtypes]
        childPos = [getForTypeChilds_pos(MD, p) for p in parentFtypes]
        for px in regPx:
            pxForArea = vSum(px.area)
            totRegionForArea += pxForArea
            bestPxExpectedRet = getMax(px.expectedReturnsNotCorrByRa)
            for i in range(len(parentFtypes)):
                pxExpReturnsByChilds = [0.0] * len(childPos[i])
                for j in range(len(childPos[i])):
                    single = px.expectedReturns[childPos[i][j]]
                    pxExpReturnsByChilds[j] = 0.0 if childIds[i][j] == "ash" else single
                    totSumExpRet_byFTypes[childPos[i][j]] += single * pxForArea
                totSumExpRet_byFTParent[i] += getMax(pxExpReturnsByChilds) * pxForArea
            totSumExpRet += bestPxExpectedRet * pxForArea
        for i in range(len(parentFtypes)):
            for j in range(len(childPos[i])):
                cp = childPos[i][j]
                sfd(m, totSumExpRet_byFTypes[cp], "sumExpReturns", regId, c.fTypes[cp], "", DATA_NOW, True)
                sfd(m, cdiv(totSumExpRet_byFTypes[cp], totRegionForArea), "expReturns", regId, c.fTypes[cp], "", DATA_NOW, True)
            sfd(m, totSumExpRet_byFTParent[i], "sumExpReturns", regId, parentFtypes[i], "", DATA_NOW, True)
            sfd(m, cdiv(totSumExpRet_byFTParent[i], totRegionForArea), "expReturns", regId, parentFtypes[i], "", DATA_NOW, True)
        sfd(m, totSumExpRet, "sumExpReturns", regId, "", "", DATA_NOW, True)
        sfd(m, cdiv(totSumExpRet, totRegionForArea), "expReturns", regId, "", "", DATA_NOW, True)

    if getBoolSetting(m, "usePathogenModule"):
        for regId in c.regIds2:
            totalForArea = 0.0
            invadedArea = 0.0
            for px in regionPixels(m, regId):
                invaded = 0
                for ft in c.fTypes:
                    for dc in c.dClasses:
                        if getPathMortality(m, px, ft, dc) > 0:
                            invaded = 1
                totalForArea += vSum(px.area)
                invadedArea += vSum(px.area) * invaded
            sfd(m, cdiv(invadedArea, totalForArea), "totalShareInvadedArea", regId, "", "", DATA_NOW, True)

    priPr = getStringVectorSetting(m, "priProducts")
    for regId in c.regIds2:
        deadTimber = toFloat32(getAvailableDeathTimber(m, priPr, regId, currentYear))  # a float in the C++ code
        sfd(m, deadTimber, "usableDeadTimber", regId, "", "", currentYear, True)


def registerCarbonEvents(m):
    c = m.core
    for r in c.regIds2:
        for ft in c.fTypes:
            deathBiomass = gfd(m, "vMort", r, ft, DIAM_ALL, DATA_NOW) + gfd(m, "vMortAdd", r, ft, DIAM_ALL, DATA_NOW)
            harvesting = gfd(m, "hV", r, ft, DIAM_ALL, DATA_NOW)
            registerDeathBiomass(m, deathBiomass, r, ft)
            registerHarvesting(m, harvesting, r, ft)
        for p in c.priProducts:
            registerProducts(m, gpd(m, "sa", r, p, DATA_NOW), r, p)
        for p in c.secProducts:
            registerProducts(m, gpd(m, "dl", r, p, DATA_NOW), r, p)
    for regs in c.l2r:
        for rfrom in regs:
            distQProd = 0.0
            for rto in regs:
                dist = gpd(m, "dist", rfrom, "", DATA_NOW, i2s(rto))
                for p in c.allProducts:
                    distQProd += dist * gpd(m, "rt", rfrom, p, DATA_NOW, i2s(rto))
            registerTransports(m, distQProd, rfrom)
    HWP_eol2energy(m)


def computeEconomicBalances(m):
    c = m.core
    MD = m.MD
    previousYear = m.SCD.year - 1
    for regId in c.regIds2:
        reg = getRegion(MD, regId)
        for p in c.priProducts:
            pol_s = gpd(m, "pol_mktDirInt_s", regId, p)
            gpd(m, "pol_mktDirInt_s", regId, p, previousYear)
            sigma = gpd(m, "sigma", regId, p)
            gpd(m, "in", regId, p)
            gpd(m, "in", regId, p, previousYear)
            pc = gpd(m, "pc", regId, p)
            gpd(m, "pc", regId, p, previousYear)
            sc = gpd(m, "sc", regId, p)
            gpd(m, "sc", regId, p, previousYear)
            gpd(m, "gamma_incr", regId, p)
            gpd(m, "gamma_decr", regId, p)
            polBal_mktDirInt_s = pc * (1 - pol_s) * sc
            carbonPrice = gpd(m, "carbonPrice", regId, "", DATA_NOW)
            intRate = getDoubleSetting(m, "ir")
            Pct = carbonPrice * intRate
            omega = gpd(m, "co2content_products", regId, p) / 1000
            bb = gpd(m, "bb", regId, p)
            surplus_prod = pc * sc - (bb * cpow(sc, cdiv(sigma + 1, sigma)) + sc * omega * Pct)
            spd(m, polBal_mktDirInt_s, "polBal_mktDirInt_s", regId, p, DATA_NOW, True)
            spd(m, surplus_prod, "surplus_prod", regId, p, DATA_NOW, True)
        for p in c.secProducts:
            pol_d = gpd(m, "pol_mktDirInt_d", regId, p)
            pol_d_1 = gpd(m, "pol_mktDirInt_d", regId, p, previousYear)
            sigma = gpd(m, "sigma", regId, p)
            pol_trSub = gpd(m, "pol_trSub", regId, p)
            pc = gpd(m, "pc", regId, p)
            pc_1 = gpd(m, "pc", regId, p, previousYear)
            dc = gpd(m, "dc", regId, p)
            dc_1 = gpd(m, "dc", regId, p, previousYear)
            dc0 = gpd(m, "dc", regId, p, c.secondYear)
            mm = gpd(m, "m", regId, p)
            sl = gpd(m, "sl", regId, p)
            polBal_mktDirInt_d = pc * (pol_d - 1) * dc
            polBal_trSub = mm * (pol_trSub - 1) * sl
            trs = 0.5
            surplus_cons = (pc_1 * pol_d_1 * cpow(dc_1, cdiv(-1, sigma)) * cpow(pol_d, -1) * cdiv(sigma, sigma + 1) *
                            (cpow(dc, cdiv(sigma + 1, sigma)) - cpow(trs * dc0, cdiv(sigma + 1, sigma))) - (dc - trs * dc0) * pc)
            spd(m, polBal_mktDirInt_d, "polBal_mktDirInt_d", regId, p, DATA_NOW, True)
            spd(m, polBal_trSub, "polBal_trSub", regId, p, DATA_NOW, True)
            spd(m, surplus_cons, "surplus_cons", regId, p, DATA_NOW, True)
        for p in c.allProducts:
            pol_tcSub = gpd(m, "pol_tcSub", regId, p)
            polBal_tcSub = 0.0
            for s in getSiblings(MD, reg):
                ct = gpd(m, "ct", regId, p, DATA_NOW, i2s(s.regId))
                rt = gpd(m, "rt", regId, p, DATA_NOW, i2s(s.regId))
                polBal_tcSub += ct * (pol_tcSub - 1) * rt
            spd(m, polBal_tcSub, "polBal_tcSub", regId, p, DATA_NOW, True)


def loadExogenousForestLayers(m, what):
    """Move volumes (what="vol") or areas (what="area") to the spatially explicit forest types defined in gis layers"""
    if not getBoolSetting(m, "useSpExplicitForestTypes"):
        return
    c = m.core
    G = m.GIS
    fTypes = c.fTypes
    nFT = len(fTypes)
    nDC = len(c.dClasses)
    allPx = [px for r in c.regIds2 for px in regionPixels(m, r)]
    pxC = len(allPx)
    if not c.original_vols:
        c.original_vols = [[[0.0] * nDC for _ in range(nFT)] for _ in range(pxC)]
    ov = c.original_vols
    if what == "vol":
        for i in range(nFT):
            for u in range(nDC):
                for p, px in enumerate(allPx):
                    ov[p][i][u] += px.vol[i][u]
        for i in range(nFT):
            for o in range(nFT):
                for u in range(1, nDC):
                    layerName = f"spInput#vol#{fTypes[o]}#{fTypes[i]}#{u}"
                    if layerExist(G, layerName):
                        for px in allPx:
                            vt = cmin(getDoubleValue(G, px, layerName, True) / 1000000, px.vol[i][u])
                            px.vol[i][u] -= vt
                            px.vol[o][u] += vt
    if what == "area":
        oa = [[[0.0] * nDC for _ in range(nFT)] for _ in range(pxC)]
        for i in range(nFT):
            for u in range(nDC):
                for p, px in enumerate(allPx):
                    oa[p][i][u] += px.area_l[i][u]
        ta = [[[0.0] * nFT for _ in range(nFT)] for _ in range(pxC)]
        for i in range(nFT):
            for o in range(nFT):
                for u in range(1, nDC):
                    layerName = f"spInput#vol#{fTypes[o]}#{fTypes[i]}#{u}"
                    if layerExist(G, layerName):
                        for p, px in enumerate(allPx):
                            vol_i_orig = ov[p][i][u]
                            vol_transfer = getDoubleValue(G, px, layerName, True) / 1000000 if vol_i_orig != 0 else 0.0
                            area_i_orig = oa[p][i][u]
                            area_transfer = area_i_orig * vol_transfer / vol_i_orig if vol_i_orig != 0 else 0.0
                            px.area_l[i][u] -= area_transfer
                            px.area[i][u] = px.area_l[i][u]
                            px.area_l[o][u] += area_transfer
                            px.area[o][u] = px.area_l[o][u]
                            ta[p][i][o] += area_transfer
        for p, px in enumerate(allPx):
            for i in range(nFT):
                for o in range(nFT):
                    area_i_orig = 0.0
                    for u in range(1, nDC):
                        area_i_orig += oa[p][i][u]
                    t0 = oa[p][i][0] * (ta[p][i][o] / area_i_orig) if area_i_orig != 0 else 0.0
                    px.area_l[i][0] -= t0
                    px.area[i][0] = px.area_l[i][0]
                    px.area_l[o][0] += t0
                    px.area[o][0] = px.area_l[0][0]   # as in the C++ code
        for i, ftid in enumerate(fTypes):
            fti = getForType(m.MD, ftid)
            if fti.memType in (3, 2):
                for px in allPx:
                    changeValue(G, px, fti.forLayer, vSum(px.area[i]) * 10000)
