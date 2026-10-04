# Port of the Opt class: the yearly partial equilibrium market problem, as a JuMP model solved with Ipopt.
#
# The model is written over sets of regions and products: every market equation is a family of constraints
# (e.g. `mkeq2[r in R, s in SEC]`, with the names of the C++ code). It is built every year with the data of that
# year (`marketData()`) and starts from the values of the previous year, as Opt::get_starting_point(). The
# derivatives (computed with ADOL-C in the C++ code) are given by JuMP.
#
# Sets
#   R          level-2 regions of the market (the non residual regions of the modelled level-1 regions)
#   dest[r]    the regions with which r can trade (those of the same level-1 region, r included)
#   PRI, SEC   primary and secondary (transformed) products; ALL = PRI ∪ SEC; OTH exogenous substitutes
#   COMBS      all the combinations (subsets) of primary products, for the resource bounds
#
# Variables (all >= 0)
#   pl  local price              [R, ALL]      pc  composite price            [R, ALL]
#   sl  local supply             [R, ALL]      dl  local demand               [R, ALL]
#   sc  composite supply         [R, PRI]      sa  supply to abroad (exports) [R, PRI]
#   dc  composite demand         [R, SEC]      da  demand from abroad (imp.)  [R, SEC]
#   rt  regional trade           [r ∈ R, p ∈ ALL, rTo ∈ dest[r]]
#
# Variables and constraints are created in the order of the C++ code (variables by name, region, product and
# destination; constraints by equation, region and product), so that Ipopt solves the same problem.

"Regions and products of the market (fixed for the whole simulation)"
struct MarketSets
    R::Vector{Int}
    dest::Dict{Int,Vector{Int}}
    PRI::Vector{String}
    SEC::Vector{String}
    ALL::Vector{String}
    OTH::Vector{String}
    COMBS::Vector{Vector{String}}
    WORLD::Int                     # region code of the world prices
end

function MarketSets(m)
    PRI = getStringVectorSetting(m, "priProducts")
    SEC = getStringVectorSetting(m, "secProducts")
    l2r = getL2r(m.MD, true)
    R = reduce(vcat, l2r; init = Int[])
    dest = Dict(r => regs for regs in l2r for r in regs)
    COMBS = [PRI[c] for c in createCombinationsVector(length(PRI))]
    return MarketSets(R, dest, PRI, SEC, vcat(PRI, SEC), getStringVectorSetting(m, "othExogenousProducts"), COMBS,
        getIntSetting(m, "worldCodeLev2"))
end

