# Port of ModelCoreSpatial: the yearly dynamic of the spatial model (market, forest resources and
# management modules) and the initialisation period.
#
# Indexes are 1-based here; the "optimal diameter class" and "optimal forest type" values stored in
# the pixels (optDc, optFtChosen, optDcChosen) are kept 0-based, as in the C++ code (they are reported
# as such in the debug output).

mutable struct CoreState
    firstYear::Int
    secondYear::Int
    thirdYear::Int
    WL2::Int
    regIds2::Vector{Int}
    priProducts::Vector{String}
    secProducts::Vector{String}
    allProducts::Vector{String}
    dClasses::Vector{String}
    pDClasses::Vector{String}
    fTypes::Vector{String}
    l2r::Vector{Vector{Int}}
    regType::String
    natRegAllocation::String
    rescaleFrequencies::Bool
    oldVol2AreaMethod::Bool
    forestAreaChangeMethod::String
    ir::Float64
    cumOverHarvesting::Float64               # a static variable in ModelCoreSpatial::runMarketModule()
    original_vols::Vector{Vector{Vector{Float64}}}  # static in loadExogenousForestLayers()
end
CoreState() = CoreState(0, 0, 0, 0, Int[], String[], String[], String[], String[], String[], String[], Vector{Int}[],
    "", "", false, false, "", 0.0, 0.0, Vector{Vector{Float64}}[])

# convenient handles
@inline gpd(m, t, r, p, y = DATA_NOW, fd = "") = getProdData(m, t, r, p, y, fd)
@inline gfd(m, t, r, ft, fd, y = DATA_NOW) = getForData(m, t, r, ft, fd, y)
@inline spd(m, v, t, r, p, y = DATA_NOW, allowCreate = false, fd = "") = setProdData!(m, v, t, r, p, y, allowCreate, fd)
@inline sfd(m, v, t, r, ft, fd, y = DATA_NOW, allowCreate = false) = setForData!(m, v, t, r, ft, fd, y, allowCreate)
@inline app(m, pr, ft, dc) = assessProdPossibility(m.MD, pr, ft, dc)
"app() as a double (as when the C++ bool is used in arithmetic)"
@inline appf(m, pr, ft, dc) = app(m, pr, ft, dc) ? 1.0 : 0.0

"C++ conversion of a double to an int function argument (truncation toward zero)"
@inline yint(x::Real) = unsafe_trunc(Int, x)

regionPixels(m, regId::Integer) = regPixels(m, getRegion(m.MD, regId))

function runInitPeriod!(m)
    cacheCoreSettings!(m)
    cacheDynamicSettings!(m)
    initializePixelVolumes!(m)
    assignSpMultiplierPropToVols!(m)
    initMarketModule!(m)
    initialiseDeathTimber!(m)
    printOutput!(m, true)
    m.SCD.year += 1
    msgOut(MSG_INFO, "### 2013 $(m.SCD.year)  year started..")
    resetPixelValues!(m)
    cachePixelExogenousData!(m)
    computeInventary!(m)
    computeCumulativeData!(m)
    initializePixelArea!(m)
    runBiologicalModule!(m)
    runManagementModule!(m)
    updateMapAreas!(m)
    updateOtherMapData!(m)
    sumRegionalForData!(m)
    initialiseCarbonModule!(m)
    printOutput!(m, true)   # Output::print() has earlyPrint=true as default argument
end

function runSimulationYear!(m)
    cacheDynamicSettings!(m)
    resetPixelValues!(m)
    cachePixelExogenousData!(m)
    computeInventary!(m)
    runMarketModule!(m)
    computeCumulativeData!(m)
    cachePixelExogenousData!(m)
    runBiologicalModule!(m)
    runManagementModule!(m)
    updateMapAreas!(m)
    updateOtherMapData!(m)
    sumRegionalForData!(m)
    registerCarbonEvents!(m)
    computeEconomicBalances!(m)
    printOutput!(m, false)
end

function cacheCoreSettings!(m)
    msgOut(MSG_INFO, "Cashing initial model settings..")
    c = m.core
    c.firstYear = getIntSetting(m, "initialYear")
    c.secondYear = c.firstYear + 1
    c.thirdYear = c.firstYear + 2
    c.WL2 = getIntSetting(m, "worldCodeLev2")
    c.regIds2 = getRegionIds(m.MD, 2)
    c.priProducts = getStringVectorSetting(m, "priProducts")
    c.secProducts = getStringVectorSetting(m, "secProducts")
    c.allProducts = vcat(c.priProducts, c.secProducts)
    c.dClasses = getStringVectorSetting(m, "dClasses")
    c.pDClasses = c.dClasses[2:end]
    c.fTypes = getForTypeIds(m.MD)
    c.l2r = getL2r(m.MD)
end

function cacheDynamicSettings!(m)
    msgOut(MSG_INFO, "Cashing model settings that may change every year..")
    c = m.core
    c.regType = getStringSetting(m, "regType", DATA_NOW)
    c.natRegAllocation = getStringSetting(m, "natRegAllocation", DATA_NOW)
    c.rescaleFrequencies = getBoolSetting(m, "rescaleFrequencies", DATA_NOW)
    c.oldVol2AreaMethod = getBoolSetting(m, "oldVol2AreaMethod", DATA_NOW)
    c.forestAreaChangeMethod = getStringSetting(m, "forestAreaChangeMethod", DATA_NOW)
    c.ir = getDoubleSetting(m, "ir", DATA_NOW)
end

# ------------------------------------------------------------------------------
# Market module
# ------------------------------------------------------------------------------

function initMarketModule!(m)
    msgOut(MSG_INFO, "Starting market module (init stage)..")
    c = m.core
    sy, fy, WL2 = c.secondYear, c.firstYear, c.WL2
    for r2 in c.regIds2
        for sp in c.secProducts
            value = 0.0
            for pp in c.priProducts
                value += gpd(m, "pl", r2, pp, sy) * gpd(m, "a", r2, pp, sy, sp)
            end
            value += gpd(m, "m", r2, sp, sy) * gpd(m, "pol_trSub", r2, sp, sy)
            spd(m, value, "pl", r2, sp, sy, true)
        end
        for pp in c.priProducts
            value = 0.0
            for sp in c.secProducts
                value += gpd(m, "sl", r2, sp, sy) * gpd(m, "a", r2, pp, sy, sp)
            end
            spd(m, value, "dl", r2, pp, sy, true)
            stvalue = gpd(m, "sl", r2, pp, sy) + gpd(m, "sa", r2, pp, sy)
            spd(m, stvalue, "st", r2, pp, sy, true)
            supply_fromForestShare = gpd(m, "supply_fromForestShare", r2, pp, sy)
            spd(m, stvalue * supply_fromForestShare, "st_fromHarv", r2, pp, sy, true)
        end
        for ap in c.allProducts
            dtvalue = gpd(m, "dl", r2, ap, sy) + gpd(m, "da", r2, ap, sy)
            spd(m, dtvalue, "dt", r2, ap, sy, true)
        end
        for sp in c.secProducts
            psi = gpd(m, "psi", r2, sp, sy)
            dl = gpd(m, "dl", r2, sp, sy)
            da = gpd(m, "da", r2, sp, sy)
            pl = gpd(m, "pl", r2, sp, sy)
            sa = gpd(m, "sa", r2, sp, sy)
            sl = gpd(m, "sl", r2, sp, sy)
            k1 = gpd(m, "k1", r2, sp, sy)
            pWo = gpd(m, "pl", WL2, sp, sy)
            stvalue = sl + sa
            q1 = 1 / (1 + cpow(dl / da, 1 / psi) * (pl / pWo))
            p1 = 1 - q1
            dc = cpow(q1 * cpow(da, (psi - 1) / psi) + p1 * cpow(dl, (psi - 1) / psi), psi / (psi - 1))
            pc = (da / dc) * pWo + (dl / dc) * pl
            pw = (dl * pl + da * pWo) / (dl + da)
            k = k1 * sl
            spd(m, stvalue, "st", r2, sp, sy, true)
            spd(m, q1, "q1", r2, sp, fy, true)
            spd(m, dc, "dc", r2, sp, sy, true)
            spd(m, pc, "pc", r2, sp, sy, true)
            spd(m, pw, "pw", r2, sp, sy, true)
            spd(m, k, "k", r2, sp, sy, true)
        end
        for pp in c.priProducts
            sl = gpd(m, "sl", r2, pp, sy)
            sa = gpd(m, "sa", r2, pp, sy)
            eta = gpd(m, "eta", r2, pp, sy)
            pl = gpd(m, "pl", r2, pp, sy)
            pWo = gpd(m, "pl", WL2, pp, sy)
            t1 = 1 / (1 + (cpow(sl / sa, 1 / eta)) * (pl / pWo))
            r1 = 1 - t1
            sc = cpow(t1 * cpow(sa, (eta - 1) / eta) + r1 * cpow(sl, (eta - 1) / eta), eta / (eta - 1))
            pc = (sa / sc) * pWo + (sl / sc) * pl
            pw = (sl * pl + sa * pWo) / (sl + sa)
            spd(m, t1, "t1", r2, pp, fy, true)
            spd(m, sc, "sc", r2, pp, sy, true)
            spd(m, pc, "pc", r2, pp, sy, true)
            spd(m, pw, "pw", r2, pp, sy, true)
        end
    end
    for regs in c.l2r, rFrom in regs
        for p in c.allProducts, rTo in regs
            spd(m, 0, "rt", rFrom, p, sy, true, i2s(rTo))
            rFrom == rTo && spd(m, 1, "ct", rFrom, p, fy, true, i2s(rTo))
        end
        for rTo in regs
            rFrom == rTo && spd(m, 0, "dist", rFrom, "", fy, true, i2s(rTo))
        end
    end
end

