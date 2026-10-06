# Builds test/data/ffsm_tests.ods, the input file of the test suite, from data/ffsmInput.ods:
# the debug and default scenarios are kept and the t_* test scenarios are added, each with its own
# (small) override sheets. Also writes test/data/scenarios/t_settingFile.csv.
#
#   julia --project=. test/build_test_input.jl
#
# All the t_* scenarios run on the two debug regions (11001, 11002) for 8 years (2013-2020).
# See doc/port_to_julia/tests.md for the description of each scenario and of what is checked.

using FFSMj, EzXML, ZipArchives, Printf  # ZipArchives only to read the master file

const ROOT = normpath(joinpath(@__DIR__, ".."))
const MASTER = joinpath(ROOT, "data", "ffsmInput.ods")
const OUTDIR = joinpath(@__DIR__, "data")

# ------------------------------------------------------------------------------
# Master tables (read with the model's own reader)
# ------------------------------------------------------------------------------

const TABLES = FFSMj.loadInput(MASTER, dirname(MASTER) * "/")
table(name) = only(filter(t -> t.tableName == name, TABLES))
const SETTING_TYPES = Dict((r[1], r[3]) => r[2] for r in table("settings").records)
settingType(name) = get(SETTING_TYPES, (name, ""), get(SETTING_TYPES, (name, "11000"), "string"))

"Values of a data record (value_0, value_1, ... skipping empty cells, as the model does)"
recValues(t, r) = [parse(Float64, r[i]) for (i, h) in enumerate(t.headers) if startswith(h, "value_") && r[i] != ""]
masterRows(tname, par; filt = r -> true) = [(r[1], r[2], r[3], r[4], recValues(table(tname), r)) for r in table(tname).records if r[1] == par && filt(r)]

const Y0 = 2011
"Time series from 2011: `before` until `from-1`, then `after`"
ts(before, after; from = 2015) = vcat(fill(Float64(before), from - Y0), [Float64(after)])
"Apply f(year, value) to a time series, extending it to 2020 first"
function remap(vals, f)
    v = copy(vals)
    while length(v) < 2020 - Y0 + 1
        push!(v, v[end])
    end
    return [f(Y0 + i - 1, x) for (i, x) in enumerate(v)]
end
scaleFrom(k; from = 2015) = (y, x) -> y >= from ? x * k : x

# ------------------------------------------------------------------------------
# Scenario definitions
# ------------------------------------------------------------------------------

Base.@kwdef struct TestScen
    id::String
    desc::String
    settings::Vector{Tuple{String,String,Vector{String}}} = Tuple{String,String,Vector{String}}[] # (name, region, values)
    forData::Vector{Tuple{String,String,String,String,Vector{Float64}}} = Tuple{String,String,String,String,Vector{Float64}}[]
    prodData::Vector{Tuple{String,String,String,String,Vector{Float64}}} = Tuple{String,String,String,String,Vector{Float64}}[]
    forToProd::Union{Nothing,Vector{NTuple{4,String}}} = nothing
    pathRules::Vector{Tuple{String,String,String,Float64,Vector{Float64}}} = Tuple{String,String,String,Float64,Vector{Float64}}[]
    settingFile::String = ""
end

const COMMON_SETTINGS = [("debugFlag", "", ["TRUE"]), ("simulationYears", "", ["8"]), ("outputLevel", "", ["15"]),
                         ("outDetailedHv", "", ["TRUE"])]
st(name, vals...; region = "") = (name, region, [string(v) for v in vals])

const FTS = ["Fut_Feu", "Fut_Mix", "Fut_Con", "Mel_Feu", "Mel_Mix", "Tail_Feu"]
const PRI = ["hardWRoundW", "softWRoundW", "pulpWFuelW"]
const SEC = ["fuelW", "hardWSawnW", "softWSawnW", "plyW", "pulpW", "pannels"]
f2pMaster() = [(r[1], r[2], r[3], r[4]) for r in table("forToProd").records]

