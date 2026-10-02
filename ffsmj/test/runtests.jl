# Tests of the Julia port of FFSM++. Run with:  julia --project=. -e 'using Pkg; Pkg.test()'

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

@testset "Short run of the debug scenario" begin
    inputFile = joinpath(@__DIR__, "..", "data", "ffsmInput.ods")
    m = runModel(inputFile, "debug"; settings = Dict("simulationYears" => 2, "outputLevel" => 0))
    # C++ reference values (6 significant digits, as printed in the C++ output files)
    @test getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, 2012) ≈ 174.062 rtol = 1e-5
    @test getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, 2014) ≈ 179.6 rtol = 1e-5
    @test getProdData(m, "pl", 11002, "hardWRoundW", 2013) ≈ 109.325 rtol = 1e-5
    @test getProdData(m, "pl", 11002, "softWSawnW", 2014) ≈ 196.064 rtol = 1e-5
    @test getProdData(m, "sl", 11002, "softWSawnW", 2014) ≈ 2.32349 rtol = 1e-5

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
