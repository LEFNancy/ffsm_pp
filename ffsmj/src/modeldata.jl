# Port of ModelData (settings, regions, forest/product data and their regional overriding),
# ModelRegion and the small data structs.

struct ScenarioData
    id::String
    shortDesc::String
    longDesc::String
    settingTable::String
    forDataTable::String
    prodDataTable::String
    forToProdTable::String
    pathTable::String
    settingFile::String
end
ScenarioData() = ScenarioData("", "", "", "", "", "", "", "", "")

mutable struct BasicData
    name::String
    values::Vector{String}
    type::Int
    regId::Int
    comment::String
end

struct ForToProd
    product::String
    forType::String
    dClass::String
    maxYears::Int
end

mutable struct ForType
    forTypeId::String
    forLabel::String
    memType::Int
    forLayer::String
    ereditatedFrom::String
end

mutable struct ReclRule
    regId::Int
    forTypeIn::String
    forTypeOut::String
    coeff::Float64
end

mutable struct PathRule
    forType::String
    dClass::String
    pathId::String
    pres_min::Float64
    mortCoefficents::Vector{Float64}
end

mutable struct ModelRegion
    regId::Int
    regSName::String
    regLName::String
    regLevel::Int
    parRegId::Int
    isResidual::Bool
    parent::Union{Nothing,ModelRegion}
    chRegions::Vector{ModelRegion}
    myPixels::Vector{Int}                   # 1-based indices of the pixels (pixel ID + 1)
    inResByAnyCombination::Vector{Float64}
end

getChildren(r::ModelRegion, excludeResidual::Bool = true) =
    excludeResidual ? [c for c in r.chRegions if !c.isResidual] : r.chRegions
getNChildren(r::ModelRegion, excludeResidual::Bool = true) = length(getChildren(r, excludeResidual))

mutable struct ModelData
    baseDirectory::String
    outputDirname::String
    tables::Vector{LLData}
    scenario::ScenarioData
    settings::Vector{BasicData}
    settingsIndex::Dict{Tuple{String,Int},Int}
    forDataMap::DataMap
    prodDataMap::DataMap
    forToProdVector::Vector{ForToProd}
    forToProdIndex::Dict{Tuple{String,String,String},Int}
    regionsVector::Vector{ModelRegion}
    regionIndex::Dict{Int,Int}
    forTypes::Vector{ForType}
    reclRules::Vector{ReclRule}
    pathRules::Vector{PathRule}
    deathTimberInventory::Dict{Tuple{Int,Int,String,String},Float64}
    diamClasses::Vector{String}
    cached_initialYear::Int
    priProducts::Vector{String}
    secProducts::Vector{String}
    allProducts::Vector{String}
    tempBool::Bool
    valueFoundBool::Bool
    errorLevel::Int
    # caches (not in the C++ code, for performance only)
    forTypeIdsCache::Dict{Bool,Vector{String}}
end

function ModelData(baseDirectory::AbstractString)
    return ModelData(String(baseDirectory), "", LLData[], ScenarioData(), BasicData[], Dict{Tuple{String,Int},Int}(),
        DataMap(), DataMap(), ForToProd[], Dict{Tuple{String,String,String},Int}(), ModelRegion[], Dict{Int,Int}(),
        ForType[], ReclRule[], PathRule[], Dict{Tuple{Int,Int,String,String},Float64}(), String[], 0,
        String[], String[], String[], true, true, MSG_ERROR, Dict{Bool,Vector{String}}())
end

# ------------------------------------------------------------------------------
# Tables & scenarios
# ------------------------------------------------------------------------------

function getTable(MD::ModelData, tableName::AbstractString; debugLevel = MSG_CRITICAL_ERROR)
    for t in MD.tables
        t.tableName == tableName && return t
    end
    msgOut(debugLevel, "No table found with name $tableName")
    return LLData("", String[], Vector{String}[])
end

function getScenarios(MD::ModelData)
    table = getTable(MD, "scenarios")
    return [getData(table, i, "id") for i in 1:nrecords(table)]
end

function setScenarioData!(m, MD::ModelData)
    table = getTable(MD, "scenarios")
    for i in 1:nrecords(table)
        if getData(table, i, "id") == m.scenarioName
            MD.scenario = ScenarioData(getData(table, i, "id"), getData(table, i, "shortDesc"), getData(table, i, "longDesc"),
                getData(table, i, "settingTable"), getData(table, i, "forDataTable"), getData(table, i, "prodDataTable"),
                getData(table, i, "forToProdTable"), getData(table, i, "pathTable"), getData(table, i, "settingFile"))
            return
        end
    end
    msgOut(MSG_CRITICAL_ERROR, "Scenario not found!")
end

