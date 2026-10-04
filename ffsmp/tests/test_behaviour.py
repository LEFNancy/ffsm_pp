"""BEHAVIOUR TESTS: each t_* scenario must move the model in the expected direction and with a "reasonable"
magnitude, compared to the t_base scenario (same regions and years, nothing changed).

These tests do not depend on the C++ model: they are meant to stay valid when the Python model evolves.
Bounds are deliberately wide: they catch a change that is ignored, applied with the wrong sign or blown out of
proportion, not small numerical differences. Same tests as the Julia port (doc/port_to_julia/tests.md).
"""

import os
import re

import pytest

from common import (runScenario, outputDir, prepareRobustnessDataDir, forTot, prodTot, pxArea, sameTypeRegShare,
                    relchange, selectedScenarios, REGIONS, Y_START, Y_END, Y_SHOCK)
import ffsmp
from ffsmp.modeldata import getForData, getProdData, getForTypeIds, getStringVectorSetting, getRegion
from ffsmp.gis import getMultiplier, getDoubleValue, regPixels
from ffsmp.carbon import getStock
from ffsmp.utils import FT_ALL, DIAM_ALL, STOCK_INV

Y1 = Y_SHOCK + 1   # first year fully affected by the changes introduced in Y_SHOCK


# --- helpers ---

def fts(m):
    return getForTypeIds(m.MD)


def priPr(m):
    return getStringVectorSetting(m, "priProducts")


def secPr(m):
    return getStringVectorSetting(m, "secProducts")


def fv(m, var, y, regions=REGIONS, ft=FT_ALL):
    """Forest variable, summed over the test regions (or on `regions`), all forest types and diameter classes"""
    return forTot(m, var, y, regions=regions, ft=ft)


def pv(m, var, prod, y, regions=REGIONS):
    """Product variable, summed over the test regions (or on `regions`)"""
    return prodTot(m, var, prod, y, regions=regions)


def forArea(m, reg, y):
    """Total forest area of a region from the forData forArea, summed over the forest types"""
    s = 0.0
    for ft in fts(m):
        s += getForData(m, "forArea", reg, ft, "", y)
    return s


def harvest(m, y):
    """Total harvest (alive trees) of the test regions"""
    return fv(m, "hV", y)


def within(x, lo, hi):
    return lo <= x <= hi


def approx(a, b, rtol=0.0, atol=0.0):
    """Julia's isapprox"""
    return abs(a - b) <= max(atol, rtol * max(abs(a), abs(b)))


def regPx(m, reg):
    return regPixels(m, getRegion(m.MD, reg))


def allPx(m):
    return [px for r in REGIONS for px in regPx(m, r)]


class Check:
    """Soft assertions: all the failed checks of a test are reported together"""

    def __init__(self):
        self.failed = []
        self.n = 0

    def __call__(self, cond, msg=""):
        self.n += 1
        if not cond:
            self.failed.append(msg or f"check {self.n}")

    def done(self):
        assert not self.failed, f"{len(self.failed)} of {self.n} checks failed: " + "; ".join(self.failed[:10])


def needs(*names):
    """Skip the test if none of its scenarios is selected (FFSMP_TESTS=t_a,t_b,...)"""
    sel = selectedScenarios()
    if not any(n in sel for n in names):
        pytest.skip("scenario not selected")


@pytest.fixture(scope="module")
def B():
    return runScenario("t_base")


def checkInvariants(chk, m, fixedArea=True, pixelArea=None):
    """Accounting identities that must hold in every scenario"""
    pixelArea = fixedArea if pixelArea is None else pixelArea
    for y in range(Y_START, Y_END + 1):
        for r in REGIONS:
            # the harvested volume is entirely allocated to the primary products
            st = 0.0
            for p in priPr(m):
                st += getProdData(m, "st_fromHarv", r, p, y)
            chk(approx(getForData(m, "hV", r, FT_ALL, DIAM_ALL, y), st, rtol=1e-8), f"hV = st_fromHarv ({r}, {y})")
            if fixedArea:
                # all the harvested area is regenerated (no area change) and nothing more
                ra = 0.0
                for ft in fts(m):
                    ra += getForData(m, "regArea", r, ft, "", y)
                chk(approx(getForData(m, "harvestedArea", r, FT_ALL, DIAM_ALL, y), ra, rtol=1e-8), f"harvestedArea = regArea ({r}, {y})")
                chk(approx(forArea(m, r, y), forArea(m, r, Y_START - 1), rtol=1e-10), f"constant forArea ({r}, {y})")
            for p in priPr(m) + secPr(m):
                chk(approx(getProdData(m, "st", r, p, y), getProdData(m, "sl", r, p, y) + getProdData(m, "sa", r, p, y), rtol=1e-10), f"st = sl + sa ({r}, {p}, {y})")
                chk(approx(getProdData(m, "dt", r, p, y), getProdData(m, "dl", r, p, y) + getProdData(m, "da", r, p, y), rtol=1e-10), f"dt = dl + da ({r}, {p}, {y})")
                pl = getProdData(m, "pl", r, p, y)
                chk(pl == pl and pl not in (float("inf"),) and pl > 0, f"positive price ({r}, {p}, {y})")
            for ft in fts(m):
                v = getForData(m, "vol", r, ft, DIAM_ALL, y)
                chk(v == v and v >= 0 and v != float("inf"), f"finite non negative volume ({r}, {ft}, {y})")
    # pixel level (last year): the harvested volume by product adds up to the harvested volume
    hvp = hv = 0.0
    for px in allPx(m):
        for v in px.hVol_byPrd:
            for x in v:
                for z in x:
                    hvp += z
        for v in px.hVol:
            for z in v:
                hv += z
    chk(approx(hvp, hv, rtol=1e-8), "pixel hVol_byPrd = hVol")
    chk(approx(hv, harvest(m, Y_END), rtol=1e-8), "pixel hVol = hV")
    # pixel areas are consistent with the regional forest area
    if pixelArea:
        for r in REGIONS:
            chk(approx(pxArea(m, r), forArea(m, r, Y_END), rtol=1e-8), f"pixel area = forArea ({r})")


