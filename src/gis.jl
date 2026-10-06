# Port of Gis, Layers and Pixel.
#
# In the C++ code every pixel owns a std::map<string,double> with the value of each layer. Here the
# values are stored by layer (one vector over all pixels); the sorted list of all the layer keys is
# kept to replicate the upper_bound based look-ups of Pixel::getSTData().

mutable struct LegendItem
    ID::Int
    label::String
    rColor::Int
    gColor::Int
    bColor::Int
    minValue::Float64
    maxValue::Float64
end

mutable struct Layer
    name::String
    label::String
    isInteger::Bool
    dynamicContent::Bool
    fullFileName::String
    display::Bool
    legendItems::Vector{LegendItem}
end

const VV = Vector{Vector{Float64}}

mutable struct Pixel
    ID::Int
    pxX::Int
    pxY::Int
    spMods::Vector{Float64}
    avalCoef::Vector{Float64}
    l2region::Union{Nothing,ModelRegion}
    vol::VV
    area::VV
    deltaArea::VV
    initialDc0Area::Vector{Float64}
    hArea::VV
    hVol::VV
    hProductivity::VV
    hVol_byPrd::Vector{VV}
    regArea::Dict{Int,Vector{Float64}}
    vReg::Vector{Float64}
    vMort::VV
    vMortAdd::VV
    expectedReturns::Vector{Float64}
    optDc::Vector{Int}
    optFtChosen::Vector{Int}
    optDcChosen::Vector{Int}
    expectedAnnualIncome_carbon::Vector{Float64}
    expectedAnnualIncome_timber::Vector{Float64}
    expectedReturnsNotCorrByRa::Vector{Float64}
    vol_l::VV
    area_l::VV
    beta::VV
    mort::VV
    addMort::VV
    tp::VV
    cumTp::VV
    vHa::VV
    cumAlive::VV
    cumTp_exp::VV
    vHa_exp::VV
    cumAlive_exp::VV
    portfolioVarRa::Float64
    expType::Float64
    expTypePrices::Float64
    usePortfolio::Bool
end

function Pixel(ID::Integer, nft::Integer)
    return Pixel(ID, 0, 0, ones(nft), ones(nft), nothing, VV(), VV(), VV(), Float64[], VV(), VV(), VV(), Vector{VV}(),
        Dict{Int,Vector{Float64}}(), Float64[], VV(), VV(), Float64[], Int[], Int[], Int[], Float64[], Float64[], Float64[],
        VV(), VV(), VV(), VV(), VV(), VV(), VV(), VV(), VV(), VV(), VV(), VV(), 0.0, 0.0, 0.0, false)
end

mutable struct Gis
    pxVector::Vector{Pixel}
    layerVector::Vector{Layer}
    layerIndex::Dict{String,Int}
    values::Dict{String,Vector{Float64}}      # pixel values by key (layer name)
    present::Dict{String,BitVector}           # for keys set only on some pixels (e.g. "ra")
    sortedKeys::Vector{String}
    keysSorted::Bool
    stCache::Dict{String,Union{Nothing,String}}
    stQueryCache::Dict{Tuple{String,String,String,Int},Union{Nothing,String}}
    xNPixels::Int
    yNPixels::Int
    xyNPixels::Int
    xMetersByPixel::Float64
    yMetersByPixel::Float64
    geoLeftX::Float64
    geoTopY::Float64
    geoRightX::Float64
    geoBottomY::Float64
    noValue::Float64
    subXL::Int
    subXR::Int
    subYT::Int
    subYB::Int
end

Gis() = Gis(Pixel[], Layer[], Dict{String,Int}(), Dict{String,Vector{Float64}}(), Dict{String,BitVector}(), String[], true,
    Dict{String,Union{Nothing,String}}(), Dict{Tuple{String,String,String,Int},Union{Nothing,String}}(), 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -9999.0, 0, 0, 0, 0)