# ------------------------------------------------------------------------------
# Settings
# ------------------------------------------------------------------------------

function _readSettingRecord(table::LLData, i::Integer)
    name = getData(table, i, "name")
    type = getType(getData(table, i, "type"))
    region_s = getData(table, i, "region")
    regId = region_s == "" ? WORLD : s2i(region_s)
    comment = getData(table, i, "comment")
    values = String[]
    for z in 0:(nheaders(table) - 5)  # don't consider name, type, comment and region headers
        v = getData(table, i, "value_$z")
        v != "" && push!(values, v)
    end
    return BasicData(name, values, type, regId, comment)
end

function _reindexSettings!(MD::ModelData)
    empty!(MD.settingsIndex)
    for (i, s) in enumerate(MD.settings)
        k = (s.name, s.regId)
        haskey(MD.settingsIndex, k) || (MD.settingsIndex[k] = i)
    end
end

function setOutputDirectory!(MD::ModelData, dirname::AbstractString)
    MD.outputDirname = isempty(dirname) ? MD.baseDirectory * "output/" : String(dirname)
end

function setDefaultSettings!(m, MD::ModelData)
    table = getTable(MD, "settings")
    for i in 1:nrecords(table)
        push!(MD.settings, _readSettingRecord(table, i))
    end
    _reindexSettings!(MD)
    msgOut(MSG_INFO, "### USING SCENARIO: $(m.scenarioName) ###")
    setOutputDirectory!(MD, getStringSetting(m, "outputDirname"))
end

function setScenarioSettings!(m, MD::ModelData)
    sc = MD.scenario
    (sc.settingTable == "" && sc.settingFile == "") && return
    if sc.settingTable != "" && sc.settingFile != ""
        msgOut(MSG_CRITICAL_ERROR, "Scenario-specific settings has been indicated to be loaded from BOTH from the spreadsheet and a file. Choose one of them.")
    end
    table = sc.settingTable != "" ? getTable(MD, sc.settingTable) : getTableFromFile("settings", MD.baseDirectory * sc.settingFile)
    for i in 1:nrecords(table)
        s = _readSettingRecord(table, i)
        found = false
        for existing in MD.settings
            if existing.name == s.name && existing.regId == s.regId
                existing.values = s.values
                existing.type = s.type
                existing.comment = s.comment
                found = true
                break
            end
        end
        found || push!(MD.settings, s)
    end
    _reindexSettings!(MD)
    setOutputDirectory!(MD, getStringSetting(m, "outputDirname"))
end

"The region chain WORLD / L1 / L2 used to look up a setting"
function _settingRegIds(MD::ModelData, regId_h::Integer)
    regId_h == WORLD && return (WORLD,)
    reg = getRegion(MD, regId_h)
    if reg.regLevel == 1
        return (regId_h, WORLD)
    elseif reg.regLevel == 2
        return (regId_h, reg.parRegId, WORLD)
    end
    msgOut(MSG_CRITICAL_ERROR, "Error in getBaseData(). Data requested for a unknown region level.")
end

function getBaseData(m, name::AbstractString, type_h::Integer, position::Integer = 0, regId_h::Integer = WORLD)
    MD = m.MD
    position == DATA_NOW && (position = m.SCD.iteration)
    for regId in _settingRegIds(MD, regId_h)
        i = get(MD.settingsIndex, (name, regId), 0)
        if i > 0
            s = MD.settings[i]
            s.type != type_h && msgOut(MSG_CRITICAL_ERROR, "mismatching type in calling getBaseData() for $name")
            if length(s.values) > position
                return s.values[position+1]
            elseif length(s.values) > 0
                return s.values[end]
            else
                msgOut(MSG_CRITICAL_ERROR, "Error: $name doesn't have any value, even on the first position(year)!")
            end
        end
    end
    msgOut(MSG_CRITICAL_ERROR, "Error calling getBaseData() for $name. No setting option or macro data found with this name.")
    return ""
end

function getVectorBaseData(m, name::AbstractString, type_h::Integer, regId_h::Integer = WORLD)
    MD = m.MD
    for regId in _settingRegIds(MD, regId_h)
        i = get(MD.settingsIndex, (name, regId), 0)
        if i > 0
            s = MD.settings[i]
            s.type != type_h && msgOut(MSG_CRITICAL_ERROR, "mismatching type in calling getVectorBaseData() for $name")
            return copy(s.values)
        end
    end
    msgOut(MSG_CRITICAL_ERROR, "Error calling getVectorBaseData() for $name. No setting option or macro data found with this name.")
    return String[]
end