function scenarios()
    S = TestScen[]
    add(id, desc; kw...) = push!(S, TestScen(; id, desc, kw...))

    add("t_base", "Test baseline: debug regions, 2013-2020, no change")
    # --- management and agents' behaviour (settings) ---
    add("t_mr0", "No managed regeneration (mr = 0): all regeneration is natural (replanting)"; settings = [st("mr", 0)])
    add("t_mr1", "Fully managed regeneration (mr = 1)"; settings = [st("mr", 1)])
    add("t_mrTime", "Management rate as time series: 0.7 until 2015, 0.1 from 2016"; settings = [st("mr", 0.7, 0.7, 0.7, 0.7, 0.7, 0.1)])
    add("t_mrRegional", "Management rate from regional data (forData mr 0.1 in 11001) and regional setting (0.95 in 11002)";
        settings = [st("mr", 0.95; region = "11002")], forData = [("mr", "11001", "", "", [0.1])])
    add("t_noRA", "No (heterogeneous) risk aversion"; settings = [st("heterogeneousRiskAversion", "FALSE"), st("riskAversionAgentAverage", 0), st("riskAversionAgentSd", 0)])
    add("t_expNone", "Adaptive expectations (expType = expTypePrices = 0, no variance)"; settings = [st("expType", 0), st("expType_cv", 0), st("expTypePrices", 0), st("expTypePrices_cv", 0)])
    add("t_expFull", "Full expectations (expType = expTypePrices = 1, no variance)"; settings = [st("expType", 1), st("expType_cv", 0), st("expTypePrices", 1), st("expTypePrices_cv", 0)])
    add("t_expFixed", "Expectations fixed at the initial values (expType = -1, GAMS compatibility)"; settings = [st("expType", -1)])
    add("t_noSpVar", "No spatial variance of the time of passage"; settings = [st("useSpatialRegionalVariance", "FALSE")])
    add("t_spPropVol", "Spatial multipliers proportional to the volumes"; settings = [st("useSpatialVarPropToVol", "TRUE")])
    add("t_natRegPP", "Natural regeneration allocated by probability of presence"; settings = [st("natRegAllocation", "pp")])
    add("t_regFixed", "Fixed (exogenous) regeneration volumes"; settings = [st("regType", "fixed")])
    add("t_oldVol2Area", "Old volume-to-area conversion method"; settings = [st("oldVol2AreaMethod", "TRUE")])
    add("t_noAvalCoef", "Availability coefficient not applied"; settings = [st("applyAvalCoef", "FALSE")])
    add("t_noDeathTimber", "Dead timber not usable"; settings = [st("useDeathTimber", "FALSE")])
    # --- climate change multipliers ---
    add("t_ccTable", "Faster growth (tp_multiplier 0.8) for all forest types and higher coniferous mortality (1.5) from 2015, from the forData table";
        forData = vcat([(p, r, ft, fd, remap(v, scaleFrom(0.8))) for (p, r, ft, fd, v) in masterRows("forData", "tp_multiplier"; filt = r -> r[2] in ("11001", "11002"))],
                       [(p, r, ft, fd, remap(v, scaleFrom(1.5))) for (p, r, ft, fd, v) in masterRows("forData", "mortCoef_multiplier"; filt = r -> r[2] in ("11001", "11002") && r[3] == "Fut_Con")]))
    add("t_ccMaps", "Faster Fut_Feu growth (0.8) and higher Fut_Con mortality (1.5) from 2015, from maps"; settings = [st("modifiersFromTable"), st("spatialDataSubfolder", "gis/tests/ccMaps")])
    add("t_avalCoefFt", "Availability halved, and 0.2 for Fut_Con (maps)"; settings = [st("spatialDataSubfolder", "gis/tests/aval")])
    # --- market: prices, policies, demand, substitution, elasticities ---
    add("t_worldPrices", "World prices +30% from 2015 (region 99999)";
        prodData = [(p, r, pr, fd, remap(v, scaleFrom(1.3))) for (p, r, pr, fd, v) in masterRows("prodData", "pl"; filt = r -> r[2] == "99999")])
    add("t_polSupplySub", "Supply subsidy (pol_mktDirInt_s 1.2, >1 is a subsidy) on hardwood roundwood in region 11001 only, from 2015";
        prodData = [("pol_mktDirInt_s", "11001", "hardWRoundW", "", ts(1, 1.2))])
    add("t_polDemandSub", "Demand subsidy (pol_mktDirInt_d 0.8) on hardwood sawnwood, from 2015"; prodData = [("pol_mktDirInt_d", "11000", "hardWSawnW", "", ts(1, 0.8))])
    add("t_polTrSub", "Transformation subsidy (pol_trSub 0.8) on softwood sawnwood, from 2015"; prodData = [("pol_trSub", "11000", "softWSawnW", "", ts(1, 0.8))])
    add("t_polTcSub", "Transport cost subsidy (pol_tcSub 0.5) on all products, from 2015"; prodData = [("pol_tcSub", "11000", p, "", ts(1, 0.5)) for p in vcat(PRI, SEC)])
    add("t_polFiSub", "Forest investment subsidy (pol_fiSub 0.5) towards Fut_Con, from 2015";
        forData = [(p, r, ft, fd, ts(1, 0.5)) for (p, r, ft, fd, v) in masterRows("forData", "pol_fiSub"; filt = r -> r[4] == "Fut_Con")])
    add("t_mktStr", "Market structure policies (pol_mktStr_s/_d 0.5)";
        prodData = vcat([("pol_mktStr_s", "11000", p, "", [0.5]) for p in PRI], [("pol_mktStr_d", "11000", p, "", [0.5]) for p in SEC]))
    add("t_substitution", "Substitution elasticities: fuelwood vs fossil fuel 0.5, hardwood vs softwood sawnwood 0.3";
        prodData = [("es_d", "11000", "fuelW", "fossilFuel", [0.5]), ("es_d", "11000", "hardWSawnW", "softWSawnW", [0.3]), ("es_d", "11000", "softWSawnW", "hardWSawnW", [0.3])])
    # additionalDemand is a yearly increment of the demand (the demand curve is recalibrated each year on the
    # previous consumption): a permanent shift is given once, a single-year shock is withdrawn the next year
    add("t_addDemand", "Permanent additional demand of 0.3 Mm3 of hardwood sawnwood in region 11001 from 2015 (0.3 in 2015, then 0)";
        prodData = [("additionalDemand", "11001", "hardWSawnW", "", vcat(zeros(2015 - Y0), [0.3, 0.0]))])
    add("t_addDemandShock", "Single-year additional demand of 0.3 Mm3 of hardwood sawnwood in region 11001 in 2015 (0.3, then -0.3 in 2016, then 0)";
        prodData = [("additionalDemand", "11001", "hardWSawnW", "", vcat(zeros(2015 - Y0), [0.3, -0.3, 0.0]))])
    add("t_elasticity", "Doubled supply elasticities to the (alive and dead) inventories";
        prodData = vcat([(p, r, pr, fd, 2 .* v) for g in ("gamma_incr", "gamma_decr", "gamma_d_incr", "gamma_d_decr") for (p, r, pr, fd, v) in masterRows("prodData", g)]))
    # --- carbon ---
    add("t_carbonPrice", "Carbon price 50 EUR/tCO2 from 2015 (market only, no Hartman payments)"; prodData = [("carbonPrice", "11000", "", "", ts(0, 50))])
    add("t_hartmanExo", "Hartman payments with exogenous reference (EXO), carbon price 50"; settings = [st("flagHartman", "EXO"; region = "11000")], prodData = [("carbonPrice", "11000", "", "", ts(0, 50))])
    add("t_hartmanEndo", "Hartman payments with endogenous (BAU) reference (ENDO), carbon price 50"; settings = [st("flagHartman", "ENDO"; region = "11000")], prodData = [("carbonPrice", "11000", "", "", ts(0, 50))])
    add("t_pickling", "Hartman EXO payments with pickling factor 0.5"; settings = [st("flagHartman", "EXO"; region = "11000"), st("pickling", 0.5)], prodData = [("carbonPrice", "11000", "", "", ts(0, 50))])
    # --- disturbances ---
    add("t_windstorm", "Windstorm in 2016 in region 11001 (additional mortality maps)"; settings = [st("spatialDataSubfolder", "gis/tests/windstorm"), st("minShareDeathIn", 0.05)])
    add("t_windstormSto", "Windstorm with dead timber usable for 4 years and halved supply elasticity to dead timber";
        settings = [st("spatialDataSubfolder", "gis/tests/windstorm"), st("minShareDeathIn", 0.05)],
        forToProd = [(p, ft, dc, "4") for (p, ft, dc, _) in f2pMaster()],
        prodData = [(p, r, pr, fd, 0.5 .* v) for g in ("gamma_d_incr", "gamma_d_decr") for (p, r, pr, fd, v) in masterRows("prodData", g)])
    add("t_pathogen", "Pathogen present in region 11001 from 2014, increasing the Fut_Feu mortality";
        settings = [st("usePathogenModule", "TRUE"), st("pathogenExogenousSpreadModelFolder", "gis/tests/pathogen"), st("pathogenExogenousSpreadModelFileExtension", ".asc")],
        pathRules = [("Fut_Feu", dc, "testpath", 0.5, [0.01, 0.03, 0.06]) for dc in vcat(["0"], string.(10:10:130))])
    add("t_spExplicitFt", "Spatially explicit forest type Fut_Hetre, created from Fut_Feu volumes in region 11001";
        settings = [st("useSpExplicitForestTypes", "TRUE"), st("spExplicitForTypesInputDir", "gis/tests/spExplicit"), st("spExplicitForTypesFileExtension", ".asc")],
        forData = spExplicitForData(), forToProd = vcat(f2pMaster(), [(p, "Fut_Hetre", dc, my) for (p, ft, dc, my) in f2pMaster() if ft == "Fut_Feu"]))
    # --- forest area changes ---
    add("t_areaRel", "Forest area +1% per year (relative method)"; settings = [st("forestAreaChangeMethod", "relative")],
        forData = [("forestChangeAreaIncrementsRel", r, "", "", [0.01]) for r in ("11001", "11002")])
    add("t_areaAbs", "Forest area increase in hectares per year (absolute method, master data)"; settings = [st("forestAreaChangeMethod", "absolute")])
    add("t_areaDecr", "Forest area -0.3% per year (relative method)"; settings = [st("forestAreaChangeMethod", "relative")],
        forData = [("forestChangeAreaIncrementsRel", r, "", "", [-0.003]) for r in ("11001", "11002")])
    # --- output options ---
    add("t_outHR", "Human readable output, detailed by diameter class, renamed scenario";
        settings = [st("outputHumanReadable", "TRUE"), st("forestDiamDetailedOutput", "TRUE"), st("overridenScenarioName", "t_outHR_renamed")])
    add("t_outSingle", "Single output files, selected output years, maps (ArcInfo format) in 2016";
        settings = [st("outputSingleFile", "TRUE"), st("outStepRange", -1), st("outYears", 2013, 2016, 2020), st("outputLevel", 20),
                    st("mapsOutYears", 2016), st("mapOutputFormat", 2)])
    # --- input mechanisms ---
    add("t_settingFile", "Scenario settings read from a csv file (same settings as t_mr1)"; settingFile = "scenarios/t_settingFile.csv")
    add("t_newSeed", "New random seed at each run (results not reproducible by construction)"; settings = [st("newRandomSeed", "TRUE")])
    add("t_shuffle", "Initial random shuffle of the layers (sensitivity option)"; settings = [st("initialRandomShuffle", "TRUE")])
    return S