getPixel(G::Gis, x::Integer, y::Integer) = G.pxVector[x+y*G.xNPixels+1]
getHaByPixel(G::Gis) = (G.xMetersByPixel * G.yMetersByPixel) / 10000

# --- pixel values ---

function _addKey!(G::Gis, key::String, v::Vector{Float64})
    G.values[key] = v
    push!(G.sortedKeys, key)
    G.keysSorted = false
    empty!(G.stCache)
    empty!(G.stQueryCache)
end

"Pixel::setValue (std::map insert: no change if the key already exists for that pixel)"
function setValue!(G::Gis, px::Pixel, key::String, value::Float64)
    v = get(G.values, key, nothing)
    if v === nothing
        v = fill(G.noValue, G.xyNPixels)
        _addKey!(G, key, v)
        G.present[key] = falses(G.xyNPixels)
    end
    pr = get(G.present, key, nothing)
    if pr === nothing
        return            # a full layer: all pixels already have the key
    end
    if !pr[px.ID+1]
        pr[px.ID+1] = true
        v[px.ID+1] = value
    end
end

_hasKey(G::Gis, px::Pixel, key::String) =
    haskey(G.values, key) && (pr = get(G.present, key, nothing); pr === nothing || pr[px.ID+1])

function getDoubleValue(G::Gis, px::Pixel, key::AbstractString, returnZeroForNoValue::Bool = false)
    if _hasKey(G, px, String(key))
        v = G.values[key][px.ID+1]
        return (returnZeroForNoValue && v == G.noValue) ? 0.0 : v
    end
    msgOut(MSG_WARNING, "No layer \"$key\" found on pixel ($(px.pxX),$(px.pxY)). Sure you didn't mispelled it?")
    return returnZeroForNoValue ? 0.0 : G.noValue
end

function changeValue!(G::Gis, px::Pixel, key::AbstractString, value::Real, setNoValueForZero::Bool = false)
    if _hasKey(G, px, String(key))
        G.values[key][px.ID+1] = (setNoValueForZero && value == 0) ? G.noValue : Float64(value)
    else
        msgOut(MSG_ERROR, "Coud not change pixel value for layer $key. Layer don't found.")
    end
end

function _sortedValueKeys(G::Gis)
    if !G.keysSorted
        sort!(G.sortedKeys)
        G.keysSorted = true
    end
    return G.sortedKeys
end

"""
Replicates `map::upper_bound(search); if(i!=begin) i--;` followed by the prefix check on the search
string without the year: returns the matching key or `nothing`.
"""
function _stResolve(G::Gis, search::String)
    c = get(G.stCache, search, missing)
    c === missing || return c
    ks = _sortedValueKeys(G)
    i = searchsortedlast(ks, search)   # last key <= search (0: none, i.e. no layer of this series up to this year)
    base = search[1:end-4]
    res = (i > 0 && startswith(ks[i], base)) ? ks[i] : nothing
    G.stCache[search] = res
    return res
end

"Pixel::getSTData() (spatio-temporal layer data: parName#fType#d2#year)"
function getSTData(m, px::Pixel, parName::AbstractString, forName::AbstractString, year::Integer, d2::AbstractString = "", naToReturn::Real = RETNA)
    G = m.GIS
    defaultNoValue = m.cache.noValue
    naToReturn == RETNA && (naToReturn = defaultNoValue)
    m.cache.usePixelData || return Float64(naToReturn)
    year == DATA_NOW && (year = m.SCD.year)
    qk = (String(parName), String(forName), String(d2), Int(year))
    key = get(G.stQueryCache, qk, missing)
    if key === missing
        key = nothing
        ys = i2s(year)
        for (f, d) in ((forName, d2), ("", d2), (forName, ""), ("", ""))
            key = _stResolve(G, string(parName, '#', f, '#', d, '#', ys))
            key === nothing || break
        end
        G.stQueryCache[qk] = key
    end
    key === nothing && return Float64(naToReturn)
    v = _hasKey(G, px, key) ? G.values[key][px.ID+1] : G.noValue
    return v == defaultNoValue ? Float64(naToReturn) : v