"The data of the market problem of the current year, as containers indexed by region and products"
function marketData(m, S::MarketSets)
    (; R, dest, PRI, SEC, ALL, OTH, COMBS, WORLD) = S
    gpd(t, r, p, y = DATA_NOW, fd = "") = getProdData(m, t, r, p, y, fd)
    y_1 = m.SCD.year - 1
    ir = getDoubleSetting(m, "ir")
    return (
        pworld = Containers.@container([p in ALL], gpd("pl", WORLD, p)),                       # world prices
        sigma = Containers.@container([r in R, p in ALL], gpd("sigma", r, p)),                 # price elasticities
        # primary products: supply and export functions, carbon cost of the harvest, transformations
        ff = Containers.@container([r in R, p in PRI], gpd("ff", r, p)),
        bb = Containers.@container([r in R, p in PRI], gpd("bb", r, p)),
        Pct = Containers.@container([r in R], gpd("carbonPrice", r, "", DATA_NOW) * ir),
        omega = Containers.@container([r in R, p in PRI], gpd("co2content_products", r, p) / 1000),
        t1 = Containers.@container([r in R, p in PRI], gpd("t1_polCorr", r, p)),
        eta = Containers.@container([r in R, p in PRI], gpd("eta", r, p)),
        a_pr = Containers.@container([r in R, p in PRI, p2 in PRI], gpd("a_pr", r, p, DATA_NOW, p2)),   # p -> p2
        a = Containers.@container([r in R, p in PRI, s in SEC], gpd("a", r, p, DATA_NOW, s)),          # p by unit of s
        inRes = Containers.@container([r in R, c in eachindex(COMBS)], getRegion(m.MD, r).inResByAnyCombination[c]),
        overharvestingAllowance = getDoubleSetting(m, "overharvestingAllowance", DATA_NOW),
        # secondary products: demand and import functions, transformation, policies
        aa = Containers.@container([r in R, s in SEC], gpd("aa", r, s)),
        gg = Containers.@container([r in R, s in SEC], gpd("gg", r, s)),
        q1 = Containers.@container([r in R, s in SEC], gpd("q1_polCorr", r, s)),
        psi = Containers.@container([r in R, s in SEC], gpd("psi", r, s)),
        k = Containers.@container([r in R, s in SEC], gpd("k", r, s)),                                  # capacity
        mm = Containers.@container([r in R, s in SEC], gpd("m", r, s) * gpd("pol_trSub", r, s)),       # margin
        additionalDemand = Containers.@container([r in R, s in SEC], gpd("additionalDemand", r, s)),
        pc_1 = Containers.@container([r in R, s in SEC], gpd("pc", r, s, y_1)),                        # previous prices
        pol_d = Containers.@container([r in R, s in SEC], gpd("pol_mktDirInt_d", r, s)),                # demand policy
        pol_d_1 = Containers.@container([r in R, s in SEC], gpd("pol_mktDirInt_d", r, s, y_1)),
        es_d = Dict((r, s, s2) => es for r in R, s in SEC, s2 in vcat(SEC, OTH)
                    for es in (gpd("es_d", r, s, DATA_NOW, s2),) if es != 0),                  # substitution
        pl_oth = Containers.@container([r in R, o in OTH], gpd("pl", r, o, DATA_NOW)),
        pl_oth_1 = Containers.@container([r in R, o in OTH], gpd("pl", r, o, y_1)),
        # trade: transport costs, with the transport subsidy
        ct = Containers.@container([r in R, p in ALL, rTo in dest[r]], gpd("ct", r, p, DATA_NOW, i2s(rTo)) * gpd("pol_tcSub", r, p)),
    )
end

