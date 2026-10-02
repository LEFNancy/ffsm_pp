# Port of the Output class (csv results, optimisation log, debug pixel values and maps)

mutable struct OutputState
    oLevel::Int
    d::Char
    inYear::Int
    nYears::Int
    baseDir::String
    oDir::String
    scenarioName::String
    oFileExt::String
    oHRedeable::Bool
    oSingleFile::Bool
    oYears::Vector{Int}
    mapsOYears::Vector{Int}
    outForVariables::Vector{String}
    outProdVariables::Vector{String}
    dClasses::Vector{String}
    outStepRange::Int
    forestDiamDetailedOutput::Bool
    fTypes::Vector{String}
    oForData::Bool
    oProdData::Bool
    oCarbonData::Bool
    priPr::Vector{String}
    secPr::Vector{String}
    allPr::Vector{String}
    l1regIds::Vector{Int}
    l2r::Vector{Vector{Int}}
    spMode::Bool
    logFilename::String
    debugPxValuesFilename::String
end
OutputState() = OutputState(0, ';', 0, 0, "", "", "", "", false, false, Int[], Int[], String[], String[], String[], 0, false,
    String[], false, false, false, String[], String[], String[], Int[], Vector{Int}[], false, "", "")

_timemessage() = (t = Dates.now(); string(Dates.hour(t), ":", Dates.minute(t), ":", Dates.second(t)))

function getOutputFieldDelimiter(m)
    id = getIntSetting(m, "outputFieldDelimiter")
    id == 1 && return ','
    id == 2 && return ';'
    id == 3 && return ':'
    id == 4 && return '\t'
    id == 5 && return ' '
    msgOut(MSG_ERROR, "You have specified an unknow output file field delimiter. Using \";\".")
    return ','
end

"Remove from an output file the rows of the given scenario (first field), keeping the other scenarios"
function cleanScenario(fileName::AbstractString, scenarioName::AbstractString, d::Char)
    if !isfile(fileName)
        msgOut(MSG_ERROR, "Error in opening the file $fileName for reading.")
        return
    end
    rows = String[]
    for row in eachline(fileName)
        tokens = tokenize(row, string(d))
        (isempty(tokens) || tokens[1] != scenarioName) && push!(rows, row)
    end
    open(fileName, "w") do io
        for r in rows
            print(io, r, "\n")
        end
    end
end

function commonInit!(m)
    O = m.DO
    O.oLevel = getIntSetting(m, "outputLevel")
    O.d = getOutputFieldDelimiter(m)
    O.inYear = getIntSetting(m, "initialYear")
    O.nYears = getIntSetting(m, "simulationYears")
    O.baseDir = m.MD.baseDirectory
    O.oDir = m.MD.outputDirname
    sn = getStringSetting(m, "overridenScenarioName")
    O.scenarioName = sn == "none" ? m.scenarioName : sn
    O.oFileExt = getStringSetting(m, "outputFileExtension")
    O.oHRedeable = getBoolSetting(m, "outputHumanReadable")
    O.oSingleFile = getBoolSetting(m, "outputSingleFile")
    O.oYears = getIntVectorSetting(m, "outYears")
    O.mapsOYears = getIntVectorSetting(m, "mapsOutYears")
    O.outForVariables = getStringVectorSetting(m, "outForVariables")
    O.outProdVariables = getStringVectorSetting(m, "outProdVariables")
    O.dClasses = vcat(getStringVectorSetting(m, "dClasses"), [""])
    O.outStepRange = getIntSetting(m, "outStepRange")
    O.forestDiamDetailedOutput = getBoolSetting(m, "forestDiamDetailedOutput")
    O.fTypes = getForTypeIds(m.MD)
    O.oForData = O.oLevel >= OUTVL_DETAILED && !isempty(O.outForVariables)
    O.oProdData = O.oLevel >= OUTVL_DETAILED && !isempty(O.outProdVariables)
    O.oCarbonData = O.oLevel >= OUTVL_DETAILED
    O.priPr = getStringVectorSetting(m, "priProducts")
    O.secPr = getStringVectorSetting(m, "secProducts")
    O.allPr = vcat(O.priPr, O.secPr)
    O.l1regIds = getRegionIds(m.MD, 1, true)
    O.spMode = getBoolSetting(m, "usePixelData")
    O.l2r = getL2r(m.MD, true)