def test_base(B):
    needs("t_base")
    chk = Check()
    checkInvariants(chk, B)
    # the inventory is growing over the period (used below to sign the effect of the elasticities)
    chk(fv(B, "vol", Y_END) > fv(B, "vol", Y_START), "growing inventory")
    # heterogeneous agents in the base: expectations and risk aversion vary by pixel
    chk(len({px.expType for px in allPx(B)}) > 1, "heterogeneous expType")
    chk(any(any(a < b for a, b in zip(px.expectedReturns, px.expectedReturnsNotCorrByRa)) for px in allPx(B)), "risk aversion correction")
    chk(any(any(s != 1.0 for s in px.spMods) for px in allPx(B)), "spatial modifiers")
    chk(all(getForData(B, "vMortAdd", r, FT_ALL, DIAM_ALL, y) == 0 for r in REGIONS for y in range(Y_START, Y_END + 1)), "no additional mortality")
    chk.done()


# --- management and agents' behaviour ---

def test_mr0(B):
    needs("t_mr0")
    chk = Check()
    m = runScenario("t_mr0")
    checkInvariants(chk, m)
    # with no managed regeneration everything is replanted with the same forest type
    for r in REGIONS:
        chk(approx(sameTypeRegShare(m, r), 1.0, atol=1e-10), f"same type share = 1 ({r})")
    chk.done()


def test_mr1(B):
    needs("t_mr1")
    chk = Check()
    m = runScenario("t_mr1")
    checkInvariants(chk, m)
    # all the regeneration follows the expected returns: more forest type changes
    for r in REGIONS:
        chk(sameTypeRegShare(m, r) < sameTypeRegShare(B, r), f"lower same type share ({r})")
    chk.done()


def test_mrTime(B):
    needs("t_mrTime")
    chk = Check()
    m = runScenario("t_mrTime")
    # mr drops to 0.1 from 2016: mostly natural (same type) regeneration at the end of the period
    for r in REGIONS:
        chk(sameTypeRegShare(m, r) > sameTypeRegShare(B, r), f"higher same type share ({r})")
        chk(sameTypeRegShare(m, r) > 0.8, f"same type share > 0.8 ({r})")
    chk.done()


def test_mrRegional(B):
    needs("t_mrRegional")
    chk = Check()
    m = runScenario("t_mrRegional")
    # forData mr (0.1) in 11001 has priority over the setting; regional setting 0.95 in 11002
    chk(sameTypeRegShare(m, 11001) > sameTypeRegShare(B, 11001), "11001 higher same type share")
    chk(sameTypeRegShare(m, 11002) < sameTypeRegShare(B, 11002), "11002 lower same type share")
    chk(approx(sameTypeRegShare(m, 11001), sameTypeRegShare(runScenario("t_mrTime"), 11001), rtol=0.05), "11001 as t_mrTime")
    chk.done()


def test_noRA(B):
    needs("t_noRA")
    chk = Check()
    m = runScenario("t_noRA")
    checkInvariants(chk, m)
    # no correction of the expected returns for risk
    chk(all(px.expectedReturns == px.expectedReturnsNotCorrByRa for px in allPx(m)), "no risk correction")
    # the forest type choice changes, but not dramatically
    chk(getForData(m, "regArea", 11001, "Fut_Con", "", Y_END) != getForData(B, "regArea", 11001, "Fut_Con", "", Y_END), "regArea changes")
    chk(abs(relchange(fv(m, "regArea", Y_END, ft="Fut_Con"), fv(B, "regArea", Y_END, ft="Fut_Con"))) < 0.2, "regArea change < 20%")
    chk.done()


def test_expectations(B):
    needs("t_expNone", "t_expFull", "t_expFixed")
    chk = Check()
    mn, mf, mx = runScenario("t_expNone"), runScenario("t_expFull"), runScenario("t_expFixed")
    chk(all(px.expType == 0.0 and px.expTypePrices == 0.0 for px in allPx(mn)), "expType 0")
    chk(all(px.expType == 1.0 and px.expTypePrices == 1.0 for px in allPx(mf)), "expType 1")
    chk(all(px.expType == -1.0 for px in allPx(mx)), "expType -1")
    # the base (expType 0.5 +- sd) mixes the adaptive and the full expectations
    for r in REGIONS:
        e0, e1, eb = (getForData(x, "expReturns", r, "", "", Y_END) for x in (mn, mf, B))
        chk(e0 != e1, f"different expected returns ({r})")
        chk(min(e0, e1) <= eb <= max(e0, e1), f"base between adaptive and full ({r})")
    # expectations change the investment choices, not the short-term market
    for x in (mn, mf, mx):
        chk(abs(relchange(harvest(x, Y_END), harvest(B, Y_END))) < 0.02, "harvest change < 2%")
    chk.done()


