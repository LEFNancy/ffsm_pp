# EXACT TESTS: the outputs of the Julia model must reproduce those of the C++ FFSM++ binary
# (test/reference/, produced by test/make_cpp_references.jl) within the tolerance of the market solver.
#
# These tests document the "exact port" state. They are expected to be dropped once the Julia model
# evolves on its own: set the environment variable FFSMJ_SKIP_EXACT=1 to skip them.
#
# All the output files of each scenario are compared line by line, in the same order:
# - fields are separated by ';' (csv files) or by spaces (ASCII grid maps)
# - non numeric fields must be identical (keys, names, file structure)
# - numbers must agree within |a-b| <= ATOL + RTOL*max(|a|,|b|)
# - regional trade flows (rt), being degenerate, have a looser tolerance
# - in the optimisation logs only the year, the solved flag and the objective are compared
#   (time stamps and Ipopt iteration counts differ by construction)

using Test
isdefined(@__MODULE__, :runScenario) || include("common.jl")

const RTOL = 1e-4
const ATOL = 1e-8
const RT_RTOL = 1e-2
const RT_ATOL = 1e-5

_num(s) = (s == "nan" || s == "-nan") ? NaN : tryparse(Float64, s)

"Compare two output texts; returns (number of differences, description of the first ones)"
function compareOutputText(ref::String, new::String; delim = ';', optLog = false)
    lr = split(chomp(ref), '\n')
    ln = split(chomp(new), '\n')
    msgs = String[]
    nbad = 0
    if length(lr) != length(ln)
        return (1, ["number of lines: reference $(length(lr)), Julia $(length(ln))"])
    end
    for (i, (a, b)) in enumerate(zip(lr, ln))
        fa = delim == ' ' ? split(a) : split(a, delim)
        fb = delim == ' ' ? split(b) : split(b, delim)
        if length(fa) != length(fb)
            nbad += 1
            length(msgs) < 5 && push!(msgs, "line $i: number of fields differ")
            continue
        end
        isrt = length(fa) >= 2 && fa[2] == "rt"
        rtol, atol = isrt ? (RT_RTOL, RT_ATOL) : (RTOL, ATOL)
        for (j, (x, y)) in enumerate(zip(fa, fb))
            optLog && i > 1 && j in (3, 5) && continue    # time stamp and iteration count
            nx, ny = _num(x), _num(y)
            ok = if nx !== nothing && ny !== nothing
                (isnan(nx) && isnan(ny)) || abs(nx - ny) <= atol + rtol * max(abs(nx), abs(ny))
            else
                x == y
            end
            if !ok
                nbad += 1
                length(msgs) < 5 && push!(msgs, "line $i field $j: reference '$x' vs Julia '$y'   [$a]")
            end
        end
    end
    return (nbad, msgs)
end

"Reference files (relative paths, without .gz) of a scenario"
function referenceFiles(sc)
    dir = joinpath(REFERENCE_DIR, sc)
    files = String[]
    for (root, _, fs) in walkdir(dir)
        for f in fs
            (endswith(f, ".gz") && f != "log.txt.gz") || continue
            push!(files, relpath(joinpath(root, f[1:end-3]), dir))
        end
    end
    return sort(files)
end

@testset "Exact reproduction of the C++ outputs" begin
    if get(ENV, "FFSMJ_SKIP_EXACT", "0") == "1"
        @info "Exact tests skipped (FFSMJ_SKIP_EXACT=1)"
    elseif !isdir(REFERENCE_DIR)
        @warn "No C++ reference outputs in $REFERENCE_DIR: exact tests skipped"
    else
        selected = haskey(ENV, "FFSMJ_TESTS") ? split(ENV["FFSMJ_TESTS"], ',') : testScenarios()
        for sc in selected
            (sc in NOT_EXACT || !isdir(joinpath(REFERENCE_DIR, sc))) && continue
            @testset "$sc" begin
                runScenario(sc)
                for f in referenceFiles(sc)
                    jl = joinpath(outputDir(), f)
                    @test isfile(jl)
                    isfile(jl) || continue
                    ref = readText(joinpath(REFERENCE_DIR, sc, f * ".gz"))
                    nbad, msgs = compareOutputText(ref, read(jl, String); optLog = startswith(f, "optimisationLogs"),
                                                   delim = startswith(f, joinpath("maps", "asciiGrids")) ? ' ' : ';')
                    nbad > 0 && @error "$sc: $f differs from the C++ reference ($nbad differences)" msgs
                    @test nbad == 0
                end
            end
        end
    end
end