getIntSetting(m, name, position = 0, reg = WORLD)    = s2i(getBaseData(m, name, TYPE_INT, position, reg))
getDoubleSetting(m, name, position = 0, reg = WORLD) = s2d(getBaseData(m, name, TYPE_DOUBLE, position, reg))
getStringSetting(m, name, position = 0, reg = WORLD) = getBaseData(m, name, TYPE_STRING, position, reg)
getBoolSetting(m, name, position = 0, reg = WORLD)   = s2b(getBaseData(m, name, TYPE_BOOL, position, reg))
getIntVectorSetting(m, name, reg = WORLD)    = s2i.(getVectorBaseData(m, name, TYPE_INT, reg))
getDoubleVectorSetting(m, name, reg = WORLD) = s2d.(getVectorBaseData(m, name, TYPE_DOUBLE, reg))
getStringVectorSetting(m, name, reg = WORLD) = getVectorBaseData(m, name, TYPE_STRING, reg)
getBoolVectorSetting(m, name, reg = WORLD)   = s2b.(getVectorBaseData(m, name, TYPE_BOOL, reg))

function cacheSettings!(m, MD::ModelData)
    MD.cached_initialYear = getIntSetting(m, "initialYear")
    MD.diamClasses = getStringVectorSetting(m, "dClasses")
    MD.priProducts = getStringVectorSetting(m, "priProducts")
    MD.secProducts = getStringVectorSetting(m, "secProducts")
    MD.allProducts = vcat(MD.priProducts, MD.secProducts)
end

# ------------------------------------------------------------------------------
# Regions
# ------------------------------------------------------------------------------

function createRegions!(m, MD::ModelData)
    table = getTable(MD, "regions")
    nInBounds = 2^length(getStringVectorSetting(m, "priProducts"))
    for i in 1:nrecords(table)
        reg = ModelRegion(s2i(getData(table, i, "regId")), getData(table, i, "regSName"), getData(table, i, "regLName"),
            s2i(getData(table, i, "regLevel")), s2i(getData(table, i, "parRegId")), s2b(getData(table, i, "isResidual")),
            nothing, ModelRegion[], Int[], zeros(nInBounds))
        push!(MD.regionsVector, reg)
        haskey(MD.regionIndex, reg.regId) || (MD.regionIndex[reg.regId] = length(MD.regionsVector))
    end
    for r in MD.regionsVector
        r.parent = getRegion(MD, r.parRegId)
        r.chRegions = [c for c in MD.regionsVector if c.parRegId == r.regId]
    end
end

function getRegion(MD::ModelData, regId::Integer)
    i = get(MD.regionIndex, regId, 0)
    i == 0 && msgOut(MSG_CRITICAL_ERROR, "Region id $regId not found, check your input data. Aborting simulation.")
    return MD.regionsVector[i]
end

regionExist(MD::ModelData, regId::Integer) = haskey(MD.regionIndex, regId)

getRegionIds(MD::ModelData, level::Integer, excludeResidual::Bool = true) =
    [r.regId for r in MD.regionsVector if r.regLevel == level && (!excludeResidual || !r.isResidual)]

getAllRegions(MD::ModelData, excludeResidual::Bool = true) =
    [r for r in MD.regionsVector if !excludeResidual || !r.isResidual]

"The l2r structure: for each (non residual) level-1 region, its (non residual) level-2 children ids"
function getL2r(MD::ModelData, excludeResidual::Bool = true)
    toReturn = Vector{Vector{Int}}()
    for l1 in getRegionIds(MD, 1, excludeResidual)
        ch = [c.regId for c in getChildren(getRegion(MD, l1), excludeResidual)]
        isempty(ch) || push!(toReturn, ch)
    end
    return toReturn
end

regId2RegSName(MD::ModelData, regId::Integer) = getRegion(MD, regId).regSName

function getSiblings(MD::ModelData, reg::ModelRegion, excludeResidual::Bool = true)
    return [r for r in getAllRegions(MD, excludeResidual) if r.parRegId == reg.parRegId && r.regId != reg.regId]
end

# ------------------------------------------------------------------------------
# Forest types, io matrix, reclassification and pathogen rules
# ------------------------------------------------------------------------------

function getForTypeIds(MD::ModelData, all::Bool = false)
    c = get(MD.forTypeIdsCache, all, nothing)
    c === nothing || return c
    v = [ft.forTypeId for ft in MD.forTypes if ft.memType != 1 || all]
    MD.forTypeIdsCache[all] = v
    return v
end

function getForType(MD::ModelData, id::AbstractString)
    for ft in MD.forTypes
        ft.forTypeId == id && return ft
    end
    msgOut(MSG_CRITICAL_ERROR, "forTypeId $id not found. Aborting.")
end

function getForTypeParentId(MD::ModelData, id::AbstractString)
    for ft in MD.forTypes
        ft.forTypeId == id && return ft.ereditatedFrom
    end
    msgOut(MSG_CRITICAL_ERROR, "forTypeId $id not found. Aborting.")
