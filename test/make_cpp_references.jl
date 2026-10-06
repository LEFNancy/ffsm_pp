# Produce the C++ reference outputs used by test/exact_tests.jl, running the C++ FFSM++ binary on all
# the t_* scenarios of test/data/ffsm_tests.ods (except those not reproducible by construction).
#
#   julia --project=. test/make_cpp_references.jl <path to the C++ ffsm binary> [scenario ...]
#
# The outputs are stored, gzipped, in test/reference/<scenario>/.

include("common.jl")

function main(args)
    isempty(args) && (println("Usage: julia --project=. test/make_cpp_references.jl <ffsm binary> [scenario ...]"); return 1)
    bin = abspath(args[1])
    scens = length(args) > 1 ? args[2:end] : filter(s -> !(s in NOT_EXACT), testScenarios())
    work = mktempdir(; prefix = "ffsm_cppref_", cleanup = false)
    base = joinpath(work, "base")
    prepareTestDataDir(base)
    println("Working folder: $work; running $(length(scens)) scenarios")
    results = asyncmap(scens; ntasks = max(1, Sys.CPU_THREADS ÷ 2)) do sc
        dir = joinpath(work, sc)
        mkpath(dir)
        cp(base, joinpath(dir, "data"))
        log = joinpath(dir, "log.txt")
        t0 = time()
        p = run(pipeline(Cmd(`$bin -c -s $sc -i data/$TEST_INPUT`; dir = dir); stdout = log, stderr = log); wait = false)
        wait(p)
        ok = success(p) && occursin("Model has ended scheduled simulation in a regular way", read(log, String))
        println(rpad(sc, 18), ok ? "ok" : "FAILED", "  ($(round(time() - t0; digits = 1)) s)")
        ok || return (sc, false)
        out = joinpath(dir, "data", "output")
        ref = joinpath(REFERENCE_DIR, sc)
        rm(ref; force = true, recursive = true)
        for sub in ("results", "optimisationLogs", "maps/asciiGrids", "maps/floatListLayers", "maps/integerListLayers")
            src = joinpath(out, sub)
            isdir(src) || continue
            for f in readdir(src)
                (f == "00_doNotRemove.txt" || isdir(joinpath(src, f))) && continue
                mkpath(joinpath(ref, sub))
                cp(joinpath(src, f), joinpath(ref, sub, f))
                run(`gzip -9 -n $(joinpath(ref, sub, f))`)
            end
        end
        cp(log, joinpath(ref, "log.txt"))
        run(`gzip -9 -n $(joinpath(ref, "log.txt"))`)
        return (sc, true)
    end
    failed = [sc for (sc, ok) in results if !ok]
    isempty(failed) ? rm(work; recursive = true) : println("FAILED: ", join(failed, ", "), "  (logs in $work)")
    return isempty(failed) ? 0 : 1
end

exit(main(ARGS))