def test_noSpVar(B):
    needs("t_noSpVar")
    chk = Check()
    m = runScenario("t_noSpVar")
    checkInvariants(chk, m)
    chk(all(all(s == 1.0 for s in px.spMods) for px in allPx(m)), "spMods = 1")
    chk.done()


def test_spPropVol(B):
    needs("t_spPropVol")
    chk = Check()
    m = runScenario("t_spPropVol")
    checkInvariants(chk, m)
    sm = [s for px in allPx(m) for s in px.spMods]
    chk(all(within(s, 0.4, 1.6) for s in sm), "spMods in [0.4, 1.6]")
    chk(sm != [s for px in allPx(B) for s in px.spMods], "spMods differ from the base")
    chk(within(sum(sm) / len(sm), 0.6, 1.4), "spMods mean in [0.6, 1.4]")
    chk.done()


def test_natRegPP(B):
    needs("t_natRegPP")
    chk = Check()
    m = runScenario("t_natRegPP")
    checkInvariants(chk, m)
    # the natural regeneration goes to the forest types by probability of presence, not to the same type
    for r in REGIONS:
        chk(sameTypeRegShare(m, r) < sameTypeRegShare(B, r), f"lower same type share ({r})")
    chk.done()


def test_regFixed(B):
    needs("t_regFixed")
    chk = Check()
    m = runScenario("t_regFixed")
    checkInvariants(chk, m)
    # the regeneration volumes stay at their exogenous (input) value, while in the base they become endogenous
    # once the time of passage of the first class has elapsed (2017-2018 for the fast types)
    for ft in fts(m):
        chk(all(approx(getForData(m, "vReg", 11001, ft, "", y), getForData(B, "vReg", 11001, ft, "", Y_START), rtol=1e-12)
                for y in range(Y_START, Y_END + 1)), f"constant vReg ({ft})")
    chk(fv(B, "vReg", Y_END) != fv(B, "vReg", Y_START), "endogenous vReg in the base")
    chk(fv(m, "vReg", Y_START) == fv(B, "vReg", Y_START), "same initial vReg")
    # little effect on the market within the test period
    chk(harvest(m, Y_START) == harvest(B, Y_START), "same first year harvest")
    chk(abs(relchange(harvest(m, Y_END), harvest(B, Y_END))) < 0.01, "harvest change < 1%")
    chk.done()


def test_oldVol2Area(B):
    needs("t_oldVol2Area")
    chk = Check()
    m = runScenario("t_oldVol2Area")
    checkInvariants(chk, m, pixelArea=False)
    # the old method lets more area enter the first class than what was harvested: the pixel forest area grows
    # a little (the reason of the new method), while the regional forArea stays constant
    for r in REGIONS:
        chk(within(relchange(pxArea(m, r), forArea(m, r, Y_END)), 0.0, 0.02), f"pixel area grows by 0-2% ({r})")
    chk(fv(m, "harvestedArea", Y_END) != fv(B, "harvestedArea", Y_END), "harvested area differs")
    chk(abs(relchange(harvest(m, Y_END), harvest(B, Y_END))) < 0.05, "harvest change < 5%")
    chk.done()


def test_noAvalCoef(B):
    needs("t_noAvalCoef")
    chk = Check()
    m = runScenario("t_noAvalCoef")
    checkInvariants(chk, m)
    # all the inventory is available for harvesting (avalCoef <= 1 no longer applied)
    for y in range(Y_START, Y_END + 1):
        chk(pv(m, "in", "hardWRoundW", y) >= pv(B, "in", "hardWRoundW", y), f"higher inventory ({y})")
    chk(within(relchange(pv(m, "in", "hardWRoundW", Y1), pv(B, "in", "hardWRoundW", Y1)), 0.001, 0.5), "inventory +0.1% to +50%")
    chk.done()


def test_noDeathTimber(B):
    needs("t_noDeathTimber")
    chk = Check()
    m = runScenario("t_noDeathTimber")
    checkInvariants(chk, m)
    chk(all(pv(m, "in_deathTimber", p, y) == 0 for p in priPr(m) for y in range(Y_START, Y_END + 1)), "no dead timber inventory")
    chk(all(getForData(m, "usableDeadTimber", r, "", "", y) == 0 for r in REGIONS for y in range(Y_START, Y_END + 1)), "no usable dead timber")
    chk(pv(B, "in_deathTimber", "softWRoundW", Y1) > 0, "dead timber in the base")
    # without dead timber the supply comes entirely from harvesting alive trees
    chk(harvest(m, Y1) > harvest(B, Y1), "higher harvest")
    chk(within(relchange(harvest(m, Y1), harvest(B, Y1)), 0.0, 0.15), "harvest +0-15%")
    chk.done()


# --- climate change multipliers ---