end

getForTypeChilds(MD::ModelData, id::AbstractString) = [ft.forTypeId for ft in MD.forTypes if ft.ereditatedFrom == id]

"1-based positions in getForTypeIds(all) of the children of a forest type"
function getForTypeChilds_pos(MD::ModelData, id::AbstractString, all::Bool = false)
    ids = getForTypeIds(MD, all)
    return [i for i in eachindex(ids) if getForType(MD, ids[i]).ereditatedFrom == id]
end

function getForTypeParents(MD::ModelData)
    parents = String[]
    for ft in MD.forTypes
        p = ft.ereditatedFrom
        (!(p in parents) && p != "") && push!(parents, p)
    end
    return parents
end

getNForTypesChilds(MD::ModelData, id::AbstractString) = count(ft -> ft.ereditatedFrom == id, MD.forTypes)

function setForestTypes!(m, MD::ModelData)
    table = getTable(MD, "forTypes")
    for i in 1:nrecords(table)
        ft = ForType(getData(table, i, "forTypeId"), getData(table, i, "forLabel"), s2i(getData(table, i, "memType")),
            getData(table, i, "forLayer"), getData(table, i, "ereditatedFrom"))
        (ft.memType == 3 && !getBoolSetting(m, "useSpExplicitForestTypes")) && continue
        push!(MD.forTypes, ft)
    end
    empty!(MD.forTypeIdsCache)
end

function setReclassificationRules!(m, MD::ModelData)
    msgOut(MSG_DEBUG, "Loading (but not yet applying) reclassification rules..")
    table = getTable(MD, "reclRules")
    for i in 1:nrecords(table)
        push!(MD.reclRules, ReclRule(s2i(getData(table, i, "regID")), getData(table, i, "forTypeIn"),
            getData(table, i, "forTypeOut"), s2d(getData(table, i, "coeff"))))
    end
end

function _readPathRule(table::LLData, i::Integer)
    values = Float64[]
    for z in 0:(nheaders(table) - 5)
        v = getData(table, i, "year_$z")
        v != "" && push!(values, s2d(v))
    end
    return PathRule(getData(table, i, "forType"), getData(table, i, "dClass"), getData(table, i, "path_name"),
        s2d(getData(table, i, "pres_min")), values)
end

function setDefaultPathogenRules!(m, MD::ModelData)
    getBoolSetting(m, "usePathogenModule") || return
    msgOut(MSG_DEBUG, "Loading pathogen rules..")
    table = getTable(MD, "pathRules")
    for i in 1:nrecords(table)
        push!(MD.pathRules, _readPathRule(table, i))
    end
end

function setScenarioPathogenRules!(m, MD::ModelData)
    MD.scenario.pathTable == "" && return
    table = getTable(MD, MD.scenario.pathTable)
    for i in 1:nrecords(table)
        PR = _readPathRule(table, i)
        found = false
        for r in MD.pathRules
            if r.forType == PR.forType && r.dClass == PR.dClass && r.pathId == PR.pathId
                r.pres_min = PR.pres_min
                r.mortCoefficents = PR.mortCoefficents
                found = true
                break
            end
        end
        found || push!(MD.pathRules, PR)
    end
end

getPathMortalityRule(MD::ModelData, ft::AbstractString, dc::AbstractString) =
    [r for r in MD.pathRules if r.forType == ft && r.dClass == dc]

function _setF2P!(MD::ModelData, table::LLData)
    empty!(MD.forToProdVector)
    empty!(MD.forToProdIndex)
    for i in 1:nrecords(table)
        f = ForToProd(getData(table, i, "product"), getData(table, i, "forType"), getData(table, i, "dClass"),
            s2i(getData(table, i, "maxYears")))
        push!(MD.forToProdVector, f)
        k = (f.product, f.forType, f.dClass)
        haskey(MD.forToProdIndex, k) || (MD.forToProdIndex[k] = length(MD.forToProdVector))
    end
end

function setDefaultProductResourceMatrixLink!(m, MD::ModelData)
    msgOut(MSG_DEBUG, "Loading forest resource to primary products io matrix..")
    _setF2P!(MD, getTable(MD, "forToProd"))
end

function setScenarioProductResourceMatrixLink!(m, MD::ModelData)
    MD.scenario.forToProdTable == "" && return
    _setF2P!(MD, getTable(MD, MD.scenario.forToProdTable))
end

assessProdPossibility(MD::ModelData, prod::AbstractString, ft::AbstractString, dc::AbstractString) =
    haskey(MD.forToProdIndex, (prod, ft, dc))