function runMarketModule!(m)
    msgOut(MSG_INFO, "Starting market module")
    c = m.core
    MD = m.MD
    thisYear = m.SCD.year
    previousYear = thisYear - 1
    useDeathTimber = getBoolSetting(m, "useDeathTimber", DATA_NOW)
    intRate = getDoubleSetting(m, "ir")

    # *** pre-optimisation yearly operations..
    for r2 in c.regIds2
        for sp in c.secProducts
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
            aa = (sigma / (sigma + 1)) * pc_1 * cpow(dc_1, -1 / sigma) * pol_d_1 / pol_d
            gg = dc_1 * cpow(pc_1 * pol_d_1, -sigma)
            spd(m, k, "k", r2, sp)
            spd(m, aa, "aa", r2, sp, DATA_NOW, true)
            spd(m, gg, "gg", r2, sp, DATA_NOW, true)
            spd(m, q1_polCorr, "q1_polCorr", r2, sp, DATA_NOW, true)
        end
        for pp in c.priProducts
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
            if thisYear == getIntSetting(m, "initialOptYear")
                in_d_1 = gpd(m, "in_deathTimber", r2, pp, thisYear)
            else
                in_d_1 = gpd(m, "in_deathTimber", r2, pp, previousYear)
            end
            min_in_d = cmax(0.00001, in_0 * getDoubleSetting(m, "minShareDeathIn"))
            pol_s = gpd(m, "pol_mktDirInt_s", r2, pp)
            pol_s_1 = gpd(m, "pol_mktDirInt_s", r2, pp, previousYear)
            t1 = gpd(m, "t1", r2, pp)
            pol_mktStr_s = gpd(m, "pol_mktStr_s", r2, pp)
            carbonPrice_1 = gpd(m, "carbonPrice", r2, "", previousYear)
            omega = gpd(m, "co2content_products", r2, pp) / 1000
            Pct_1 = carbonPrice_1 * intRate
            t1_polCorr = cmin(1.0, (t1 - 0.5) * pol_mktStr_s + 0.5)
            gamma = inv > in_1 ? gamma_incr : gamma_decr
            gamma_d = in_d > in_d_1 ? gamma_d_incr : gamma_d_decr
            in_ratio = inv / in_1
            in_d_ratio = useDeathTimber ? cmax(min_in_d, in_d) / cmax(min_in_d, in_d_1) : 1.0
            ff = sc_1 * cpow((pc_1 - omega * Pct_1) * pol_s_1, -sigma) * cpow(in_ratio, gamma) * cpow(in_d_ratio, gamma_d) * cpow(pol_s, sigma)
            bb = cpow(ff, -1.0 / sigma) * (sigma / (sigma + 1.0))
            spd(m, bb, "bb", r2, pp, DATA_NOW, true)
            spd(m, ff, "ff", r2, pp, DATA_NOW, true)
            spd(m, sigma, "sigma", r2, pp, DATA_NOW, true)
            spd(m, t1_polCorr, "t1_polCorr", r2, pp, DATA_NOW, true)
        end
    end

    # *** optimisation..
    solved, iters, objv = solveMarket!(m)
    if solved
        @printf("\n*** The problem solved year %d in %d iterations!\n", thisYear, iters)
        @printf("\n*** The final value of the objective function is %e.\n", objv)
        msgOut(MSG_INFO, "The problem solved successfully year $thisYear in $iters iterations.")
        printOptLog(m, true, iters, objv)
    else
        println("***ERROR: MODEL DIDN'T SOLVE FOR THIS YEAR ($thisYear)")
        printOptLog(m, false, 0, 0.0)
        msgOut(MSG_CRITICAL_ERROR, "Model DIDN'T SOLVE for this year ($thisYear)")
    end

    # *** post optimisation..
    priPrCombs = createCombinationsVector(length(c.priProducts))
    WL2 = c.WL2
    for regId in c.regIds2
        REG = getRegion(MD, regId)
        for p in c.allProducts
            st = gpd(m, "sl", regId, p) + gpd(m, "sa", regId, p)
            dt = gpd(m, "dl", regId, p) + gpd(m, "da", regId, p)
            spd(m, st, "st", regId, p)
            spd(m, st, "st_or", regId, p, DATA_NOW, true)
            spd(m, dt, "dt", regId, p)
        end
        for p in c.secProducts
            dl = gpd(m, "dl", regId, p)
            pl = gpd(m, "pl", regId, p)
            da = gpd(m, "da", regId, p)
            pworld = gpd(m, "pl", WL2, p)
            dt = gpd(m, "dt", regId, p)
            pw = dt != 0 ? (dl * pl + da * pworld) / dt : 0.0
            spd(m, pw, "pw", regId, p)
        end
        for p in c.priProducts
            sl = gpd(m, "sl", regId, p)
            pl = gpd(m, "pl", regId, p)
            sa = gpd(m, "sa", regId, p)
            pworld = gpd(m, "pl", WL2, p)
            st = gpd(m, "st", regId, p)
            st_fromFor = st * gpd(m, "supply_fromForestShare", regId, p)
            pw = st != 0 ? (sl * pl + sa * pworld) / st : 0.0
            spd(m, pw, "pw", regId, p)
            spd(m, st_fromFor, "st_fromFor", regId, p, DATA_NOW, true)
        end
        # correcting st if this is over the in
        for (i, comb) in enumerate(priPrCombs)
            stMkMod = 0.0
            sumIn = REG.inResByAnyCombination[i]
            for p in comb
                stMkMod += gpd(m, "st_fromFor", regId, c.priProducts[p])
            end
            if stMkMod > sumIn
                pInvolved = join((c.priProducts[p] * "; " for p in comb))
                inV_over_hV_ratio = stMkMod != 0 ? sumIn / stMkMod : 0.0
                c.cumOverHarvesting += (stMkMod - sumIn)
                msgOut(MSG_DEBUG, "Overharvesting has happened. Year: $(thisYear)Region: $(regId)Involved products:  $(pInvolved). sumIn: $(d2s(sumIn)) stMkMod:$(d2s(stMkMod)) cumOverHarvesting: $(d2s(c.cumOverHarvesting))")
                for p in comb
                    orig = gpd(m, "st_fromFor", regId, c.priProducts[p])
                    spd(m, orig * inV_over_hV_ratio, "st_fromFor", regId, c.priProducts[p])
                end
            end
        end
        total_st = [gpd(m, "st_fromFor", regId, pp) for pp in c.priProducts]
        for pp in c.priProducts
            gpd(m, "in_deathTimber", regId, pp)
            gpd(m, "in", regId, pp)
        end
        st_fromHarv = allocateHarvesting!(m, total_st, regId)
        for (i, pp) in enumerate(c.priProducts)
            spd(m, st_fromHarv[i], "st_fromHarv", regId, pp, DATA_NOW, true)
        end
    end
    if c.cumOverHarvesting > 0.0
        msgOut(MSG_DEBUG, "Overharvesting is present. Year: $thisYear cumOverHarvesting: $(d2s(c.cumOverHarvesting))")
    end
end

"Allocate the total supply first to the usable death timber (newer to older years, higher to smaller dc) and return the remaining part, that comes from harvesting"
function allocateHarvesting!(m, total_st::Vector{Float64}, regId::Integer)
    c = m.core
    MD = m.MD
    getBoolSetting(m, "useDeathTimber", DATA_NOW) || return total_st
    total_st = copy(total_st)
    maxYears = getMaxYearUsableDeathTimber(MD)
    currentYear = m.SCD.year
    for y in (currentYear-1):-1:(currentYear - maxYears)
        for u in length(c.dClasses):-1:1
            dc = c.dClasses[u]
            for ft in c.fTypes
                allocable = getAllocableProductIdsFromDeathTimber(m, regId, ft, dc, y, currentYear - 1)
                key = (y, regId, ft, dc)
                deathTimber = deathTimberInventory_get(MD, key)
                s = 0.0
                for ap in allocable
                    s += total_st[ap]
                end
                for ap in allocable
                    share = s != 0 ? total_st[ap] / s : 0.0
                    allocated = cmin(total_st[ap], deathTimber * share)
                    deathTimberInventory_incrOrAdd!(MD, key, -allocated)
                    total_st[ap] -= allocated
                end
            end
        end
    end
    return total_st
end

# ------------------------------------------------------------------------------
# Biological (forest resources) module
# ------------------------------------------------------------------------------

getAvgAgeByDc(m, px::Pixel, ft::Int, dc::Int) = dc == length(m.core.dClasses) ? px.cumTp[ft][dc] * 1.2 : (px.cumTp[ft][dc] + px.cumTp[ft][dc+1]) / 2