def test_ccTable(B):
    needs("t_ccTable")
    chk = Check()
    m = runScenario("t_ccTable")
    checkInvariants(chk, m)
    # faster growth: more volume; higher coniferous mortality
    chk(fv(m, "vol", Y_END) > fv(B, "vol", Y_END), "more volume")
    chk(within(relchange(fv(m, "vol", Y_END), fv(B, "vol", Y_END)), 0.005, 0.3), "volume +0.5% to +30%")
    chk(fv(m, "vMort", Y1, ft="Fut_Con") > fv(B, "vMort", Y1, ft="Fut_Con"), "higher Fut_Con mortality")
    chk(approx(fv(m, "vol", Y_SHOCK - 1), fv(B, "vol", Y_SHOCK - 1), rtol=1e-6), "no effect before 2015")
    chk.done()


def test_ccMaps(B):
    needs("t_ccMaps")
    chk = Check()
    m = runScenario("t_ccMaps")
    checkInvariants(chk, m)
    px = regPx(m, 11001)[0]
    chk(getMultiplier(m, px, "tp_multiplier", "Fut_Feu", 2014) == 1.0, "tp multiplier 2014")
    chk(getMultiplier(m, px, "tp_multiplier", "Fut_Feu", Y1) == 0.8, "tp multiplier Fut_Feu")
    chk(getMultiplier(m, px, "tp_multiplier", "Fut_Con", Y1) == 1.0, "tp multiplier Fut_Con")
    chk(getMultiplier(m, px, "mortCoef_multiplier", "Fut_Con", Y1) == 1.5, "mortality multiplier Fut_Con")
    chk(getMultiplier(m, px, "mortCoef_multiplier", "Fut_Feu", Y1) == 1.0, "mortality multiplier Fut_Feu")
    chk(fv(m, "vMort", Y1, ft="Fut_Con") > fv(B, "vMort", Y1, ft="Fut_Con"), "higher Fut_Con mortality")
    # broadleaved high forest becomes more attractive for the regeneration
    chk(fv(m, "regArea", Y_END, ft="Fut_Feu") > fv(B, "regArea", Y_END, ft="Fut_Feu"), "more Fut_Feu regeneration")
    chk.done()


def test_avalCoefFt(B):
    needs("t_avalCoefFt")
    chk = Check()
    m = runScenario("t_avalCoefFt")
    checkInvariants(chk, m)
    rh = pv(m, "in", "hardWRoundW", Y1) / pv(B, "in", "hardWRoundW", Y1)
    rs = pv(m, "in", "softWRoundW", Y1) / pv(B, "in", "softWRoundW", Y1)
    # halved availability: the inventory available to the market is about halved...
    chk(within(rh, 0.35, 0.65), "hardwood inventory about halved")
    # ...and much more for the coniferous products (Fut_Con at 0.2)
    chk(rs < rh, "softwood reduced more")
    chk.done()


# --- market ---

def test_worldPrices(B):
    needs("t_worldPrices")
    chk = Check()
    m = runScenario("t_worldPrices")
    checkInvariants(chk, m)
    # higher world prices: more exports of primary products, fewer imports of secondary products
    chk(sum(pv(m, "sa", p, Y1) for p in priPr(m)) > sum(pv(B, "sa", p, Y1) for p in priPr(m)), "more exports")
    chk(sum(pv(m, "da", p, Y1) for p in secPr(m)) < sum(pv(B, "da", p, Y1) for p in secPr(m)), "fewer imports")
    chk(within(relchange(harvest(m, Y1), harvest(B, Y1)), 0.0, 0.3), "harvest +0-30%")
    chk(approx(pv(m, "sa", "hardWRoundW", Y_SHOCK - 1), pv(B, "sa", "hardWRoundW", Y_SHOCK - 1), rtol=1e-6), "no effect before 2015")
    chk.done()


def test_polSupplySub(B):
    needs("t_polSupplySub")
    chk = Check()
    m = runScenario("t_polSupplySub")
    checkInvariants(chk, m)
    p, r = "hardWRoundW", 11001
    # subsidised supply in 11001: more supply, lower local price, a cost for the public budget
    chk(within(relchange(getProdData(m, "sc", r, p, Y1), getProdData(B, "sc", r, p, Y1)), 0.01, 0.5), "supply +1% to +50%")
    chk(getProdData(m, "pl", r, p, Y1) < getProdData(B, "pl", r, p, Y1), "lower price")
    chk(getProdData(m, "polBal_mktDirInt_s", r, p, Y1) < 0, "public cost")
    chk(getProdData(B, "polBal_mktDirInt_s", r, p, Y1) == 0, "no public cost in the base")
    # the non subsidised region loses market share
    chk(getProdData(m, "sc", 11002, p, Y1) < getProdData(B, "sc", 11002, p, Y1), "lower supply in 11002")
    chk.done()


def test_polDemandSub(B):
    needs("t_polDemandSub")
    chk = Check()
    m = runScenario("t_polDemandSub")
    checkInvariants(chk, m)
    p = "hardWSawnW"
    chk(within(relchange(pv(m, "dc", p, Y1), pv(B, "dc", p, Y1)), 0.01, 0.5), "consumption +1% to +50%")
    chk(getProdData(m, "pl", 11001, p, Y1) > getProdData(B, "pl", 11001, p, Y1), "higher price")
    chk(pv(m, "polBal_mktDirInt_d", p, Y1) < 0, "public cost")
    chk.done()