end

function getSpModifier(m, px::Pixel, ft::AbstractString)
    ftypes = getForTypeIds(m.MD)
    for i in eachindex(ftypes)
        ftypes[i] == ft && return px.spMods[i]
    end
    msgOut(MSG_CRITICAL_ERROR, "Asked spatial modifier for a forest type that doesn't exist")
end

"Pixel::getMultiplier()"
function getMultiplier(m, px::Pixel, multiplierName::AbstractString, forName::AbstractString, year::Integer = DATA_NOW)
    year == DATA_NOW && (year = m.SCD.year)
    multiplierSpVar = multiplierName == "tp_multiplier" ? getSpModifier(m, px, forName) : 1.0
    if multiplierName in m.cache.modifiersFromTable
        regId = px.l2region.regId
        multiplier = getForData(m, multiplierName, regId, forName, "", year)
        if multiplierName == "mortCoef_multiplier"
            return multiplier^getDoubleSetting(m, "mortMultiplier_link", DATA_NOW) * multiplierSpVar
        end
        return multiplier * multiplierSpVar
    else
        return getSTData(m, px, multiplierName, forName, year, "", 1.0)
    end
end

"Pixel::getPathMortality(): increased mortality due to the pathogens present in the pixel"
function getPathMortality(m, px::Pixel, forType::AbstractString, dC::AbstractString, year::Integer = DATA_NOW)
    m.cache.usePathogenModule || return 0.0
    initialOptYear = getIntSetting(m, "initialOptYear")
    simulationYears = getIntSetting(m, "simulationYears")
    maxYear = initialOptYear + simulationYears
    rules = getPathMortalityRule(m.MD, forType, dC)
    pathMort = 0.0
    year == DATA_NOW && (year = m.SCD.year)
    for r in rules
        pathMort_thispath = 0.0
        nc = length(r.mortCoefficents)
        for y in year:-1:(year - nc + 1)
            i = year - y
            y2 = y >= maxYear ? maxYear - 1 : y
            layerName = "pathogen_pp#$(r.pathId)#$(y2)"
            if layerExist(m.GIS, layerName)
                if getDoubleValue(m.GIS, px, layerName, true) >= r.pres_min
                    pathMort_thispath = r.mortCoefficents[i+1]
                end
            end
        end
        pathMort += pathMort_thispath
    end
    return pathMort
end

function getPastRegArea(px::Pixel, ft_idx::Integer, year::Integer)
    v = get(px.regArea, year, nothing)
    if v === nothing
        msgOut(MSG_ERROR, "Asking for a pastRegArea of a not-registered year. I don't have year $year!")
        return 0.0
    end
    return v[ft_idx]
end

# --- layers ---

layerExist(G::Gis, name::AbstractString) = haskey(G.layerIndex, name)

function getLayer(G::Gis, name::AbstractString)
    i = get(G.layerIndex, name, 0)
    i == 0 && msgOut(MSG_CRITICAL_ERROR, "Layer $name not found. Aborting.")
    return G.layerVector[i]
end

function addLayer!(G::Gis, name::AbstractString, label::AbstractString, isInteger::Bool, dynamicContent::Bool, fullFileName::AbstractString = "", display::Bool = true)
    if layerExist(G, name)
        msgOut(MSG_ERROR, "Layer already exist with that name")
        return
    end
    push!(G.layerVector, Layer(String(name), String(label), isInteger, dynamicContent, String(fullFileName), display, LegendItem[]))
    G.layerIndex[String(name)] = length(G.layerVector)
    # every pixel gets the noValue (map insert: existing keys are not changed)
    v = get(G.values, name, nothing)
    if v === nothing
        _addKey!(G, String(name), fill(G.noValue, G.xyNPixels))
    else
        pr = get(G.present, name, nothing)
        if pr !== nothing
            for i in 1:G.xyNPixels
                pr[i] || (v[i] = G.noValue)
            end
            delete!(G.present, name)
        end
    end
