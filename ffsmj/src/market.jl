# Port of the Opt class: the yearly partial equilibrium market problem, here written with JuMP and
# solved with Ipopt. Derivatives (computed with ADOL-C in the C++ code) are given by JuMP.
#
# The variables are created in the same order as in the C++ concatenated array (variables in
# alphabetical order, then by l1 region, l2 region, product and destination region) and they start
# from the previous year values, as in Opt::get_starting_point().

# variable name => domain (the C++ `vars` std::map is ordered alphabetically)
const MARKET_VARS = [("da", DOM_SEC_PR), ("dc", DOM_SEC_PR), ("dl", DOM_ALL_PR), ("pc", DOM_ALL_PR), ("pl", DOM_ALL_PR),
                     ("rt", DOM_R2_ALL_PR), ("sa", DOM_PRI_PR), ("sc", DOM_PRI_PR), ("sl", DOM_ALL_PR)]

mutable struct MarketLayout
    priPr::Vector{String}
    secPr::Vector{String}
    allPr::Vector{String}
    othPr::Vector{String}
    l2r::Vector{Vector{Int}}
    priPrCombs::Vector{Vector{Int}}
    initPos::Dict{String,Int}   # 0-based initial position of each variable
    nVar::Int
    worldCodeLev2::Int
end

function _domainElements(L::MarketLayout, domain)
    nL2r = sum(length, L.l2r; init = 0)
    domain == DOM_PRI_PR && return nL2r * length(L.priPr)
    domain == DOM_SEC_PR && return nL2r * length(L.secPr)
    domain == DOM_ALL_PR && return nL2r * length(L.allPr)
    domain == DOM_R2_ALL_PR && return sum(length(r)^2 for r in L.l2r; init = 0) * length(L.allPr)
    domain == DOM_PRI_PR_ALLCOMBS && return nL2r * length(L.priPrCombs)
    msgOut(MSG_CRITICAL_ERROR, "Asking for an unknown domain type ($domain)")
end

function MarketLayout(m)
    MD = m.MD
    priPr = getStringVectorSetting(m, "priProducts")
    secPr = getStringVectorSetting(m, "secProducts")
    othPr = getStringVectorSetting(m, "othExogenousProducts")
    L = MarketLayout(priPr, secPr, vcat(priPr, secPr), othPr, getL2r(MD, true), createCombinationsVector(length(priPr)),
        Dict{String,Int}(), 0, getIntSetting(m, "worldCodeLev2"))
    pos = 0
    for (v, dom) in MARKET_VARS
        L.initPos[v] = pos
        pos += _domainElements(L, dom)
    end
    L.nVar = pos
    return L
end

"1-based index in the concatenated variable vector (Opt::gix_uncached), r1,r2,p,r2To 1-based"
function gix(L::MarketLayout, var::AbstractString, r1::Int, r2::Int, p::Int, r2To::Int = 1)
    dom = MARKET_VARS[findfirst(x -> x[1] == var, MARKET_VARS)][2]
    oth = 0
    oth_r2 = 0
    for i in 1:(r1-1)
        oth += length(L.l2r[i])
        oth_r2 += length(L.l2r[i])^2
    end
    ip = L.initPos[var]
    (r1, r2, p, r2To) = (r1 - 1, r2 - 1, p - 1, r2To - 1)
    if dom == DOM_PRI_PR
        return ip + (oth + r2) * length(L.priPr) + p + 1
    elseif dom == DOM_SEC_PR
        return ip + (oth + r2) * length(L.secPr) + p + 1
    elseif dom == DOM_ALL_PR
        return ip + (oth + r2) * length(L.allPr) + p + 1
    elseif dom == DOM_R2_ALL_PR
        return ip + oth_r2 * length(L.allPr) + (r2 * length(L.allPr) + p) * length(L.l2r[r1+1]) + r2To + 1
    end
    msgOut(MSG_CRITICAL_ERROR, "Try to calculate the position of a variable (or constrain) of unknow type.")
end

_varProducts(L::MarketLayout, dom) = dom == DOM_PRI_PR ? L.priPr : dom == DOM_SEC_PR ? L.secPr : L.allPr

