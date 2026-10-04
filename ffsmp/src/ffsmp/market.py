"""Port of the Opt class: the yearly partial equilibrium market problem, as a Pyomo model solved with Ipopt.

The model is written as an algebraic model over sets of regions and products. It is built every year with the
data of that year (read by `marketData()`) and it starts from the values of the previous year, as
Opt::get_starting_point(). The optimal values are then stored in the product data (Opt::finalize_solution()).

Sets
  R          level-2 regions of the market (the non residual regions of the modelled level-1 regions)
  ROUTES     pairs (r, rTo) of regions of the same level-1 region, between which trade is possible
  PRI, SEC   primary and secondary (transformed) products; ALL = PRI + SEC
  COMBS      all the combinations (subsets) of primary products, for the resource bounds

Variables (all with lower bound 0)
  pl  local price              (R x ALL)       pc  composite price            (R x ALL)
  sl  local supply             (R x ALL)       dl  local demand               (R x ALL)
  sc  composite supply         (R x PRI)       sa  supply to abroad (exports) (R x PRI)
  dc  composite demand         (R x SEC)       da  demand from abroad (imp.)  (R x SEC)
  rt  regional trade           (ROUTES x ALL)

The welfare (sum of the surpluses of consumers, producers, transformers and transport agents) is maximised
subject to the market equations. The constraint names are those of the C++ code (mkeq2, mkeq6, ...).
"""

import contextlib
import logging
import os
import sys
import tempfile

import pyomo.environ as pyo

from .utils import msgOut, i2s, createCombinationsVector, MSG_INFO, MSG_CRITICAL_ERROR, DATA_NOW
from .modeldata import (getProdData, setProdData, getStringVectorSetting, getIntSetting, getDoubleSetting,
                        getStringSetting, getL2r, getRegion)

# the market variables and their product domain
MARKET_VARS = [("da", "SEC"), ("dc", "SEC"), ("dl", "ALL"), ("pc", "ALL"), ("pl", "ALL"), ("rt", "ALL"),
               ("sa", "PRI"), ("sc", "PRI"), ("sl", "ALL")]


class MarketSets:
    """Regions and products of the market (fixed for the whole simulation)"""

    def __init__(self, m):
        self.PRI = getStringVectorSetting(m, "priProducts")
        self.SEC = getStringVectorSetting(m, "secProducts")
        self.ALL = self.PRI + self.SEC
        self.OTH = getStringVectorSetting(m, "othExogenousProducts")    # exogenous substitutes (e.g. fossil fuels)
        self.l2r = getL2r(m.MD, True)
        self.R = [r for regs in self.l2r for r in regs]
        self.ROUTES = [(r, rTo) for regs in self.l2r for r in regs for rTo in regs]
        self.destinations = {r: [rTo for regs in self.l2r if r in regs for rTo in regs] for r in self.R}
        self.COMBS = createCombinationsVector(len(self.PRI))           # subsets of PRI (as positions)
        self.WORLD = getIntSetting(m, "worldCodeLev2")                 # region code of the world prices

    def products(self, domain):
        return {"PRI": self.PRI, "SEC": self.SEC, "ALL": self.ALL}[domain]