end

function resetLayer!(G::Gis, name::AbstractString)
    if layerExist(G, name)
        fill!(G.values[name], G.noValue)
        return
    end
    msgOut(MSG_ERROR, "I could not reset layer $name as it doesn't exist!")
end

function addLegendItem!(G::Gis, name::AbstractString, ID::Integer, label::AbstractString, r::Integer, g::Integer, b::Integer, minV::Real, maxV::Real)
    if !layerExist(G, name)
        msgOut(MSG_ERROR, "Trying to add a legend item to a layer that doesn't exist.")
        return
    end
    L = getLayer(G, name)
    for li in L.legendItems
        if li.ID == ID
            msgOut(MSG_ERROR, "Trying to add a legend item that already exist on this layer (layer: $(L.label) - legend label: $label)")
            return
        end
    end
    push!(L.legendItems, LegendItem(ID, String(label), r, g, b, Float64(minV), Float64(maxV)))
end

function addLegendItems!(L::Layer, items::Vector{LegendItem})
    toAdd = LegendItem[]
    for it in items
        if any(li -> li.ID == it.ID, L.legendItems)
            msgOut(MSG_WARNING, "Legend item $(it.ID) non added on layer $(L.name) as already existing.")
        else
            push!(toAdd, it)
        end
    end
    append!(L.legendItems, toAdd)
end

function getColor(G::Gis, L::Layer, v::Float64)
    nocolor = (255, 255, 255)
    v == G.noValue && return nocolor
    if L.isInteger
        for li in L.legendItems
            li.ID == unsafe_trunc(Int, v) && return (li.rColor, li.gColor, li.bColor)
        end
    else
        for li in L.legendItems
            (v < li.maxValue && v >= li.minValue) && return (li.rColor, li.gColor, li.bColor)
        end
    end
    return nocolor
end

# --- space initialisation ---

_xmlText(el, name) = (n = findfirst(".//*[local-name()='$name']", el); n === nothing ? "" : nodecontent(n))
function _xmlBool(el, name)
    c = _xmlText(el, name)
    (c == "false" || c == "falso" || c == "FALSE" || c == "0") && return false
    (c == "true" || c == "vero" || c == "TRUE" || c == "1") && return true
    msgOut(MSG_WARNING, "Sorry, I don't know how to convert $c to a bool value. I return true... hope for the best")
    return true
end

function initLayers!(m)
    G = m.GIS
    MD = m.MD
    filename = m.baseDirectory * getStringSetting(m, "gisFilename")
    isfile(filename) || msgOut(MSG_CRITICAL_ERROR, "Error opening the gis file $filename.")
    doc = readxml(filename)
    ftIds = getForTypeIds(MD)
    for layerNode in findall("//*[local-name()='layer']", root(doc))
        nameOrig = _xmlText(layerNode, "name")
        labelOrig = _xmlText(layerNode, "label")
        isInteger = _xmlBool(layerNode, "isInteger")
        dynamicContent = _xmlBool(layerNode, "dynamicContent")
        expandByFt = _xmlBool(layerNode, "expandByFt")
        _xmlText(layerNode, "readAtStart") == "true" || continue
        dirName = _xmlText(layerNode, "dirName")
        fileName = _xmlText(layerNode, "fileName")
        nLoop = expandByFt ? length(ftIds) : 1
        for z in 1:nLoop
            ftExt = expandByFt ? "_" * ftIds[z] : ""
            labelExt = expandByFt ? " (" * ftIds[z] * ")" : ""
            name = nameOrig * ftExt
            label = labelOrig * labelExt
            fullFileName = (dirName == "" || fileName == "") ? "" : MD.baseDirectory * dirName * fileName * ftExt
            addLayer!(G, name, label, isInteger, dynamicContent, fullFileName)
            for li in findall(".//*[local-name()='legendItem']", layerNode)
                lID = s2i(strip(nodecontent(li)))
                at(n) = (a = _attr(li, n); a === nothing ? (msgOut(MSG_ERROR, "Element doens't have attribute $n"); "") : a)
                llabel = at("label")
                r = s2i(at("rColor")); g = s2i(at("gColor")); b = s2i(at("bColor"))
                if isInteger
                    minV = Float64(lID); maxV = Float64(lID)
                else
                    minV = s2d(at("minValue")); maxV = s2d(at("maxValue"))
                end
                addLegendItem!(G, name, lID, llabel, r, g, b, minV, maxV)
            end
        end
    end
    initLayersPixelData!(m)