end

"forData rows of the spatially explicit forest type Fut_Hetre, copied from Fut_Feu (zero initial volumes)"
function spExplicitForData()
    t = table("forData")
    rows = Tuple{String,String,String,String,Vector{Float64}}[]
    for r in t.records
        v = recValues(t, r)
        ft, fd = r[3], r[4]
        if ft == "Fut_Feu" || fd == "Fut_Feu"
            ft2 = ft == "Fut_Feu" ? "Fut_Hetre" : ft
            fd2 = fd == "Fut_Feu" ? "Fut_Hetre" : fd
            push!(rows, (r[1], r[2], ft2, fd2, r[1] == "vol" ? zero(v) : v))
            if ft == "Fut_Feu" && fd == "Fut_Feu"       # e.g. invTransCost Fut_Feu -> Fut_Feu: also the mixed pairs
                push!(rows, (r[1], r[2], "Fut_Hetre", "Fut_Feu", v))
                push!(rows, (r[1], r[2], "Fut_Feu", "Fut_Hetre", v))
            end
        end
    end
    return rows
end

# ------------------------------------------------------------------------------
# ODS writing
# ------------------------------------------------------------------------------

fmtnum(x) = @sprintf("%.15g", x)

function cell(v; repeated = 1)
    c = ElementNode("table:table-cell")
    repeated > 1 && (c["table:number-columns-repeated"] = string(repeated))
    v === nothing && return c
    if v isa Real
        c["office:value-type"] = "float"
        c["office:value"] = fmtnum(v)
        c["calcext:value-type"] = "float"
        txt = fmtnum(v)
    else
        c["office:value-type"] = "string"
        c["calcext:value-type"] = "string"
        txt = string(v)
    end
    p = ElementNode("text:p")
    link!(p, TextNode(txt))
    link!(c, p)
    return c