function getMaxYearUsableDeathTimber(MD::ModelData)
    maxMaxYears = 0
    for f in MD.forToProdVector
        f.maxYears > maxMaxYears && (maxMaxYears = f.maxYears)
    end
    return maxMaxYears
end

function getMaxYearUsableDeathTimber(MD::ModelData, prod::AbstractString, ft::AbstractString, dc::AbstractString)
    i = get(MD.forToProdIndex, (prod, ft, dc), 0)
    i == 0 && msgOut(MSG_CRITICAL_ERROR, "In getMaxYearUsableDeathTimber() I has been asked of a combination that I don't know how to handle.")
    return MD.forToProdVector[i].maxYears
end

# ------------------------------------------------------------------------------
# Forest and product data
# ------------------------------------------------------------------------------

makeKeyProdData(parName, regId, prod, freeDim = "") = string(parName, '#', regId, '#', prod, '#', freeDim, '#')
makeKeyForData(parName, regId, forType, diamClass) = string(parName, '#', regId, '#', forType, '#', diamClass, '#')

function _readDataValues(table::LLData, i::Integer)
    values = Float64[]
    for z in 0:(nheaders(table) - 5)
        v = getData(table, i, "value_$z")
        v != "" && push!(values, s2d(v))
    end
    return values
end

function setDefaultForData!(m, MD::ModelData)
    msgOut(MSG_DEBUG, "Loading forest sector data..")
    table = getTable(MD, "forData")
    for i in 1:nrecords(table)
        values = _readDataValues(table, i)
        key = makeKeyForData(getData(table, i, "parName"), getData(table, i, "region"), getData(table, i, "forType"), getData(table, i, "freeDim"))
        mapinsert!(MD.forDataMap, key, values)
    end
end

function setScenarioForData!(m, MD::ModelData)
    MD.scenario.forDataTable == "" && return
    table = getTable(MD, MD.scenario.forDataTable)
    for i in 1:nrecords(table)
        key = makeKeyForData(getData(table, i, "parName"), getData(table, i, "region"), getData(table, i, "forType"), getData(table, i, "freeDim"))
        values = _readDataValues(table, i)
        haskey(MD.forDataMap, key) ? mapreplace!(MD.forDataMap, key, values) : mapinsert!(MD.forDataMap, key, values)
    end
end

function setDefaultProdData!(m, MD::ModelData)
    msgOut(MSG_DEBUG, "Loading products data..")
    table = getTable(MD, "prodData")
    for i in 1:nrecords(table)
        values = _readDataValues(table, i)
        key = makeKeyProdData(getData(table, i, "parName"), getData(table, i, "region"), getData(table, i, "prod"), getData(table, i, "freeDim"))
        mapinsert!(MD.prodDataMap, key, values)
    end
end

function setScenarioProdData!(m, MD::ModelData)
    MD.scenario.prodDataTable == "" && return
    table = getTable(MD, MD.scenario.prodDataTable)
    for i in 1:nrecords(table)
        key = makeKeyProdData(getData(table, i, "parName"), getData(table, i, "region"), getData(table, i, "prod"), getData(table, i, "freeDim"))
        values = _readDataValues(table, i)
        haskey(MD.prodDataMap, key) ? mapreplace!(MD.prodDataMap, key, values) : mapinsert!(MD.prodDataMap, key, values)
    end
end

function _unpackKey(key::AbstractString)
    parts = split(key, '#')
    length(parts) < 5 && msgOut(MSG_CRITICAL_ERROR, "Error in unpacking a key in the map of production data.")
    return String(parts[1]), s2i(parts[2]), String(parts[3]), String(parts[4])
end

function _overrideLevel!(MD::ModelData, dm::DataMap, level::Integer, makeKey, what::AbstractString)
    toBeAdded = Dict{String,Vector{Float64}}()
    toBeAddedOrder = String[]
    keysToRemove = String[]
    for k in copy(sortedkeys(dm))
        parName, regId, d1, d2 = _unpackKey(k)
        if !regionExist(MD, regId)
            msgOut(MSG_CRITICAL_ERROR, "$what variable has a unknown region code If you want not to model a region, set residual=true.: $parName\t$regId")
            continue
        end
        if getRegion(MD, regId).regLevel == level
            for ch in getChildren(getRegion(MD, regId), false)
                key = makeKey(parName, ch.regId, d1, d2)
                if !haskey(dm, key) && !haskey(toBeAdded, key)
                    toBeAdded[key] = dm.d[k]
                    push!(toBeAddedOrder, key)
                end
            end
            push!(keysToRemove, k)
        end
    end
    for key in toBeAddedOrder
        mapinsert!(dm, key, copy(toBeAdded[key]))
    end
    for k in keysToRemove
        mapdelete!(dm, k)
    end
end