"The market problem of the current year as a JuMP model"
function buildMarketModel(m, S::MarketSets, d)
    (; R, dest, PRI, SEC, ALL, OTH, COMBS) = S
    y_1 = m.SCD.year - 1
    prev(v, r, p) = getProdData(m, v, r, p, y_1)          # values of the previous year (starting point)
    σ = d.sigma

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

    @variables(model, begin
        da[r in R, s in SEC] >= 0, (start = prev("da", r, s))        # demand from abroad (imports)
        dc[r in R, s in SEC] >= 0, (start = prev("dc", r, s))        # composite demand
        dl[r in R, p in ALL] >= 0, (start = prev("dl", r, p))        # local demand
        pc[r in R, p in ALL] >= 0, (start = prev("pc", r, p))        # composite price
        pl[r in R, p in ALL] >= 0, (start = prev("pl", r, p))        # local price
        rt[r in R, p in ALL, rTo in dest[r]] >= 0, (start = getProdData(m, "rt", r, p, y_1, i2s(rTo)))  # regional trade
        sa[r in R, p in PRI] >= 0, (start = prev("sa", r, p))        # supply to abroad (exports)
        sc[r in R, p in PRI] >= 0, (start = prev("sc", r, p))        # composite supply
        sl[r in R, p in ALL] >= 0, (start = prev("sl", r, p))        # local supply
    end)

    # *** objective: total welfare ***
    @objective(model, Max,
        # consumers' surplus (secondary products)
        sum(d.aa[r, s] * max(0.0, dc[r, s])^((σ[r, s] + 1) / σ[r, s]) - pc[r, s] * dc[r, s] for r in R, s in SEC)
        # producers' surplus (primary products), net of the carbon cost of the harvest
        + sum(pc[r, p] * sc[r, p] - (d.bb[r, p] * max(0.0, sc[r, p])^((σ[r, p] + 1) / σ[r, p]) + sc[r, p] * d.omega[r, p] * d.Pct[r])
              for r in R, p in PRI)
        # transformations between primary products
        + sum(pc[r, p2] * d.a_pr[r, p, p2] * sc[r, p] - d.bb[r, p2] * max(0.0, d.a_pr[r, p, p2] * sc[r, p])^((σ[r, p2] + 1) / σ[r, p2])
              for r in R, p in PRI, p2 in PRI)
        # transport agents' surplus
        + sum((pl[rTo, p] - pl[r, p] - d.ct[r, p, rTo]) * rt[r, p, rTo] for r in R, p in ALL, rTo in dest[r])
        # transformers' surplus: sales of the transformed products net of the margin, minus the primary inputs
        + sum((pl[r, s] - d.mm[r, s]) * sl[r, s] for r in R, s in SEC)
        - sum(pl[r, p] * dl[r, p] for r in R, p in PRI)
    )

    "trade balance of region r for product p: what it sends minus what it receives"
    netExports(r, p) = sum(rt[r, p, rTo] - rt[rTo, p, r] for rTo in dest[r])

    "demand function of the secondary product s, with the substitution elasticities (C++: mkeq6)"
    function demand(r, s)
        pcp = pc[r, s] * d.pol_d[r, s]                       # consumer price, with the demand policy
        pcp_1 = d.pc_1[r, s] * d.pol_d_1[r, s]               # ... in the previous year
        g = -d.gg[r, s] * pcp^σ[r, s]
        for s2 in SEC                                        # substitutes among the products
            es = get(d.es_d, (r, s, s2), 0.0)
            es != 0 && (g *= ((pcp / (pc[r, s2] * d.pol_d[r, s2])) / (pcp_1 / (d.pc_1[r, s2] * d.pol_d_1[r, s2])))^es)
        end
        for o in OTH                                         # exogenous substitutes
            es = get(d.es_d, (r, s, o), 0.0)
            es != 0 && (g *= ((pcp / d.pl_oth[r, o]) / (pcp_1 / d.pl_oth_1[r, o]))^es)
        end
        return g
    end

    # *** constraints (in the order of Opt::declareConstrains()) ***
    @constraints(model, begin
        # [h1] conservation of matter of the transformed products
        mkeq2[r in R, s in SEC], dl[r, s] - sl[r, s] + netExports(r, s) == 0
        # [eq 20] demand of the transformed products
        mkeq6[r in R, s in SEC], demand(r, s) + dc[r, s] - d.additionalDemand[r, s] == 0
        # [h7, h3] imports of the transformed products
        mkeq7[r in R, s in SEC], da[r, s] / dl[r, s] - ((d.q1[r, s] * pl[r, s]) / ((1 - d.q1[r, s]) * d.pworld[s]))^d.psi[r, s] == 0
        # [h9] composite price of the transformed products
        mkeq13[r in R, s in SEC], pc[r, s] * dc[r, s] - dl[r, s] * pl[r, s] - da[r, s] * d.pworld[s] == 0
        # [h3] composite demand (CES aggregation of local and imported products)
        mkeq23[r in R, s in SEC], dc[r, s] - (d.q1[r, s] * da[r, s]^((d.psi[r, s] - 1) / d.psi[r, s]) +
            (1 - d.q1[r, s]) * dl[r, s]^((d.psi[r, s] - 1) / d.psi[r, s]))^(d.psi[r, s] / (d.psi[r, s] - 1)) == 0
        # [h2] conservation of matter of the primary products
        mkeq3[r in R, p in PRI], dl[r, p] - sl[r, p] + netExports(r, p) - sum(d.a_pr[r, p2, p] * sl[r, p2] for p2 in PRI) == 0
        # [eq 13] Leontief transformation function: demand of primary products by the transformers
        mkeq4[r in R, p in PRI], dl[r, p] - sum(d.a[r, p, s] * sl[r, s] for s in SEC) == 0
        # [eq 21] supply of the primary products
        mkeq5[r in R, p in PRI], sc[r, p] - d.ff[r, p] * (pc[r, p] - d.omega[r, p] * d.Pct[r])^σ[r, p] == 0
        # [h8, h4] exports of the primary products
        mkeq8[r in R, p in PRI], sa[r, p] / sl[r, p] - ((d.t1[r, p] * pl[r, p]) / ((1 - d.t1[r, p]) * d.pworld[p]))^d.eta[r, p] == 0
        # [h10] composite price of the primary products
        mkeq14[r in R, p in PRI], pc[r, p] * sc[r, p] - sl[r, p] * pl[r, p] - sa[r, p] * d.pworld[p] == 0
        # [h4] composite supply (CET aggregation of local supply and exports)
        mkeq24[r in R, p in PRI], sc[r, p] - (d.t1[r, p] * sa[r, p]^((d.eta[r, p] - 1) / d.eta[r, p]) +
            (1 - d.t1[r, p]) * sl[r, p]^((d.eta[r, p] - 1) / d.eta[r, p]))^(d.eta[r, p] / (d.eta[r, p] - 1)) == 0
        # [h16] the transformation is limited by the production capacity
        mkeq17[r in R, s in SEC], sl[r, s] - d.k[r, s] <= 0
        # no profit for the transport agents: price differences not higher than the transport costs
        mkeq26[r in R, p in ALL, rTo in dest[r]], pl[rTo, p] - pl[r, p] - d.ct[r, p, rTo] <= 0
        # no profit for the transformers: the price covers the primary inputs and the transformation margin
        mkeq25[r in R, s in SEC], d.mm[r, s] - pl[r, s] + sum(d.a[r, p, s] * pl[r, p] for p in PRI) >= 0
        # the supply of any combination of primary products is limited by the available inventory
        resbounds[r in R, c in eachindex(COMBS)],
            -d.inRes[r, c] + sum(sa[r, p] + sl[r, p] for p in COMBS[c]; init = 0.0) - d.overharvestingAllowance <= 0
    end)
    return model
