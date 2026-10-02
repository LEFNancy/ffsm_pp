# Compare the csv outputs of two FFSM runs (e.g. the C++ and the Julia versions of the model).
#
# Usage: julia compare_outputs.jl <outputDirA> <outputDirB> <scenarioName> [relTol]
#
# For each results file (forestData, productData, carbonBalance and, if present, debugPxValues) rows are
# matched on all the non-value fields and the relative differences of the values are reported.

using Printf

const DELIM = ';'

"Read a long-format FFSM csv in a Dict key => value, where key are all the fields except the value"
function readLong(fn)
    d = Dict{String,String}()
    header = String[]
    for (i, line) in enumerate(eachline(fn))
        f = split(line, DELIM)
        isempty(f[end]) && pop!(f)
        if i == 1
            header = String.(f)
            continue
        end
        d[join(f[1:end-1], DELIM)] = String(f[end])
    end
    return d, header
end

"Read the debugPxValues file: key = scenario;year;region;pxId;pxX;pxY;var;column => value"
function readPx(fn)
    d = Dict{String,String}()
    header = String[]
    for (i, line) in enumerate(eachline(fn))
        f = split(line, DELIM)
        if i == 1
            header = String.(f)
            continue
        end
        k0 = join(f[1:7], DELIM)
        for j in 8:min(length(f), length(header))
            isempty(f[j]) && continue
            d[k0 * DELIM * header[j]] = String(f[j])
        end
    end
    return d, header
end

_num(s) = (v = tryparse(Float64, s); v === nothing ? (s == "nan" || s == "-nan" ? NaN : s == "inf" ? Inf : s == "-inf" ? -Inf : NaN) : v)

reldiff(a, b) = (a == b || (isnan(a) && isnan(b))) ? 0.0 : abs(a - b) / max(abs(a), abs(b), 1e-12)

"Group key used in the report: the variable name and the year"
function groupOf(k, kind)
    f = split(k, DELIM)
    if kind == :forest       # scen;parName;country;region;forType;freeDim;year
        return (f[2], parse(Int, f[7]))
    elseif kind == :product  # scen;parName;country;region;prod;freeDim;year
        return (f[2], parse(Int, f[7]))
    elseif kind == :carbon   # scen;country;region;balItem;year
        return (f[4], parse(Int, f[5]))
    elseif kind == :hv       # scen;parName;country;region;forType;freeDim;prod;year
        return (f[2] * ":" * f[7], parse(Int, f[8]))
    else                     # scen;year;region;pxId;pxX;pxY;var;col
        return (f[7], parse(Int, f[2]))
    end
end

function compareFiles(fa, fb, kind; relTol = 1e-6, io = stdout)
    reader = kind == :px ? readPx : readLong
    a, _ = reader(fa)
    b, _ = reader(fb)
    onlyA = setdiff(keys(a), keys(b))
    onlyB = setdiff(keys(b), keys(a))
    stats = Dict{Tuple{String,Int},Tuple{Float64,Int,Int,String}}()  # group => (maxRel, nOver, n, worstKey)
    for (k, va) in a
        haskey(b, k) || continue
        x, y = _num(va), _num(b[k])
        r = reldiff(x, y)
        g = groupOf(k, kind)
        mx, nOver, n, wk = get(stats, g, (0.0, 0, 0, ""))
        if r > mx
            mx, wk = r, k
        end
        stats[g] = (mx, nOver + (r > relTol), n + 1, wk)
    end
    println(io, "== ", basename(fa), "   (", length(a), " vs ", length(b), " values; only in A: ", length(onlyA), ", only in B: ", length(onlyB), ")")
    for k in first(sort(collect(onlyA)), 5)
        println(io, "   only in A: ", k)
    end
    for k in first(sort(collect(onlyB)), 5)
        println(io, "   only in B: ", k)
    end
    vars = sort(unique(first.(collect(keys(stats)))))
    for v in vars
        gs = sort([g for g in keys(stats) if g[1] == v]; by = last)
        worst = maximum(stats[g][1] for g in gs)
        nOver = sum(stats[g][2] for g in gs)
        n = sum(stats[g][3] for g in gs)
        firstBad = findfirst(g -> stats[g][2] > 0, gs)
        fb_s = firstBad === nothing ? "-" : string(gs[firstBad][2])
        wg = gs[argmax([stats[g][1] for g in gs])]
        @printf(io, "   %-32s n=%8d  maxRelDiff=%10.3e  n(>tol)=%7d  firstYear(>tol)=%s  worst: %s\n", v, n, worst, nOver, fb_s, stats[wg][4])
    end
    return stats