function runBiologicalModule!(m)
    msgOut(MSG_INFO, "Starting resource module..")
    c = m.core
    MD = m.MD
    G = m.GIS
    thisYear = m.SCD.year
    useDeathTimber = getBoolSetting(m, "useDeathTimber", DATA_NOW)
    nDc = length(c.dClasses)
    for r2 in c.regIds2
        regId = r2
        REG = getRegion(MD, r2)
        regPx = regPixels(m, REG)
        shareMortalityUsableTimber = 0.0
        # cache of the regional values used in the pixel loop (pure look-ups)
        st_in = [(gpd(m, "st_fromHarv", r2, pp), gpd(m, "in", r2, pp)) for pp in c.priProducts]
        regForAreas = Dict{String,Float64}()
        for px in regPx
            for (j, ft) in enumerate(c.fTypes)
                hV_byDiam = Float64[]
                productivity_byDc = Float64[]
                hV_byDiamAndPrd = Vector{Vector{Float64}}()
                hArea_byDc = Float64[]
                newVol_byDiam = Float64[]
                vMort_byDc = Float64[]
                vMortAdd_byDc = Float64[]
                areasMovingUp = zeros(nDc)
                areaFirstProdClass = 0.0

                # A - regeneration
                tp_u0 = yint(px.tp[j][1])
                if thisYear == c.secondYear
                    push!(px.initialDc0Area, px.area_l[j][1])
                end
                if c.regType != "fixed" && (thisYear - c.secondYear) >= tp_u0
                    pastRegArea = getPastRegArea(px, j, thisYear - tp_u0)
                    availableArea = px.area_l[j][1]
                    vHa = px.vHa[j][2]
                    if c.oldVol2AreaMethod
                        areaFirstProdClass = pastRegArea
                    else
                        areaFirstProdClass = cmin(availableArea, pastRegArea)
                    end
                    push!(px.vReg, areaFirstProdClass * vHa / 1000000.0)
                else
                    regAreaFt = get!(() -> regionLayerSum(m, REG, "forArea_" * ft), regForAreas, ft)
                    pxArea = getDoubleValue(G, px, "forArea_" * ft, true)
                    regRegVolumes = gfd(m, "vReg", r2, ft, "")
                    newVReg = regAreaFt != 0 ? regRegVolumes * pxArea / regAreaFt : 0.0
                    push!(px.vReg, newVReg)
                    areaFirstProdClass = (1.0 / Float64(tp_u0)) * px.initialDc0Area[j]
                end

                if useDeathTimber
                    shareMortalityUsableTimber = gfd(m, "shareMortalityUsableTimber", r2, ft, "")
                else
                    shareMortalityUsableTimber = 0.0
                end

                for u in 1:nDc
                    dc = c.dClasses[u]
                    hr = 0.0
                    age = getAvgAgeByDc(m, px, j, u)
                    pastYearVol = px.vol_l[j][u]
                    hV_byPrd = Float64[]
                    hr_byPrd = Float64[]
                    for (pi, pp) in enumerate(c.priProducts)
                        st, inv = st_in[pi]
                        hr_pr = inv != 0 ? appf(m, pp, ft, dc) * st / inv : 0.0
                        push!(hr_byPrd, hr_pr)
                        hr += hr_pr
                    end
                    origHr = hr
                    hr = cmin(1.0, hr)
                    for pi in eachindex(c.priProducts)
                        hr_pr = origHr != 0 ? hr_byPrd[pi] * cmin(1.0, 1.0 / origHr) : 0.0
                        push!(hV_byPrd, hr_pr * pastYearVol * px.avalCoef[j])
                    end
                    hV = hr * pastYearVol * px.avalCoef[j]
                    push!(hV_byDiam, hV)
                    push!(hV_byDiamAndPrd, hV_byPrd)

                    tp = px.tp[j][u]
                    mort = px.mort[j][u]
                    addMort = px.addMort[j][u]
                    vReg = px.vReg[j]
                    beta = px.beta[j][u]
                    vHa = px.vHa[j][u]
                    finalHarvestFlag = gfd(m, "finalHarvestFlag", regId, ft, dc)
                    vMort = mort * pastYearVol
                    vMortAdd = addMort * pastYearVol
                    push!(vMort_byDc, vMort)
                    push!(vMortAdd_byDc, vMortAdd)
                    if useDeathTimber
                        deathTimberInventory_incrOrAdd!(MD, (thisYear, r2, ft, dc), (vMort + vMortAdd) * shareMortalityUsableTimber)
                    end
                    local vol::Float64
                    if u == 1
                        vol = 0.0
                    elseif u == 2
                        vol = cmax(0.0, (1 - 1 / tp - mort - addMort)) * pastYearVol + vReg
                    else
                        inc = (u == nDc) ? 0.0 : 1.0 / tp
                        tp_1 = px.tp[j][u-1]
                        pastYearVol_1 = px.vol_l[j][u-1]
                        vol = cmax(0.0, (1 - inc - mort - addMort) * pastYearVol - hV + (1 / tp_1) * beta * pastYearVol_1)
                    end
                    if u != 1
                        inc = (u == nDc) ? 0.0 : 1.0 / tp
                        pastArea = px.area_l[j][u]
                        areasMovingUp[u] = inc * pastArea
                        if c.oldVol2AreaMethod
                            push!(hArea_byDc, finalHarvestFlag * 1000000 * hV / vHa)
                        else
                            finalHarvestedVolumes = finalHarvestFlag * hV
                            finalHarvestedRate = pastYearVol != 0 ? finalHarvestedVolumes / pastYearVol : 0.0
                            push!(hArea_byDc, finalHarvestedRate * pastArea)
                        end
                        px.area[j][u] = cmax(0.0, px.area_l[j][u] - areasMovingUp[u] + areasMovingUp[u-1] - hArea_byDc[u])
                    else
                        areasMovingUp[u] = areaFirstProdClass
                        push!(hArea_byDc, 0.0)
                        px.area[j][u] = px.area_l[j][u] - areasMovingUp[u] - hArea_byDc[u]
                    end
                    push!(newVol_byDiam, vol)
                    productivity = hArea_byDc[u] != 0 ? (1000000.0 * hV_byDiam[u]) / (hArea_byDc[u] * age) : 0.0
                    push!(productivity_byDc, productivity)
                end
                push!(px.hVol, hV_byDiam)
                push!(px.hVol_byPrd, hV_byDiamAndPrd)
                push!(px.hArea, hArea_byDc)
                push!(px.vol, newVol_byDiam)
                push!(px.vMort, vMort_byDc)
                push!(px.vMortAdd, vMortAdd_byDc)
                push!(px.hProductivity, productivity_byDc)
            end
        end
    end
end

"ModelRegion::getValue(layer, OP_SUM)"
function regionLayerSum(m, REG::ModelRegion, layer::AbstractString)
    G = m.GIS
    s = 0.0
    for i in REG.myPixels
        s += getDoubleValue(G, G.pxVector[i], layer, true)
    end
    return s
end

"ModelRegion::getArea(): whole forest area (from the pixel->area matrix)"
function regionArea(m, REG::ModelRegion)
    s = 0.0
    for i in REG.myPixels
        s += vSum(m.GIS.pxVector[i].area)
    end
    return s
end

# ------------------------------------------------------------------------------
# Management module
# ------------------------------------------------------------------------------

function _computeExpectedPrice(curLocPrice, worldCurPrice, worldFutPrice, sl, sa, expCoef)
    fullExpWPrice = (curLocPrice * (worldFutPrice / worldCurPrice) * sl + worldFutPrice * sa) / (sa + sl)
    curWPrice = (curLocPrice * sl + worldCurPrice * sa) / (sl + sa)
    return curWPrice * (1 - expCoef) + fullExpWPrice * expCoef
end

"Volume/ha of a forest type `year` years after planting (linear interpolation between the expected dc volumes)"
function getVHaByYear(m, px::Pixel, ft::Int, year::Integer, extraBiomass_ratio::Float64, regId::Integer)
    c = m.core
    nDc = length(c.dClasses)
    for u in 1:nDc
        cumTP = px.cumTp_exp[ft][u]
        if !(year >= cumTP)
            cumTP_plus = px.cumTp_exp[ft][u]
            cumTP_minus = px.cumTp_exp[ft][u-1]
            volHa_plus = px.vHa_exp[ft][u]
            volHa_minus = px.vHa_exp[ft][u-1]
            slope = (volHa_plus - volHa_minus) / (cumTP_plus - cumTP_minus)
            return (volHa_minus + (year - cumTP_minus) * slope) * (1 + extraBiomass_ratio)
        end
    end
    mortCoeff = gfd(m, "mort", regId, c.fTypes[ft], c.dClasses[nDc], m.SCD.year)
    cumTP_max = px.cumTp_exp[ft][nDc]
    time_elapsed = year - cumTP_max
    volHa_max = px.cumTp_exp[ft][nDc]  # as in the C++ code (it uses cumTp_exp, not vHa_exp)
    return volHa_max * cpow(1 - mortCoeff, time_elapsed) * (1 + extraBiomass_ratio)
end

