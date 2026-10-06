# Tests of FFSM. Run with:  julia --project=. -e 'using Pkg; Pkg.test()'
#
# 1. unit tests of the helpers, a short run of the debug scenario and the random seeding (this file);
# 2. scenario tests on test/data/ffsm_tests.ods (all the t_* scenarios, each run once):
#    behaviour_tests.jl, direction and magnitude of the effect of each scenario, accounting identities.
#
# Environment variables:
#    FFSM_SKIP_SCENARIOS=1  skip the scenario tests (only the unit tests and the short runs)
#    FFSM_TESTS=t_a,t_b     run only these scenarios
# See doc/port_to_julia/tests.md.

using Test, FFSM
using FFSM: getMaxPos, getMinPos, createCombinationsVector, fmtg, s2i, s2d, getForData, getProdData, DIAM_ALL,
             DataMap, mapinsert!, prefixValues, truncatedNormalDraw
using StableRNGs: StableRNG

@testset "Helpers" begin
    # std::minmax_element: max is the last of the largest elements, min the first of the smallest
    @test getMaxPos([1.0, 3.0, 3.0, 2.0]) == 3
    @test getMinPos([2.0, 1.0, 1.0]) == 2
    @test createCombinationsVector(2) == [Int[], [1], [2], [1, 2]]
    @test fmtg(0.000123456789) == "0.000123457"
    @test fmtg(1.0e7) == "1e+07"
    @test s2i("10.5") == 10 && s2i("abc") == 0
    @test s2d("2.5e3") == 2500.0
    dm = DataMap()
    mapinsert!(dm, "a#1#p##", [1.0])
    mapinsert!(dm, "a#1#p#x#", [2.0])
    mapinsert!(dm, "a#1#p2##", [4.0])
    @test sum(first.(prefixValues(dm, "a#1#p#"))) == 3.0
    # truncated normal: within the bounds; with σ = 0 the mean, also on a bound (Distributions never ends there)
    @test all(0 <= truncatedNormalDraw(StableRNG(1), 0.1, 0.5, 0.0, 1.0) <= 1 for _ in 1:1000)
    @test truncatedNormalDraw(StableRNG(1), 0.0, 0.0, 0.0, 1.0) == 0.0
    @test truncatedNormalDraw(StableRNG(1), 1.0, 0.0, 0.0, 1.0) == 1.0
end

@testset "Map images" begin
    # a 3x2 integer layer: every pixel, including the last row/column, is painted with its legend colour
    G = FFSM.Gis()
    G.xNPixels, G.yNPixels, G.xyNPixels = 3, 2, 6
    G.subXL, G.subXR, G.subYT, G.subYB = 0, 2, 0, 1
    L = FFSM.Layer("t", "t", true, true, "", true, [FFSM.LegendItem(1, "one", 10, 20, 30, 1, 1)])
    G.values["t"] = [1.0, 1.0, 1.0, 1.0, 1.0, G.noValue]
    img = FFSM.layerImage(G, L)
    @test size(img) == (3, 3, 2)
    @test img[:, 3, 1] == [10, 20, 30] && img[:, 1, 2] == [10, 20, 30]   # last column / last row painted
    @test img[:, 3, 2] == [255, 255, 255]                                 # no-value pixel stays white
end

@testset "Spatio-temporal layer look-up" begin
    # the layer of a year is the last one of the series up to that year; none before the first year of the
    # series, even when that layer is the first key of the whole (sorted) layer map (bug fixed in C++ too)
    G = FFSM.Gis()
    for k in ("additionalMortality#Fut_Con#20#2016", "additionalMortality#Fut_Con#20#2017", "avalCoef###")
        G.values[k] = [1.0]
        push!(G.sortedKeys, k)
    end
    G.keysSorted = false
    @test FFSM._stResolve(G, "additionalMortality#Fut_Con#20#2014") === nothing
    @test FFSM._stResolve(G, "additionalMortality#Fut_Con#20#2016") == "additionalMortality#Fut_Con#20#2016"
    @test FFSM._stResolve(G, "additionalMortality#Fut_Con#20#2030") == "additionalMortality#Fut_Con#20#2017"
    @test FFSM._stResolve(G, "additionalMortality#Fut_Con#30#2016") === nothing
    @test FFSM._stResolve(FFSM.Gis(), "avalCoef###2016") === nothing        # empty layer map
end

@testset "Short runs of the debug scenario" begin
    inputFile = joinpath(@__DIR__, "..", "data", "ffsmInput.ods")
    run(settings...) = runModel(inputFile, "debug"; settings = Dict{String,Any}("simulationYears" => 1, "outputLevel" => 0, settings...))
    expTypes(m) = [px.expType for px in m.GIS.pxVector if px.l2region !== nothing]
    m = run()
    # the expectation types are bounded to [0, 1], also with a large coefficient of variation
    mcv = run("expType_cv" => 1.0, "expTypePrices_cv" => 1.0)
    @test all(0 <= px.expType <= 1 && 0 <= px.expTypePrices <= 1 for px in mcv.GIS.pxVector if px.l2region !== nothing)
    @test getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, 2013) > 0
    @test getProdData(m, "pl", 11002, "softWSawnW", 2013) > 0

    # time series bounds: setting a value before the initial year is an error (was an infinite loop in C++)
    v = [1.0, 2.0]
    FFSM.setTimedData!(m, 3.0, v, 2013)
    @test v == [1.0, 2.0, 3.0]
    @test_throws FFSM.FFSMCriticalError FFSM.setTimedData!(m, 3.0, v, 2010)
    # reading a year before the initial year: the first value (with a warning), after the last year: the last one
    @test FFSM.getTimedData(m, v, 2005) == 1.0
    @test FFSM.getTimedData(m, v, 2030) == 3.0

    # random seeding: the same randomSeed gives the same run, another randomSeed or newRandomSeed a different one
    m2 = run()
    @test expTypes(m2) == expTypes(m)
    @test getProdData(m2, "pl", 11002, "softWSawnW", 2013) == getProdData(m, "pl", 11002, "softWSawnW", 2013)
    m7 = run("randomSeed" => 7)
    @test expTypes(m7) != expTypes(m)
    mn = run("newRandomSeed" => true)
    @test expTypes(mn) != expTypes(m)
    @test startswith(mn.scenarioName, "debug_")

    # initialRandomShuffle: every layer read from file is a permutation of the original one, with the same no-data mask
    ms = run("initialRandomShuffle" => true)
    for layer in ("dtm", "regLev_2", "avalCoef###")
        a, b = m.GIS.values[layer], ms.GIS.values[layer]
        @test (a .== m.GIS.noValue) == (b .== ms.GIS.noValue)
        @test sort(a) == sort(b)
        @test a != b
    end
    # same seed: same permutation at every run
    ms2 = run("initialRandomShuffle" => true)
    @test ms.GIS.values["regLev_2"] == ms2.GIS.values["regLev_2"]
end

if get(ENV, "FFSM_SKIP_SCENARIOS", "0") != "1"
    include("common.jl")
    include("behaviour_tests.jl")
end