"""
    solveMarket!(m)

Build and solve the market equilibrium of the current year. The optimal values are stored in the
product data (as Opt::finalize_solution()). Returns (solved, iterations, objective).
"""
function solveMarket!(m)
    MD = m.MD
    if m.market === nothing
        m.market = MarketLayout(m)
        L = m.market
        nCons = 0
        nEq = 0; nLe = 0; nGe = 0
        for (dom, dir) in ((DOM_SEC_PR, 1), (DOM_SEC_PR, 1), (DOM_SEC_PR, 1), (DOM_SEC_PR, 1), (DOM_SEC_PR, 1), (DOM_PRI_PR, 1),
                           (DOM_PRI_PR, 1), (DOM_PRI_PR, 1), (DOM_PRI_PR, 1), (DOM_PRI_PR, 1), (DOM_PRI_PR, 1), (DOM_SEC_PR, 2),
                           (DOM_R2_ALL_PR, 2), (DOM_SEC_PR, 3), (DOM_PRI_PR_ALLCOMBS, 2))
            n = _domainElements(L, dom)
            nCons += n
            dir == 1 ? (nEq += n) : dir == 2 ? (nLe += n) : (nGe += n)
        end
        msgOut(MSG_INFO, "The model will work with $(L.nVar) variables and $nCons constrains ($nEq equalities, $nLe lower than 0 and $nGe greater than 0)")
    end
    L = m.market
    thisYear = m.SCD.year
    previousYear = thisYear - 1
    gpd(t, r, p, y = DATA_NOW, fd = "") = getProdData(m, t, r, p, y, fd)
    priPr, secPr, allPr, othPr, l2r = L.priPr, L.secPr, L.allPr, L.othPr, L.l2r
    nPri = length(priPr)
    WL2 = L.worldCodeLev2
    overharvestingAllowance = getDoubleSetting(m, "overharvestingAllowance", DATA_NOW)
    intRate = getDoubleSetting(m, "ir")

    model = Model(Ipopt.Optimizer)
    set_attribute(model, "linear_solver", getStringSetting(m, "linearSolver", DATA_NOW))
    set_attribute(model, "max_cpu_time", 1800.0)
    set_attribute(model, "check_derivatives_for_naninf", "yes")
    set_attribute(model, "tol", 1e-06)
    set_attribute(model, "acceptable_tol", 1e-05)
    m.hsllib === nothing || set_attribute(model, "hsllib", m.hsllib)
    for (k, v) in m.ipoptOptions
        set_attribute(model, k, v)
    end

    @variable(model, x[1:L.nVar] >= 0)

    # starting point: previous year values
    for (v, dom) in MARKET_VARS
        prods = _varProducts(L, dom)
        for r1 in eachindex(l2r), r2 in eachindex(l2r[r1]), p in eachindex(prods)
            if dom == DOM_R2_ALL_PR
                for r2To in eachindex(l2r[r1])
                    set_start_value(x[gix(L, v, r1, r2, p, r2To)], gpd(v, l2r[r1][r2], prods[p], previousYear, i2s(l2r[r1][r2To])))
                end
            else
                set_start_value(x[gix(L, v, r1, r2, p)], gpd(v, l2r[r1][r2], prods[p], previousYear))
            end
        end
    end
    X(v, r1, r2, p, r2To = 1) = x[gix(L, v, r1, r2, p, r2To)]

    # *** objective ***
    obj = Any[]
    for r1 in eachindex(l2r), r2 in eachindex(l2r[r1])
        reg = l2r[r1][r2]
        # consumer's surplus..
        for p in eachindex(secPr)
            aa = gpd("aa", reg, secPr[p])
            sigma = gpd("sigma", reg, secPr[p])
            gpd("dc", reg, secPr[p], m.core.secondYear)
            push!(obj, aa * max(0.0, X("dc", r1, r2, p))^((sigma + 1) / sigma) - X("pc", r1, r2, p + nPri) * X("dc", r1, r2, p))
        end
        # producers surplus..
        for p in eachindex(priPr)
            bb = gpd("bb", reg, priPr[p])
            sigma = gpd("sigma", reg, priPr[p])
            gpd("pol_mktDirInt_s", reg, priPr[p])
            carbonPrice = gpd("carbonPrice", reg, "", DATA_NOW)
            Pct = carbonPrice * intRate
            omega = gpd("co2content_products", reg, priPr[p]) / 1000
            push!(obj, X("pc", r1, r2, p) * X("sc", r1, r2, p) - (bb * max(0.0, X("sc", r1, r2, p))^((sigma + 1) / sigma) + X("sc", r1, r2, p) * omega * Pct))
        end
        # transformations between primary products..
        for p1 in eachindex(priPr), p2 in eachindex(priPr)
            a_pr = gpd("a_pr", reg, priPr[p1], DATA_NOW, priPr[p2])
            bb = gpd("bb", reg, priPr[p2])
            sigma = gpd("sigma", reg, priPr[p2])
            push!(obj, X("pc", r1, r2, p2) * a_pr * X("sc", r1, r2, p1) - bb * max(0.0, a_pr * X("sc", r1, r2, p1))^((sigma + 1) / sigma))
        end
        # surplus of transport agents..
        for p in eachindex(allPr), r2To in eachindex(l2r[r1])
            ct = gpd("ct", reg, allPr[p], DATA_NOW, i2s(l2r[r1][r2To])) * gpd("pol_tcSub", reg, allPr[p])
            push!(obj, (X("pl", r1, r2To, p) - X("pl", r1, r2, p) - ct) * X("rt", r1, r2, p, r2To))
        end
        # transformers surplus..
        for p in eachindex(secPr)
            mm = gpd("m", reg, secPr[p]) * gpd("pol_trSub", reg, secPr[p])
            push!(obj, (X("pl", r1, r2, p + nPri) - mm) * X("sl", r1, r2, p + nPri))
        end
        for p in eachindex(priPr)
            push!(obj, -X("pl", r1, r2, p) * X("dl", r1, r2, p))
        end
    end
    @objective(model, Max, sum(obj))

    # *** constraints (same order as in Opt::declareConstrains()) ***
    loopRegs(f) = (for r1 in eachindex(l2r), r2 in eachindex(l2r[r1]); f(r1, r2, l2r[r1][r2]); end)

    # mkeq2 [h1] Conservation of matters of transformed products
    loopRegs() do r1, r2, reg
        for p in eachindex(secPr)
            psec = p + nPri
            @constraint(model, X("dl", r1, r2, psec) - X("sl", r1, r2, psec) +
                sum(X("rt", r1, r2, psec, t) - X("rt", r1, t, psec, r2) for t in eachindex(l2r[r1])) == 0)
        end
    end
    # mkeq6 [eq 20] Transformed products demand function (with substitution elasticities)
    loopRegs() do r1, r2, reg
        for p in eachindex(secPr)
            psec = p + nPri
            gg = gpd("gg", reg, secPr[p])
            sigma = gpd("sigma", reg, secPr[p])
            pc_1 = gpd("pc", reg, secPr[p], previousYear)
            pol_d = gpd("pol_mktDirInt_d", reg, secPr[p])
            pol_d_1 = gpd("pol_mktDirInt_d", reg, secPr[p], previousYear)
            additionalDemand = gpd("additionalDemand", reg, secPr[p])
            g = -gg * (X("pc", r1, r2, psec) * pol_d)^sigma
            for p2 in eachindex(secPr)
                es_d = gpd("es_d", reg, secPr[p], DATA_NOW, secPr[p2])
                if es_d != 0
                    pc_1_s = gpd("pc", reg, secPr[p2], previousYear)
                    pol_d_s = gpd("pol_mktDirInt_d", reg, secPr[p2])
                    pol_d_1_s = gpd("pol_mktDirInt_d", reg, secPr[p2], previousYear)
                    g = g * (((X("pc", r1, r2, psec) * pol_d) / (X("pc", r1, r2, nPri + p2) * pol_d_s)) /
                             ((pc_1 * pol_d_1) / (pc_1_s * pol_d_1_s)))^es_d
                end
            end
            for p3 in eachindex(othPr)
                es_d = gpd("es_d", reg, secPr[p], DATA_NOW, othPr[p3])
                if es_d != 0
                    pc_s = gpd("pl", reg, othPr[p3], DATA_NOW)
                    pc_1_s = gpd("pl", reg, othPr[p3], previousYear)
                    g = g * ((X("pc", r1, r2, psec) * pol_d / pc_s) / (pc_1 * pol_d_1 / pc_1_s))^es_d
                end
            end
            @constraint(model, g + X("dc", r1, r2, p) - additionalDemand == 0)
        end
    end
    # mkeq7 [h7 and h3] Transformed products import function
    loopRegs() do r1, r2, reg
        for p in eachindex(secPr)
            psec = p + nPri
            q1 = gpd("q1_polCorr", reg, secPr[p])
            p1v = 1 - q1
            psi = gpd("psi", reg, secPr[p])
            pworld = gpd("pl", WL2, secPr[p])
            @constraint(model, X("da", r1, r2, p) / X("dl", r1, r2, psec) - ((q1 * X("pl", r1, r2, psec)) / (p1v * pworld))^psi == 0)
        end
    end
    # mkeq13 [h9] Composite price of transformed products
    loopRegs() do r1, r2, reg
        for p in eachindex(secPr)
            psec = p + nPri
            pworld = gpd("pl", WL2, secPr[p])
            @constraint(model, X("pc", r1, r2, psec) * X("dc", r1, r2, p) - X("dl", r1, r2, psec) * X("pl", r1, r2, psec) - X("da", r1, r2, p) * pworld == 0)
        end
    end
    # mkeq23 [h3] Composite demand
    loopRegs() do r1, r2, reg
        for p in eachindex(secPr)
            psec = p + nPri
            q1 = gpd("q1_polCorr", reg, secPr[p])
            psi = gpd("psi", reg, secPr[p])
            p1v = 1 - q1
            @constraint(model, X("dc", r1, r2, p) - (q1 * X("da", r1, r2, p)^((psi - 1) / psi) + p1v * X("dl", r1, r2, psec)^((psi - 1) / psi))^(psi / (psi - 1)) == 0)
        end
    end
    # mkeq3 [h2] Conservation of matters of raw products
    loopRegs() do r1, r2, reg
        for p in eachindex(priPr)
            a_prs = [gpd("a_pr", reg, priPr[p2], DATA_NOW, priPr[p]) for p2 in eachindex(priPr)]
            @constraint(model, X("dl", r1, r2, p) - X("sl", r1, r2, p) +
                sum(X("rt", r1, r2, p, t) - X("rt", r1, t, p, r2) for t in eachindex(l2r[r1])) -
                sum(a_prs[p2] * X("sl", r1, r2, p2) for p2 in eachindex(priPr)) == 0)
        end
    end
    # mkeq4 [eq 13] Leontief transformation function
    loopRegs() do r1, r2, reg
        for p in eachindex(priPr)
            as = [gpd("a", reg, priPr[p], DATA_NOW, secPr[p2]) for p2 in eachindex(secPr)]
            @constraint(model, X("dl", r1, r2, p) - sum(as[p2] * X("sl", r1, r2, p2 + nPri) for p2 in eachindex(secPr)) == 0)
        end
    end
    # mkeq5 [eq 21] Raw product supply function
    loopRegs() do r1, r2, reg
        for p in eachindex(priPr)
            ff = gpd("ff", reg, priPr[p])
            gpd("pol_mktDirInt_s", reg, priPr[p])
            sigma = gpd("sigma", reg, priPr[p])
            carbonPrice = gpd("carbonPrice", reg, "", DATA_NOW)
            Pct = carbonPrice * intRate
            omega = gpd("co2content_products", reg, priPr[p]) / 1000
            @constraint(model, X("sc", r1, r2, p) - ff * (X("pc", r1, r2, p) - omega * Pct)^sigma == 0)
        end
    end
    # mkeq8 [h8 and h4] Raw products export function
    loopRegs() do r1, r2, reg
        for p in eachindex(priPr)
            t1 = gpd("t1_polCorr", reg, priPr[p])
            r1v = 1 - t1
            eta = gpd("eta", reg, priPr[p])
            pworld = gpd("pl", WL2, priPr[p])
            @constraint(model, X("sa", r1, r2, p) / X("sl", r1, r2, p) - ((t1 * X("pl", r1, r2, p)) / (r1v * pworld))^eta == 0)
        end
    end
    # mkeq14 [h10] Composite price of raw products
    loopRegs() do r1, r2, reg
        for p in eachindex(priPr)
            pworld = gpd("pl", WL2, priPr[p])
            @constraint(model, X("pc", r1, r2, p) * X("sc", r1, r2, p) - X("sl", r1, r2, p) * X("pl", r1, r2, p) - X("sa", r1, r2, p) * pworld == 0)
        end
    end
    # mkeq24 [h4] Composite supply
    loopRegs() do r1, r2, reg
        for p in eachindex(priPr)
            t1 = gpd("t1_polCorr", reg, priPr[p])
            r1v = 1 - t1
            eta = gpd("eta", reg, priPr[p])
            @constraint(model, X("sc", r1, r2, p) - (t1 * X("sa", r1, r2, p)^((eta - 1) / eta) + r1v * X("sl", r1, r2, p)^((eta - 1) / eta))^(eta / (eta - 1)) == 0)
        end
    end
    # mkeq17 [h16] Transformation supply lower than the regional production capacity
    loopRegs() do r1, r2, reg
        for p in eachindex(secPr)
            k = gpd("k", reg, secPr[p])
            @constraint(model, X("sl", r1, r2, p + nPri) - k <= 0)
        end
    end
    # mkeq26 Null transport agents supply
    loopRegs() do r1, r2, reg
        for p in eachindex(allPr), t in eachindex(l2r[r1])
            ct = gpd("ct", reg, allPr[p], DATA_NOW, i2s(l2r[r1][t])) * gpd("pol_tcSub", reg, allPr[p])
            @constraint(model, X("pl", r1, t, p) - X("pl", r1, r2, p) - ct <= 0)
        end
    end
    # mkeq25 Null transformers supply
    loopRegs() do r1, r2, reg
        for p in eachindex(secPr)
            mv = gpd("m", reg, secPr[p]) * gpd("pol_trSub", reg, secPr[p])
            as = [gpd("a", reg, priPr[p2], DATA_NOW, secPr[p]) for p2 in eachindex(priPr)]
            @constraint(model, mv - X("pl", r1, r2, p + nPri) + sum(as[p2] * X("pl", r1, r2, p2) for p2 in eachindex(priPr)) >= 0)
        end
    end
    # resbounds: total supply lower than the available stock, for each combination of primary products
    loopRegs() do r1, r2, reg
        REG = getRegion(MD, reg)
        for (c, comb) in enumerate(L.priPrCombs)
            inv = REG.inResByAnyCombination[c]
            @constraint(model, -inv + sum((X("sa", r1, r2, i) + X("sl", r1, r2, i) for i in comb); init = 0.0) - overharvestingAllowance <= 0)
        end
    end

    # as Opt::intermediate_callback()
    MOI.set(model, Ipopt.CallbackFunction(), function (alg_mod, iter, args...)
        iter % 10 == 0 && (msgOut(MSG_DEBUG, "Running ($iter iter) .."); flush(stdout))
        return true
    end)

    msgOut(MSG_INFO, "Running optimisation problem for this year (it may take a few minutes for large models)..")
    flush(stdout)   # keep the order with the (C level) Ipopt output
    optimize!(model)
    Libc.flush_cstdio()

    status = termination_status(model)
    xs = value.(x)
    objv = objective_value(model)
    println("\n\nObjective value")
    @printf("f(x*) = %e\n", objv)
    # --> finalize_solution(): optimal values to the product data
    for (v, dom) in MARKET_VARS
        prods = _varProducts(L, dom)
        for r1 in eachindex(l2r), r2 in eachindex(l2r[r1]), p in eachindex(prods)
            if dom == DOM_R2_ALL_PR
                for r2To in eachindex(l2r[r1])
                    setProdData!(m, xs[gix(L, v, r1, r2, p, r2To)], v, l2r[r1][r2], prods[p], DATA_NOW, false, i2s(l2r[r1][r2To]))
                end
            else
                setProdData!(m, xs[gix(L, v, r1, r2, p)], v, l2r[r1][r2], prods[p])
            end
        end
    end
    solved = status == MOI.LOCALLY_SOLVED
    iters = solved ? MOI.get(model, MOI.BarrierIterations()) : 0
    return solved, iters, objv
end