function runManagementModule!(m)
    msgOut(MSG_INFO, "Starting management module..")
    c = m.core
    MD = m.MD
    G = m.GIS
    fTypes = c.fTypes
    nft = length(fTypes)
    dClasses = c.dClasses
    allFTypes = getForTypeIds(MD, true)
    hAreaByFTypeGroup = Dict(ft => 0.0 for ft in allFTypes)
    thisYear = m.SCD.year
    ir = getDoubleSetting(m, "ir")
    WL2 = c.WL2
    heteroRA = getBoolSetting(m, "heterogeneousRiskAversion", DATA_NOW)

    for r2 in c.regIds2
        regId = r2
        REG = getRegion(MD, r2)
        regPx = regPixels(m, REG)
        flagHartman = getStringSetting(m, "flagHartman", DATA_NOW, r2)
        pickling = getDoubleSetting(m, "pickling")
        carbonPrice_NOW = gpd(m, "carbonPrice", r2, "", thisYear + 1)
        futureCarbonPrices = Float64[]
        if flagHartman != "NONE"
            for y in 0:999
                push!(futureCarbonPrices, gpd(m, "carbonPrice", r2, "", thisYear + y))
            end
        end
        polBal_fiSub = zeros(nft)
        fArea_reg = regionArea(m, REG)
        fArea_diff = 0.0
        fArea_reldiff = 0.0
        if c.forestAreaChangeMethod == "relative"
            fArea_reldiff = gfd(m, "forestChangeAreaIncrementsRel", r2, "", "", DATA_NOW)
            fArea_diff = fArea_reg * fArea_reldiff
        elseif c.forestAreaChangeMethod == "absolute"
            fArea_diff = gfd(m, "forestChangeAreaIncrementsHa", r2, "", "", DATA_NOW)
            fArea_reldiff = fArea_diff / fArea_reg
        end
        regHArea = 0.0

        # management rate: from forData if there, otherwise from the settings
        origErrorLevel = MD.errorLevel
        MD.tempBool = true
        MD.errorLevel = MSG_NO_MSG
        mr = getForData(m, "mr", regId, "", "")
        if !MD.tempBool
            MD.errorLevel = MSG_CRITICAL_ERROR
            mr = getDoubleSetting(m, "mr", DATA_NOW, regId)
        end
        MD.errorLevel = origErrorLevel

        futureCarbonRef = Vector{Vector{Float64}}()
        futureExtraBiomass_ratio = Vector{Vector{Float64}}()
        if flagHartman != "NONE"
            for ft in fTypes
                cr = Float64[]
                eb = Float64[]
                for y in 0:999
                    push!(cr, gfd(m, "carbonRef", r2, ft, "", thisYear + y))
                    push!(eb, gfd(m, "extraBiomass_ratio", regId, ft, "", DATA_NOW))
                end
                push!(futureCarbonRef, cr)
                push!(futureExtraBiomass_ratio, eb)
            end
        end

        # regional values used in the pixel loop (pure look-ups, cached for performance)
        prices = Dict{Tuple{Int,Int},NTuple{4,Float64}}()   # (pp, year of cumTp) -> (pol_s_fut, worldFutPrice)
        pinfo = [(gpd(m, "pol_mktDirInt_s", regId, pp), gpd(m, "pl", regId, pp), gpd(m, "pl", WL2, pp),
                  gpd(m, "sl", regId, pp), gpd(m, "sa", regId, pp)) for pp in c.priProducts]
        fhf = [gfd(m, "finalHarvestFlag", regId, ft, dc) for ft in fTypes, dc in dClasses]
        co2inv = [gfd(m, "co2content_inventory", r2, ft, "", DATA_NOW) / 1000 for ft in fTypes]
        appm = [appf(m, pp, ft, dc) for pp in c.priProducts, ft in fTypes, dc in dClasses]
        function futPrices(pi, fy)
            k = (pi, fy)
            v = get(prices, k, nothing)
            if v === nothing
                pp = c.priProducts[pi]
                pol_s_fut = gpd(m, "pol_mktDirInt_s", regId, pp, fy)
                worldFut = gpd(m, "pl", WL2, pp, fy)
                v = (pol_s_fut, worldFut, 0.0, 0.0)
                prices[k] = v
            end
            return v
        end
        transCost = [(gfd(m, "invTransCost", regId, ft, ft2, DATA_NOW), gfd(m, "pol_fiSub", regId, ft, ft2, DATA_NOW)) for ft in fTypes, ft2 in fTypes]

        for px in regPx
            empty!(px.expectedReturns)
            empty!(px.expectedReturnsNotCorrByRa)
            empty!(px.optDc)
            empty!(px.optFtChosen)
            empty!(px.optDcChosen)
            empty!(px.expectedAnnualIncome_carbon)
            empty!(px.expectedAnnualIncome_timber)
            for k in keys(hAreaByFTypeGroup)
                hAreaByFTypeGroup[k] = 0.0
            end
            totalHarvestedAreaOrig = vSum(px.hArea)
            thisYearRegAreas = zeros(nft)
            expectedReturns = zeros(nft)
            rotation_dc = zeros(Int, nft)
            deltaAreas = [zeros(nft + 1) for _ in 1:(nft+1)]

            fArea_px = vSum(px.area)
            fArea_diff_px = fArea_px * fArea_diff / fArea_reg
            fArea_incr = cmax(0.0, fArea_diff_px)
            fArea_incr_rel = totalHarvestedAreaOrig != 0 ? fArea_incr / totalHarvestedAreaOrig : 0.0
            fArea_decr = -cmin(0.0, fArea_diff_px)
            fArea_decr_rel = totalHarvestedAreaOrig != 0 ? cmin(1.0, fArea_decr / totalHarvestedAreaOrig) : 0.0
            regHArea += totalHarvestedAreaOrig
            totalHarvestedArea = totalHarvestedAreaOrig * (1 - fArea_decr_rel)

            # A - harvested area by parent ft group
            for (j, ft) in enumerate(fTypes)
                parentFt = getForTypeParentId(MD, ft)
                hAreaThisFt = vSum(px.hArea[j]) * (1 - fArea_decr_rel)
                haskey(hAreaByFTypeGroup, parentFt) || msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                hAreaByFTypeGroup[parentFt] += hAreaThisFt
            end

            # B - uncorrected expected returns
            for (j, ft) in enumerate(fTypes)
                co2content_inventory = co2inv[j]
                expReturns = -floatmax(Float64)
                expReturns_carbon = -floatmax(Float64)
                expReturns_timber = -floatmax(Float64)
                optDc = 0
                for u in eachindex(dClasses)
                    vHa = px.vHa_exp[j][u]
                    finalHarvestFlag = fhf[j, u]
                    cumTp_u = px.cumTp_exp[j][u]
                    fy = yint(thisYear + cumTp_u)
                    for pi in eachindex(c.priProducts)
                        pol_s, plv, plw, sl, sa = pinfo[pi]
                        pol_s_fut, worldFutPrice0, _, _ = futPrices(pi, fy)
                        pl = plv * pol_s
                        worldCurPrice = plw * pol_s * pol_s
                        worldFutPrice = worldFutPrice0 * pol_s_fut
                        pw_exp = _computeExpectedPrice(pl, worldCurPrice, worldFutPrice, sl, sa, px.expTypePrices)
                        raw_amount = finalHarvestFlag * pw_exp * vHa * appm[pi, j, u]
                        anualised_amount_timber = calculateAnnualisedEquivalent(raw_amount, cumTp_u, ir)
                        anualised_amount = anualised_amount_timber
                        raw_amount_carbon = 0.0
                        if flagHartman == "EXO"
                            carbonPrice_y = carbonPrice_NOW
                            y = 0
                            while y < cumTp_u
                                carbonRef = futureCarbonRef[j][y+1]
                                expectedVHa_y = getVHaByYear(m, px, j, y, futureExtraBiomass_ratio[j][y+1], regId)
                                carbonPayment_y = carbonPrice_y * ir * (co2content_inventory * expectedVHa_y - carbonRef)
                                raw_amount_carbon += carbonPayment_y * cpow(1 + ir, cumTp_u - y)
                                y += 1
                            end
                            raw_amount_carbon += pickling * carbonPrice_y * finalHarvestFlag * vHa * appm[pi, j, u]
                        end
                        anualised_amount_carbon = calculateAnnualisedEquivalent(raw_amount_carbon, cumTp_u, ir)
                        anualised_amount += anualised_amount_carbon
                        if anualised_amount > expReturns
                            expReturns = anualised_amount
                            optDc = u - 1
                            expReturns_carbon = anualised_amount_carbon
                            expReturns_timber = anualised_amount_timber
                        end
                    end
                end
                if flagHartman == "NONE" || flagHartman == "EXO"
                    push!(px.expectedAnnualIncome_timber, expReturns_timber)
                    push!(px.expectedAnnualIncome_carbon, expReturns_carbon)
                    push!(px.expectedReturnsNotCorrByRa, expReturns)
                    push!(px.optDc, optDc)
                end
                if heteroRA
                    ra = getDoubleValue(G, px, "ra")
                    cumMort = 1 - px.cumAlive_exp[j][optDc+1]
                    origExpReturns = expReturns
                    expReturns = origExpReturns * (1.0 - ra * cumMort)
                end
                if flagHartman == "NONE" || flagHartman == "EXO"
                    push!(px.expectedReturns, expReturns)
                end
                expectedReturns[j] = expReturns
                rotation_dc[j] = optDc
            end

            # Hartman with endogenous (BAU) reference
            if flagHartman == "ENDO"
                ft_opt_index = getMaxPos(expectedReturns)
                ft_opt = fTypes[ft_opt_index]
                dc_opt_index = rotation_dc[ft_opt_index] + 1
                cumTP_opt = px.cumTp_exp[ft_opt_index][dc_opt_index]
                co2content_inventory_opt = gfd(m, "co2content_inventory", r2, ft_opt, "", DATA_NOW) / 1000
                FutureCarbonContent_opt = Float64[]
                y = 0
                while y <= cumTP_opt
                    expectedVHa_opt_y = getVHaByYear(m, px, ft_opt_index, y, futureExtraBiomass_ratio[ft_opt_index][y+1], regId)
                    push!(FutureCarbonContent_opt, co2content_inventory_opt * expectedVHa_opt_y)
                    y += 1
                end
                while length(FutureCarbonContent_opt) <= 2000
                    append!(FutureCarbonContent_opt, copy(FutureCarbonContent_opt))
                end
                for (j, ft) in enumerate(fTypes)
                    co2content_inventory = gfd(m, "co2content_inventory", r2, ft, "", DATA_NOW) / 1000
                    expReturns_hartman = -10000000000.0
                    expReturns_timber_hartman = 0.0
                    expReturns_carbon_hartman = 0.0
                    optDc_hartman = -1
                    for u in 2:length(dClasses)
                        dc = dClasses[u]
                        cumTp_u = px.cumTp_exp[j][u]
                        raw_amount_carbon = 0.0
                        carbonPrice_y = carbonPrice_NOW
                        y = 0
                        while y <= cumTp_u
                            expectedVHa_y = getVHaByYear(m, px, j, y, futureExtraBiomass_ratio[j][y+1], regId)
                            carbonPayment_y = carbonPrice_y * ir * (co2content_inventory * expectedVHa_y - FutureCarbonContent_opt[y+1])
                            raw_amount_carbon += carbonPayment_y * cpow(1 + ir, cumTp_u - y)
                            y += 1
                        end
                        vHa = px.vHa_exp[j][u]
                        finalHarvestFlag = gfd(m, "finalHarvestFlag", regId, ft, dc)
                        for pp in c.priProducts
                            pl = gpd(m, "pl", regId, pp)
                            worldCurPrice = gpd(m, "pl", WL2, pp)
                            worldFutPrice = gpd(m, "pl", WL2, pp, yint(thisYear + cumTp_u))
                            sl = gpd(m, "sl", regId, pp)
                            sa = gpd(m, "sa", regId, pp)
                            pw_exp = _computeExpectedPrice(pl, worldCurPrice, worldFutPrice, sl, sa, px.expTypePrices)
                            raw_amount = finalHarvestFlag * pw_exp * vHa * appf(m, pp, ft, dc)
                            anualised_amount_timber = calculateAnnualisedEquivalent(raw_amount, cumTp_u, ir)
                            raw_amount_carbon += pickling * carbonPrice_y * finalHarvestFlag * vHa * appf(m, pp, ft, dc)
                            anualised_amount_carbon = calculateAnnualisedEquivalent(raw_amount_carbon, cumTp_u, ir)
                            anualised_amount = anualised_amount_timber + anualised_amount_carbon
                            if anualised_amount >= expReturns_hartman
                                expReturns_hartman = anualised_amount
                                expReturns_carbon_hartman = anualised_amount_carbon
                                expReturns_timber_hartman = anualised_amount_timber
                                optDc_hartman = u - 1
                            end
                        end
                    end
                    push!(px.expectedAnnualIncome_timber, expReturns_timber_hartman)
                    push!(px.expectedAnnualIncome_carbon, expReturns_carbon_hartman)
                    push!(px.expectedReturnsNotCorrByRa, expReturns_hartman)
                    push!(px.optDc, optDc_hartman)
                    if heteroRA
                        ra = getDoubleValue(G, px, "ra")
                        cumMort = 1 - px.cumAlive_exp[j][optDc_hartman+1]
                        expReturns_hartman = expReturns_hartman * (1.0 - ra * cumMort)
                    end
                    push!(px.expectedReturns, expReturns_hartman)
                    expectedReturns[j] = expReturns_hartman
                    rotation_dc[j] = optDc_hartman
                end
            end

            # C, D, E - corrected expected returns and allocation of the managed and unmanaged regeneration areas
            for (j, ft) in enumerate(fTypes)
                harvestedAreaForThisFT = vSum(px.hArea[j]) * (1 - fArea_decr_rel)
                deltaAreas[j][nft+1] += vSum(px.hArea[j]) * (fArea_decr_rel)
                corrExpectedReturns = zeros(nft)
                polBal_fiSub_unitvalue = zeros(nft)
                for j2 in 1:nft
                    itc, pfs = transCost[j, j2]
                    invTransCost = itc * pfs
                    polBal_fiSub_unitvalue[j2] = itc * (pfs - 1)
                    corrExpectedReturns[j2] = (expectedReturns[j2] / ir) - invTransCost
                end
                optFtChosen = getMaxPos(corrExpectedReturns)
                optFtER = getMaxPos(expectedReturns)
                push!(px.optFtChosen, optFtChosen - 1)
                push!(px.optDcChosen, px.optDc[optFtChosen])
                thisYearRegAreas[optFtChosen] += harvestedAreaForThisFT * mr
                thisYearRegAreas[optFtER] += fArea_incr * mr / Float64(nft)
                deltaAreas[j][optFtChosen] += harvestedAreaForThisFT * mr
                deltaAreas[nft+1][optFtER] += fArea_incr * mr / Float64(nft)
                polBal_fiSub[optFtChosen] += harvestedAreaForThisFT * mr * polBal_fiSub_unitvalue[optFtChosen]

                if c.natRegAllocation == "pp"
                    parentFt = getForTypeParentId(MD, ft)
                    freq = c.rescaleFrequencies ? gfd(m, "freq_norm", regId, parentFt, "") : gfd(m, "freq", regId, parentFt, "")
                    haskey(hAreaByFTypeGroup, parentFt) || msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                    hAreaThisFtGroup = hAreaByFTypeGroup[parentFt]
                    hRatio = 1.0
                    if hAreaThisFtGroup > 0
                        hRatio = harvestedAreaForThisFT / hAreaThisFtGroup
                    else
                        hRatio = 1.0 / getNForTypesChilds(MD, parentFt)
                    end
                    thisYearRegAreas[j] += totalHarvestedArea * (1 - mr) * freq * hRatio
                    thisYearRegAreas[j] += fArea_incr * (1 - mr) * freq * hRatio
                    for j2 in 1:nft
                        deltaAreas[j2][j] += vSum(px.hArea[j2]) * (1 - fArea_decr_rel) * (1 - mr) * freq * hRatio
                    end
                    deltaAreas[nft+1][j] += fArea_incr * (1 - mr) * freq * hRatio
                else
                    mortRatePath = getPathMortality(m, px, ft, "0")
                    if mortRatePath > 0
                        parentFt = getForTypeParentId(MD, ft)
                        siblings = getForTypeChilds(MD, parentFt)
                        siblingAreas = Float64[]
                        for s in siblings
                            push!(siblingAreas, s == ft ? 0.0 : vSum(px.area[getPos(s, fTypes)]))
                        end
                        areaAllSiblings = vSum(siblingAreas)
                        thisYearRegAreas[j] += harvestedAreaForThisFT * (1 - mr) * (1 - mortRatePath)
                        deltaAreas[j][j] += harvestedAreaForThisFT * (1 - mr) * (1 - mortRatePath)
                        if areaAllSiblings > 0.0
                            for (k, s) in enumerate(siblings)
                                v = harvestedAreaForThisFT * (1.0 - mr) * (mortRatePath) * (siblingAreas[k] / areaAllSiblings)
                                thisYearRegAreas[getPos(s, fTypes)] += v
                                deltaAreas[j][getPos(s, fTypes)] += v
                            end
                        elseif length(siblings) > 1
                            for s in siblings
                                if s != ft
                                    v = harvestedAreaForThisFT * (1.0 - mr) * (mortRatePath) * 1.0 / (Float64(Float32(length(siblings))) - 1.0)
                                    thisYearRegAreas[getPos(s, fTypes)] += v
                                    deltaAreas[j][getPos(s, fTypes)] += v
                                end
                            end
                        end
                    else
                        thisYearRegAreas[j] += harvestedAreaForThisFT * (1.0 - mr)
                        deltaAreas[j][j] += harvestedAreaForThisFT * (1.0 - mr)
                    end
                    totArea = vSum(px.area)
                    newAreaThisFt = totArea != 0 ? fArea_incr * (1 - mr) * vSum(px.area[j]) / totArea : 0.0
                    thisYearRegAreas[j] += newAreaThisFt
                    deltaAreas[nft+1][j] += newAreaThisFt
                    if !(thisYearRegAreas[j] >= 0.0)
                        msgOut(MSG_ERROR, "thisYearRegAreas[j] is not non negative (j: $(j-1), thisYearRegAreas[j]: $(i2s(thisYearRegAreas[j]))).")
                    end
                end
            end
            for j in 1:nft
                px.area[j][1] += thisYearRegAreas[j]
            end
            haskey(px.regArea, thisYear) || (px.regArea[thisYear] = thisYearRegAreas)
            px.deltaArea = deltaAreas
        end
        if -fArea_diff > regHArea
            msgOut(MSG_WARNING, "In region $regId the exogenous area decrement ($(d2s(-fArea_diff)) ha) is higger than the harvesting ($(d2s(regHArea)) ha). Ratio forced to 1.")
        end
        for (j, ft) in enumerate(fTypes)
            sfd(m, polBal_fiSub[j] / 1000000.0, "polBal_fiSub", regId, ft, "", DATA_NOW, true)
        end
    end