function _overrideReclRules!(MD::ModelData, isOverridable)
    i = 1
    while i <= length(MD.reclRules)
        rr = MD.reclRules[i]
        if !regionExist(MD, rr.regId)
            msgOut(MSG_CRITICAL_ERROR, "Reclassification rule has a unknown region code. If you want not to model a region, set residual=true.: $(rr.regId)")
            i += 1
            continue
        end
        if isOverridable(rr)
            reg = getRegion(MD, rr.regId)
            for ch in getChildren(reg, false)
                found = any(z -> z.regId == ch.regId && z.forTypeIn == rr.forTypeIn && z.forTypeOut == rr.forTypeOut, MD.reclRules)
                found || push!(MD.reclRules, ReclRule(ch.regId, rr.forTypeIn, rr.forTypeOut, rr.coeff))
            end
            deleteat!(MD.reclRules, i)
        else
            i += 1
        end
    end
end

"Cancel all reg0/reg1 level data and transform them in reg2 level data if not already existing"
function applyOverrides!(m, MD::ModelData)
    getBoolSetting(m, "applyOverriding") || return
    msgOut(MSG_INFO, "Starting regional overriding analysis..")
    _overrideLevel!(MD, MD.forDataMap, 0, makeKeyForData, "Forest")
    _overrideLevel!(MD, MD.forDataMap, 1, makeKeyForData, "Forest")
    _overrideLevel!(MD, MD.prodDataMap, 0, makeKeyProdData, "Product")
    _overrideLevel!(MD, MD.prodDataMap, 1, makeKeyProdData, "Product")
    _overrideReclRules!(MD, rr -> rr.regId == 0)
    _overrideReclRules!(MD, rr -> getRegion(MD, rr.regId).regLevel == 1)
end

function applyDebugMode!(m, MD::ModelData)
    getBoolSetting(m, "debugFlag") || return
    debugRegions = getIntVectorSetting(m, "debugRegions")
    debugPriProducts = getStringVectorSetting(m, "debugPriProducts")
    debugSecProducts = getStringVectorSetting(m, "debugSecProducts")
    for r in MD.regionsVector
        if r.regLevel == 2 && !(r.regId in debugRegions)
            r.isResidual = true
        end
    end
    for s in MD.settings
        if s.name == "priProducts"
            s.values = copy(debugPriProducts)
        elseif s.name == "secProducts"
            s.values = copy(debugSecProducts)
        end
    end
end

# --- timed data ---

function getTimedData(m, v::Vector{Float64}, year_h::Integer)
    position = (year_h == DATA_NOW ? m.SCD.year : year_h) - m.MD.cached_initialYear
    if position >= 0 && length(v) > position
        return @inbounds v[position+1]
    elseif length(v) > 0
        return @inbounds v[end]       # also for negative positions, as in the C++ unsigned comparison
    end
    msgOut(MSG_CRITICAL_ERROR, "Error in getTimedData: requested value doesn't have any value, even on the first position(year)!")
    return 0.0
end

function setTimedData!(m, value::Float64, v::Vector{Float64}, year_h::Integer, MSG_LEVEL::Integer = MSG_WARNING)
    position = (year_h == DATA_NOW ? m.SCD.year : year_h) - m.MD.cached_initialYear
    position < 0 && msgOut(MSG_CRITICAL_ERROR, "setTimedData: negative position in time.")
    orig = length(v)
    if orig > position
        v[position+1] = value
    else
        for _ in 1:(position - orig + 1)
            push!(v, value)
        end
        if position - orig > 0
            msgOut(MSG_LEVEL, "setTimedData: a dated vector has been filled several years ($(1 + position - orig)) with incoming values to reach desidered position in time.")
        end
    end
end

"Level-2 regions for a data request on a level 1 or 2 region"
function _dataRegIds(MD::ModelData, regId_h::Integer, fname)::Vector{Int}
    reg = getRegion(MD, regId_h)
    if reg.regLevel == 2
        return Int[regId_h]
    elseif reg.regLevel == 1
        return Int[c.regId for c in getChildren(reg)]
    end
    msgOut(MSG_CRITICAL_ERROR, "Error in $fname. Setting a value for the whole World is not supported.")
    return Int[]
end

function _productsFor(MD::ModelData, prodId_h::AbstractString, withEmpty::Bool)::Vector{String}
    prodId_h == PROD_PRI && return MD.priProducts
    prodId_h == PROD_SEC && return MD.secProducts
    if prodId_h == PROD_ALL || (withEmpty && prodId_h == "")
        return withEmpty ? vcat(MD.allProducts, [""]) : MD.allProducts
    end
    return String[prodId_h]
end