end

_ofile(O::OutputState, sub, name) = O.baseDir * O.oDir * sub * name

function initOutput!(m)
    commonInit!(m)
    initOutputMaps!(m)
    initDebugPixelValues!(m)
    initOutputForestData!(m)
    initOutputProductData!(m)
    initOptimisationLog!(m)
    initCarbonBalance!(m)
end

function initOptimisationLog!(m)
    O = m.DO
    O.oLevel < OUTVL_AGGREGATED && return
    d = O.d
    O.logFilename = O.oSingleFile ? _ofile(O, "optimisationLogs/", "optimisationLogs.txt") : _ofile(O, "optimisationLogs/", O.scenarioName * ".txt")
    if isfile(O.logFilename)
        cleanScenario(O.logFilename, O.scenarioName, d)
        open(O.logFilename, "a") do io
            print(io, O.scenarioName, d, "0000", d, _timemessage(), d, d, d, "\n")
        end
    else
        open(O.logFilename, "w") do io
            print(io, "scenario", d, "year", d, "time", d, "opt flag", d, "iterations", d, "\n")
            print(io, O.scenarioName, d, "0000", d, _timemessage(), d, d, d, "\n")
        end
    end
end

function initDebugPixelValues!(m)
    O = m.DO
    (O.oLevel < OUTVL_ALL || !O.spMode) && return
    d = O.d
    O.debugPxValuesFilename = O.oSingleFile ? _ofile(O, "debugs/", "debugPxValues.csv") : _ofile(O, "debugs/", "debugPxValues_" * O.scenarioName * ".csv")
    if isfile(O.debugPxValuesFilename)
        cleanScenario(O.debugPxValuesFilename, O.scenarioName, d)
        return
    end
    open(O.debugPxValuesFilename, "w") do io
        print(io, "scenario", d, "year", d, "region", d, "pxId", d, "pxX", d, "pxY", d, "var", d)
        for ft in O.fTypes, dc in O.dClasses
            print(io, ft, "_", dc, d)
        end
        nft = length(O.fTypes)
        for f in 1:(nft+1), f2 in 1:(nft+1)
            ft1 = f == nft + 1 ? "NonFor" : O.fTypes[f]
            ft2 = f2 == nft + 1 ? "NonFor" : O.fTypes[f2]
            print(io, ft1, "_2_", ft2, d)
        end
        print(io, "\n")
    end
end

function _initResultFile!(m, name, headerFields)
    O = m.DO
    d = O.d
    if O.oSingleFile
        fn = _ofile(O, "results/", name * O.oFileExt)
        if isfile(fn)
            cleanScenario(fn, O.scenarioName, d)
            return false
        end
    else
        fn = _ofile(O, "results/", name * "_" * O.scenarioName * O.oFileExt)
    end
    open(fn, "w") do io
        for h in headerFields
            print(io, h, d)
        end
        if name != "carbonBalance" && O.oHRedeable
            for i in 0:(O.nYears-1)
                print(io, i + O.inYear, d)
            end
        else
            print(io, "year", d, "value", d)
        end
        print(io, "\n")
    end
    return true
end

function initOutputForestData!(m)
    O = m.DO
    O.oForData || return
    _initResultFile!(m, "forestData", ("scen", "parName", "country", "region", "forType", "freeDim")) || return
    getBoolSetting(m, "outDetailedHv", DATA_NOW) || return
    d = O.d
    open(_ofile(O, "results/", "detailedHV_" * O.scenarioName * O.oFileExt), "w") do io
        for h in ("scen", "parName", "country", "region", "forType", "freeDim", "prod", "year", "value")
            print(io, h, d)
        end
        print(io, "\n")
    end