end

# ------------------------------------------------------------------------------
# Initialisation of pixel values
# ------------------------------------------------------------------------------

function initializePixelVolumes!(m)
    msgOut(MSG_INFO, "Starting initializing pixel-level values")
    c = m.core
    G = m.GIS
    getBoolSetting(m, "usePixelData") || return
    for r in c.regIds2
        reg = getRegion(m.MD, r)
        rpx = regPixels(m, reg)
        for px in rpx
            px.vol = VV()
        end
        for (y, ft) in enumerate(c.fTypes)
            regForArea = regionLayerSum(m, reg, "forArea_" * ft)
            regVols = [z > 1 ? gfd(m, "vol", r, ft, c.dClasses[z], c.firstYear) : 0.0 for z in eachindex(c.dClasses)]
            for px in rpx
                vol_byu = Float64[]
                for z in eachindex(c.dClasses)
                    regVol = regVols[z]
                    pxArea = getDoubleValue(G, px, "forArea_" * ft, true)
                    pxArea < 0.0 && msgOut(MSG_CRITICAL_ERROR, "Error in initializePixelVolumes, negative pxArea!")
                    pxVol = regForArea != 0 ? regVol * pxArea / regForArea : 0.0
                    push!(vol_byu, pxVol)
                end
                push!(px.vol, vol_byu)
            end
        end
    end
    loadExogenousForestLayers!(m, "vol")
end

function assignSpMultiplierPropToVols!(m)
    getBoolSetting(m, "useSpatialVarPropToVol") || return
    c = m.core
    for r in c.regIds2
        rpx = regionPixels(m, r)
        for (f, ft) in enumerate(c.fTypes)
            agr = gfd(m, "agr", r, ft, "")
            sStDev = gfd(m, "sStDev", r, ft, "")
            vols = [vSum(px.vol[f]) for px in rpx]
            vol_max = getMax(vols)
            diffVols = [vol_max - v for v in vols]
            diffVols_avg = getAvg(diffVols)
            rescale_ratio_avg = (diffVols_avg != 0.0) ? agr / diffVols_avg : 1.0
            diffVols_rescaled = [d * rescale_ratio_avg for d in diffVols]
            diffVols_rescaled_avg = getAvg(diffVols_rescaled)
            diffVols_rescaled_sd = getSd(diffVols_rescaled; sample = false)
            rescale_ratio_sd = (diffVols_rescaled_sd != 0.0) ? sStDev / diffVols_rescaled_sd : 1.0
            for (p, px) in enumerate(rpx)
                dev = diffVols_rescaled[p] - diffVols_rescaled_avg
                devr = dev * rescale_ratio_sd
                final_value = diffVols_rescaled_avg + devr
                px.spMods[f] = (agr != 0.0) ? cmin(1.6, cmax(0.4, final_value / agr)) : 1.0
            end
        end
    end
end

function initialiseCarbonModule!(m)
    c = m.core
    initialiseEmissionCounters!(m)
    for r in c.regIds2
        deathBiomass = [gfd(m, "vMort", r, ft, DIAM_ALL, DATA_NOW) + gfd(m, "vMortAdd", r, ft, DIAM_ALL, DATA_NOW) for ft in c.fTypes]
        initialiseDeathBiomassStocks!(m, deathBiomass, r)
        qProducts = Float64[]
        for p in c.priProducts
            push!(qProducts, gpd(m, "sa", r, p, DATA_NOW))
        end
        for p in c.secProducts
            push!(qProducts, gpd(m, "dl", r, p, DATA_NOW))
        end
        initialiseProductsStocks!(m, qProducts, r)
    end
end

function initialiseDeathTimber!(m)
    c = m.core
    currentYear = m.SCD.year
    for y in currentYear:-1:(currentYear-29), r in c.regIds2, ft in c.fTypes, dc in c.dClasses
        deathTimberInventory_incrOrAdd!(m.MD, (y, r, ft, dc), 0.0)
    end
end

function initializePixelArea!(m)
    msgOut(MSG_INFO, "Starting initializing pixel-level area")
    c = m.core
    G = m.GIS
    getBoolSetting(m, "usePixelData") || return
    for r in c.regIds2
        reg = getRegion(m.MD, r)
        rpx = regPixels(m, reg)
        rinfo = Dict{String,Tuple{Float64,Float64,Float64}}()
        for ft in c.fTypes
            rinfo[ft] = (regionLayerSum(m, reg, "forArea_" * ft) / 10000.0, gfd(m, "vReg", r, ft, ""), gfd(m, "entryVolHa", r, ft, ""))
        end
        for px in rpx
            for (j, ft) in enumerate(c.fTypes)
                tempAreas = Float64[]
                pxArea = getDoubleValue(G, px, "forArea_" * ft, true) / 10000.0
                for u in eachindex(c.dClasses)
                    if u == 1
                        regionAreaV, regRegVolumes, entryVolHa = rinfo[ft]
                        newVReg = regionAreaV != 0 ? regRegVolumes * pxArea / regionAreaV : 0.0
                        tp_u0 = px.tp[j][1]
                        push!(tempAreas, (newVReg * 1000000.0 / entryVolHa) * tp_u0)
                    else
                        dcVol = px.vol_l[j][u] * 1000000.0
                        dcVHa = px.vHa[j][u]
                        push!(tempAreas, dcVHa != 0 ? dcVol / dcVHa : 0.0)
                    end
                end
                sumTempArea = vSum(tempAreas)
                normCoef = sumTempArea != 0 ? pxArea / sumTempArea : 0.0
                push!(px.area_l, [a * normCoef for a in tempAreas])
            end
            px.area = deepcopy(px.area_l)
        end
    end
    loadExogenousForestLayers!(m, "area")