def test_polTrSub(B):
    needs("t_polTrSub")
    chk = Check()
    m = runScenario("t_polTrSub")
    checkInvariants(chk, m)
    # cheaper transformation: more sawnwood produced, more demand (and price) of its raw material
    chk(within(relchange(pv(m, "sl", "softWSawnW", Y1), pv(B, "sl", "softWSawnW", Y1)), 0.002, 0.5), "production +0.2% to +50%")
    chk(getProdData(m, "pl", 11001, "softWRoundW", Y1) > getProdData(B, "pl", 11001, "softWRoundW", Y1), "higher raw material price")
    chk(pv(m, "polBal_trSub", "softWSawnW", Y1) < 0, "public cost")
    chk.done()


def test_polTcSub(B):
    needs("t_polTcSub")
    chk = Check()
    m = runScenario("t_polTcSub")
    checkInvariants(chk, m)

    def rtTot(x, y):
        s = 0.0
        for r in REGIONS:
            for r2 in REGIONS:
                for p in priPr(x) + secPr(x):
                    s += getProdData(x, "rt", r, p, y, str(r2))
        return s
    # cheaper transport: more interregional trade
    chk(rtTot(m, Y1) > rtTot(B, Y1), "more trade")
    chk(approx(rtTot(m, Y_SHOCK - 1), rtTot(B, Y_SHOCK - 1), rtol=1e-3), "no effect before 2015")
    chk.done()


def test_polFiSub(B):
    needs("t_polFiSub")
    chk = Check()
    m = runScenario("t_polFiSub")
    checkInvariants(chk, m)
    # subsidised investments towards Fut_Con: more Fut_Con regeneration, public expenditure
    chk(fv(m, "regArea", Y_END, ft="Fut_Con") > fv(B, "regArea", Y_END, ft="Fut_Con"), "more Fut_Con regeneration")
    chk(within(relchange(fv(m, "regArea", Y_END, ft="Fut_Con"), fv(B, "regArea", Y_END, ft="Fut_Con")), 0.0, 0.5), "regeneration +0-50%")
    chk(fv(m, "polBal_fiSub", Y1, ft="Fut_Con") < 0, "public cost")
    chk.done()


def test_mktStr(B):
    needs("t_mktStr")
    chk = Check()
    m = runScenario("t_mktStr")
    checkInvariants(chk, m)
    # less competitive markets: less traded, less harvested
    chk(sum(pv(m, "dc", p, Y1) for p in secPr(m)) < sum(pv(B, "dc", p, Y1) for p in secPr(m)), "lower consumption")
    chk(within(relchange(harvest(m, Y1), harvest(B, Y1)), -0.6, -0.05), "harvest -5% to -60%")
    chk.done()


def test_substitution(B):
    needs("t_substitution")
    chk = Check()
    m = runScenario("t_substitution")
    checkInvariants(chk, m)
    # NB: es_d enters the demand as (own price / substitute price)^es_d (see tests.md for the sign)
    chk(pv(m, "dc", "fuelW", Y1) != pv(B, "dc", "fuelW", Y1), "fuelwood consumption changes")
    chk(abs(relchange(pv(m, "dc", "fuelW", Y1), pv(B, "dc", "fuelW", Y1))) < 0.3, "fuelwood change < 30%")
    # hardwood and softwood sawnwood move in opposite directions
    dh = relchange(pv(m, "dc", "hardWSawnW", Y_END), pv(B, "dc", "hardWSawnW", Y_END))
    ds = relchange(pv(m, "dc", "softWSawnW", Y_END), pv(B, "dc", "softWSawnW", Y_END))
    chk(dh * ds < 0, "opposite directions")
    chk.done()


def test_addDemand(B):
    needs("t_addDemand")
    chk = Check()
    m = runScenario("t_addDemand")
    checkInvariants(chk, m)
    p = "hardWSawnW"

    def dcDiff(y):
        return getProdData(m, "dc", 11001, p, y) - getProdData(B, "dc", 11001, p, y)
    # 0.3 Mm3 given once in 2015 (then 0): a permanent increase of the consumption in 11001, a bit less than 0.3
    # because the price goes up
    diffs = [dcDiff(y) for y in range(Y_SHOCK, Y_END + 1)]
    chk(dcDiff(Y_SHOCK - 1) == 0, "no effect before 2015")
    chk(all(within(x, 0.05, 0.3) for x in diffs), "extra consumption 0.05-0.3")
    chk(max(diffs) / min(diffs) < 1.2, "roughly constant")
    chk(all(getProdData(m, "pl", 11001, p, y) > getProdData(B, "pl", 11001, p, y) for y in range(Y_SHOCK, Y_END + 1)), "higher price")
    chk(harvest(m, Y1) > harvest(B, Y1), "higher harvest")
    chk.done()


def test_addDemandShock(B):
    needs("t_addDemandShock")
    chk = Check()
    m, mp = runScenario("t_addDemandShock"), runScenario("t_addDemand")
    checkInvariants(chk, m)
    p = "hardWSawnW"

    def dcDiff(x, y):
        return getProdData(x, "dc", 11001, p, y) - getProdData(B, "dc", 11001, p, y)
    # 0.3 in 2015, -0.3 in 2016: same as the permanent shift in 2015, then the shock is withdrawn
    chk(dcDiff(m, Y_SHOCK) == dcDiff(mp, Y_SHOCK), "as t_addDemand in 2015")
    chk(all(dcDiff(m, y) < 0.5 * dcDiff(mp, y) for y in range(Y_SHOCK + 2, Y_END + 1)), "withdrawn from 2017")
    chk(all(getProdData(m, "pl", 11001, p, y) < getProdData(mp, "pl", 11001, p, y) for y in range(Y_SHOCK + 1, Y_END + 1)), "lower price than t_addDemand")
    chk.done()


