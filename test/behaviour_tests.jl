# BEHAVIOUR TESTS: each t_* scenario must move the model in the expected direction and with a "reasonable"
# magnitude, compared to the t_base scenario (same regions and years, nothing changed).
#
# These tests do not depend on the C++ model: they are meant to stay valid when the Julia model evolves.
# Bounds are deliberately wide: they catch a change that is ignored, applied with the wrong sign or blown
# out of proportion, not small numerical differences. See doc/port_to_julia/tests.md for the rationale.

using Test
isdefined(@__MODULE__, :runScenario) || include("common.jl")
using FFSM: getForTypeIds, getStringVectorSetting, getMultiplier, getDoubleValue, FT_ALL, getStock, STOCK_INV

const Y1 = Y_SHOCK + 1   # first year fully affected by the changes introduced in Y_SHOCK

# --- helpers ---
fts(m) = getForTypeIds(m.MD)
priPr(m) = getStringVectorSetting(m, "priProducts")
secPr(m) = getStringVectorSetting(m, "secProducts")
"Forest variable, summed over the test regions (or on `regions`), all forest types and diameter classes"
fv(m, var, y; regions = REGIONS, ft = FT_ALL) = forTot(m, var, y; regions = regions, ft = ft)
"Product variable, summed over the test regions (or on `regions`)"
pv(m, var, prod, y; regions = REGIONS) = prodTot(m, var, prod, y; regions = regions)
"Total forest area of a region from the forData forArea, summed over the forest types"
forArea(m, reg, y) = sum(getForData(m, "forArea", reg, ft, "", y) for ft in fts(m))
"Total harvest (alive trees) of the test regions"
harvest(m, y) = fv(m, "hV", y)
"x is in [lo, hi]"
within(x, lo, hi) = lo <= x <= hi
regPx(m, reg) = regPixels(m, getRegion(m.MD, reg))
allPx(m) = [px for r in REGIONS for px in regPx(m, r)]

const SELECTED = haskey(ENV, "FFSM_TESTS") ? split(ENV["FFSM_TESTS"], ',') : nothing
"Is (one of) the scenario(s) of a test set selected? (FFSM_TESTS=t_a,t_b,... selects the scenarios)"
selected(name) = SELECTED === nothing || any(in(SELECTED), split(name, '/'))

"Accounting identities that must hold in every scenario"
function checkInvariants(m; fixedArea = true, pixelArea = fixedArea)
    for y in Y_START:Y_END, r in REGIONS
        # the harvested volume is entirely allocated to the primary products
        @test getForData(m, "hV", r, FT_ALL, DIAM_ALL, y) ≈ sum(getProdData(m, "st_fromHarv", r, p, y) for p in priPr(m)) rtol = 1e-8
        # all the harvested area is regenerated (no area change) and nothing more
        fixedArea && @test getForData(m, "harvestedArea", r, FT_ALL, DIAM_ALL, y) ≈ sum(getForData(m, "regArea", r, ft, "", y) for ft in fts(m)) rtol = 1e-8
        fixedArea && @test forArea(m, r, y) ≈ forArea(m, r, Y_START - 1) rtol = 1e-10
        for p in vcat(priPr(m), secPr(m))
            @test getProdData(m, "st", r, p, y) ≈ getProdData(m, "sl", r, p, y) + getProdData(m, "sa", r, p, y) rtol = 1e-10
            @test getProdData(m, "dt", r, p, y) ≈ getProdData(m, "dl", r, p, y) + getProdData(m, "da", r, p, y) rtol = 1e-10
            pl = getProdData(m, "pl", r, p, y)
            @test isfinite(pl) && pl > 0
        end
        @test all([isfinite(getForData(m, "vol", r, ft, DIAM_ALL, y)) && getForData(m, "vol", r, ft, DIAM_ALL, y) >= 0 for ft in fts(m)])
    end
    # pixel level (last year): the harvested volume by product adds up to the harvested volume
    hvp = sum(sum(sum(x) for x in v) for px in allPx(m) for v in px.hVol_byPrd)
    hv = sum(sum(sum(v) for v in px.hVol) for px in allPx(m))
    @test hvp ≈ hv rtol = 1e-8
    @test hv ≈ harvest(m, Y_END) rtol = 1e-8
    # pixel areas are consistent with the regional forest area
    pixelArea && for r in REGIONS
        @test pxArea(m, r) ≈ forArea(m, r, Y_END) rtol = 1e-8
    end
end