end

function initOutputProductData!(m)
    m.DO.oProdData || return
    _initResultFile!(m, "productData", ("scen", "parName", "country", "region", "prod", "freeDim"))
end

function initCarbonBalance!(m)
    m.DO.oCarbonData || return
    _initResultFile!(m, "carbonBalance", ("scen", "country", "region", "balItem"))
end

function initOutputMaps!(m)
    O = m.DO
    O.oLevel < OUTVL_MAPS && return
    mapBase = O.baseDir * O.oDir * "maps/"
    open(mapBase * "scenarioNames/" * O.scenarioName, "w") do io
        print(io, O.scenarioName, "\n")
    end
    open(identity, mapBase * "integerListLayers/" * O.scenarioName, "w")
    open(identity, mapBase * "floatListLayers/" * O.scenarioName, "w")
end

"Output::print()"
function printOutput!(m, earlyPrint::Bool = false)
    O = m.DO
    cYear = m.SCD.year
    initialSimulationYear = getIntSetting(m, "initialOptYear")
    if O.outStepRange != -1 && rem(cYear - initialSimulationYear, O.outStepRange) != 0 && cYear != (initialSimulationYear + O.nYears) - 1
        println("$cYear not printed")
        return
    end
    printThisYear = any(y -> O.outStepRange == -1 && y == cYear, O.oYears)
    (O.outStepRange == -1 && !printThisYear) && return
    printMaps(m)
    m.MD.errorLevel = MSG_NO_MSG
    printForestData(m, false)
    printProductData(m, false)
    printCarbonBalance(m)
    earlyPrint || printDebugPixelValues(m)
    m.MD.errorLevel = MSG_ERROR
end

function printFinalOutput(m)
    O = m.DO
    if O.oHRedeable
        m.MD.errorLevel = MSG_NO_MSG
        printForestData(m, true)
        printProductData(m, true)
        m.MD.errorLevel = MSG_ERROR
    end
end

function printForestData(m, finalFlush::Bool)
    O = m.DO
    O.oForData || return
    (O.oHRedeable && !finalFlush) && return
    msgOut(MSG_INFO, "Printing forest data..")
    MD = m.MD
    d = O.d
    currentYear = m.SCD.year
    fn = O.oSingleFile ? _ofile(O, "results/", "forestData" * O.oFileExt) : _ofile(O, "results/", "forestData_" * O.scenarioName * O.oFileExt)
    io = IOBuffer()
    for v in O.outForVariables
        fTypes_temp = copy(O.fTypes)
        if v in ("expReturns", "sumExpReturns", "totalShareInvadedArea", "usableDeadTimber")
            push!(fTypes_temp, "")
            append!(fTypes_temp, getForTypeParents(MD))
        end
        for r1 in eachindex(O.l2r), r2 in O.l2r[r1], ft in fTypes_temp
            c1 = regId2RegSName(MD, O.l1regIds[r1])
            c2 = regId2RegSName(MD, r2)
            if O.forestDiamDetailedOutput
                for dc in O.dClasses
                    print(io, O.scenarioName, d, v, d, c1, d, c2, d, ft, d, dc, d)
                    if O.oHRedeable
                        for y in 0:(O.nYears-1)
                            print(io, fmtg(getForData(m, v, r2, ft, dc, y + O.inYear)), d)
                        end
                        print(io, "\n")
                    else
                        print(io, currentYear, d, fmtg(getForData(m, v, r2, ft, dc)), d, "\n")
                    end
                end
            else
                print(io, O.scenarioName, d, v, d, c1, d, c2, d, ft, d, d)
                if O.oHRedeable
                    for y in 0:(O.nYears-1)
                        print(io, fmtg(getForData(m, v, r2, ft, DIAM_ALL, y + O.inYear)), d)
                    end
                    print(io, "\n")
                else
                    print(io, currentYear, d, fmtg(getForData(m, v, r2, ft, DIAM_ALL)), d, "\n")
                end
            end
        end
    end
    open(f -> write(f, take!(io)), fn, "a")