end

"""
    solveMarket!(m)

Build and solve the market equilibrium of the current year. The optimal values are stored in the product data
(as Opt::finalize_solution()). Returns (solved, iterations, objective).
"""
function solveMarket!(m)
    firstYear = m.market === nothing
    firstYear && (m.market = MarketSets(m))
    S = m.market
    model = buildMarketModel(m, S, marketData(m, S))
    if firstYear
        nEq = num_constraints(model, NonlinearExpr, MOI.EqualTo{Float64}) + num_constraints(model, AffExpr, MOI.EqualTo{Float64})
        nLe = num_constraints(model, AffExpr, MOI.LessThan{Float64})
        nGe = num_constraints(model, AffExpr, MOI.GreaterThan{Float64})
        msgOut(MSG_INFO, "The model will work with $(num_variables(model)) variables and $(nEq + nLe + nGe) constrains ($nEq equalities, $nLe lower than 0 and $nGe greater than 0)")
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

    solved = termination_status(model) == MOI.LOCALLY_SOLVED
    objv = objective_value(model)
    println("\n\nObjective value")
    @printf("f(x*) = %e\n", objv)
    # --> finalize_solution(): optimal values to the product data
    for (name, PRODS) in (("da", S.SEC), ("dc", S.SEC), ("dl", S.ALL), ("pc", S.ALL), ("pl", S.ALL), ("sa", S.PRI),
                          ("sc", S.PRI), ("sl", S.ALL))
        x = model[Symbol(name)]
        for r in S.R, p in PRODS
            setProdData!(m, value(x[r, p]), name, r, p)
        end
    end
    rt = model[:rt]
    for r in S.R, p in S.ALL, rTo in S.dest[r]
        setProdData!(m, value(rt[r, p, rTo]), "rt", r, p, DATA_NOW, false, i2s(rTo))
    end
    iters = solved ? MOI.get(model, MOI.BarrierIterations()) : 0
    return solved, iters, objv
end