@testset "Behaviour of the model under the test scenarios" begin
    B = runScenario("t_base")

    selected("t_base") && @testset "t_base" begin
        checkInvariants(B)
        # the inventory is growing over the period (used below to sign the effect of the elasticities)
        @test fv(B, "vol", Y_END) > fv(B, "vol", Y_START)
        # heterogeneous agents in the base: expectations and risk aversion vary by pixel
        @test length(unique(px.expType for px in allPx(B))) > 1
        @test any([any(px.expectedReturns .< px.expectedReturnsNotCorrByRa) for px in allPx(B)])
        @test any([any(px.spMods .!= 1.0) for px in allPx(B)])
        @test all([getForData(B, "vMortAdd", r, FT_ALL, DIAM_ALL, y) == 0 for r in REGIONS, y in Y_START:Y_END])
    end

    # --- management and agents' behaviour ---
    selected("t_mr0") && @testset "t_mr0" begin
        m = runScenario("t_mr0")
        checkInvariants(m)
        # with no managed regeneration everything is replanted with the same forest type
        for r in REGIONS
            @test sameTypeRegShare(m, r) ≈ 1.0 atol = 1e-10
        end
    end
    selected("t_mr1") && @testset "t_mr1" begin
        m = runScenario("t_mr1")
        checkInvariants(m)
        # all the regeneration follows the expected returns: more forest type changes
        for r in REGIONS
            @test sameTypeRegShare(m, r) < sameTypeRegShare(B, r)
        end
    end
    selected("t_mrTime") && @testset "t_mrTime" begin
        m = runScenario("t_mrTime")
        # mr drops to 0.1 from 2016: mostly natural (same type) regeneration at the end of the period
        for r in REGIONS
            @test sameTypeRegShare(m, r) > sameTypeRegShare(B, r)
            @test sameTypeRegShare(m, r) > 0.8
        end
    end
    selected("t_mrRegional") && @testset "t_mrRegional" begin
        m = runScenario("t_mrRegional")
        # forData mr (0.1) in 11001 has priority over the setting; regional setting 0.95 in 11002
        @test sameTypeRegShare(m, 11001) > sameTypeRegShare(B, 11001)
        @test sameTypeRegShare(m, 11002) < sameTypeRegShare(B, 11002)
        @test sameTypeRegShare(m, 11001) ≈ sameTypeRegShare(runScenario("t_mrTime"), 11001) rtol = 0.05
    end
    selected("t_noRA") && @testset "t_noRA" begin
        m = runScenario("t_noRA")
        checkInvariants(m)
        # no correction of the expected returns for risk
        @test all([px.expectedReturns == px.expectedReturnsNotCorrByRa for px in allPx(m)])
        # the forest type choice changes, but not dramatically
        @test getForData(m, "regArea", 11001, "Fut_Con", "", Y_END) != getForData(B, "regArea", 11001, "Fut_Con", "", Y_END)
        @test abs(relchange(fv(m, "regArea", Y_END; ft = "Fut_Con"), fv(B, "regArea", Y_END; ft = "Fut_Con"))) < 0.2
    end
    selected("t_expNone/t_expFull/t_expFixed") && @testset "t_expNone/t_expFull/t_expFixed" begin
        mn, mf, mx = runScenario("t_expNone"), runScenario("t_expFull"), runScenario("t_expFixed")
        @test all([px.expType == 0.0 && px.expTypePrices == 0.0 for px in allPx(mn)])
        @test all([px.expType == 1.0 && px.expTypePrices == 1.0 for px in allPx(mf)])
        @test all([px.expType == -1.0 for px in allPx(mx)])
        # the base (expType 0.5 ± sd) mixes the adaptive and the full expectations
        for r in REGIONS
            e0, e1, eb = (getForData(x, "expReturns", r, "", "", Y_END) for x in (mn, mf, B))
            @test e0 != e1
            @test min(e0, e1) <= eb <= max(e0, e1)
        end
        # expectations change the investment choices, not the short-term market
        @test abs(relchange(harvest(mn, Y_END), harvest(B, Y_END))) < 0.02
        @test abs(relchange(harvest(mf, Y_END), harvest(B, Y_END))) < 0.02
        @test abs(relchange(harvest(mx, Y_END), harvest(B, Y_END))) < 0.02
    end
    selected("t_noSpVar") && @testset "t_noSpVar" begin
        m = runScenario("t_noSpVar")
        checkInvariants(m)
        @test all([all(px.spMods .== 1.0) for px in allPx(m)])
    end
    selected("t_spPropVol") && @testset "t_spPropVol" begin
        m = runScenario("t_spPropVol")
        checkInvariants(m)
        sm = vcat((px.spMods for px in allPx(m))...)
        @test all(within.(sm, 0.4, 1.6))
        @test sm != vcat((px.spMods for px in allPx(B))...)
        @test within(sum(sm) / length(sm), 0.6, 1.4)
    end
    selected("t_natRegPP") && @testset "t_natRegPP" begin
        m = runScenario("t_natRegPP")
        checkInvariants(m)
        # the natural regeneration goes to the forest types by probability of presence, not to the same type
        for r in REGIONS
            @test sameTypeRegShare(m, r) < sameTypeRegShare(B, r)
        end
    end
    selected("t_regFixed") && @testset "t_regFixed" begin
        m = runScenario("t_regFixed")
        checkInvariants(m)
        # the regeneration volumes stay at their exogenous (input) value, while in the base they become
        # endogenous once the time of passage of the first class has elapsed (2017-2018 for the fast types)
        for ft in fts(m)
            @test all([isapprox(getForData(m, "vReg", 11001, ft, "", y), getForData(B, "vReg", 11001, ft, "", Y_START); rtol = 1e-12) for y in Y_START:Y_END])
        end
        @test fv(B, "vReg", Y_END) != fv(B, "vReg", Y_START)
        @test fv(m, "vReg", Y_START) == fv(B, "vReg", Y_START)
        # little effect on the market within the test period
        @test harvest(m, Y_START) == harvest(B, Y_START)
        @test abs(relchange(harvest(m, Y_END), harvest(B, Y_END))) < 0.01
    end
    selected("t_oldVol2Area") && @testset "t_oldVol2Area" begin
        m = runScenario("t_oldVol2Area")
        checkInvariants(m; pixelArea = false)
        # the old method lets more area enter the first class than what was harvested: the pixel forest
        # area grows a little (the reason of the new method), while the regional forArea stays constant
        for r in REGIONS
            @test within(relchange(pxArea(m, r), forArea(m, r, Y_END)), 0.0, 0.02)
        end
        # the old method does not cap the area entering the first class: harvested/regenerated areas differ
        @test fv(m, "harvestedArea", Y_END) != fv(B, "harvestedArea", Y_END)
        @test abs(relchange(harvest(m, Y_END), harvest(B, Y_END))) < 0.05
    end
    selected("t_noAvalCoef") && @testset "t_noAvalCoef" begin
        m = runScenario("t_noAvalCoef")
        checkInvariants(m)
        # all the inventory is available for harvesting (avalCoef <= 1 no longer applied)
        for y in Y_START:Y_END
            @test pv(m, "in", "hardWRoundW", y) >= pv(B, "in", "hardWRoundW", y)
        end
        @test within(relchange(pv(m, "in", "hardWRoundW", Y1), pv(B, "in", "hardWRoundW", Y1)), 0.001, 0.5)
    end
    selected("t_noDeathTimber") && @testset "t_noDeathTimber" begin
        m = runScenario("t_noDeathTimber")
        checkInvariants(m)
        @test all([pv(m, "in_deathTimber", p, y) == 0 for p in priPr(m), y in Y_START:Y_END])
        @test all([getForData(m, "usableDeadTimber", r, "", "", y) == 0 for r in REGIONS, y in Y_START:Y_END])
        @test pv(B, "in_deathTimber", "softWRoundW", Y1) > 0
        # without dead timber the supply comes entirely from harvesting alive trees
        @test harvest(m, Y1) > harvest(B, Y1)
        @test within(relchange(harvest(m, Y1), harvest(B, Y1)), 0.0, 0.15)
    end

    # --- climate change multipliers ---
    selected("t_ccTable") && @testset "t_ccTable" begin
        m = runScenario("t_ccTable")
        checkInvariants(m)
        # faster growth: more volume; higher coniferous mortality
        @test fv(m, "vol", Y_END) > fv(B, "vol", Y_END)
        @test within(relchange(fv(m, "vol", Y_END), fv(B, "vol", Y_END)), 0.005, 0.3)
        @test fv(m, "vMort", Y1; ft = "Fut_Con") > fv(B, "vMort", Y1; ft = "Fut_Con")
        @test fv(m, "vol", Y_SHOCK - 1) ≈ fv(B, "vol", Y_SHOCK - 1) rtol = 1e-6   # no effect before 2015
    end
    selected("t_ccMaps") && @testset "t_ccMaps" begin
        m = runScenario("t_ccMaps")
        checkInvariants(m)
        px = first(regPx(m, 11001))
        @test getMultiplier(m, px, "tp_multiplier", "Fut_Feu", 2014) == 1.0
        @test getMultiplier(m, px, "tp_multiplier", "Fut_Feu", Y1) == 0.8
        @test getMultiplier(m, px, "tp_multiplier", "Fut_Con", Y1) == 1.0
        @test getMultiplier(m, px, "mortCoef_multiplier", "Fut_Con", Y1) == 1.5
        @test getMultiplier(m, px, "mortCoef_multiplier", "Fut_Feu", Y1) == 1.0
        @test fv(m, "vMort", Y1; ft = "Fut_Con") > fv(B, "vMort", Y1; ft = "Fut_Con")
        # broadleaved high forest becomes more attractive for the regeneration
        @test fv(m, "regArea", Y_END; ft = "Fut_Feu") > fv(B, "regArea", Y_END; ft = "Fut_Feu")
    end
    selected("t_avalCoefFt") && @testset "t_avalCoefFt" begin
        m = runScenario("t_avalCoefFt")
        checkInvariants(m)
        # halved availability: the inventory available to the market is about halved...
        @test within(pv(m, "in", "hardWRoundW", Y1) / pv(B, "in", "hardWRoundW", Y1), 0.35, 0.65)
        # ...and much more for the coniferous products (Fut_Con at 0.2)
        @test pv(m, "in", "softWRoundW", Y1) / pv(B, "in", "softWRoundW", Y1) < pv(m, "in", "hardWRoundW", Y1) / pv(B, "in", "hardWRoundW", Y1)
    end

    # --- market ---
    selected("t_worldPrices") && @testset "t_worldPrices" begin
        m = runScenario("t_worldPrices")
        checkInvariants(m)
        # higher world prices: more exports of primary products, fewer imports of secondary products
        @test sum(pv(m, "sa", p, Y1) for p in priPr(m)) > sum(pv(B, "sa", p, Y1) for p in priPr(m))
        @test sum(pv(m, "da", p, Y1) for p in secPr(m)) < sum(pv(B, "da", p, Y1) for p in secPr(m))
        @test within(relchange(harvest(m, Y1), harvest(B, Y1)), 0.0, 0.3)
        @test pv(m, "sa", "hardWRoundW", Y_SHOCK - 1) ≈ pv(B, "sa", "hardWRoundW", Y_SHOCK - 1) rtol = 1e-6
    end
    selected("t_polSupplySub") && @testset "t_polSupplySub" begin
        m = runScenario("t_polSupplySub")
        checkInvariants(m)
        p, r = "hardWRoundW", 11001
        # subsidised supply in 11001: more supply, lower local price, a cost for the public budget
        @test within(relchange(getProdData(m, "sc", r, p, Y1), getProdData(B, "sc", r, p, Y1)), 0.01, 0.5)
        @test getProdData(m, "pl", r, p, Y1) < getProdData(B, "pl", r, p, Y1)
        @test getProdData(m, "polBal_mktDirInt_s", r, p, Y1) < 0
        @test getProdData(B, "polBal_mktDirInt_s", r, p, Y1) == 0
        # the non subsidised region loses market share
        @test getProdData(m, "sc", 11002, p, Y1) < getProdData(B, "sc", 11002, p, Y1)
    end
    selected("t_polDemandSub") && @testset "t_polDemandSub" begin
        m = runScenario("t_polDemandSub")
        checkInvariants(m)
        p = "hardWSawnW"
        @test within(relchange(pv(m, "dc", p, Y1), pv(B, "dc", p, Y1)), 0.01, 0.5)
        @test getProdData(m, "pl", 11001, p, Y1) > getProdData(B, "pl", 11001, p, Y1)
        @test pv(m, "polBal_mktDirInt_d", p, Y1) < 0
    end
    selected("t_polTrSub") && @testset "t_polTrSub" begin
        m = runScenario("t_polTrSub")
        checkInvariants(m)
        # cheaper transformation: more sawnwood produced, more demand (and price) of its raw material
        @test within(relchange(pv(m, "sl", "softWSawnW", Y1), pv(B, "sl", "softWSawnW", Y1)), 0.002, 0.5)
        @test getProdData(m, "pl", 11001, "softWRoundW", Y1) > getProdData(B, "pl", 11001, "softWRoundW", Y1)
        @test pv(m, "polBal_trSub", "softWSawnW", Y1) < 0
    end
    selected("t_polTcSub") && @testset "t_polTcSub" begin
        m = runScenario("t_polTcSub")
        checkInvariants(m)
        rtTot(x, y) = sum(getProdData(x, "rt", r, p, y, string(r2)) for r in REGIONS, r2 in REGIONS, p in vcat(priPr(x), secPr(x)))
        # cheaper transport: more interregional trade
        @test rtTot(m, Y1) > rtTot(B, Y1)
        @test rtTot(m, Y_SHOCK - 1) ≈ rtTot(B, Y_SHOCK - 1) rtol = 1e-3
    end
    selected("t_polFiSub") && @testset "t_polFiSub" begin
        m = runScenario("t_polFiSub")
        checkInvariants(m)
        # subsidised investments towards Fut_Con: more Fut_Con regeneration, public expenditure
        @test fv(m, "regArea", Y_END; ft = "Fut_Con") > fv(B, "regArea", Y_END; ft = "Fut_Con")
        @test within(relchange(fv(m, "regArea", Y_END; ft = "Fut_Con"), fv(B, "regArea", Y_END; ft = "Fut_Con")), 0.0, 0.5)
        @test fv(m, "polBal_fiSub", Y1; ft = "Fut_Con") < 0
    end
    selected("t_mktStr") && @testset "t_mktStr" begin
        m = runScenario("t_mktStr")
        checkInvariants(m)
        # less competitive markets: less traded, less harvested
        @test sum(pv(m, "dc", p, Y1) for p in secPr(m)) < sum(pv(B, "dc", p, Y1) for p in secPr(m))
        @test within(relchange(harvest(m, Y1), harvest(B, Y1)), -0.6, -0.05)
    end
    selected("t_substitution") && @testset "t_substitution" begin
        m = runScenario("t_substitution")
        checkInvariants(m)
        # NB: es_d enters the demand as (own price / substitute price)^es_d (see tests.md for the sign)
        @test pv(m, "dc", "fuelW", Y1) != pv(B, "dc", "fuelW", Y1)
        @test abs(relchange(pv(m, "dc", "fuelW", Y1), pv(B, "dc", "fuelW", Y1))) < 0.3
        # hardwood and softwood sawnwood move in opposite directions
        dh = relchange(pv(m, "dc", "hardWSawnW", Y_END), pv(B, "dc", "hardWSawnW", Y_END))
        ds = relchange(pv(m, "dc", "softWSawnW", Y_END), pv(B, "dc", "softWSawnW", Y_END))
        @test dh * ds < 0
    end
    selected("t_addDemand") && @testset "t_addDemand" begin
        m = runScenario("t_addDemand")
        checkInvariants(m)
        p = "hardWSawnW"
        dcDiff(y) = getProdData(m, "dc", 11001, p, y) - getProdData(B, "dc", 11001, p, y)
        # 0.3 Mm3 given once in 2015 (then 0): a permanent increase of the consumption in 11001, a bit less
        # than 0.3 because the price goes up
        @test dcDiff(Y_SHOCK - 1) == 0
        @test all([within(dcDiff(y), 0.05, 0.3) for y in Y_SHOCK:Y_END])
        @test maximum(dcDiff.(Y_SHOCK:Y_END)) / minimum(dcDiff.(Y_SHOCK:Y_END)) < 1.2
        @test all([getProdData(m, "pl", 11001, p, y) > getProdData(B, "pl", 11001, p, y) for y in Y_SHOCK:Y_END])
        @test harvest(m, Y1) > harvest(B, Y1)
    end
    selected("t_addDemandShock") && @testset "t_addDemandShock" begin
        m, mp = runScenario("t_addDemandShock"), runScenario("t_addDemand")
        checkInvariants(m)
        p = "hardWSawnW"
        dcDiff(x, y) = getProdData(x, "dc", 11001, p, y) - getProdData(B, "dc", 11001, p, y)
        # 0.3 in 2015, -0.3 in 2016: same as the permanent shift in 2015, then the shock is withdrawn
        @test dcDiff(m, Y_SHOCK) == dcDiff(mp, Y_SHOCK)
        @test all([dcDiff(m, y) < 0.5 * dcDiff(mp, y) for y in (Y_SHOCK+2):Y_END])
        @test all([getProdData(m, "pl", 11001, p, y) < getProdData(mp, "pl", 11001, p, y) for y in (Y_SHOCK+1):Y_END])
        # the withdrawal is only partial: the curve recalibrated on the 2015 consumption includes the shock as
        # price-elastic demand (scaled ~1.7x instead of shifted by 0.3); -0.3 doesn't fully undo it, the price
        # drops in 2016 (308 vs 490) and +0.08 Mm3 (~18% of the base consumption) remain. See tests.md.
        @test_broken abs(dcDiff(m, Y_END)) < 0.02
    end
    selected("t_elasticity") && @testset "t_elasticity" begin
        m = runScenario("t_elasticity")
        checkInvariants(m)
        # the inventory grows (t_base): a supply more elastic to it gives more harvest
        @test harvest(m, Y_END) > harvest(B, Y_END)
        @test within(relchange(harvest(m, Y_END), harvest(B, Y_END)), 0.0, 0.2)
    end

    # --- carbon ---
    selected("t_carbonPrice") && @testset "t_carbonPrice" begin
        m = runScenario("t_carbonPrice")
        checkInvariants(m)
        # carbon emissions of the harvest are a cost for the producers: less harvest, more stock
        @test harvest(m, Y1) < harvest(B, Y1)
        @test within(relchange(harvest(m, Y1), harvest(B, Y1)), -0.2, 0.0)
        @test sum(getStock(m, r, STOCK_INV) for r in REGIONS) > sum(getStock(B, r, STOCK_INV) for r in REGIONS)
        @test all([all(px.expectedAnnualIncome_carbon .== 0) for px in allPx(m)])   # no Hartman payments
    end
    selected("t_hartman") && @testset "t_hartman" begin
        mc, me, mn, mp = runScenario("t_carbonPrice"), runScenario("t_hartmanExo"), runScenario("t_hartmanEndo"), runScenario("t_pickling")
        for x in (me, mn, mp)
            checkInvariants(x)
            # carbon payments enter the expected returns (and the forest type choice)...
            @test any([any(px.expectedAnnualIncome_carbon .> 0) for px in allPx(x)])
            @test getForData(x, "expReturns", 11001, "", "", Y_END) > getForData(mc, "expReturns", 11001, "", "", Y_END)
            # ...but not the short-term market (the regeneration has not reached the market yet)
            @test harvest(x, Y1) ≈ harvest(mc, Y1) rtol = 1e-6
        end
        @test getForData(me, "expReturns", 11001, "", "", Y_END) != getForData(mn, "expReturns", 11001, "", "", Y_END)
        # with pickling part of the harvested carbon stays stored: less penalty for harvesting
        @test getForData(mp, "expReturns", 11001, "", "", Y_END) > getForData(me, "expReturns", 11001, "", "", Y_END)
    end

    # --- disturbances ---
    selected("t_windstorm") && @testset "t_windstorm" begin
        m = runScenario("t_windstorm")
        checkInvariants(m)
        # additional mortality only in 11001, only in 2016
        @test getForData(m, "vMortAdd", 11001, FT_ALL, DIAM_ALL, 2016) > 0
        @test getForData(m, "vMortAdd", 11002, FT_ALL, DIAM_ALL, 2016) == 0
        @test all([getForData(m, "vMortAdd", 11001, FT_ALL, DIAM_ALL, y) == 0 for y in Y_START:Y_END if y != 2016])
        # about 40% of the Fut_Con volume of the affected classes, i.e. a few % up to ~30% of the regional stock
        @test within(getForData(m, "vMortAdd", 11001, FT_ALL, DIAM_ALL, 2016) / getForData(B, "vol", 11001, FT_ALL, DIAM_ALL, 2015), 0.01, 0.3)
        # the dead timber feeds the market: less alive trees harvested the year after
        @test getForData(m, "usableDeadTimber", 11001, "", "", 2016) > 10 * getForData(B, "usableDeadTimber", 11001, "", "", 2016)
        @test pv(m, "in_deathTimber", "softWRoundW", 2017; regions = (11001,)) > pv(B, "in_deathTimber", "softWRoundW", 2017; regions = (11001,))
        @test getForData(m, "hV", 11001, FT_ALL, DIAM_ALL, 2017) < getForData(B, "hV", 11001, FT_ALL, DIAM_ALL, 2017)
        @test fv(m, "vol", Y_END; regions = (11001,)) < fv(B, "vol", Y_END; regions = (11001,))
        # before the storm only a small anticipation effect: agents with (partially) full expectations read
        # the future mortality layers and change their harvesting choices (-0.2% harvest in 2015)
        @test fv(m, "vol", 2013) ≈ fv(B, "vol", 2013) rtol = 1e-8
        @test abs(relchange(harvest(m, 2015), harvest(B, 2015))) < 0.01
    end
    selected("t_windstormSto") && @testset "t_windstormSto" begin
        m, mw = runScenario("t_windstormSto"), runScenario("t_windstorm")
        checkInvariants(m)
        # dead timber usable for 4 years instead of the master value: still available in 2018
        dead(x, y) = getForData(x, "usableDeadTimber", 11001, "", "", y)
        @test dead(m, 2018) > dead(mw, 2018)
        @test dead(m, 2018) > 0.1 * dead(m, 2016)
        # carbon stock of the inventory (alive + dead biomass) increases with the windstorm (dead biomass)
        @test sum(getStock(m, r, STOCK_INV) for r in REGIONS) > sum(getStock(B, r, STOCK_INV) for r in REGIONS)
    end
    selected("t_pathogen") && @testset "t_pathogen" begin
        m = runScenario("t_pathogen")
        checkInvariants(m)
        # the pathogen is present only in 11001 and increases the broadleaved mortality there
        @test getForData(m, "totalShareInvadedArea", 11001, "", "", Y1) > 0
        @test getForData(m, "totalShareInvadedArea", 11002, "", "", Y1) == 0
        @test getForData(m, "vMort", 11001, "Fut_Feu", DIAM_ALL, Y1) > 2 * getForData(B, "vMort", 11001, "Fut_Feu", DIAM_ALL, Y1)
        @test getForData(m, "vMort", 11002, "Fut_Feu", DIAM_ALL, Y1) ≈ getForData(B, "vMort", 11002, "Fut_Feu", DIAM_ALL, Y1) rtol = 0.02
        @test getForData(m, "vMort", 11001, "Fut_Con", DIAM_ALL, Y1) ≈ getForData(B, "vMort", 11001, "Fut_Con", DIAM_ALL, Y1) rtol = 0.02
        # mortality coefficients increase with the years of presence (0.01, 0.03, 0.06)
        @test getForData(m, "vMort", 11001, "Fut_Feu", DIAM_ALL, 2016) > getForData(m, "vMort", 11001, "Fut_Feu", DIAM_ALL, 2014)
        @test getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, Y_END) < getForData(B, "vol", 11001, "Fut_Feu", DIAM_ALL, Y_END)
    end
    selected("t_spExplicitFt") && @testset "t_spExplicitFt" begin
        m = runScenario("t_spExplicitFt")
        checkInvariants(m)
        @test "Fut_Hetre" in fts(m)
        # Fut_Hetre exists only where its input maps put it (11001), taken from Fut_Feu: same total area
        @test getForData(m, "vol", 11001, "Fut_Hetre", DIAM_ALL, Y_START) > 0
        # in 11002 Fut_Hetre only comes from the regeneration choices (young classes only)
        @test getForData(m, "vol", 11001, "Fut_Hetre", DIAM_ALL, Y_START) > 20 * getForData(m, "vol", 11002, "Fut_Hetre", DIAM_ALL, Y_START)
        @test all([getForData(m, "vol", 11002, "Fut_Hetre", dc, Y_START) == 0 for dc in m.core.dClasses[4:end]])
        @test all([getForData(m, "vol", 11001, "Fut_Hetre", dc, Y_START) > 0 for dc in m.core.dClasses[4:8]])
        @test getForData(m, "forArea", 11001, "Fut_Hetre", "", Y_START) > 0
        for r in REGIONS
            @test forArea(m, r, Y_END) ≈ forArea(B, r, Y_END) rtol = 1e-8
        end
        @test getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, Y_START) < getForData(B, "vol", 11001, "Fut_Feu", DIAM_ALL, Y_START)
    end

    # --- forest area changes ---
    selected("t_areaRel/t_areaAbs/t_areaDecr") && @testset "t_areaRel/t_areaAbs/t_areaDecr" begin
        mr, ma, md = runScenario("t_areaRel"), runScenario("t_areaAbs"), runScenario("t_areaDecr")
        for x in (mr, ma, md)
            checkInvariants(x; fixedArea = false)
        end
        for r in REGIONS, y in (Y_START+1):Y_END
            @test forArea(mr, r, y) / forArea(mr, r, y - 1) ≈ 1.01 rtol = 1e-3
            @test forArea(md, r, y) / forArea(md, r, y - 1) ≈ 0.997 rtol = 1e-3
            @test forArea(ma, r, y) > forArea(ma, r, y - 1)
        end
        for r in REGIONS
            inc = [forArea(ma, r, y) - forArea(ma, r, y - 1) for y in (Y_START+1):Y_END]
            @test maximum(inc) / minimum(inc) < 1.5    # roughly constant yearly increments (hectares)
            @test pxArea(mr, r) ≈ forArea(mr, r, Y_END) rtol = 1e-6
        end
        # new forest area is planted: more regeneration area than in the base, less with the decrease
        @test fv(mr, "regArea", Y_END) > fv(B, "regArea", Y_END)
        @test fv(md, "regArea", Y_END) < fv(B, "regArea", Y_END)
    end

    # --- output options and input mechanisms ---
    selected("t_base") && @testset "Cached input tables and file without extension" begin
        # forData/prodData read from cachedInput/*.csv and an extra file without extension in the spatial data
        # folder (both used to fail in C++): same results as reading the ODS sheets
        d = mktempdir(; prefix = "ffsm_robust_")
        input = prepareRobustnessDataDir(d)
        log = redirect_stdout(() -> (runModel(input, "t_base"); nothing), devnull)
        for f in ("forestData_t_base.csv", "productData_t_base.csv", "carbonBalance_t_base.csv")
            @test read(joinpath(d, "output", "results", f), String) == read(joinpath(outputDir(), "results", f), String)
        end
        rm(d; recursive = true)
    end
    selected("t_outHR") && @testset "t_outHR" begin
        runScenario("t_outHR")
        f = joinpath(outputDir(), "results", "forestData_t_outHR_renamed.csv")
        @test isfile(f)
        lines = readlines(f)
        @test startswith(lines[1], "scen;parName;country;region;forType;freeDim;")
        @test all([startswith(l, "t_outHR_renamed;") for l in lines[2:end]])
        @test any(l -> occursin(";AlsChaLor;", l), lines)                                # region names, not ids
        @test any(l -> occursin(r"^[^;]*;vol;[^;]*;[^;]*;Fut_Feu;10;", l), lines)        # by diameter class
        @test isfile(joinpath(outputDir(), "results", "productData_t_outHR_renamed.csv"))
    end
    selected("t_outSingle") && @testset "t_outSingle" begin
        runScenario("t_outSingle")
        f = joinpath(outputDir(), "results", "forestData.csv")
        @test isfile(f)
        lines = readlines(f)
        @test lines[1] == "scen;parName;country;region;forType;freeDim;year;value;"
        @test Set(split(l, ';')[7] for l in lines[2:end] if startswith(l, "t_outSingle;")) == Set(["2013", "2016", "2020"])
        maps = readdir(joinpath(outputDir(), "maps", "asciiGrids"))
        mine = filter(x -> endswith(x, "_t_outSingle"), maps)
        @test !isempty(mine) && all([occursin("_2016_", x) for x in mine])
        g = readlines(joinpath(outputDir(), "maps", "asciiGrids", "vol_Fut_Feu_2016_t_outSingle"))
        nc = parse(Int, split(g[1])[2])
        nr = parse(Int, split(g[2])[2])
        @test startswith(g[1], "ncols")
        data = filter(!isempty, g[7:end])
        @test length(data) == nr && all([length(split(l)) == nc for l in data])
    end
    selected("t_settingFile") && @testset "t_settingFile" begin
        m, m1 = runScenario("t_settingFile"), runScenario("t_mr1")
        # same settings as t_mr1, read from a csv file: same results
        @test harvest(m, Y_END) == harvest(m1, Y_END)
        @test all([sameTypeRegShare(m, r) == sameTypeRegShare(m1, r) for r in REGIONS])
        @test fv(m, "regArea", Y_END; ft = "Fut_Con") == fv(m1, "regArea", Y_END; ft = "Fut_Con")
    end
    selected("t_newSeed") && @testset "t_newSeed" begin
        m = runScenario("t_newSeed")
        checkInvariants(m)
        # different random draws (agents' characteristics), statistically equivalent results
        @test [px.expType for px in allPx(m)] != [px.expType for px in allPx(B)]
        @test abs(relchange(harvest(m, Y_END), harvest(B, Y_END))) < 0.02
        @test abs(relchange(fv(m, "vol", Y_END), fv(B, "vol", Y_END))) < 0.02
    end
    selected("t_shuffle") && @testset "t_shuffle" begin
        m = runScenario("t_shuffle")
        checkInvariants(m)
        # every layer read from file is a permutation of the original one
        for layer in ("dtm", "regLev_2")
            a, b = B.GIS.values[layer], m.GIS.values[layer]
            @test sort(a) == sort(b)
            @test a != b
        end
        # the regions are reshuffled too: different forest areas, the model still runs consistently
        @test pxArea(m, 11001) != pxArea(B, 11001)
    end
end