@pytest.mark.xfail(reason="the withdrawal of a single-year additional demand is only partial (see tests.md)", strict=True)
def test_addDemandShock_fullWithdrawal(B):
    needs("t_addDemandShock")
    m = runScenario("t_addDemandShock")
    # the curve recalibrated on the 2015 consumption includes the shock as price-elastic demand (scaled ~1.7x
    # instead of shifted by 0.3); -0.3 doesn't fully undo it: +0.08 Mm3 (~18% of the base consumption) remain
    assert abs(getProdData(m, "dc", 11001, "hardWSawnW", Y_END) - getProdData(B, "dc", 11001, "hardWSawnW", Y_END)) < 0.02


def test_elasticity(B):
    needs("t_elasticity")
    chk = Check()
    m = runScenario("t_elasticity")
    checkInvariants(chk, m)
    # the inventory grows (t_base): a supply more elastic to it gives more harvest
    chk(harvest(m, Y_END) > harvest(B, Y_END), "higher harvest")
    chk(within(relchange(harvest(m, Y_END), harvest(B, Y_END)), 0.0, 0.2), "harvest +0-20%")
    chk.done()


# --- carbon ---

def test_carbonPrice(B):
    needs("t_carbonPrice")
    chk = Check()
    m = runScenario("t_carbonPrice")
    checkInvariants(chk, m)
    # carbon emissions of the harvest are a cost for the producers: less harvest, more stock
    chk(harvest(m, Y1) < harvest(B, Y1), "lower harvest")
    chk(within(relchange(harvest(m, Y1), harvest(B, Y1)), -0.2, 0.0), "harvest 0 to -20%")
    chk(sum(getStock(m, r, STOCK_INV) for r in REGIONS) > sum(getStock(B, r, STOCK_INV) for r in REGIONS), "higher stock")
    chk(all(all(v == 0 for v in px.expectedAnnualIncome_carbon) for px in allPx(m)), "no Hartman payments")
    chk.done()


def test_hartman(B):
    needs("t_hartmanExo", "t_hartmanEndo", "t_pickling")
    chk = Check()
    mc, me, mn, mp = runScenario("t_carbonPrice"), runScenario("t_hartmanExo"), runScenario("t_hartmanEndo"), runScenario("t_pickling")
    for x in (me, mn, mp):
        checkInvariants(chk, x)
        # carbon payments enter the expected returns (and the forest type choice)...
        chk(any(any(v > 0 for v in px.expectedAnnualIncome_carbon) for px in allPx(x)), "carbon income")
        chk(getForData(x, "expReturns", 11001, "", "", Y_END) > getForData(mc, "expReturns", 11001, "", "", Y_END), "higher expected returns")
        # ...but not the short-term market (the regeneration has not reached the market yet)
        chk(approx(harvest(x, Y1), harvest(mc, Y1), rtol=1e-6), "same short-term harvest")
    chk(getForData(me, "expReturns", 11001, "", "", Y_END) != getForData(mn, "expReturns", 11001, "", "", Y_END), "EXO != ENDO")
    # with pickling part of the harvested carbon stays stored: less penalty for harvesting
    chk(getForData(mp, "expReturns", 11001, "", "", Y_END) > getForData(me, "expReturns", 11001, "", "", Y_END), "pickling raises returns")
    chk.done()


# --- disturbances ---

def test_windstorm(B):
    needs("t_windstorm")
    chk = Check()
    m = runScenario("t_windstorm")
    checkInvariants(chk, m)
    # additional mortality only in 11001, only in 2016
    chk(getForData(m, "vMortAdd", 11001, FT_ALL, DIAM_ALL, 2016) > 0, "additional mortality in 11001")
    chk(getForData(m, "vMortAdd", 11002, FT_ALL, DIAM_ALL, 2016) == 0, "none in 11002")
    chk(all(getForData(m, "vMortAdd", 11001, FT_ALL, DIAM_ALL, y) == 0 for y in range(Y_START, Y_END + 1) if y != 2016), "only in 2016")
    chk(within(getForData(m, "vMortAdd", 11001, FT_ALL, DIAM_ALL, 2016) / getForData(B, "vol", 11001, FT_ALL, DIAM_ALL, 2015), 0.01, 0.3), "1-30% of the stock")
    # the dead timber feeds the market: less alive trees harvested the year after
    chk(getForData(m, "usableDeadTimber", 11001, "", "", 2016) > 10 * getForData(B, "usableDeadTimber", 11001, "", "", 2016), "dead timber > 10x")
    chk(pv(m, "in_deathTimber", "softWRoundW", 2017, regions=(11001,)) > pv(B, "in_deathTimber", "softWRoundW", 2017, regions=(11001,)), "more dead timber supply")
    chk(getForData(m, "hV", 11001, FT_ALL, DIAM_ALL, 2017) < getForData(B, "hV", 11001, FT_ALL, DIAM_ALL, 2017), "less alive harvest")
    chk(fv(m, "vol", Y_END, regions=(11001,)) < fv(B, "vol", Y_END, regions=(11001,)), "less volume")
    # before the storm only a small anticipation effect (agents with full expectations read the future layers)
    chk(approx(fv(m, "vol", 2013), fv(B, "vol", 2013), rtol=1e-8), "no effect in 2013")
    chk(abs(relchange(harvest(m, 2015), harvest(B, 2015))) < 0.01, "small anticipation in 2015")
    chk.done()