end

function computeCumulativeData!(m)
    msgOut(MSG_INFO, "Starting computing some cumulative values..")
    c = m.core
    MD = m.MD
    thisYear = m.SCD.year
    firstYear = c.firstYear
    dClasses = c.dClasses
    for regId in c.regIds2
        regPx = regionPixels(m, regId)
        # regional data by ft and dc (pure look-ups) cached by (name, ft index, dc index, year)
        dcache = Dict{Tuple{String,Int,Int,Int},Float64}()
        function rd(name, j, u, y)
            k = (name, j, u, y)
            v = get(dcache, k, nothing)
            if v === nothing
                v = u == 0 ? gfd(m, name, regId, c.fTypes[j], "", y) : gfd(m, name, regId, c.fTypes[j], dClasses[u], y)
                dcache[k] = v
            end
            return v
        end
        for px in regPx
            empty!(px.cumTp); empty!(px.cumTp_exp); empty!(px.vHa_exp); empty!(px.vHa); empty!(px.cumAlive); empty!(px.cumAlive_exp)
            expType = px.expType
            for (j, ft) in enumerate(c.fTypes)
                tp_multiplier_now = getMultiplier(m, px, "tp_multiplier", ft, DATA_NOW)
                tp_multiplier_t0 = getMultiplier(m, px, "tp_multiplier", ft, firstYear)
                mortCoef_multiplier_now = getMultiplier(m, px, "mortCoef_multiplier", ft, DATA_NOW)
                mortCoef_multiplier_t0 = getMultiplier(m, px, "mortCoef_multiplier", ft, firstYear)
                betaCoef_multiplier_now = getMultiplier(m, px, "betaCoef_multiplier", ft, DATA_NOW)
                betaCoef_multiplier_t0 = getMultiplier(m, px, "betaCoef_multiplier", ft, firstYear)
                cumTp_temp = Float64[]; vHa_temp = Float64[]; cumAlive_temp = Float64[]
                cumTp_exp_temp = Float64[]; vHa_exp_temp = Float64[]; cumAlive_exp_temp = Float64[]
                MD.errorLevel = MSG_NO_MSG
                for u in eachindex(dClasses)
                    dc = dClasses[u]
                    pathMort_now = getPathMortality(m, px, ft, dc, DATA_NOW)
                    pathMort_t0 = getPathMortality(m, px, ft, dc, firstYear)
                    am_mult = rd("additionalMortality_multiplier", j, u, thisYear)
                    am_mult_t0 = rd("additionalMortality_multiplier", j, u, firstYear)
                    additionalMortality_now = getSTData(m, px, "additionalMortality", ft, DATA_NOW, dc, 0.0) * am_mult
                    additionalMortality_t0 = getSTData(m, px, "additionalMortality", ft, firstYear, dc, 0.0) * am_mult_t0
                    if u == 1
                        push!(cumTp_temp, 0.0); push!(vHa_temp, 0.0)
                        push!(cumTp_exp_temp, 0.0); push!(vHa_exp_temp, 0.0)
                        push!(cumAlive_temp, 1.0); push!(cumAlive_exp_temp, 1.0)
                    else
                        tp = rd("tp", j, u - 1, thisYear) * tp_multiplier_now
                        cumTp_u = cumTp_temp[u-1] + tp
                        local vHa_u::Float64, mort::Float64
                        if u == 2
                            vHa_u = rd("entryVolHa", j, 0, thisYear)
                            mort = 0.0
                        else
                            mort = 1 - cpow(1 - cmin(rd("mortCoef", j, u - 1, thisYear) * mortCoef_multiplier_now + pathMort_now + additionalMortality_now, 1.0), tp)
                            beta = rd("betaCoef", j, u, thisYear) * betaCoef_multiplier_now
                            vHa_u = vHa_temp[u-1] * beta * (1 - mort)
                        end
                        cumAlive_u = cmax(0.0, cumAlive_temp[u-1] * (1 - mort))
                        push!(cumAlive_temp, cumAlive_u)
                        push!(cumTp_temp, cumTp_u)
                        push!(vHa_temp, vHa_u)
                        local vHa_u_exp::Float64, mort_exp::Float64
                        if expType == -1
                            tp_exp = rd("tp", j, u - 1, firstYear) * tp_multiplier_t0
                            push!(cumTp_exp_temp, cumTp_exp_temp[u-1] + tp_exp)
                            if u == 2
                                vHa_u_exp = rd("entryVolHa", j, 0, firstYear)
                                mort_exp = 0.0
                            else
                                mort_exp = 1 - cpow(1 - cmin(rd("mortCoef", j, u - 1, firstYear) * mortCoef_multiplier_t0 + pathMort_t0 + additionalMortality_t0, 1.0), tp_exp)
                                beta_exp = rd("betaCoef", j, u, firstYear) * betaCoef_multiplier_t0
                                vHa_u_exp = vHa_exp_temp[u-1] * beta_exp * (1 - mort_exp)
                            end
                        else
                            yDyn = yint(thisYear + ceil(cumTp_exp_temp[u-1]))
                            tp_multiplier_dynamic = getMultiplier(m, px, "tp_multiplier", ft, yDyn)
                            tp_noExp = rd("tp", j, u - 1, thisYear) * tp_multiplier_now
                            cumTp_u_noExp = cumTp_exp_temp[u-1] + tp_noExp
                            tp_fullExp = rd("tp", j, u - 1, yDyn) * tp_multiplier_dynamic
                            cumTp_u_fullExp = cumTp_exp_temp[u-1] + tp_fullExp
                            cumTp_u_exp = cumTp_u_fullExp * expType + cumTp_u_noExp * (1 - expType)
                            push!(cumTp_exp_temp, cumTp_u_exp)
                            if u == 2
                                vHa_u_noExp = rd("entryVolHa", j, 0, thisYear)
                                vHa_u_fullExp = rd("entryVolHa", j, 0, yint(thisYear + ceil(cumTp_u)))
                                vHa_u_exp = vHa_u_fullExp * expType + vHa_u_noExp * (1 - expType)
                                mort_exp = 0.0
                            else
                                mort_noExp = 1 - cpow(1 - cmin(1.0, rd("mortCoef", j, u - 1, thisYear) * mortCoef_multiplier_now + pathMort_now + additionalMortality_now), tp_noExp)
                                mortCoef_multiplier_dynamic = getMultiplier(m, px, "mortCoef_multiplier", ft, yDyn)
                                pathMort_dynamic = getPathMortality(m, px, ft, dc, yDyn)
                                am_mult_dyn = rd("additionalMortality_multiplier", j, u, yDyn)
                                additionalMortality_dynamic = getSTData(m, px, "additionalMortality", ft, yDyn, dc, 0.0) * am_mult_dyn
                                mort_fullExp = 1 - cpow(1 - cmin(1.0, rd("mortCoef", j, u - 1, yDyn) * mortCoef_multiplier_dynamic + pathMort_dynamic + additionalMortality_dynamic), tp_fullExp)
                                beta_noExp = rd("betaCoef", j, u, thisYear) * betaCoef_multiplier_now
                                yB = yint(thisYear + ceil(cumTp_u))
                                betaCoef_multiplier_dynamic = getMultiplier(m, px, "betaCoef_multiplier", ft, yB)
                                beta_fullExp = rd("betaCoef", j, u, yB) * betaCoef_multiplier_dynamic
                                mort_exp = mort_fullExp * expType + mort_noExp * (1 - expType)
                                beta_exp = beta_fullExp * expType + beta_noExp * (1 - expType)
                                vHa_u_exp = vHa_exp_temp[u-1] * beta_exp * (1 - mort_exp)
                            end
                        end
                        push!(vHa_exp_temp, vHa_u_exp)
                        push!(cumAlive_exp_temp, cmax(0.0, cumAlive_exp_temp[u-1] * (1 - mort_exp)))
                    end
                end
                push!(px.cumTp, cumTp_temp)
                push!(px.vHa, vHa_temp)
                push!(px.cumAlive, cumAlive_temp)
                push!(px.cumTp_exp, cumTp_exp_temp)
                push!(px.vHa_exp, vHa_exp_temp)
                push!(px.cumAlive_exp, cumAlive_exp_temp)
            end
        end
    end
    MD.errorLevel = MSG_ERROR
end

function resetPixelValues!(m)
    msgOut(MSG_INFO, "Starting resetting pixel level values")
    for regId in m.core.regIds2
        for px in regionPixels(m, regId)
            px.vol_l = px.vol                 # vol_l = vol (and vol is then cleared)
            px.area_l = px.area               # area_l = area
            px.vol = VV()
            px.area = deepcopy(px.area_l)     # area = area_l (a copy)
            px.hArea = VV()
            px.deltaArea = VV()
            px.hVol = VV()
            px.hVol_byPrd = Vector{VV}()
            px.vReg = Float64[]
            px.expectedReturns = Float64[]
            px.beta = VV(); px.mort = VV(); px.addMort = VV(); px.tp = VV()
            px.cumTp = VV(); px.vHa = VV(); px.cumTp_exp = VV(); px.vHa_exp = VV()
            px.cumAlive = VV(); px.cumAlive_exp = VV()
            px.vMort = VV(); px.vMortAdd = VV()
        end
    end
end