end

function initLayersPixelData!(m)
    G = m.GIS
    MD = m.MD
    getBoolSetting(m, "usePixelData") || return
    function addDirLayers(dir, ext)
        for f in getFilenamesByDir(dir, ext)
            name = f[1:findlast('.', f)-1]
            addLayer!(G, name, name, false, false, dir * "/" * f, false)
        end
    end
    addDirLayers(MD.baseDirectory * getStringSetting(m, "spatialDataSubfolder"), getStringSetting(m, "spatialDataFileExtension"))
    if getBoolSetting(m, "useSpExplicitForestTypes")
        addDirLayers(MD.baseDirectory * getStringSetting(m, "spExplicitForTypesInputDir"), getStringSetting(m, "spExplicitForTypesFileExtension"))
    end
    if getBoolSetting(m, "usePathogenModule")
        addDirLayers(MD.baseDirectory * getStringSetting(m, "pathogenExogenousSpreadModelFolder"), getStringSetting(m, "pathogenExogenousSpreadModelFileExtension"))
    end
    addLayer!(G, "pixelIds", "Pixel IDs", true, false, "", true)
    # In the C++ code getAllPlots(false) is called here, but as the pixels are not yet assigned to the regions
    # (and because of a "masked = false" assignment in the condition) it returns an empty vector, so the
    # pixelIds layer remains filled with noValue.
    msgOut(MSG_WARNING, "We can't find any free plot.")
end

"""
Layers::randomShuffle(): randomly reassign the values of a layer among its pixels with data (the
no-value mask is kept). Used, for sensitivity analysis only, when `initialRandomShuffle` is true.

As in the C++ code it applies to every layer read from a file (not only to the integer layers, as the
setting description says), including the region layers `regLev_1`/`regLev_2`. Unlike the other random
draws of the port it does not replicate the C++ draws (`std::rand()` and `std::random_shuffle()`): it
uses Julia's `shuffle!` with a dedicated generator (fixed seed, or a random one with `newRandomSeed`).
"""
function randomShuffle!(m, L::Layer)
    G = m.GIS
    vals = G.values[L.name]
    idx = [i for i in eachindex(vals) if vals[i] != G.noValue]
    vals[idx] = shuffle!(m.shuffleRng, vals[idx])
    return nothing
end