function _dClassesFor(MD::ModelData, freeDim_h::AbstractString, withEmpty::Bool)::Vector{String}
    freeDim_h == DIAM_ALL && return withEmpty ? vcat(MD.diamClasses, [""]) : MD.diamClasses
    freeDim_h == DIAM_PROD && return MD.diamClasses[2:end]
    freeDim_h == DIAM_FIRST && return [MD.diamClasses[1]]
    return String[freeDim_h]
end

_isProdKeyword(p) = p == PROD_PRI || p == PROD_SEC || p == PROD_ALL || p == ""
_isDiamKeyword(d) = d == DIAM_ALL || d == DIAM_PROD || d == DIAM_FIRST

function getProdData(m, type_h::AbstractString, regId_h::Integer, prodId_h::AbstractString, year::Integer = DATA_NOW, freeDim_h::AbstractString = "")
    MD = m.MD
    exactMatch = freeDim_h != ""
    value = 0.0
    found = false
    if !_isProdKeyword(prodId_h) && getRegion(MD, regId_h).regLevel == 2
        # fast path: a single product in a single region
        if exactMatch
            v = exactValues(MD.prodDataMap, type_h, regId_h, prodId_h, freeDim_h)
            MD.tempBool = v !== nothing
            if v !== nothing
                value += getTimedData(m, v, year)
                found = true
            end
        else
            vs = prefixValues(MD.prodDataMap, type_h, regId_h, prodId_h)
            MD.tempBool = !isempty(vs)
            for v in vs
                value += getTimedData(m, v, year)
            end
            found = !isempty(vs)
        end
    else
        regIds = _dataRegIds(MD, regId_h, "setProdData()")
        products = _productsFor(MD, prodId_h, true)
        for r in regIds, p in products
            if exactMatch
                v = exactValues(MD.prodDataMap, type_h, r, p, freeDim_h)
                MD.tempBool = v !== nothing
                if v !== nothing
                    value += getTimedData(m, v, year)
                    found = true
                end
            else
                # the last '#' of the full key is removed (bug 20140402): prefix query on type#reg#prod#
                vs = prefixValues(MD.prodDataMap, type_h, r, p)
                MD.tempBool = !isempty(vs)
                for v in vs
                    value += getTimedData(m, v, year)
                end
                isempty(vs) || (found = true)
            end
        end
    end
    if !found
        MD.valueFoundBool = false
        msgOut(MD.errorLevel, "Error in getProdData: no combination found for $type_h, $regId_h, $prodId_h, $year, $freeDim_h. Returning 0, but double check that this is ok for your model.")
    end
    return value
end

function getForData(m, type_h::AbstractString, regId_h::Integer, forType_h::AbstractString, freeDim_h::AbstractString, year::Integer = DATA_NOW)
    MD = m.MD
    value = 0.0
    found = false
    if forType_h != FT_ALL && !_isDiamKeyword(freeDim_h) && getRegion(MD, regId_h).regLevel == 2
        # fast path: a single key
        v = exactValues(MD.forDataMap, type_h, regId_h, forType_h, freeDim_h)
        MD.tempBool = v !== nothing
        if v !== nothing
            value += getTimedData(m, v, year)
            found = true
        end
    else
        fTypes = forType_h == FT_ALL ? vcat(getForTypeIds(MD), [""]) : String[forType_h]
        dClasses = _dClassesFor(MD, freeDim_h, true)
        regIds = _dataRegIds(MD, regId_h, "getProdData()")
        for r in regIds, dc in dClasses, ft in fTypes
            v = exactValues(MD.forDataMap, type_h, r, ft, dc)
            MD.tempBool = v !== nothing
            if v !== nothing
                value += getTimedData(m, v, year)
                found = true
            end
        end
    end
    if !found
        MD.valueFoundBool = false
        msgOut(MD.errorLevel, "Error in getForData(): no combination found for $type_h, $regId_h, $forType_h, $year, $freeDim_h. Returning 0, but double check that this is ok for your model.")
    end
    return value
end

function setProdData!(m, value::Real, type_h::AbstractString, regId_h::Integer, prodId_h::AbstractString,
                      year::Integer = DATA_NOW, allowCreate::Bool = false, freeDim_h::AbstractString = "")
    MD = m.MD
    value = Float64(value)
    products = _productsFor(MD, prodId_h, false)
    regIds = _dataRegIds(MD, regId_h, "setProdData()")
    found = false
    for r in regIds, p in products
        v = exactValues(MD.prodDataMap, type_h, r, p, freeDim_h)
        if v !== nothing
            found = true
            setTimedData!(m, value, v, year, MD.errorLevel)
        end
    end
    if !found
        if !allowCreate
            msgOut(MSG_CRITICAL_ERROR, "Error in setProdData: no combination found for $type_h, $regId_h, $prodId_h, $year, $freeDim_h. You can allow new variables to be created using the \"allowCreate\" flag.")
        else
            for r in regIds, p in products
                values = Float64[]
                setTimedData!(m, value, values, year, MSG_NO_MSG)
                mapinsert!(MD.prodDataMap, makeKeyProdData(type_h, r, p, freeDim_h), values)
            end
        end
    end