end

"""
Streaming comparison of two debugPxValues files (too big to be loaded in memory): the files are walked in
lockstep, year blocks present only in one file are skipped, and within a year the rows must be in the same order.
"""
function comparePxStreaming(fa, fb; relTol = 1e-6, io = stdout)
    ia, ib = open(fa), open(fb)
    ha = split(readline(ia), DELIM); readline(ib)
    yearOf(l) = parse(Int, split(l, DELIM; limit = 3)[2])
    stats = Dict{String,Tuple{Float64,Int,Int,String,Int}}()  # var => (maxRel, nOver, n, worst, firstYearOver)
    skippedA = Set{Int}(); skippedB = Set{Int}(); mismatches = 0
    la = eof(ia) ? nothing : readline(ia)
    lb = eof(ib) ? nothing : readline(ib)
    while la !== nothing && lb !== nothing
        ya, yb = yearOf(la), yearOf(lb)
        if ya < yb
            push!(skippedA, ya); la = eof(ia) ? nothing : readline(ia); continue
        elseif yb < ya
            push!(skippedB, yb); lb = eof(ib) ? nothing : readline(ib); continue
        end
        fa_ = split(la, DELIM); fb_ = split(lb, DELIM)
        if fa_[1:7] != fb_[1:7]
            mismatches += 1
            mismatches <= 5 && println(io, "   row mismatch: ", join(fa_[1:7], DELIM), "  vs  ", join(fb_[1:7], DELIM))
        else
            var = String(fa_[7])
            mx, nOver, n, wk, fy = get(stats, var, (0.0, 0, 0, "", 0))
            for j in 8:min(length(fa_), length(fb_), length(ha))
                (isempty(fa_[j]) && isempty(fb_[j])) && continue
                r = reldiff(_num(fa_[j]), _num(fb_[j]))
                n += 1
                if r > relTol
                    nOver += 1
                    fy == 0 && (fy = ya)
                end
                if r > mx
                    mx = r; wk = join(fa_[1:7], DELIM) * DELIM * ha[j] * " (" * fa_[j] * " vs " * fb_[j] * ")"
                end
            end
            stats[var] = (mx, nOver, n, wk, fy)
        end
        la = eof(ia) ? nothing : readline(ia)
        lb = eof(ib) ? nothing : readline(ib)
    end
    close(ia); close(ib)
    println(io, "== ", basename(fa), "  (years only in A: ", sort(collect(skippedA)), ", only in B: ", sort(collect(skippedB)), ", row mismatches: ", mismatches, ")")
    for v in sort(collect(keys(stats)))
        mx, nOver, n, wk, fy = stats[v]
        @printf(io, "   %-32s n=%9d  maxRelDiff=%10.3e  n(>tol)=%8d  firstYear(>tol)=%s  worst: %s\n", v, n, mx, nOver, fy == 0 ? "-" : string(fy), wk)
    end
end

function main(args)
    length(args) < 3 && (println("Usage: julia compare_outputs.jl <outputDirA> <outputDirB> <scenarioName> [relTol]"); return)
    dA, dB, sc = args[1], args[2], args[3]
    relTol = length(args) >= 4 ? parse(Float64, args[4]) : 1e-6
    for (name, kind) in (("results/forestData_$sc.csv", :forest), ("results/productData_$sc.csv", :product),
                         ("results/carbonBalance_$sc.csv", :carbon), ("results/detailedHV_$sc.csv", :hv),
                         ("debugs/debugPxValues_$sc.csv", :px))
        fa, fb = joinpath(dA, name), joinpath(dB, name)
        (isfile(fa) && isfile(fb)) || continue
        kind == :px ? comparePxStreaming(fa, fb; relTol = relTol) : compareFiles(fa, fb, kind; relTol = relTol)
    end
end

if abspath(PROGRAM_FILE) == @__FILE__
    main(ARGS)
end
