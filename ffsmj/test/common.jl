# Shared helpers of the test suite (data preparation, cached scenario runs, output readers).

using FFSMj
using FFSMj: getForData, getProdData, getRegion, regPixels, DIAM_ALL, vSum
import FFSMj.Zlib_jll

const TEST_SRC = joinpath(@__DIR__, "data")                    # ffsm_tests.ods, scenarios/*.csv, gis/tests/
const MODEL_DATA = normpath(joinpath(@__DIR__, "..", "data"))  # model data folder (gis, output skeleton)
const REFERENCE_DIR = joinpath(@__DIR__, "reference")          # C++ reference outputs (gzipped)
const TEST_INPUT = "ffsm_tests.ods"

# scenarios whose outputs are not reproducible between C++ and Julia by construction
const NOT_EXACT = ["t_newSeed", "t_shuffle"]
# test years
const Y_START = 2013
const Y_END = 2020
const Y_SHOCK = 2015    # most scenario changes start this year
const REGIONS = (11001, 11002)

"Copy a folder tree; with `skeletonOnly` only the folders and their placeholder files are copied"
function copyDataTree(src, dst; skeletonOnly = false)
    for (root, dirs, files) in walkdir(src)
        rel = relpath(root, src)
        d = rel == "." ? dst : joinpath(dst, rel)
        mkpath(d)
        for f in files
            (skeletonOnly && f != "00_doNotRemove.txt" && !endswith(f, ".tar.gz")) && continue
            cp(joinpath(root, f), joinpath(d, f); force = true)
        end
    end
end

"""
    prepareTestDataDir(dest)

Create in `dest` a complete data folder for the test scenarios: the model gis data and output skeleton,
the test input file, the scenario csv settings and the synthetic test layers (test/data/gis/tests, written by
test/test_layers.jl). Returns the input file path.
"""
function prepareTestDataDir(dest)
    copyDataTree(joinpath(MODEL_DATA, "gis"), joinpath(dest, "gis"))
    copyDataTree(joinpath(MODEL_DATA, "output"), joinpath(dest, "output"); skeletonOnly = true)
    mkpath(joinpath(dest, "cachedInput"))
    cp(joinpath(TEST_SRC, TEST_INPUT), joinpath(dest, TEST_INPUT); force = true)
    cp(joinpath(TEST_SRC, "scenarios"), joinpath(dest, "scenarios"); force = true)
    cp(joinpath(TEST_SRC, "gis", "tests"), joinpath(dest, "gis", "tests"); force = true)
    return joinpath(dest, TEST_INPUT)
end

"The t_* scenarios of the test input file"
function testScenarios()
    t = FFSMj.loadInput(joinpath(TEST_SRC, TEST_INPUT), TEST_SRC * "/")
    sc = only(filter(x -> x.tableName == "scenarios", t))
    return [r[1] for r in sc.records if startswith(r[1], "t_")]
end

# --- cached runs (each scenario is run once per test session) ---

const TEST_DATA_DIR = Ref{String}("")
const RUNS = Dict{String,Any}()

function testDataDir()
    if TEST_DATA_DIR[] == ""
        d = mktempdir(; prefix = "ffsmj_tests_")
        prepareTestDataDir(d)
        TEST_DATA_DIR[] = d
    end
    return TEST_DATA_DIR[]
end

"Run (once) a test scenario with the Julia model; returns the model object"
function runScenario(name::AbstractString; settings = Dict{String,Any}())
    key = isempty(settings) ? String(name) : string(name, settings)
    get!(RUNS, key) do
        input = joinpath(testDataDir(), TEST_INPUT)
        redirect_stdout(devnull) do
            runModel(input, name; settings = settings)
        end
    end
end

"Output folder of the Julia test runs"
outputDir() = joinpath(testDataDir(), "output")

# --- readers ---

"Read a (possibly gzipped) text file"
function readText(path)
    endswith(path, ".gz") || return read(path, String)
    gz = ccall((:gzopen, Zlib_jll.libz), Ptr{Cvoid}, (Cstring, Cstring), path, "rb")
    gz == C_NULL && error("cannot open $path")
    buf = Vector{UInt8}(undef, 1 << 20)
    out = IOBuffer()
    while true
        n = ccall((:gzread, Zlib_jll.libz), Cint, (Ptr{Cvoid}, Ptr{UInt8}, Cuint), gz, buf, length(buf))
        n <= 0 && break
        write(out, view(buf, 1:n))
    end
    ccall((:gzclose, Zlib_jll.libz), Cint, (Ptr{Cvoid},), gz)
    return String(take!(out))
end

# --- model quantities used by the tests ---

"Sum over the test regions of a forest variable (all forest types and diameter classes)"
forTot(m, var, year; regions = REGIONS, ft = FFSMj.FT_ALL) = sum(getForData(m, var, r, ft, DIAM_ALL, year) for r in regions)
"Sum over the test regions of a product variable"
prodTot(m, var, prod, year; regions = REGIONS) = sum(getProdData(m, var, r, prod, year) for r in regions)
"Total forest area of the pixels of a region [ha]"
pxArea(m, reg) = sum(vSum(px.area) for px in regPixels(m, getRegion(m.MD, reg)))
"Share of the regeneration of the last year that stays on the same forest type (diagonal of deltaArea)"
function sameTypeRegShare(m, reg)
    d = 0.0
    t = 0.0
    for px in regPixels(m, getRegion(m.MD, reg))
        n = length(px.deltaArea) - 1
        for i in 1:n, j in 1:n
            t += px.deltaArea[i][j]
            i == j && (d += px.deltaArea[i][j])
        end
    end
    return t > 0 ? d / t : NaN
end
relchange(new, old) = (new - old) / abs(old)