end

function setForData!(m, value::Real, type_h::AbstractString, regId_h::Integer, forType_h::AbstractString, freeDim_h::AbstractString,
                     year::Integer = DATA_NOW, allowCreate::Bool = false)
    MD = m.MD
    value = Float64(value)
    fTypes = forType_h == FT_ALL ? getForTypeIds(MD) : String[forType_h]
    dClasses = _dClassesFor(MD, freeDim_h, false)
    regIds = _dataRegIds(MD, regId_h, "setProdData()")
    found = false
    for r in regIds, dc in dClasses, ft in fTypes
        v = exactValues(MD.forDataMap, type_h, r, ft, dc)
        if v !== nothing
            found = true
            setTimedData!(m, value, v, year, MD.errorLevel)
        end
    end
    if !found
        if !allowCreate
            msgOut(MSG_CRITICAL_ERROR, "Error in setForData: no combination found for $type_h, $regId_h, $forType_h, $year, $freeDim_h. You can allow new variables to be created using the \"allowCreate\" flag.")
        else
            for r in regIds, dc in dClasses, ft in fTypes
                values = Float64[]
                setTimedData!(m, value, values, year, MSG_NO_MSG)
                mapinsert!(MD.forDataMap, makeKeyForData(type_h, r, ft, dc), values)
            end
        end
    end
end

# ------------------------------------------------------------------------------
# Misc
# ------------------------------------------------------------------------------

function calculateAnnualisedEquivalent(amount::Float64, years::Integer, ir::Float64)
    years < 0 && return 0.0
    years == 0 && return amount
    return amount * ir / (cpow(1.0 + ir, years) - 1.0)
end
calculateAnnualisedEquivalent(amount::Float64, years::Float64, ir::Float64) =
    calculateAnnualisedEquivalent(amount, unsafe_trunc(Int, ceil(years)), ir)

"Files in a directory whose extension matches `filter` (ModelData::getFilenamesByDir)"
function getFilenamesByDir(dir::AbstractString, filter::AbstractString = "")
    if !isdir(dir)
        msgOut(MSG_ERROR, "Error 2 opening the $dir directory.")
        return String[]
    end
    files = String[]
    for f in readdir(dir)
        i = findlast('.', f)
        ext = i === nothing ? "" : f[i:end]
        if (filter != "" && ext == filter) || (filter == "" && i !== nothing && length(f) > i)
            push!(files, f)
        end
    end
    return files
end

# --- death timber ---

function getAvailableDeathTimber(m, primProd::AbstractVector{<:AbstractString}, regId::Integer, year::Integer)
    MD = m.MD
    getBoolSetting(m, "useDeathTimber", DATA_NOW) || return 0.0
    toReturn = 0.0
    for ft in getForTypeIds(MD), dc in MD.diamClasses
        possible = false
        maxYears = 0
        for pp in primProd
            if assessProdPossibility(MD, pp, ft, dc)
                possible = true
                maxYears = max(maxYears, getMaxYearUsableDeathTimber(MD, pp, ft, dc))
            end
        end
        if possible
            for y in year:-1:(year - maxYears + 1)
                toReturn += get(MD.deathTimberInventory, (y, regId, ft, dc), 0.0)
            end
        end
    end
    return toReturn
end

"1-based ids of the primary products obtainable from the death timber of a given ft/dc/year"
function getAllocableProductIdsFromDeathTimber(m, regId::Integer, ft::AbstractString, dc::AbstractString, harvesting_year::Integer, request_year::Integer = DATA_NOW)
    MD = m.MD
    ids = Int[]
    getBoolSetting(m, "useDeathTimber", DATA_NOW) || return ids
    request_year == DATA_NOW && (request_year = m.SCD.year)
    for (p, pp) in enumerate(MD.priProducts)
        if assessProdPossibility(MD, pp, ft, dc)
            if request_year - harvesting_year < getMaxYearUsableDeathTimber(MD, pp, ft, dc)
                push!(ids, p)
            end
        end
    end
    return ids
end

function deathTimberInventory_incrOrAdd!(MD::ModelData, key::Tuple{Int,Int,String,String}, value::Float64)
    MD.deathTimberInventory[key] = get(MD.deathTimberInventory, key, 0.0) + value
end

function deathTimberInventory_get(MD::ModelData, key::Tuple{Int,Int,String,String})
    v = get(MD.deathTimberInventory, key, nothing)
    v === nothing && (msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)"); return 0.0)
    return v
end