end

"A row with the given values, padded with empty cells to `width` cells"
function rownode(vals, width)
    r = ElementNode("table:table-row")
    for v in vals
        link!(r, cell(v))
    end
    length(vals) < width && link!(r, cell(nothing; repeated = width - length(vals)))
    return r
end

function newtable(name, header, rows)
    t = ElementNode("table:table")
    t["table:name"] = name
    col = ElementNode("table:table-column")
    col["table:number-columns-repeated"] = string(length(header))
    link!(t, col)
    w = length(header)
    link!(t, rownode(header, w))
    for r in rows
        link!(t, rownode(r, w))
    end
    return t
end

valueHeaders(prefix, n) = ["$(prefix)_$i" for i in 0:(max(n, 1)-1)]

function settingsTable(name, settings)
    n = maximum((length(s[3]) for s in settings); init = 1)
    rows = [vcat([s[1], settingType(s[1]), s[2], ""], s[3]) for s in settings]
    return newtable(name, vcat(["name", "type", "region", "comment"], valueHeaders("value", n)), rows)
end

function dataTable(name, dim1, rows)
    n = maximum((length(r[5]) for r in rows); init = 1)
    return newtable(name, vcat(["parName", "region", dim1, "freeDim"], valueHeaders("value", n)),
        [vcat([r[1], r[2], r[3], r[4]], r[5]) for r in rows])