end

function printProductData(m, finalFlush::Bool)
    O = m.DO
    O.oProdData || return
    (O.oHRedeable && !finalFlush) && return
    msgOut(MSG_INFO, "Printing market data..")
    MD = m.MD
    d = O.d
    currentYear = m.SCD.year
    fn = O.oSingleFile ? _ofile(O, "results/", "productData" * O.oFileExt) : _ofile(O, "results/", "productData_" * O.scenarioName * O.oFileExt)
    io = IOBuffer()
    for v in O.outProdVariables
        for r1 in eachindex(O.l2r), r2 in O.l2r[r1], p in O.allPr
            c1 = regId2RegSName(MD, O.l1regIds[r1])
            c2 = regId2RegSName(MD, r2)
            if v == "rt"
                for r2b in O.l2r[r1]
                    print(io, O.scenarioName, d, v, d, c1, d, c2, d, p, d, r2b, d)
                    if O.oHRedeable
                        for y in 0:(O.nYears-1)
                            print(io, fmtg(getProdData(m, v, r2, p, y + O.inYear, i2s(r2b))), d)
                        end
                        print(io, "\n")
                    else
                        print(io, currentYear, d, fmtg(getProdData(m, v, r2, p, currentYear, i2s(r2b))), d, "\n")
                    end
                end
            else
                print(io, O.scenarioName, d, v, d, c1, d, c2, d, p, d, d)
                if O.oHRedeable
                    for y in 0:(O.nYears-1)
                        print(io, fmtg(getProdData(m, v, r2, p, y + O.inYear)), d)
                    end
                    print(io, "\n")
                else
                    print(io, currentYear, d, fmtg(getProdData(m, v, r2, p)), d, "\n")
                end
            end
        end
    end
    open(f -> write(f, take!(io)), fn, "a")
end

function printCarbonBalance(m)
    O = m.DO
    O.oCarbonData || return
    currentYear = m.SCD.year
    currentYear == O.inYear && return
    msgOut(MSG_INFO, "Printing forest data..")
    MD = m.MD
    d = O.d
    fn = O.oSingleFile ? _ofile(O, "results/", "carbonBalance" * O.oFileExt) : _ofile(O, "results/", "carbonBalance_" * O.scenarioName * O.oFileExt)
    io = IOBuffer()
    items = ((STOCK_INV, "STOCK_INV"), (STOCK_EXTRA, "STOCK_EXTRA"), (STOCK_PRODUCTS, "STOCK_PRODUCTS"),
             (EM_ENSUB, "EM_ENSUB"), (EM_MATSUB, "EM_MATSUB"), (EM_FOROP, "EM_FOROP"))
    for r1 in eachindex(O.l2r), regId in O.l2r[r1]
        for (b, bname) in items
            print(io, O.scenarioName, d, regId2RegSName(MD, O.l1regIds[r1]), d, regId2RegSName(MD, regId), d)
            v = b <= STOCK_PRODUCTS ? getStock(m, regId, b) : getCumSavedEmissions(m, regId, b)
            print(io, bname, d, currentYear, d, fmtg(v), d, "\n")
        end
    end
    open(f -> write(f, take!(io)), fn, "a")
end

function printOptLog(m, optimal::Bool, nIterations::Integer, obj::Real)
    O = m.DO
    O.oLevel < OUTVL_AGGREGATED && return
    d = O.d
    open(O.logFilename, "a") do io
        print(io, O.scenarioName, d, m.SCD.year, d, _timemessage(), d, optimal ? 1 : 0, d, nIterations, d, fmtg(obj), "\n")
    end
end