function cachePixelExogenousData!(m)
    msgOut(MSG_INFO, "Starting cashing on pixel spatial-level exogenous data")
    c = m.core
    applyAvalCoef = getBoolSetting(m, "applyAvalCoef")
    noValue = getDoubleSetting(m, "noValue")
    for regId in c.regIds2
        regPx = regionPixels(m, regId)
        rdata = Dict{Tuple{Int,Int},NTuple{5,Float64}}()
        for (j, ft) in enumerate(c.fTypes), (u, dc) in enumerate(c.dClasses)
            rdata[(j, u)] = (gfd(m, "additionalMortality_multiplier", regId, ft, dc, DATA_NOW),
                gfd(m, "tp", regId, ft, dc, c.firstYear), gfd(m, "tp", regId, ft, dc, DATA_NOW),
                u > 1 ? gfd(m, "betaCoef", regId, ft, dc, DATA_NOW) : 0.0, u > 1 ? gfd(m, "mortCoef", regId, ft, dc, DATA_NOW) : 0.0)
        end
        for px in regPx
            px.tp = VV(); px.beta = VV(); px.mort = VV(); px.addMort = VV()
            px.avalCoef = Float64[]
            for (j, ft) in enumerate(c.fTypes)
                tp_byu = Float64[]; beta_byu = Float64[]; mort_byu = Float64[]; addMort_byu = Float64[]
                tp_multiplier_now = getMultiplier(m, px, "tp_multiplier", ft, DATA_NOW)
                mortCoef_multiplier_now = getMultiplier(m, px, "mortCoef_multiplier", ft, DATA_NOW)
                betaCoef_multiplier_now = getMultiplier(m, px, "betaCoef_multiplier", ft, DATA_NOW)
                avalCoef_ft = applyAvalCoef ? getSTData(m, px, "avalCoef", ft, DATA_NOW) : 1.0
                if avalCoef_ft == noValue
                    msgOut(MSG_CRITICAL_ERROR, "applyAvalCoef has benn asked for, but I can't find the avalCoef in the data!")
                end
                for (u, dc) in enumerate(c.dClasses)
                    pathMortality = getPathMortality(m, px, ft, dc, DATA_NOW)
                    am_mult, tp_t0, tp_now, betaC, mortC = rdata[(j, u)]
                    additionalMortality = getSTData(m, px, "additionalMortality", ft, DATA_NOW, dc, 0.0) * am_mult
                    if u == 1
                        tp = tp_t0 * getMultiplier(m, px, "tp_multiplier", ft, c.firstYear)
                    else
                        tp = tp_now * tp_multiplier_now
                    end
                    beta_real = u > 1 ? betaC * betaCoef_multiplier_now : 0.0
                    mort_real = cmin(u > 1 ? mortC * mortCoef_multiplier_now + pathMortality : 0.0, 1.0)
                    push!(tp_byu, tp); push!(beta_byu, beta_real); push!(mort_byu, mort_real); push!(addMort_byu, additionalMortality)
                end
                push!(px.tp, tp_byu); push!(px.beta, beta_byu); push!(px.mort, mort_byu); push!(px.addMort, addMort_byu)
                push!(px.avalCoef, avalCoef_ft)
            end
        end
    end
end

"ModelData::getAvailableAliveTimber()"
function getAvailableAliveTimber(m, primProd::Vector{String}, regId::Integer)
    MD = m.MD
    toReturn = 0.0
    regPx = regionPixels(m, regId)
    for (i, ft) in enumerate(getForTypeIds(MD)), (u, dc) in enumerate(MD.diamClasses)
        possible = any(pp -> assessProdPossibility(MD, pp, ft, dc), primProd)
        if possible
            for px in regPx
                toReturn += px.vol_l[i][u] * px.avalCoef[i]
            end
        end
    end
    return toReturn
end

function computeInventary!(m)
    msgOut(MSG_INFO, "Starting computing inventary available for this year..")
    c = m.core
    MD = m.MD
    combs = createCombinationsVector(length(c.priProducts))
    currentYear = m.SCD.year
    for r2 in c.regIds2
        REG = getRegion(MD, r2)
        regPx = regPixels(m, REG)
        in_reg = zeros(length(c.priProducts))
        in_deathTimber_reg = zeros(length(c.priProducts))
        appv = [appf(m, pp, ft, dc) for pp in c.priProducts, ft in c.fTypes, dc in c.dClasses]
        for px in regPx
            for pp in eachindex(c.priProducts)
                inv = 0.0
                for ft in eachindex(c.fTypes), dc in eachindex(c.dClasses)
                    inv += appv[pp, ft, dc] * px.vol_l[ft][dc] * px.avalCoef[ft]
                end
                in_reg[pp] += inv
            end
        end
        for (pi, pp) in enumerate(c.priProducts)
            in_deathTimber_reg[pi] += getAvailableDeathTimber(m, [pp], r2, currentYear - 1)
            spd(m, in_reg[pi], "in", r2, pp, DATA_NOW, true)
            spd(m, in_deathTimber_reg[pi], "in_deathTimber", r2, pp, DATA_NOW, true)
        end
        for (i, comb) in enumerate(combs)
            ids = [c.priProducts[k] for k in comb]
            bound_alive = getAvailableAliveTimber(m, ids, r2)
            bound_deathTimber = getAvailableDeathTimber(m, ids, r2, currentYear - 1)
            REG.inResByAnyCombination[i] = bound_alive + bound_deathTimber
        end
    end
end

function updateMapAreas!(m)
    msgOut(MSG_INFO, "Updating map areas..")
    c = m.core
    G = m.GIS
    if !c.oldVol2AreaMethod
        getBoolSetting(m, "usePixelData") || return
        for r in c.regIds2
            for px in regionPixels(m, r)
                for (j, ft) in enumerate(c.fTypes)
                    changeValue!(G, px, "forArea_" * ft, vSum(px.area[j]) * 10000)
                end
            end
        end
    else
        currentYear = m.SCD.year
        fTypes = getForTypeIds(m.MD)
        for regId in getRegionIds(m.MD, 2, true)
            rpx = regionPixels(m, regId)
            for (j, ft) in enumerate(fTypes)
                for px in rpx
                    oldValue = getDoubleValue(G, px, "forArea_" * ft, true) / 10000
                    hArea = vSum(px.hArea[j])
                    haskey(px.regArea, currentYear) || msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                    regArea = px.regArea[currentYear][j]
                    changeValue!(G, px, "forArea_" * ft, (oldValue - hArea + regArea) * 10000)
                end
            end
        end
    end
end

function updateOtherMapData!(m)
    G = m.GIS
    fTypes = getForTypeIds(m.MD)
    for regId in getRegionIds(m.MD, 2, true)
        rpx = regionPixels(m, regId)
        for (j, ft) in enumerate(fTypes)
            for px in rpx
                vol = vSum(px.vol[j])
                expectedReturns = px.expectedReturns[j]
                layerExist(G, "vol_" * ft) && changeValue!(G, px, "vol_" * ft, vol)
                layerExist(G, "expectedReturns_" * ft) && changeValue!(G, px, "expectedReturns_" * ft, expectedReturns)
            end
        end
    end
    for ft in fTypes
        updateImage(m, "vol_" * ft)
        updateImage(m, "expectedReturns_" * ft)
    end
end

function sumRegionalForData!(m)
    msgOut(MSG_INFO, "Summing data pixels->region..")
    c = m.core
    MD = m.MD
    G = m.GIS
    currentYear = m.SCD.year
    hVol_byPrd = Dict{NTuple{4,String},Float64}()
    outDetailedHv = getBoolSetting(m, "outDetailedHv", DATA_NOW)
    for regId in c.regIds2
        regPx = regionPixels(m, regId)
        for (j, ft) in enumerate(c.fTypes)
            regArea = 0.0
            sumAreaByFt = 0.0
            vReg = 0.0
            sumSumHProductivity = 0.0
            sumHArea = 0.0
            for (u, dc) in enumerate(c.dClasses)
                vol = 0.0; hV = 0.0; hArea = 0.0; vMort = 0.0; vMortAdd = 0.0; sumHProductivity = 0.0
                for px in regPx
                    vol += px.vol[j][u]
                    hV += px.hVol[j][u]
                    hArea += px.hArea[j][u]
                    vMort += px.vMort[j][u]
                    vMortAdd += px.vMortAdd[j][u]
                    sumHProductivity += (px.hProductivity[j][u] * px.hArea[j][u])
                    if outDetailedHv
                        for (pp, pr) in enumerate(c.priProducts)
                            k = (i2s(regId), ft, dc, pr)
                            hVol_byPrd[k] = get(hVol_byPrd, k, 0.0) + px.hVol_byPrd[j][u][pp]
                        end
                    end
                end
                if u > 1
                    sfd(m, vol, "vol", regId, ft, dc, DATA_NOW)
                    sfd(m, hV, "hV", regId, ft, dc, DATA_NOW, true)
                    hProductivityByDc = hArea != 0 ? sumHProductivity / hArea : 0.0
                    sumSumHProductivity += (hProductivityByDc * hArea)
                    sumHArea += hArea
                    sfd(m, hProductivityByDc, "hProductivityByDc", regId, ft, dc, DATA_NOW, true)
                    sfd(m, hArea, "harvestedArea", regId, ft, dc, DATA_NOW, true)
                    sfd(m, vMort, "vMort", regId, ft, dc, DATA_NOW, true)
                    sfd(m, vMortAdd, "vMortAdd", regId, ft, dc, DATA_NOW, true)
                    vol_1 = gfd(m, "vol", regId, ft, dc, currentYear - 1)
                    if vol_1 != 0
                        sfd(m, hV / vol_1, "hr", regId, ft, dc, DATA_NOW, true)
                    else
                        sfd(m, 0.0, "hr", regId, ft, dc, DATA_NOW, true)
                    end
                end
            end
            hProductivity = sumHArea != 0 ? sumSumHProductivity / sumHArea : 0.0
            sfd(m, hProductivity, "hProductivity", regId, ft, "", DATA_NOW, true)
            for px in regPx
                vReg += px.vReg[j]
                haskey(px.regArea, currentYear) || msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                regArea += px.regArea[currentYear][j]
                sumAreaByFt += getDoubleValue(G, px, "forArea_" * ft, true) / 10000
                (sumAreaByFt >= 0.0) || msgOut(MSG_CRITICAL_ERROR, "sumAreaByFt is not non negative.")
            end
            sfd(m, vReg, "vReg", regId, ft, "", DATA_NOW, true)
            sfd(m, regArea, "regArea", regId, ft, "", DATA_NOW, true)
            sfd(m, sumAreaByFt, "forArea", regId, ft, "", DATA_NOW, true)
        end
    end
    printDetailedHV(m, hVol_byPrd)

    parentFtypes = getForTypeParents(MD)
    for regId in c.regIds2
        regPx = regionPixels(m, regId)
        totRegionForArea = 0.0
        totSumExpRet = 0.0
        totSumExpRet_byFTParent = zeros(length(parentFtypes))
        totSumExpRet_byFTypes = zeros(length(c.fTypes))
        childIds = [getForTypeChilds(MD, p) for p in parentFtypes]
        childPos = [getForTypeChilds_pos(MD, p) for p in parentFtypes]
        for px in regPx
            pxForArea = vSum(px.area)
            totRegionForArea += pxForArea
            bestPxExpectedRet = getMax(px.expectedReturnsNotCorrByRa)
            for i in eachindex(parentFtypes)
                pxExpReturnsByChilds = zeros(length(childPos[i]))
                for j in eachindex(childPos[i])
                    single = px.expectedReturns[childPos[i][j]]
                    pxExpReturnsByChilds[j] = (childIds[i][j] == "ash") ? 0.0 : single
                    totSumExpRet_byFTypes[childPos[i][j]] += single * pxForArea
                end
                totSumExpRet_byFTParent[i] += getMax(pxExpReturnsByChilds) * pxForArea
            end
            totSumExpRet += bestPxExpectedRet * pxForArea
        end
        for i in eachindex(parentFtypes)
            for j in eachindex(childPos[i])
                cp = childPos[i][j]
                sfd(m, totSumExpRet_byFTypes[cp], "sumExpReturns", regId, c.fTypes[cp], "", DATA_NOW, true)
                sfd(m, totSumExpRet_byFTypes[cp] / totRegionForArea, "expReturns", regId, c.fTypes[cp], "", DATA_NOW, true)
            end
            sfd(m, totSumExpRet_byFTParent[i], "sumExpReturns", regId, parentFtypes[i], "", DATA_NOW, true)
            sfd(m, totSumExpRet_byFTParent[i] / totRegionForArea, "expReturns", regId, parentFtypes[i], "", DATA_NOW, true)
        end
        sfd(m, totSumExpRet, "sumExpReturns", regId, "", "", DATA_NOW, true)
        sfd(m, totSumExpRet / totRegionForArea, "expReturns", regId, "", "", DATA_NOW, true)
    end

    if getBoolSetting(m, "usePathogenModule")
        for regId in c.regIds2
            totalForArea = 0.0
            invadedArea = 0.0
            for px in regionPixels(m, regId)
                invaded = 0
                for ft in c.fTypes, dc in c.dClasses
                    getPathMortality(m, px, ft, dc) > 0 && (invaded = 1)
                end
                totalForArea += vSum(px.area)
                invadedArea += vSum(px.area) * invaded
            end
            sfd(m, invadedArea / totalForArea, "totalShareInvadedArea", regId, "", "", DATA_NOW, true)
        end
    end

    priPr = getStringVectorSetting(m, "priProducts")
    for regId in c.regIds2
        deadTimber = Float64(Float32(getAvailableDeathTimber(m, priPr, regId, currentYear)))  # a float in the C++ code
        sfd(m, deadTimber, "usableDeadTimber", regId, "", "", currentYear, true)
    end