end

pathRulesTable(name, rules) = newtable(name, vcat(["forType", "dClass", "path_name", "pres_min"], valueHeaders("year", maximum(length(r[5]) for r in rules))),
    [vcat([r[1], r[2], r[3], r[4]], r[5]) for r in rules])

forToProdTable(name, rows) = newtable(name, ["product", "forType", "dClass", "maxYears"], [[r[1], r[2], r[3], parse(Float64, r[4])] for r in rows])

_lname(n) = nodename(n)
_attr(n, a) = (for x in attributes(n); nodename(x) == a && return nodecontent(x); end; nothing)
rowWidth(row) = sum((x = _attr(c, "number-columns-repeated"); x === nothing ? 1 : parse(Int, x)) for c in eachelement(row) if _lname(c) == "table-cell"; init = 0)

"Append rows to an existing table, after its last row with content"
function appendRows!(tableEl, rows)
    trs = [r for r in eachelement(tableEl) if _lname(r) == "table-row"]
    width = rowWidth(trs[1])
    last = trs[1]
    for r in trs
        c1 = firstelement(r)
        (c1 !== nothing && any(x -> _lname(x) == "p", eachelement(c1))) && (last = r)
    end
    for vals in rows
        n = rownode(vals, width)
        linknext!(last, n)
        last = n
    end
end