function printDetailedHV(m, hVol_byPrd::Dict{NTuple{4,String},Float64})
    getBoolSetting(m, "outDetailedHv", DATA_NOW) || return
    O = m.DO
    MD = m.MD
    d = O.d
    currentYear = m.SCD.year
    io = IOBuffer()
    for r1 in eachindex(O.l2r)
        reg1Name = regId2RegSName(MD, O.l1regIds[r1])
        for r2 in O.l2r[r1]
            reg2Name = regId2RegSName(MD, r2)
            for ft in O.fTypes, dc in O.dClasses
                dc == "" && continue
                for prd in O.priPr
                    k = (i2s(r2), ft, dc, prd)
                    haskey(hVol_byPrd, k) || msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                    print(io, O.scenarioName, d, "hVbyPrd", d, reg1Name, d, reg2Name, d, ft, d, dc, d, prd, d, currentYear, d, fmtg(hVol_byPrd[k]), d, "\n")
                end
            end
        end
    end
    open(f -> write(f, take!(io)), _ofile(O, "results/", "detailedHV_" * O.scenarioName * O.oFileExt), "a")
end

function printDebugPixelValues(m)
    O = m.DO
    O.oLevel < OUTVL_ALL && return
    O.spMode || return
    G = m.GIS
    d = O.d
    currentYear = m.SCD.year
    outVars = getStringVectorSetting(m, "debugVariablesToPrint")
    nDc = length(O.dClasses) - 1
    io = IOBuffer()
    for rId in getRegionIds(m.MD, 2)
        for px in regionPixels(m, rId)
            common = string(O.scenarioName, d, currentYear, d, rId, d, px.ID, d, px.pxX, d, px.pxY, d)
            function full(name, mat; total = true)
                print(io, common, name, d)
                for f in eachindex(O.fTypes)
                    for u in 1:nDc
                        print(io, fmtg(mat[f][u]), d)
                    end
                    print(io, total ? fmtg(vSum(mat[f])) : "0", d)
                end
                print(io, "\n")
            end
            function byft(name, vals)
                print(io, common, name, d)
                for f in eachindex(O.fTypes)
                    for _ in 1:nDc
                        print(io, d)
                    end
                    print(io, fmtg(vals[f]), d)
                end
                print(io, "\n")
            end
            "vol" in outVars && full("vol", px.vol)
            "area" in outVars && full("area", px.area)
            "hArea" in outVars && full("hArea", px.hArea)
            "hVol" in outVars && full("hVol", px.hVol)
            "tp" in outVars && full("tp", px.tp; total = false)
            "mort" in outVars && full("mort", px.mort; total = false)
            "tp_multiplier" in outVars && byft("tp_multiplier", [getMultiplier(m, px, "tp_multiplier", ft) for ft in O.fTypes])
            "mortCoef_multiplier" in outVars && byft("mortCoef_multiplier", [getMultiplier(m, px, "mortCoef_multiplier", ft) for ft in O.fTypes])
            "vMort" in outVars && full("vMort", px.vMort)
            "vMortAdd" in outVars && full("vMortAdd", px.vMortAdd)
            "optDc" in outVars && byft("optDc", px.optDc)
            "expectedAnnualIncome_timber" in outVars && byft("expectedAnnualIncome_timber", px.expectedAnnualIncome_timber)
            "expectedAnnualIncome_carbon" in outVars && byft("expectedAnnualIncome_carbon", px.expectedAnnualIncome_carbon)
            "expectedReturnsNotCorrByRa" in outVars && byft("expectedReturnsNotCorrByRa", px.expectedReturnsNotCorrByRa)
            "expectedReturns" in outVars && byft("expectedReturns", px.expectedReturns)
            "optFtChosen" in outVars && byft("optFtChosen", px.optFtChosen)
            "optDcChosen" in outVars && byft("optDcChosen", px.optDcChosen)
            if "deltaAreas" in outVars
                print(io, common, "deltaAreas", d)
                for _ in eachindex(O.fTypes)
                    for _ in 1:nDc
                        print(io, d)
                    end
                    print(io, d)
                end
                for f in 1:(length(O.fTypes)+1), f2 in 1:(length(O.fTypes)+1)
                    print(io, fmtg(px.deltaArea[f][f2]), d)
                end
                print(io, "\n")
            end
        end
    end
    open(f -> write(f, take!(io)), O.debugPxValuesFilename, "a")