def test_windstormSto(B):
    needs("t_windstormSto")
    chk = Check()
    m, mw = runScenario("t_windstormSto"), runScenario("t_windstorm")
    checkInvariants(chk, m)

    def dead(x, y):
        return getForData(x, "usableDeadTimber", 11001, "", "", y)
    # dead timber usable for 4 years instead of the master value: still available in 2018
    chk(dead(m, 2018) > dead(mw, 2018), "more dead timber in 2018")
    chk(dead(m, 2018) > 0.1 * dead(m, 2016), "dead timber still available in 2018")
    # carbon stock of the inventory (alive + dead biomass) increases with the windstorm (dead biomass)
    chk(sum(getStock(m, r, STOCK_INV) for r in REGIONS) > sum(getStock(B, r, STOCK_INV) for r in REGIONS), "higher inventory carbon stock")
    chk.done()


def test_pathogen(B):
    needs("t_pathogen")
    chk = Check()
    m = runScenario("t_pathogen")
    checkInvariants(chk, m)
    # the pathogen is present only in 11001 and increases the broadleaved mortality there
    chk(getForData(m, "totalShareInvadedArea", 11001, "", "", Y1) > 0, "invaded share in 11001")
    chk(getForData(m, "totalShareInvadedArea", 11002, "", "", Y1) == 0, "none in 11002")
    chk(getForData(m, "vMort", 11001, "Fut_Feu", DIAM_ALL, Y1) > 2 * getForData(B, "vMort", 11001, "Fut_Feu", DIAM_ALL, Y1), "Fut_Feu mortality > 2x")
    chk(approx(getForData(m, "vMort", 11002, "Fut_Feu", DIAM_ALL, Y1), getForData(B, "vMort", 11002, "Fut_Feu", DIAM_ALL, Y1), rtol=0.02), "11002 unchanged")
    chk(approx(getForData(m, "vMort", 11001, "Fut_Con", DIAM_ALL, Y1), getForData(B, "vMort", 11001, "Fut_Con", DIAM_ALL, Y1), rtol=0.02), "Fut_Con unchanged")
    # mortality coefficients increase with the years of presence (0.01, 0.03, 0.06)
    chk(getForData(m, "vMort", 11001, "Fut_Feu", DIAM_ALL, 2016) > getForData(m, "vMort", 11001, "Fut_Feu", DIAM_ALL, 2014), "increasing mortality")
    chk(getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, Y_END) < getForData(B, "vol", 11001, "Fut_Feu", DIAM_ALL, Y_END), "less Fut_Feu volume")
    chk.done()


def test_spExplicitFt(B):
    needs("t_spExplicitFt")
    chk = Check()
    m = runScenario("t_spExplicitFt")
    checkInvariants(chk, m)
    chk("Fut_Hetre" in fts(m), "Fut_Hetre exists")
    # Fut_Hetre exists only where its input maps put it (11001), taken from Fut_Feu: same total area
    chk(getForData(m, "vol", 11001, "Fut_Hetre", DIAM_ALL, Y_START) > 0, "Fut_Hetre volume in 11001")
    # in 11002 Fut_Hetre only comes from the regeneration choices (young classes only)
    chk(getForData(m, "vol", 11001, "Fut_Hetre", DIAM_ALL, Y_START) > 20 * getForData(m, "vol", 11002, "Fut_Hetre", DIAM_ALL, Y_START), "11001 >> 11002")
    chk(all(getForData(m, "vol", 11002, "Fut_Hetre", dc, Y_START) == 0 for dc in m.core.dClasses[3:]), "only young classes in 11002")
    chk(all(getForData(m, "vol", 11001, "Fut_Hetre", dc, Y_START) > 0 for dc in m.core.dClasses[3:8]), "mid classes in 11001")
    chk(getForData(m, "forArea", 11001, "Fut_Hetre", "", Y_START) > 0, "Fut_Hetre area in 11001")
    for r in REGIONS:
        chk(approx(forArea(m, r, Y_END), forArea(B, r, Y_END), rtol=1e-8), f"same total area ({r})")
    chk(getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, Y_START) < getForData(B, "vol", 11001, "Fut_Feu", DIAM_ALL, Y_START), "less Fut_Feu volume")
    chk.done()


# --- forest area changes ---