end

function registerCarbonEvents!(m)
    c = m.core
    for r in c.regIds2
        for ft in c.fTypes
            deathBiomass = gfd(m, "vMort", r, ft, DIAM_ALL, DATA_NOW) + gfd(m, "vMortAdd", r, ft, DIAM_ALL, DATA_NOW)
            harvesting = gfd(m, "hV", r, ft, DIAM_ALL, DATA_NOW)
            registerDeathBiomass!(m, deathBiomass, r, ft)
            registerHarvesting!(m, harvesting, r, ft)
        end
        for p in c.priProducts
            registerProducts!(m, gpd(m, "sa", r, p, DATA_NOW), r, p)
        end
        for p in c.secProducts
            registerProducts!(m, gpd(m, "dl", r, p, DATA_NOW), r, p)
        end
    end
    for regs in c.l2r, rfrom in regs
        distQProd = 0.0
        for rto in regs
            dist = gpd(m, "dist", rfrom, "", DATA_NOW, i2s(rto))
            for p in c.allProducts
                distQProd += dist * gpd(m, "rt", rfrom, p, DATA_NOW, i2s(rto))
            end
        end
        registerTransports!(m, distQProd, rfrom)
    end
    HWP_eol2energy!(m)
end

function computeEconomicBalances!(m)
    c = m.core
    MD = m.MD
    previousYear = m.SCD.year - 1
    for regId in c.regIds2
        reg = getRegion(MD, regId)
        for p in c.priProducts
            pol_s = gpd(m, "pol_mktDirInt_s", regId, p)
            gpd(m, "pol_mktDirInt_s", regId, p, previousYear)
            sigma = gpd(m, "sigma", regId, p)
            gpd(m, "in", regId, p); gpd(m, "in", regId, p, previousYear)
            pc = gpd(m, "pc", regId, p)
            gpd(m, "pc", regId, p, previousYear)
            sc = gpd(m, "sc", regId, p)
            gpd(m, "sc", regId, p, previousYear)
            gpd(m, "gamma_incr", regId, p); gpd(m, "gamma_decr", regId, p)
            polBal_mktDirInt_s = pc * (1 - pol_s) * sc
            carbonPrice = gpd(m, "carbonPrice", regId, "", DATA_NOW)
            intRate = getDoubleSetting(m, "ir")
            Pct = carbonPrice * intRate
            omega = gpd(m, "co2content_products", regId, p) / 1000
            bb = gpd(m, "bb", regId, p)
            surplus_prod = pc * sc - (bb * cpow(sc, (sigma + 1) / sigma) + sc * omega * Pct)
            spd(m, polBal_mktDirInt_s, "polBal_mktDirInt_s", regId, p, DATA_NOW, true)
            spd(m, surplus_prod, "surplus_prod", regId, p, DATA_NOW, true)
        end
        for p in c.secProducts
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
            surplus_cons = pc_1 * pol_d_1 * cpow(dc_1, -1 / sigma) * cpow(pol_d, -1) * (sigma / (sigma + 1)) *
                           (cpow(dc, (sigma + 1) / sigma) - cpow(trs * dc0, (sigma + 1) / sigma)) - (dc - trs * dc0) * pc
            spd(m, polBal_mktDirInt_d, "polBal_mktDirInt_d", regId, p, DATA_NOW, true)
            spd(m, polBal_trSub, "polBal_trSub", regId, p, DATA_NOW, true)
            spd(m, surplus_cons, "surplus_cons", regId, p, DATA_NOW, true)
        end
        for p in c.allProducts
            pol_tcSub = gpd(m, "pol_tcSub", regId, p)
            polBal_tcSub = 0.0
            for s in getSiblings(MD, reg)
                ct = gpd(m, "ct", regId, p, DATA_NOW, i2s(s.regId))
                rt = gpd(m, "rt", regId, p, DATA_NOW, i2s(s.regId))
                polBal_tcSub += ct * (pol_tcSub - 1) * rt
            end
            spd(m, polBal_tcSub, "polBal_tcSub", regId, p, DATA_NOW, true)
        end
    end
end

"Move volumes (what=\"vol\") or areas (what=\"area\") to the spatially explicit forest types defined in gis layers"
function loadExogenousForestLayers!(m, what::AbstractString)
    getBoolSetting(m, "useSpExplicitForestTypes") || return
    c = m.core
    G = m.GIS
    fTypes = c.fTypes
    nFT = length(fTypes)
    nDC = length(c.dClasses)
    allPx = vcat([regionPixels(m, r) for r in c.regIds2]...)
    pxC = length(allPx)
    if isempty(c.original_vols)
        c.original_vols = [[zeros(nDC) for _ in 1:nFT] for _ in 1:pxC]
    end
    ov = c.original_vols
    if what == "vol"
        for i in 1:nFT, u in 1:nDC, (p, px) in enumerate(allPx)
            ov[p][i][u] += px.vol[i][u]
        end
        for i in 1:nFT, o in 1:nFT, u in 2:nDC
            layerName = "spInput#vol#$(fTypes[o])#$(fTypes[i])#$(u-1)"
            if layerExist(G, layerName)
                for px in allPx
                    vt = cmin(getDoubleValue(G, px, layerName, true) / 1000000, px.vol[i][u])
                    px.vol[i][u] -= vt
                    px.vol[o][u] += vt
                end
            end
        end
    end
    if what == "area"
        oa = [[zeros(nDC) for _ in 1:nFT] for _ in 1:pxC]
        for i in 1:nFT, u in 1:nDC, (p, px) in enumerate(allPx)
            oa[p][i][u] += px.area_l[i][u]
        end
        ta = [[zeros(nFT) for _ in 1:nFT] for _ in 1:pxC]
        for i in 1:nFT, o in 1:nFT, u in 2:nDC
            layerName = "spInput#vol#$(fTypes[o])#$(fTypes[i])#$(u-1)"
            if layerExist(G, layerName)
                for (p, px) in enumerate(allPx)
                    vol_i_orig = ov[p][i][u]
                    vol_transfer = vol_i_orig != 0 ? getDoubleValue(G, px, layerName, true) / 1000000 : 0.0
                    area_i_orig = oa[p][i][u]
                    area_transfer = vol_i_orig != 0 ? area_i_orig * vol_transfer / vol_i_orig : 0.0
                    px.area_l[i][u] -= area_transfer
                    px.area[i][u] = px.area_l[i][u]
                    px.area_l[o][u] += area_transfer
                    px.area[o][u] = px.area_l[o][u]
                    ta[p][i][o] += area_transfer
                end
            end
        end
        for (p, px) in enumerate(allPx)
            for i in 1:nFT, o in 1:nFT
                area_i_orig = 0.0
                for u in 2:nDC
                    area_i_orig += oa[p][i][u]
                end
                t0 = area_i_orig != 0 ? oa[p][i][1] * (ta[p][i][o] / area_i_orig) : 0.0
                px.area_l[i][1] -= t0
                px.area[i][1] = px.area_l[i][1]
                px.area_l[o][1] += t0
                px.area[o][1] = px.area_l[1][1]   # as in the C++ code
            end
        end
        for (i, ftid) in enumerate(fTypes)
            fti = getForType(m.MD, ftid)
            if fti.memType == 3 || fti.memType == 2
                for px in allPx
                    changeValue!(G, px, fti.forLayer, vSum(px.area[i]) * 10000)
                end
            end
        end
    end
end