end

# --- maps ---

function printMaps(m)
    O = m.DO
    O.oLevel < OUTVL_MAPS && return
    cYear = m.SCD.year
    if cYear in O.mapsOYears
        printLayers(m)
        O.oLevel < OUTVL_BINMAPS && return
        printBinMaps(m)
    end
end

function printLayers(m)
    msgOut(MSG_DEBUG, "Printing the layers")
    it = m.SCD.iteration
    for L in m.GIS.layerVector
        (it == 0 || L.dynamicContent) && printLayer(m, L)
    end
end

function printBinMaps(m)
    msgOut(MSG_DEBUG, "Printing the maps as images")
    it = m.SCD.iteration
    for L in m.GIS.layerVector
        (it == 0 || L.dynamicContent) && printBinMap(m, L)
    end
end

"Layers::print(): the layer as GRASS (or ArcInfo) ASCII grid, plus the cats and colours files"
function printLayer(m, L::Layer)
    getIntSetting(m, "outputLevel", DATA_NOW) < OUTVL_MAPS && return
    G = m.GIS
    base = m.MD.baseDirectory * m.MD.outputDirname * "maps/"
    suffix = L.name * "_" * i2s(m.SCD.year) * "_" * m.scenarioName
    fmt = getIntSetting(m, "mapOutputFormat", DATA_NOW)
    if fmt == 1
        header = "north: " * d2s(G.geoTopY) * "\n" * "south: " * d2s(G.geoBottomY) * "\n" * "east: " * d2s(G.geoRightX) * "\n" *
                 "west: " * d2s(G.geoLeftX) * "\n" * "rows: " * i2s(G.yNPixels) * "\n" * "cols: " * i2s(G.xNPixels) * "\n" *
                 "null: " * d2s(G.noValue) * "\n"
    elseif fmt == 2
        header = "ncols: " * i2s(G.xNPixels) * "\n" * "lrows: " * i2s(G.yNPixels) * "\n" * "xllcornel: " * d2s(G.geoLeftX) * "\n" *
                 "yllcorner: " * d2s(G.geoBottomY) * "\n" * "cellsize: " * d2s(G.xMetersByPixel) * "\n" * "nodata_value: " * d2s(G.noValue) * "\n"
        if G.xMetersByPixel != G.yMetersByPixel
            msgOut(MSG_ERROR, "The X resolution is different to the Y resolution. I am exporting the map in ArcInfo ASCII Grid format using the X resolution, but be aware that it is incorrect, as this format doesn't support different X-Y resolutions.")
        end
    else
        header = ""
        msgOut(MSG_ERROR, "Map not print for unknow output type.")
    end
    vals = G.values[L.name]
    io = IOBuffer()
    print(io, header, "\n")
    for i in 0:(G.yNPixels-1)
        for j in 0:(G.xNPixels-1)
            print(io, fmtg(vals[j+i*G.xNPixels+1]), " ")
        end
        print(io, "\n")
    end
    open(f -> write(f, take!(io)), base * "asciiGrids/" * suffix, "w")
    open(base * "cats/" * suffix, "w") do io
        print(io, "# ", L.name, "_-_", i2s(m.SCD.year), "\n\n\n")
        print(io, "0.00 0.00 0.00 0.00", "\n")
        for li in L.legendItems
            if L.isInteger
                print(io, li.ID, ":", li.label, "\n")
            else
                print(io, fmtg(li.minValue), ":", fmtg(li.maxValue), ":", li.label, "\n")
            end
        end
    end
    open(base * "colr/" * suffix, "w") do io
        print(io, "% ", L.name, "_-_", i2s(m.SCD.year), "\n\n\n")
        for li in L.legendItems
            if L.isInteger
                print(io, li.ID, ":", li.rColor, ":", li.gColor, ":", li.bColor, "\n")
            else
                print(io, fmtg(li.minValue), ":", li.rColor, ":", li.gColor, ":", li.bColor, " ", fmtg(li.maxValue), ":", li.rColor, ":", li.gColor, ":", li.bColor, "\n")
            end
        end
    end
    listFile = base * (L.isInteger ? "integerListLayers/" : "floatListLayers/") * m.scenarioName
    open(listFile, "a") do io
        print(io, L.name, "_", m.SCD.year, "_", m.scenarioName, "\n")
    end