def marketData(m, S):
    """The data of the market problem of the current year (one dict by parameter, keyed by region and products)"""
    previousYear = m.SCD.year - 1
    intRate = getDoubleSetting(m, "ir")
    D = {k: {} for k in ("aa", "bb", "ff", "gg", "sigma", "Pct", "omega", "a_pr", "a", "ct", "mm", "k", "pc_1",
                         "pol_d", "pol_d_1", "additionalDemand", "es_d", "pl_oth", "pl_oth_1", "q1", "psi", "t1",
                         "eta", "inRes")}

    def gpd(t, r, p, y=DATA_NOW, fd=""):
        return getProdData(m, t, r, p, y, fd)

    D["pworld"] = {p: gpd("pl", S.WORLD, p) for p in S.ALL}
    D["overharvestingAllowance"] = getDoubleSetting(m, "overharvestingAllowance", DATA_NOW)
    for r in S.R:
        carbonPrice = gpd("carbonPrice", r, "", DATA_NOW)
        for p in S.ALL:
            D["sigma"][r, p] = gpd("sigma", r, p)
            for rTo in S.destinations[r]:
                # transport cost, with the transport subsidy
                D["ct"][r, rTo, p] = gpd("ct", r, p, DATA_NOW, i2s(rTo)) * gpd("pol_tcSub", r, p)
        for p in S.PRI:
            D["bb"][r, p] = gpd("bb", r, p)
            D["ff"][r, p] = gpd("ff", r, p)
            D["Pct"][r, p] = carbonPrice * intRate                            # carbon cost of the harvest
            D["omega"][r, p] = gpd("co2content_products", r, p) / 1000
            D["t1"][r, p] = gpd("t1_polCorr", r, p)
            D["eta"][r, p] = gpd("eta", r, p)
            for p2 in S.PRI:
                D["a_pr"][r, p, p2] = gpd("a_pr", r, p, DATA_NOW, p2)        # transformation p -> p2
            for s in S.SEC:
                D["a"][r, p, s] = gpd("a", r, p, DATA_NOW, s)                # input of p by unit of s
        for s in S.SEC:
            D["pc_1"][r, s] = gpd("pc", r, s, previousYear)
            D["pol_d"][r, s] = gpd("pol_mktDirInt_d", r, s)                  # demand policy (< 1: subsidy)
            D["pol_d_1"][r, s] = gpd("pol_mktDirInt_d", r, s, previousYear)
            D["aa"][r, s] = gpd("aa", r, s)
            D["gg"][r, s] = gpd("gg", r, s)
            D["mm"][r, s] = gpd("m", r, s) * gpd("pol_trSub", r, s)          # transformation margin, with the subsidy
            D["k"][r, s] = gpd("k", r, s)                                    # production capacity
            D["additionalDemand"][r, s] = gpd("additionalDemand", r, s)
            D["q1"][r, s] = gpd("q1_polCorr", r, s)
            D["psi"][r, s] = gpd("psi", r, s)
            for s2 in S.SEC + S.OTH:
                es_d = gpd("es_d", r, s, DATA_NOW, s2)
                if es_d != 0:
                    D["es_d"][r, s, s2] = es_d                              # substitution elasticity s -> s2
        for o in S.OTH:
            D["pl_oth"][r, o] = gpd("pl", r, o, DATA_NOW)
            D["pl_oth_1"][r, o] = gpd("pl", r, o, previousYear)
        REG = getRegion(m.MD, r)
        for c in range(len(S.COMBS)):
            D["inRes"][r, c] = REG.inResByAnyCombination[c]                  # available inventory
    return D


def startingValues(m, S, name, domain):
    """The values of a market variable in the previous year (the starting point of the solver)"""
    previousYear = m.SCD.year - 1
    if name == "rt":
        return {(r, rTo, p): getProdData(m, "rt", r, p, previousYear, i2s(rTo)) for (r, rTo) in S.ROUTES for p in S.ALL}
    return {(r, p): getProdData(m, name, r, p, previousYear) for r in S.R for p in S.products(domain)}


@contextlib.contextmanager
def _toleratedBoundViolations():
    """Ipopt relaxes the bounds (bound_relax_factor): values slightly below 0 (~1e-9) are normal and used as such,
    as in the C++ code. Pyomo would log a warning (W1002) for each of them when setting the variables."""
    logger = logging.getLogger("pyomo.core")
    level = logger.level
    logger.setLevel(logging.ERROR)
    try:
        yield
    finally:
        logger.setLevel(level)


def positivePart(x):
    """max(0, x), as in the C++ code (the variables are non negative, but Ipopt slightly relaxes the bounds)"""
    return pyo.Expr_if(IF=x >= 0, THEN=x, ELSE=0.0)