"Read the GRASS/ArcInfo ASCII grids associated to the layers (Gis::loadLayersDataFromFile)"
function loadLayersDataFromFile!(m)
    G = m.GIS
    initialRandomShuffle = getBoolSetting(m, "initialRandomShuffle")
    localNoValue = G.noValue  # NOTE: as in the C++ code it persists across the files
    _qdouble(s) = (v = tryparse(Float64, s); v === nothing ? 0.0 : v)
    _qint(s) = (v = tryparse(Int, s); v === nothing ? 0 : v)
    for L in G.layerVector
        L.fullFileName == "" && continue
        if !isfile(L.fullFileName)
            println(stderr, "Cannot open file for reading: No such file or directory")
            msgOut(MSG_ERROR, "Cannot open map file $(L.fullFileName) for reading.")
            continue
        end
        vals = G.values[L.name]
        countRow = 0
        for line in eachline(L.fullFileName)
            fields = split(line, ' ')   # QString::split keeps the empty parts
            f0 = fields[1]
            if (f0 == "north:" && _qdouble(fields[2]) != G.geoTopY) ||
               ((f0 == "south:" || f0 == "yllcorner") && _qdouble(fields[2]) != G.geoBottomY) ||
               (f0 == "east:" && _qdouble(fields[2]) != G.geoRightX) ||
               ((f0 == "west:" || f0 == "xllcorner") && _qdouble(fields[2]) != G.geoLeftX) ||
               ((f0 == "rows:" || f0 == "nrows") && _qint(fields[2]) != G.yNPixels) ||
               ((f0 == "cols:" || f0 == "ncols") && _qint(fields[2]) != G.xNPixels)
                msgOut(MSG_ERROR, "Layer $(L.name) has different coordinates. Aborting reading.")
                break
            elseif f0 == "null:" || f0 == "NODATA_value" || f0 == "nodata_value"
                localNoValue = _qdouble(fields[2])
            elseif length(fields) > 5
                for c in 1:G.xNPixels
                    v = _qdouble(fields[c])
                    vals[countRow*G.xNPixels+c] = v == localNoValue ? G.noValue : v  # filterExogenousDataset() is the identity (no reclass rules)
                end
                countRow += 1
            end
        end
        initialRandomShuffle && randomShuffle!(m, L)
        # filterSubRegion()
        G.subXL = 0; G.subYT = 0; G.subXR = G.xNPixels - 1; G.subYB = G.yNPixels - 1
    end
end

function setMyPixels!(m, reg::ModelRegion)
    G = m.GIS
    l1 = G.values["regLev_1"]
    l2 = G.values["regLev_2"]
    for i in 1:G.xyNPixels
        if l1[i] == reg.regId || l2[i] == reg.regId
            push!(reg.myPixels, i)
            reg.regLevel == 2 && (G.pxVector[i].l2region = reg)
        end
    end
end

regPixels(m, reg::ModelRegion) = (G = m.GIS; [G.pxVector[i] for i in reg.myPixels])

function applySpatialStochasticValues!(m)
    MD = m.MD
    getBoolSetting(m, "usePixelData") || return
    fTypes = getForTypeIds(MD)
    for regId in getRegionIds(MD, 2)
        rpx = regPixels(m, getRegion(MD, regId))
        if getBoolSetting(m, "useSpatialRegionalVariance")
            for j in eachindex(fTypes)
                sStDev = getForData(m, "sStDev", regId, fTypes[j], "")
                agr = getForData(m, "agr", regId, fTypes[j], "")
                d = NormalDist(1.0, sStDev / agr)
                for px in rpx
                    c = d(m.gen)
                    px.spMods[j] = cmax(0.4, cmin(1.6, c))
                end
            end
        end
        avgExpTypes = getDoubleSetting(m, "expType", DATA_NOW, regId)
        avgExpTypesPrices = getDoubleSetting(m, "expTypePrices", DATA_NOW, regId)
        expTypes_cv = getDoubleSetting(m, "expType_cv", DATA_NOW, regId)
        expTypesPrices_cv = getDoubleSetting(m, "expTypePrices_cv", DATA_NOW, regId)
        if (avgExpTypes < 0 || avgExpTypes > 1) && avgExpTypes != -1
            msgOut(MSG_CRITICAL_ERROR, "expType parameter must be between 1 (expectations) and 0 (adaptative) or -1 (fixed).")
        end
        if avgExpTypesPrices < 0 || avgExpTypesPrices > 1
            msgOut(MSG_CRITICAL_ERROR, "vgExpTypesPrices parameter must be between 1 (expectations) and 0 (adaptative).")
        end
        exp_distr = NormalDist(avgExpTypes, expTypes_cv * avgExpTypes)
        expPrices_distr = NormalDist(avgExpTypesPrices, expTypesPrices_cv * avgExpTypesPrices)
        for px in rpx
            if avgExpTypes == -1
                px.expType = -1
            else
                px.expType = normSample(exp_distr, m.gen, 0.0, 1.0)
            end
            px.expTypePrices = normSample(expPrices_distr, m.gen, 0.0, 1.0)
        end
    end
