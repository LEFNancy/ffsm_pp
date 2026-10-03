# Tests of the Julia port of FFSM++. Run with:  julia --project=. -e 'using Pkg; Pkg.test()'
#
# 1. unit tests of the C++ compatible helpers and a short run of the debug scenario (this file);
# 2. scenario tests on test/data/ffsm_tests.ods (all the t_* scenarios, each run once and shared):
#    - exact_tests.jl:     outputs equal to the C++ ones (test/reference/) within the solver tolerance;
#    - behaviour_tests.jl: direction and magnitude of the effect of each scenario, accounting identities.
#
# Environment variables:
#    FFSMJ_SKIP_EXACT=1      skip the exact (C++ reproduction) tests
#    FFSMJ_SKIP_SCENARIOS=1  skip all the scenario tests (only the unit tests and the short run)
#    FFSMJ_TESTS=t_a,t_b     run only these scenarios
# See doc/port_to_julia/tests.md.

using Test, FFSMj
using FFSMj: MT19937, next_u32!, NormalDist, normSample, getMaxPos, getMinPos, createCombinationsVector,
             fmtg, s2i, s2d, getForData, getProdData, DIAM_ALL, DataMap, mapinsert!, prefixValues

@testset "C++ compatible helpers" begin
    # values produced by g++ (libstdc++) with std::mt19937(0) and std::normal_distribution
    g = MT19937(0)
    @test [next_u32!(g) for _ in 1:3] == UInt32[2357136044, 2546248239, 3071714933]
    d = NormalDist(1.0, 0.3)
    @test [d(g) for _ in 1:3] == [1.2936213849866203, 1.6722679814376837, 1.5602673905076454]
    @test normSample(NormalDist(0.5, 10.0), MT19937(1), 0.0, 1.0) <= 1.0
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
end

@testset "Map images" begin
    # a 3x2 integer layer: every pixel, including the last row/column, is painted with its legend colour
    G = FFSMj.Gis()
    G.xNPixels, G.yNPixels, G.xyNPixels = 3, 2, 6
    G.subXL, G.subXR, G.subYT, G.subYB = 0, 2, 0, 1
    L = FFSMj.Layer("t", "t", true, true, "", true, [FFSMj.LegendItem(1, "one", 10, 20, 30, 1, 1)])
    G.values["t"] = [1.0, 1.0, 1.0, 1.0, 1.0, G.noValue]
    img = FFSMj.layerImage(G, L)
    @test size(img) == (3, 3, 2)
    @test img[:, 3, 1] == [10, 20, 30] && img[:, 1, 2] == [10, 20, 30]   # last column / last row painted
    @test img[:, 3, 2] == [255, 255, 255]                                 # no-value pixel stays white
end

@testset "Spatio-temporal layer look-up" begin
    # the layer of a year is the last one of the series up to that year; none before the first year of the
    # series, even when that layer is the first key of the whole (sorted) layer map (bug fixed in C++ too)
    G = FFSMj.Gis()
    for k in ("additionalMortality#Fut_Con#20#2016", "additionalMortality#Fut_Con#20#2017", "avalCoef###")
        G.values[k] = [1.0]
        push!(G.sortedKeys, k)
    end
    G.keysSorted = false
    @test FFSMj._stResolve(G, "additionalMortality#Fut_Con#20#2014") === nothing
    @test FFSMj._stResolve(G, "additionalMortality#Fut_Con#20#2016") == "additionalMortality#Fut_Con#20#2016"
    @test FFSMj._stResolve(G, "additionalMortality#Fut_Con#20#2030") == "additionalMortality#Fut_Con#20#2017"
    @test FFSMj._stResolve(G, "additionalMortality#Fut_Con#30#2016") === nothing
    @test FFSMj._stResolve(FFSMj.Gis(), "avalCoef###2016") === nothing        # empty layer map
end

@testset "Short run of the debug scenario" begin
    inputFile = joinpath(@__DIR__, "..", "data", "ffsmInput.ods")
    m = runModel(inputFile, "debug"; settings = Dict("simulationYears" => 2, "outputLevel" => 0))
    # C++ reference values (6 significant digits, as printed in the C++ output files)
    @test getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, 2012) ≈ 174.062 rtol = 1e-5
    @test getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, 2014) ≈ 179.6 rtol = 1e-5
    @test getProdData(m, "pl", 11002, "hardWRoundW", 2013) ≈ 109.325 rtol = 1e-5
    @test getProdData(m, "pl", 11002, "softWSawnW", 2014) ≈ 196.064 rtol = 1e-5
    @test getProdData(m, "sl", 11002, "softWSawnW", 2014) ≈ 2.32349 rtol = 1e-5
    # time series bounds: setting a value before the initial year is an error (was an infinite loop in C++)
    v = [1.0, 2.0]
    FFSMj.setTimedData!(m, 3.0, v, 2013)
    @test v == [1.0, 2.0, 3.0]
    @test_throws FFSMj.FFSMCriticalError FFSMj.setTimedData!(m, 3.0, v, 2010)
    # reading a year before the initial year: the first value (with a warning), after the last year: the last one
    @test FFSMj.getTimedData(m, v, 2005) == 1.0
    @test FFSMj.getTimedData(m, v, 2030) == 3.0

    # initialRandomShuffle: every layer read from file is a permutation of the original one, with the same no-data mask
    ms = runModel(inputFile, "debug"; settings = Dict("simulationYears" => 1, "outputLevel" => 0, "initialRandomShuffle" => true))
    for layer in ("dtm", "regLev_2", "avalCoef###")
        a, b = m.GIS.values[layer], ms.GIS.values[layer]
        @test (a .== m.GIS.noValue) == (b .== ms.GIS.noValue)
        @test sort(a) == sort(b)
        @test a != b
    end
    # same seed (newRandomSeed = false): same permutation at every run
    ms2 = runModel(inputFile, "debug"; settings = Dict("simulationYears" => 1, "outputLevel" => 0, "initialRandomShuffle" => true))
    @test ms.GIS.values["regLev_2"] == ms2.GIS.values["regLev_2"]
end

if get(ENV, "FFSMJ_SKIP_SCENARIOS", "0") != "1"
    include("common.jl")
    @testset "Test scenarios" begin    # one parent test set: a failure in the exact tests doesn't stop the behaviour ones
        include("exact_tests.jl")
        include("behaviour_tests.jl")
    end
end