def buildMarketModel(m, S, D):
    """The market problem of the current year as a Pyomo model"""
    M = pyo.ConcreteModel(name="FFSM market")
    M.R = pyo.Set(initialize=S.R, ordered=True)
    M.ROUTES = pyo.Set(initialize=S.ROUTES, dimen=2, ordered=True)
    M.PRI = pyo.Set(initialize=S.PRI, ordered=True)
    M.SEC = pyo.Set(initialize=S.SEC, ordered=True)
    M.ALL = pyo.Set(initialize=S.ALL, ordered=True)
    M.COMBS = pyo.Set(initialize=range(len(S.COMBS)), ordered=True)

    def var(name, index, domain):
        # lower bound 0 (not a NonNegativeReals domain: the starting values, from the previous solution, can be
        # slightly negative, as Ipopt relaxes the bounds)
        return pyo.Var(*index, bounds=(0, None), initialize=startingValues(m, S, name, domain))
    M.da = var("da", (M.R, M.SEC), "SEC")
    M.dc = var("dc", (M.R, M.SEC), "SEC")
    M.dl = var("dl", (M.R, M.ALL), "ALL")
    M.pc = var("pc", (M.R, M.ALL), "ALL")
    M.pl = var("pl", (M.R, M.ALL), "ALL")
    M.rt = var("rt", (M.ROUTES, M.ALL), "ALL")
    M.sa = var("sa", (M.R, M.PRI), "PRI")
    M.sc = var("sc", (M.R, M.PRI), "PRI")
    M.sl = var("sl", (M.R, M.ALL), "ALL")

    pworld = D["pworld"]
    sigma = D["sigma"]
    dest = S.destinations

    # *** objective: total welfare ***
    def welfare(M):
        w = 0
        for r in M.R:
            # consumers' surplus (secondary products)
            for s in M.SEC:
                e = (sigma[r, s] + 1) / sigma[r, s]
                w += D["aa"][r, s] * positivePart(M.dc[r, s]) ** e - M.pc[r, s] * M.dc[r, s]
            # producers' surplus (primary products), net of the carbon cost of the harvest
            for p in M.PRI:
                e = (sigma[r, p] + 1) / sigma[r, p]
                w += (M.pc[r, p] * M.sc[r, p]
                      - (D["bb"][r, p] * positivePart(M.sc[r, p]) ** e + M.sc[r, p] * D["omega"][r, p] * D["Pct"][r, p]))
            # transformations between primary products (only the non null coefficients)
            for p1 in M.PRI:
                for p2 in M.PRI:
                    a_pr = D["a_pr"][r, p1, p2]
                    if a_pr != 0:
                        e = (sigma[r, p2] + 1) / sigma[r, p2]
                        w += M.pc[r, p2] * a_pr * M.sc[r, p1] - D["bb"][r, p2] * positivePart(a_pr * M.sc[r, p1]) ** e
            # transport agents' surplus
            for p in M.ALL:
                for rTo in dest[r]:
                    w += (M.pl[rTo, p] - M.pl[r, p] - D["ct"][r, rTo, p]) * M.rt[r, rTo, p]
            # transformers' surplus: sales of the transformed products net of the transformation margin...
            for s in M.SEC:
                w += (M.pl[r, s] - D["mm"][r, s]) * M.sl[r, s]
            # ... minus the purchase of the primary products
            for p in M.PRI:
                w += -M.pl[r, p] * M.dl[r, p]
        return w
    M.welfare = pyo.Objective(rule=welfare, sense=pyo.maximize)

    def netExports(M, r, p):
        """Trade balance of region r for product p: what it sends minus what it receives"""
        return sum(M.rt[r, rTo, p] - M.rt[rTo, r, p] for rTo in dest[r])

    # *** constraints (in the order of Opt::declareConstrains()) ***

    # mkeq2 [h1] conservation of matter of the transformed products
    M.mkeq2 = pyo.Constraint(M.R, M.SEC, rule=lambda M, r, s:
                             M.dl[r, s] - M.sl[r, s] + netExports(M, r, s) == 0)

    # mkeq6 [eq 20] demand of the transformed products (with substitution elasticities)
    def demand(M, r, s):
        pcp = M.pc[r, s] * D["pol_d"][r, s]                                 # consumer price, with the policy
        pcp_1 = D["pc_1"][r, s] * D["pol_d_1"][r, s]                        # ... the previous year
        g = -D["gg"][r, s] * pcp ** sigma[r, s]
        for s2 in S.SEC:                                                    # substitutes among the products
            es_d = D["es_d"].get((r, s, s2))
            if es_d is not None:
                ratio = pcp / (M.pc[r, s2] * D["pol_d"][r, s2])
                ratio_1 = pcp_1 / (D["pc_1"][r, s2] * D["pol_d_1"][r, s2])
                g = g * (ratio / ratio_1) ** es_d
        for o in S.OTH:                                                     # exogenous substitutes
            es_d = D["es_d"].get((r, s, o))
            if es_d is not None:
                g = g * ((pcp / D["pl_oth"][r, o]) / (pcp_1 / D["pl_oth_1"][r, o])) ** es_d
        return g + M.dc[r, s] - D["additionalDemand"][r, s] == 0
    M.mkeq6 = pyo.Constraint(M.R, M.SEC, rule=demand)

    # mkeq7 [h7 and h3] imports of the transformed products
    def imports(M, r, s):
        q1, psi = D["q1"][r, s], D["psi"][r, s]
        return M.da[r, s] / M.dl[r, s] - ((q1 * M.pl[r, s]) / ((1 - q1) * pworld[s])) ** psi == 0
    M.mkeq7 = pyo.Constraint(M.R, M.SEC, rule=imports)

    # mkeq13 [h9] composite price of the transformed products
    M.mkeq13 = pyo.Constraint(M.R, M.SEC, rule=lambda M, r, s:
                              M.pc[r, s] * M.dc[r, s] - M.dl[r, s] * M.pl[r, s] - M.da[r, s] * pworld[s] == 0)

    # mkeq23 [h3] composite demand (CES aggregation of local and imported products)
    def compositeDemand(M, r, s):
        q1, psi = D["q1"][r, s], D["psi"][r, s]
        e = (psi - 1) / psi
        return M.dc[r, s] - (q1 * M.da[r, s] ** e + (1 - q1) * M.dl[r, s] ** e) ** (psi / (psi - 1)) == 0
    M.mkeq23 = pyo.Constraint(M.R, M.SEC, rule=compositeDemand)

    # mkeq3 [h2] conservation of matter of the primary products
    M.mkeq3 = pyo.Constraint(M.R, M.PRI, rule=lambda M, r, p:
                             M.dl[r, p] - M.sl[r, p] + netExports(M, r, p)
                             - sum(D["a_pr"][r, p2, p] * M.sl[r, p2] for p2 in M.PRI) == 0)

    # mkeq4 [eq 13] Leontief transformation function: demand of primary products by the transformers
    M.mkeq4 = pyo.Constraint(M.R, M.PRI, rule=lambda M, r, p:
                             M.dl[r, p] - sum(D["a"][r, p, s] * M.sl[r, s] for s in M.SEC) == 0)

    # mkeq5 [eq 21] supply of the primary products
    M.mkeq5 = pyo.Constraint(M.R, M.PRI, rule=lambda M, r, p:
                             M.sc[r, p] - D["ff"][r, p] * (M.pc[r, p] - D["omega"][r, p] * D["Pct"][r, p]) ** sigma[r, p] == 0)

    # mkeq8 [h8 and h4] exports of the primary products
    def exports(M, r, p):
        t1, eta = D["t1"][r, p], D["eta"][r, p]
        return M.sa[r, p] / M.sl[r, p] - ((t1 * M.pl[r, p]) / ((1 - t1) * pworld[p])) ** eta == 0
    M.mkeq8 = pyo.Constraint(M.R, M.PRI, rule=exports)

    # mkeq14 [h10] composite price of the primary products
    M.mkeq14 = pyo.Constraint(M.R, M.PRI, rule=lambda M, r, p:
                              M.pc[r, p] * M.sc[r, p] - M.sl[r, p] * M.pl[r, p] - M.sa[r, p] * pworld[p] == 0)

    # mkeq24 [h4] composite supply (CET aggregation of local supply and exports)
    def compositeSupply(M, r, p):
        t1, eta = D["t1"][r, p], D["eta"][r, p]
        e = (eta - 1) / eta
        return M.sc[r, p] - (t1 * M.sa[r, p] ** e + (1 - t1) * M.sl[r, p] ** e) ** (eta / (eta - 1)) == 0
    M.mkeq24 = pyo.Constraint(M.R, M.PRI, rule=compositeSupply)

    # mkeq17 [h16] the transformation is limited by the regional production capacity
    M.mkeq17 = pyo.Constraint(M.R, M.SEC, rule=lambda M, r, s: M.sl[r, s] - D["k"][r, s] <= 0)

    # mkeq26 no profit for the transport agents: price differences not higher than the transport costs
    M.mkeq26 = pyo.Constraint(M.ROUTES, M.ALL, rule=lambda M, r, rTo, p:
                              M.pl[rTo, p] - M.pl[r, p] - D["ct"][r, rTo, p] <= 0)

    # mkeq25 no profit for the transformers: the price covers the inputs and the transformation margin
    M.mkeq25 = pyo.Constraint(M.R, M.SEC, rule=lambda M, r, s:
                              D["mm"][r, s] - M.pl[r, s] + sum(D["a"][r, p, s] * M.pl[r, p] for p in M.PRI) >= 0)

    # resbounds: the supply of any combination of primary products is limited by the available inventory
    def resourceBound(M, r, c):
        if not S.COMBS[c]:
            return pyo.Constraint.Skip     # the empty combination (a constant row in the C++ code)
        supply = sum(M.sa[r, S.PRI[i]] + M.sl[r, S.PRI[i]] for i in S.COMBS[c])
        return -D["inRes"][r, c] + supply - D["overharvestingAllowance"] <= 0
    M.resbounds = pyo.Constraint(M.R, M.COMBS, rule=resourceBound)
    return M