def test_areaChanges(B):
    needs("t_areaRel", "t_areaAbs", "t_areaDecr")
    chk = Check()
    mr, ma, md = runScenario("t_areaRel"), runScenario("t_areaAbs"), runScenario("t_areaDecr")
    for x in (mr, ma, md):
        checkInvariants(chk, x, fixedArea=False)
    for r in REGIONS:
        for y in range(Y_START + 1, Y_END + 1):
            chk(approx(forArea(mr, r, y) / forArea(mr, r, y - 1), 1.01, rtol=1e-3), f"+1% per year ({r}, {y})")
            chk(approx(forArea(md, r, y) / forArea(md, r, y - 1), 0.997, rtol=1e-3), f"-0.3% per year ({r}, {y})")
            chk(forArea(ma, r, y) > forArea(ma, r, y - 1), f"absolute increase ({r}, {y})")
    for r in REGIONS:
        inc = [forArea(ma, r, y) - forArea(ma, r, y - 1) for y in range(Y_START + 1, Y_END + 1)]
        chk(max(inc) / min(inc) < 1.5, f"roughly constant increments ({r})")
        chk(approx(pxArea(mr, r), forArea(mr, r, Y_END), rtol=1e-6), f"pixel areas follow ({r})")
    # new forest area is planted: more regeneration area than in the base, less with the decrease
    chk(fv(mr, "regArea", Y_END) > fv(B, "regArea", Y_END), "more regeneration")
    chk(fv(md, "regArea", Y_END) < fv(B, "regArea", Y_END), "less regeneration")
    chk.done()


# --- output options and input mechanisms ---

def test_cachedInputAndFileWithoutExtension(B, tmp_path):
    needs("t_base")
    # forData/prodData read from cachedInput/*.csv and an extra file without extension in the spatial data folder
    # (both used to fail in C++): same results as reading the ODS sheets
    import contextlib
    import io
    d = str(tmp_path)
    input_ = prepareRobustnessDataDir(d)
    with contextlib.redirect_stdout(io.StringIO()):
        ffsmp.runModel(input_, "t_base")
    for f in ("forestData_t_base.csv", "productData_t_base.csv", "carbonBalance_t_base.csv"):
        with open(os.path.join(d, "output", "results", f)) as a, open(os.path.join(outputDir(), "results", f)) as b:
            assert a.read() == b.read(), f


def test_outHR():
    needs("t_outHR")
    runScenario("t_outHR")
    f = os.path.join(outputDir(), "results", "forestData_t_outHR_renamed.csv")
    assert os.path.isfile(f)
    with open(f) as io_:
        lines = io_.read().splitlines()
    assert lines[0].startswith("scen;parName;country;region;forType;freeDim;")
    assert all(l.startswith("t_outHR_renamed;") for l in lines[1:])
    assert any(";AlsChaLor;" in l for l in lines)                                        # region names, not ids
    assert any(re.match(r"^[^;]*;vol;[^;]*;[^;]*;Fut_Feu;10;", l) for l in lines)       # by diameter class
    assert os.path.isfile(os.path.join(outputDir(), "results", "productData_t_outHR_renamed.csv"))


def test_outSingle():
    needs("t_outSingle")
    runScenario("t_outSingle")
    f = os.path.join(outputDir(), "results", "forestData.csv")
    assert os.path.isfile(f)
    with open(f) as io_:
        lines = io_.read().splitlines()
    assert lines[0] == "scen;parName;country;region;forType;freeDim;year;value;"
    assert {l.split(";")[6] for l in lines[1:] if l.startswith("t_outSingle;")} == {"2013", "2016", "2020"}
    mine = [x for x in os.listdir(os.path.join(outputDir(), "maps", "asciiGrids")) if x.endswith("_t_outSingle")]
    assert mine and all("_2016_" in x for x in mine)
    with open(os.path.join(outputDir(), "maps", "asciiGrids", "vol_Fut_Feu_2016_t_outSingle")) as io_:
        g = io_.read().splitlines()
    nc = int(g[0].split()[1])
    nr = int(g[1].split()[1])
    assert g[0].startswith("ncols")
    data = [l for l in g[6:] if l]
    assert len(data) == nr and all(len(l.split()) == nc for l in data)


def test_settingFile():
    needs("t_settingFile")
    m, m1 = runScenario("t_settingFile"), runScenario("t_mr1")
    # same settings as t_mr1, read from a csv file: same results
    assert harvest(m, Y_END) == harvest(m1, Y_END)
    assert all(sameTypeRegShare(m, r) == sameTypeRegShare(m1, r) for r in REGIONS)
    assert fv(m, "regArea", Y_END, ft="Fut_Con") == fv(m1, "regArea", Y_END, ft="Fut_Con")


def test_newSeed(B):
    needs("t_newSeed")
    chk = Check()
    m = runScenario("t_newSeed")
    checkInvariants(chk, m)
    # different random draws (agents' characteristics), statistically equivalent results
    chk([px.expType for px in allPx(m)] != [px.expType for px in allPx(B)], "different draws")
    chk(abs(relchange(harvest(m, Y_END), harvest(B, Y_END))) < 0.02, "harvest within 2%")
    chk(abs(relchange(fv(m, "vol", Y_END), fv(B, "vol", Y_END))) < 0.02, "volume within 2%")
    chk.done()


def test_shuffle(B):
    needs("t_shuffle")
    chk = Check()
    m = runScenario("t_shuffle")
    checkInvariants(chk, m)
    # every layer read from file is a permutation of the original one
    for layer in ("dtm", "regLev_2"):
        a, b = list(B.GIS.values[layer]), list(m.GIS.values[layer])
        chk(sorted(a) == sorted(b), f"permutation ({layer})")
        chk(a != b, f"shuffled ({layer})")
    # the regions are reshuffled too: different forest areas, the model still runs consistently
    chk(pxArea(m, 11001) != pxArea(B, 11001), "different region areas")
    chk.done()