end

"Layers::printBinMap(): the layer as png image coloured using the legend"
function printBinMap(m, L::Layer)
    (!L.display || !L.dynamicContent) && return
    img = layerImage(m.GIS, L)
    fn = m.MD.baseDirectory * m.MD.outputDirname * "maps/bitmaps/" * L.name * "_" * i2s(m.SCD.year) * "_" * m.scenarioName * ".png"
    writePNG(fn, img)
end

"RGB image (3 x width x height) of a layer, coloured using its legend (white for no-value)"
function layerImage(G::Gis, L::Layer)
    w = G.subXR - G.subXL + 1
    h = G.subYB - G.subYT + 1
    img = fill(UInt8(255), 3, w, h)
    vals = G.values[L.name]
    # the whole map is painted (the C++ loop stops one row and one column short, leaving them white)
    for row in G.subYT:G.subYB, col in G.subXL:G.subXR
        r, g, b = getColor(G, L, vals[row*G.xNPixels+col+1])
        img[1, col-G.subXL+1, row-G.subYT+1] = r
        img[2, col-G.subXL+1, row-G.subYT+1] = g
        img[3, col-G.subXL+1, row-G.subYT+1] = b
    end
    return img
end

# Minimal RGB png writer (zlib from Zlib_jll)
function _crc32(data::Vector{UInt8}, crc::UInt32 = UInt32(0))
    return ccall((:crc32, Zlib_jll.libz), Culong, (Culong, Ptr{UInt8}, Cuint), crc, data, length(data)) % UInt32
end

function _zcompress(data::Vector{UInt8})
    bound = ccall((:compressBound, Zlib_jll.libz), Culong, (Culong,), length(data))
    out = Vector{UInt8}(undef, bound)
    outlen = Ref{Culong}(bound)
    r = ccall((:compress2, Zlib_jll.libz), Cint, (Ptr{UInt8}, Ref{Culong}, Ptr{UInt8}, Culong, Cint), out, outlen, data, length(data), 9)
    r == 0 || error("zlib compression error $r")
    return out[1:outlen[]]
end

function writePNG(fn::AbstractString, img::Array{UInt8,3})
    _, w, h = size(img)
    raw = UInt8[]
    sizehint!(raw, h * (3w + 1))
    for y in 1:h
        push!(raw, 0x00)
        for x in 1:w
            push!(raw, img[1, x, y], img[2, x, y], img[3, x, y])
        end
    end
    be32(x) = [UInt8((x >> 24) & 0xff), UInt8((x >> 16) & 0xff), UInt8((x >> 8) & 0xff), UInt8(x & 0xff)]
    function chunk(io, typ::String, data::Vector{UInt8})
        write(io, be32(length(data)))
        td = vcat(Vector{UInt8}(typ), data)
        write(io, td)
        write(io, be32(_crc32(td)))
    end
    open(fn, "w") do io
        write(io, UInt8[0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a])
        chunk(io, "IHDR", vcat(be32(w), be32(h), UInt8[8, 2, 0, 0, 0]))
        chunk(io, "IDAT", _zcompress(raw))
        chunk(io, "IEND", UInt8[])
    end
end