def ipoptOptions(m):
    opts = {"linear_solver": getStringSetting(m, "linearSolver", DATA_NOW),
            "max_cpu_time": 1800.0,
            "check_derivatives_for_naninf": "yes",
            "tol": 1e-06,
            "acceptable_tol": 1e-05}
    if m.hsllib is not None:
        opts["hsllib"] = m.hsllib
    opts.update(m.ipoptOptions)
    return opts


def _iterations(logfile):
    with open(logfile, errors="replace") as f:
        for line in f:
            if line.startswith("Number of Iterations"):
                return int(line.split(":")[1])
    return 0


def solveMarket(m):
    """Build and solve the market equilibrium of the current year and store the optimal values in the product
    data (as Opt::finalize_solution()). Returns (solved, iterations, objective)."""
    if m.market is None:
        S = m.market = MarketSets(m)
        nVar = len(S.R) * (2 * len(S.SEC) + 4 * len(S.ALL) + 2 * len(S.PRI)) + len(S.ROUTES) * len(S.ALL)
        nEq = len(S.R) * (5 * len(S.SEC) + 6 * len(S.PRI))
        nLe = len(S.R) * (len(S.SEC) + len(S.COMBS)) + len(S.ROUTES) * len(S.ALL)
        nGe = len(S.R) * len(S.SEC)
        msgOut(MSG_INFO, f"The model will work with {nVar} variables and {nEq + nLe + nGe} constrains ({nEq} equalities, {nLe} lower than 0 and {nGe} greater than 0)")
    S = m.market
    with _toleratedBoundViolations():
        M = buildMarketModel(m, S, marketData(m, S))

    msgOut(MSG_INFO, "Running optimisation problem for this year (it may take a few minutes for large models)..")
    sys.stdout.flush()   # keep the order with the Ipopt output
    solver = pyo.SolverFactory("ipopt")
    if not solver.available(exception_flag=False):
        msgOut(MSG_CRITICAL_ERROR, "The ipopt executable was not found (install it, e.g. from conda-forge or with the system package manager).")
    fd, logfile = tempfile.mkstemp(prefix="ffsmp_ipopt_", suffix=".log")
    os.close(fd)
    try:
        results = solver.solve(M, tee=True, logfile=logfile, load_solutions=False, options=ipoptOptions(m))
        iters = _iterations(logfile)
    finally:
        os.remove(logfile)
    solved = (results.solver.termination_condition == pyo.TerminationCondition.optimal
              and "Optimal Solution Found" in str(results.solver.message))
    if len(results.solution) > 0:
        with _toleratedBoundViolations():
            M.solutions.load_from(results)
    objv = pyo.value(M.welfare, exception=False) or 0.0
    print("\n\nObjective value")
    print("f(x*) = %e" % objv)
    # --> finalize_solution(): optimal values to the product data
    for name, domain in MARKET_VARS:
        v = getattr(M, name)
        if name == "rt":
            for (r, rTo) in S.ROUTES:
                for p in S.ALL:
                    setProdData(m, pyo.value(v[r, rTo, p]), "rt", r, p, DATA_NOW, False, i2s(rTo))
        else:
            for r in S.R:
                for p in S.products(domain):
                    setProdData(m, pyo.value(v[r, p]), name, r, p)
    return solved, (iters if solved else 0), objv