end

function applyStochasticRiskAdversion!(m)
    MD = m.MD
    getBoolSetting(m, "usePixelData") || return
    raEnabled = getBoolSetting(m, "heterogeneousRiskAversion")
    for regId in getRegionIds(MD, 2)
        for px in regPixels(m, getRegion(MD, regId))
            if raEnabled
                raStDev = getDoubleSetting(m, "riskAversionAgentSd", DATA_NOW, regId)
                avg = getDoubleSetting(m, "riskAversionAgentAverage", DATA_NOW, regId)
                d = NormalDist(avg, raStDev)
                setValue!(m.GIS, px, "ra", d(m.gen))
            else
                setValue!(m.GIS, px, "ra", 0.0)
            end
        end
    end
end

function setSpace!(m)
    G = m.GIS
    msgOut(MSG_INFO, "Creating the space...")
    G.geoTopY = getDoubleSetting(m, "geoNorthEdge")
    G.geoBottomY = getDoubleSetting(m, "geoSouthEdge")
    G.geoLeftX = getDoubleSetting(m, "geoWestEdge")
    G.geoRightX = getDoubleSetting(m, "geoEastEdge")
    G.xNPixels = getIntSetting(m, "nCols")
    G.yNPixels = getIntSetting(m, "nRows")
    G.noValue = getDoubleSetting(m, "noValue")
    G.xyNPixels = G.xNPixels * G.yNPixels
    G.xMetersByPixel = (G.geoRightX - G.geoLeftX) / G.xNPixels
    G.yMetersByPixel = (G.geoTopY - G.geoBottomY) / G.yNPixels
    nft = length(getForTypeIds(m.MD))
    for i in 0:(G.yNPixels-1), j in 0:(G.xNPixels-1)
        px = Pixel(i * G.xNPixels + j, nft)
        px.pxX = j
        px.pxY = i
        push!(G.pxVector, px)
    end
    initLayers!(m)
    loadLayersDataFromFile!(m)
    for reg in getAllRegions(m.MD)
        setMyPixels!(m, reg)
    end
    applySpatialStochasticValues!(m)
    applyStochasticRiskAdversion!(m)
end

function updateImage(m, layerName::AbstractString)
    # no GUI: we only keep the debug message of the C++ code
    msgOut(MSG_DEBUG, "Update image $layerName...")
    layerExist(m.GIS, layerName) || msgOut(MSG_CRITICAL_ERROR, "Layer not found in Gis::updateImage()")
end

function applyForestReclassification!(m)
    G = m.GIS
    MD = m.MD
    RRs = MD.reclRules
    for FT in MD.forTypes
        if !layerExist(G, FT.forLayer)
            addLayer!(G, FT.forLayer, "Are layer for forest type " * FT.forTypeId, false, true)
            resetLayer!(G, FT.forLayer)
            newLayer = getLayer(G, FT.forLayer)
            ereditated = getLayer(G, getForType(MD, FT.ereditatedFrom).forLayer)
            addLegendItems!(newLayer, copy(ereditated.legendItems))
        end
    end
    regL2v = G.values["regLev_2"]
    for FT in MD.forTypes
        target = G.values[FT.forLayer]
        for j in 1:G.xyNPixels
            regL2 = unsafe_trunc(Int, regL2v[j])
            v0 = target[j]
            value = v0 == G.noValue ? 0.0 : v0
            for RR in RRs
                if RR.regId == regL2 && RR.forTypeOut == FT.forTypeId
                    vin = G.values[getForType(MD, RR.forTypeIn).forLayer][j]
                    value += (vin == G.noValue ? 0.0 : vin) * RR.coeff
                end
            end
            target[j] = value == 0 ? G.noValue : value
        end
        updateImage(m, FT.forLayer)
    end
end