function buildOds(outfile)
    zr = ZipReader(read(MASTER))
    entries = [(n, zip_readentry(zr, n)) for n in zip_names(zr)]
    doc = parsexml(String(zip_readentry(zr, "content.xml")))
    sheetParent = nothing
    tables = Dict{String,EzXML.Node}()
    for t in findall("//*[local-name()='table']", root(doc))
        tables[_attr(t, "name")] = t
        sheetParent = parentnode(t)
    end
    lastTable = [t for t in eachelement(sheetParent) if _lname(t) == "table"][end]
    addTable!(t) = (linknext!(lastTable, t); global_last[] = t)
    global_last = Ref(lastTable)
    function addSheet!(t)
        linknext!(global_last[], t)
        global_last[] = t
    end

    scenRows = Vector{Vector{Any}}()
    for s in scenarios()
        setName = s.settingFile == "" ? s.id * "_settings" : ""
        if s.settingFile == ""
            addSheet!(settingsTable(setName, vcat(COMMON_SETTINGS, s.settings)))
        end
        fdName = isempty(s.forData) ? "" : s.id * "_forData"
        isempty(s.forData) || addSheet!(dataTable(fdName, "forType", s.forData))
        pdName = isempty(s.prodData) ? "" : s.id * "_prodData"
        isempty(s.prodData) || addSheet!(dataTable(pdName, "prod", s.prodData))
        f2pName = s.forToProd === nothing ? "" : s.id * "_forToProd"
        s.forToProd === nothing || addSheet!(forToProdTable(f2pName, s.forToProd))
        prName = isempty(s.pathRules) ? "" : s.id * "_pathRules"
        isempty(s.pathRules) || addSheet!(pathRulesTable(prName, s.pathRules))
        push!(scenRows, [s.id, "", s.desc, setName, fdName, pdName, f2pName, prName, s.settingFile])
    end
    appendRows!(tables["scenarios"], scenRows)
    # the spatially explicit forest type (used only when useSpExplicitForestTypes is true)
    appendRows!(tables["forTypes"], [["Fut_Hetre", "Beech high forest (spatially explicit, tests only)", 3.0, "forArea_Fut_Hetre", "broadL"]])

    content = sprint(print, doc)
    mkpath(dirname(outfile))
    # written with the standard zip tool (the C++ model's unzip code only reads classic zip archives),
    # with the uncompressed mimetype entry first, as required by ODF
    tmp = mktempdir()
    for (n, data) in entries
        endswith(n, "/") && (mkpath(joinpath(tmp, n)); continue)
        mkpath(dirname(joinpath(tmp, n)))
        write(joinpath(tmp, n), n == "content.xml" ? content : data)
    end
    out = abspath(outfile)
    rm(out; force = true)
    run(Cmd(`zip -q -X -0 $out mimetype`; dir = tmp))
    run(Cmd(`zip -q -X -r -9 $out . -x mimetype`; dir = tmp))
    rm(tmp; recursive = true)
    # re-saved by LibreOffice: a genuine LibreOffice file (and a validity check), whose zip headers can
    # also be read by the unzip code bundled with the C++ model
    lo = mktempdir()
    run(`soffice --headless --norestore -env:UserInstallation=file://$lo/profile --convert-to ods --outdir $lo/out $out`)
    cp(joinpath(lo, "out", basename(out)), out; force = true)
    rm(lo; recursive = true)
    return outfile
end

function writeSettingFile(fn)
    mkpath(dirname(fn))
    open(fn, "w") do io
        println(io, "name;type;region;comment;value_0")
        for (n, r, v) in vcat(COMMON_SETTINGS, [st("mr", 1)])
            println(io, join([n, settingType(n), r, "", v[1]], ";"))
        end
    end
end

if abspath(PROGRAM_FILE) == @__FILE__
    f = buildOds(joinpath(OUTDIR, "ffsm_tests.ods"))
    writeSettingFile(joinpath(OUTDIR, "scenarios", "t_settingFile.csv"))
    println("Written ", f, " with ", length(scenarios()), " test scenarios")
end
