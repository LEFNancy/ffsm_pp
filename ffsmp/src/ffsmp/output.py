"""Port of the Output class (csv results, optimisation log, debug pixel values and maps)"""

import datetime
import os
import struct
import zlib

from .utils import (msgOut, fmtg, i2s, d2s, tokenize, vSum, MSG_INFO, MSG_DEBUG, MSG_ERROR, MSG_NO_MSG,
                    MSG_CRITICAL_ERROR, DATA_NOW, DIAM_ALL, OUTVL_AGGREGATED, OUTVL_DETAILED, OUTVL_MAPS,
                    OUTVL_BINMAPS, OUTVL_ALL, STOCK_INV, STOCK_EXTRA, STOCK_PRODUCTS, EM_ENSUB, EM_MATSUB, EM_FOROP)
from .modeldata import (getIntSetting, getStringSetting, getBoolSetting, getIntVectorSetting, getStringVectorSetting,
                        getForTypeIds, getRegionIds, getL2r, getForData, getProdData, regId2RegSName,
                        getForTypeParents)
from .gis import getMultiplier, getColor
from .carbon import getStock, getCumSavedEmissions


class OutputState:
    def __init__(self):
        self.oLevel = 0
        self.d = ";"
        self.inYear = 0
        self.nYears = 0
        self.baseDir = ""
        self.oDir = ""
        self.scenarioName = ""
        self.oFileExt = ""
        self.oHRedeable = False
        self.oSingleFile = False
        self.oYears = []
        self.mapsOYears = []
        self.outForVariables = []
        self.outProdVariables = []
        self.dClasses = []
        self.outStepRange = 0
        self.forestDiamDetailedOutput = False
        self.fTypes = []
        self.oForData = False
        self.oProdData = False
        self.oCarbonData = False
        self.priPr = []
        self.secPr = []
        self.allPr = []
        self.l1regIds = []
        self.l2r = []
        self.spMode = False
        self.logFilename = ""
        self.debugPxValuesFilename = ""


def _timemessage():
    t = datetime.datetime.now()
    return f"{t.hour}:{t.minute}:{t.second}"


def _crem(a, b):
    """C/Julia remainder (sign of the dividend)"""
    r = abs(a) % abs(b)
    return -r if a < 0 else r


def getOutputFieldDelimiter(m):
    id_ = getIntSetting(m, "outputFieldDelimiter")
    d = {1: ",", 2: ";", 3: ":", 4: "\t", 5: " "}.get(id_)
    if d is None:
        msgOut(MSG_ERROR, 'You have specified an unknow output file field delimiter. Using ";".')
        return ","
    return d


def _append(fn, text):
    with open(fn, "a", encoding="utf-8", newline="") as f:
        f.write(text)


def _write(fn, text):
    with open(fn, "w", encoding="utf-8", newline="") as f:
        f.write(text)


def cleanScenario(fileName, scenarioName, d):
    """Remove from an output file the rows of the given scenario (first field), keeping the other scenarios"""
    if not os.path.isfile(fileName):
        msgOut(MSG_ERROR, f"Error in opening the file {fileName} for reading.")
        return
    rows = []
    with open(fileName, encoding="utf-8") as f:
        for row in f:
            row = row.rstrip("\n")
            tokens = tokenize(row, d)
            if not tokens or tokens[0] != scenarioName:
                rows.append(row)
    _write(fileName, "".join(r + "\n" for r in rows))


def commonInit(m):
    O = m.DO
    O.oLevel = getIntSetting(m, "outputLevel")
    O.d = getOutputFieldDelimiter(m)
    O.inYear = getIntSetting(m, "initialYear")
    O.nYears = getIntSetting(m, "simulationYears")
    O.baseDir = m.MD.baseDirectory
    O.oDir = m.MD.outputDirname
    sn = getStringSetting(m, "overridenScenarioName")
    O.scenarioName = m.scenarioName if sn == "none" else sn
    O.oFileExt = getStringSetting(m, "outputFileExtension")
    O.oHRedeable = getBoolSetting(m, "outputHumanReadable")
    O.oSingleFile = getBoolSetting(m, "outputSingleFile")
    O.oYears = getIntVectorSetting(m, "outYears")
    O.mapsOYears = getIntVectorSetting(m, "mapsOutYears")
    O.outForVariables = getStringVectorSetting(m, "outForVariables")
    O.outProdVariables = getStringVectorSetting(m, "outProdVariables")
    O.dClasses = getStringVectorSetting(m, "dClasses") + [""]
    O.outStepRange = getIntSetting(m, "outStepRange")
    O.forestDiamDetailedOutput = getBoolSetting(m, "forestDiamDetailedOutput")
    O.fTypes = getForTypeIds(m.MD)
    O.oForData = O.oLevel >= OUTVL_DETAILED and bool(O.outForVariables)
    O.oProdData = O.oLevel >= OUTVL_DETAILED and bool(O.outProdVariables)
    O.oCarbonData = O.oLevel >= OUTVL_DETAILED
    O.priPr = getStringVectorSetting(m, "priProducts")
    O.secPr = getStringVectorSetting(m, "secProducts")
    O.allPr = O.priPr + O.secPr
    O.l1regIds = getRegionIds(m.MD, 1, True)
    O.spMode = getBoolSetting(m, "usePixelData")
    O.l2r = getL2r(m.MD, True)


def _ofile(O, sub, name):
    return O.baseDir + O.oDir + sub + name


def initOutput(m):
    commonInit(m)
    initOutputMaps(m)
    initDebugPixelValues(m)
    initOutputForestData(m)
    initOutputProductData(m)
    initOptimisationLog(m)
    initCarbonBalance(m)


def initOptimisationLog(m):
    O = m.DO
    if O.oLevel < OUTVL_AGGREGATED:
        return
    d = O.d
    O.logFilename = _ofile(O, "optimisationLogs/", "optimisationLogs.txt") if O.oSingleFile else _ofile(O, "optimisationLogs/", O.scenarioName + ".txt")
    first = f"{O.scenarioName}{d}0000{d}{_timemessage()}{d}{d}{d}\n"
    if os.path.isfile(O.logFilename):
        cleanScenario(O.logFilename, O.scenarioName, d)
        _append(O.logFilename, first)
    else:
        _write(O.logFilename, f"scenario{d}year{d}time{d}opt flag{d}iterations{d}\n" + first)


def initDebugPixelValues(m):
    O = m.DO
    if O.oLevel < OUTVL_ALL or not O.spMode:
        return
    d = O.d
    O.debugPxValuesFilename = _ofile(O, "debugs/", "debugPxValues.csv") if O.oSingleFile else _ofile(O, "debugs/", "debugPxValues_" + O.scenarioName + ".csv")
    if os.path.isfile(O.debugPxValuesFilename):
        cleanScenario(O.debugPxValuesFilename, O.scenarioName, d)
        return
    out = [f"scenario{d}year{d}region{d}pxId{d}pxX{d}pxY{d}var{d}"]
    for ft in O.fTypes:
        for dc in O.dClasses:
            out.append(f"{ft}_{dc}{d}")
    nft = len(O.fTypes)
    for f in range(nft + 1):
        for f2 in range(nft + 1):
            ft1 = "NonFor" if f == nft else O.fTypes[f]
            ft2 = "NonFor" if f2 == nft else O.fTypes[f2]
            out.append(f"{ft1}_2_{ft2}{d}")
    out.append("\n")
    _write(O.debugPxValuesFilename, "".join(out))


def _initResultFile(m, name, headerFields):
    O = m.DO
    d = O.d
    if O.oSingleFile:
        fn = _ofile(O, "results/", name + O.oFileExt)
        if os.path.isfile(fn):
            cleanScenario(fn, O.scenarioName, d)
            return False
    else:
        fn = _ofile(O, "results/", name + "_" + O.scenarioName + O.oFileExt)
    out = [h + d for h in headerFields]
    if name != "carbonBalance" and O.oHRedeable:
        for i in range(O.nYears):
            out.append(f"{i + O.inYear}{d}")
    else:
        out.append(f"year{d}value{d}")
    out.append("\n")
    _write(fn, "".join(out))
    return True


def initOutputForestData(m):
    O = m.DO
    if not O.oForData:
        return
    if not _initResultFile(m, "forestData", ("scen", "parName", "country", "region", "forType", "freeDim")):
        return
    if not getBoolSetting(m, "outDetailedHv", DATA_NOW):
        return
    d = O.d
    hdr = "".join(h + d for h in ("scen", "parName", "country", "region", "forType", "freeDim", "prod", "year", "value"))
    _write(_ofile(O, "results/", "detailedHV_" + O.scenarioName + O.oFileExt), hdr + "\n")


def initOutputProductData(m):
    if not m.DO.oProdData:
        return
    _initResultFile(m, "productData", ("scen", "parName", "country", "region", "prod", "freeDim"))


def initCarbonBalance(m):
    if not m.DO.oCarbonData:
        return
    _initResultFile(m, "carbonBalance", ("scen", "country", "region", "balItem"))


def initOutputMaps(m):
    O = m.DO
    if O.oLevel < OUTVL_MAPS:
        return
    mapBase = O.baseDir + O.oDir + "maps/"
    _write(mapBase + "scenarioNames/" + O.scenarioName, O.scenarioName + "\n")
    _write(mapBase + "integerListLayers/" + O.scenarioName, "")
    _write(mapBase + "floatListLayers/" + O.scenarioName, "")


def printOutput(m, earlyPrint=False):
    """Output::print()"""
    O = m.DO
    cYear = m.SCD.year
    initialSimulationYear = getIntSetting(m, "initialOptYear")
    if O.outStepRange != -1 and _crem(cYear - initialSimulationYear, O.outStepRange) != 0 and cYear != (initialSimulationYear + O.nYears) - 1:
        print(f"{cYear} not printed")
        return
    printThisYear = any(O.outStepRange == -1 and y == cYear for y in O.oYears)
    if O.outStepRange == -1 and not printThisYear:
        return
    printMaps(m)
    m.MD.errorLevel = MSG_NO_MSG
    printForestData(m, False)
    printProductData(m, False)
    printCarbonBalance(m)
    if not earlyPrint:
        printDebugPixelValues(m)
    m.MD.errorLevel = MSG_ERROR


def printFinalOutput(m):
    O = m.DO
    if O.oHRedeable:
        m.MD.errorLevel = MSG_NO_MSG
        printForestData(m, True)
        printProductData(m, True)
        m.MD.errorLevel = MSG_ERROR


def printForestData(m, finalFlush):
    O = m.DO
    if not O.oForData:
        return
    if O.oHRedeable and not finalFlush:
        return
    msgOut(MSG_INFO, "Printing forest data..")
    MD = m.MD
    d = O.d
    currentYear = m.SCD.year
    fn = _ofile(O, "results/", "forestData" + O.oFileExt) if O.oSingleFile else _ofile(O, "results/", "forestData_" + O.scenarioName + O.oFileExt)
    out = []
    w = out.append
    for v in O.outForVariables:
        fTypes_temp = list(O.fTypes)
        if v in ("expReturns", "sumExpReturns", "totalShareInvadedArea", "usableDeadTimber"):
            fTypes_temp.append("")
            fTypes_temp.extend(getForTypeParents(MD))
        for r1 in range(len(O.l2r)):
            c1 = regId2RegSName(MD, O.l1regIds[r1])
            for r2 in O.l2r[r1]:
                c2 = regId2RegSName(MD, r2)
                for ft in fTypes_temp:
                    if O.forestDiamDetailedOutput:
                        for dc in O.dClasses:
                            w(f"{O.scenarioName}{d}{v}{d}{c1}{d}{c2}{d}{ft}{d}{dc}{d}")
                            if O.oHRedeable:
                                for y in range(O.nYears):
                                    w(fmtg(getForData(m, v, r2, ft, dc, y + O.inYear)) + d)
                                w("\n")
                            else:
                                w(f"{currentYear}{d}{fmtg(getForData(m, v, r2, ft, dc))}{d}\n")
                    else:
                        w(f"{O.scenarioName}{d}{v}{d}{c1}{d}{c2}{d}{ft}{d}{d}")
                        if O.oHRedeable:
                            for y in range(O.nYears):
                                w(fmtg(getForData(m, v, r2, ft, DIAM_ALL, y + O.inYear)) + d)
                            w("\n")
                        else:
                            w(f"{currentYear}{d}{fmtg(getForData(m, v, r2, ft, DIAM_ALL))}{d}\n")
    _append(fn, "".join(out))


def printProductData(m, finalFlush):
    O = m.DO
    if not O.oProdData:
        return
    if O.oHRedeable and not finalFlush:
        return
    msgOut(MSG_INFO, "Printing market data..")
    MD = m.MD
    d = O.d
    currentYear = m.SCD.year
    fn = _ofile(O, "results/", "productData" + O.oFileExt) if O.oSingleFile else _ofile(O, "results/", "productData_" + O.scenarioName + O.oFileExt)
    out = []
    w = out.append
    for v in O.outProdVariables:
        for r1 in range(len(O.l2r)):
            c1 = regId2RegSName(MD, O.l1regIds[r1])
            for r2 in O.l2r[r1]:
                c2 = regId2RegSName(MD, r2)
                for p in O.allPr:
                    if v == "rt":
                        for r2b in O.l2r[r1]:
                            w(f"{O.scenarioName}{d}{v}{d}{c1}{d}{c2}{d}{p}{d}{r2b}{d}")
                            if O.oHRedeable:
                                for y in range(O.nYears):
                                    w(fmtg(getProdData(m, v, r2, p, y + O.inYear, i2s(r2b))) + d)
                                w("\n")
                            else:
                                w(f"{currentYear}{d}{fmtg(getProdData(m, v, r2, p, currentYear, i2s(r2b)))}{d}\n")
                    else:
                        w(f"{O.scenarioName}{d}{v}{d}{c1}{d}{c2}{d}{p}{d}{d}")
                        if O.oHRedeable:
                            for y in range(O.nYears):
                                w(fmtg(getProdData(m, v, r2, p, y + O.inYear)) + d)
                            w("\n")
                        else:
                            w(f"{currentYear}{d}{fmtg(getProdData(m, v, r2, p))}{d}\n")
    _append(fn, "".join(out))


def printCarbonBalance(m):
    O = m.DO
    if not O.oCarbonData:
        return
    currentYear = m.SCD.year
    if currentYear == O.inYear:
        return
    msgOut(MSG_INFO, "Printing forest data..")
    MD = m.MD
    d = O.d
    fn = _ofile(O, "results/", "carbonBalance" + O.oFileExt) if O.oSingleFile else _ofile(O, "results/", "carbonBalance_" + O.scenarioName + O.oFileExt)
    out = []
    items = ((STOCK_INV, "STOCK_INV"), (STOCK_EXTRA, "STOCK_EXTRA"), (STOCK_PRODUCTS, "STOCK_PRODUCTS"),
             (EM_ENSUB, "EM_ENSUB"), (EM_MATSUB, "EM_MATSUB"), (EM_FOROP, "EM_FOROP"))
    for r1 in range(len(O.l2r)):
        for regId in O.l2r[r1]:
            for b, bname in items:
                v = getStock(m, regId, b) if b <= STOCK_PRODUCTS else getCumSavedEmissions(m, regId, b)
                out.append(f"{O.scenarioName}{d}{regId2RegSName(MD, O.l1regIds[r1])}{d}{regId2RegSName(MD, regId)}{d}"
                           f"{bname}{d}{currentYear}{d}{fmtg(v)}{d}\n")
    _append(fn, "".join(out))


def printOptLog(m, optimal, nIterations, obj):
    O = m.DO
    if O.oLevel < OUTVL_AGGREGATED:
        return
    d = O.d
    _append(O.logFilename, f"{O.scenarioName}{d}{m.SCD.year}{d}{_timemessage()}{d}{1 if optimal else 0}{d}{nIterations}{d}{fmtg(float(obj))}\n")


def printDetailedHV(m, hVol_byPrd):
    if not getBoolSetting(m, "outDetailedHv", DATA_NOW):
        return
    O = m.DO
    MD = m.MD
    d = O.d
    currentYear = m.SCD.year
    out = []
    for r1 in range(len(O.l2r)):
        reg1Name = regId2RegSName(MD, O.l1regIds[r1])
        for r2 in O.l2r[r1]:
            reg2Name = regId2RegSName(MD, r2)
            for ft in O.fTypes:
                for dc in O.dClasses:
                    if dc == "":
                        continue
                    for prd in O.priPr:
                        k = (i2s(r2), ft, dc, prd)
                        if k not in hVol_byPrd:
                            msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
                        out.append(f"{O.scenarioName}{d}hVbyPrd{d}{reg1Name}{d}{reg2Name}{d}{ft}{d}{dc}{d}{prd}{d}{currentYear}{d}{fmtg(hVol_byPrd[k])}{d}\n")
    _append(_ofile(O, "results/", "detailedHV_" + O.scenarioName + O.oFileExt), "".join(out))


def printDebugPixelValues(m):
    from .core import regionPixels
    O = m.DO
    if O.oLevel < OUTVL_ALL or not O.spMode:
        return
    d = O.d
    currentYear = m.SCD.year
    outVars = getStringVectorSetting(m, "debugVariablesToPrint")
    nDc = len(O.dClasses) - 1
    out = []
    w = out.append
    nft = len(O.fTypes)
    for rId in getRegionIds(m.MD, 2):
        for px in regionPixels(m, rId):
            common = f"{O.scenarioName}{d}{currentYear}{d}{rId}{d}{px.ID}{d}{px.pxX}{d}{px.pxY}{d}"

            def full(name, mat, total=True):
                w(common + name + d)
                for f in range(nft):
                    for u in range(nDc):
                        w(fmtg(mat[f][u]) + d)
                    w((fmtg(vSum(mat[f])) if total else "0") + d)
                w("\n")

            def byft(name, vals):
                w(common + name + d)
                for f in range(nft):
                    w(d * nDc)
                    w(fmtg(vals[f]) + d)
                w("\n")
            if "vol" in outVars:
                full("vol", px.vol)
            if "area" in outVars:
                full("area", px.area)
            if "hArea" in outVars:
                full("hArea", px.hArea)
            if "hVol" in outVars:
                full("hVol", px.hVol)
            if "tp" in outVars:
                full("tp", px.tp, total=False)
            if "mort" in outVars:
                full("mort", px.mort, total=False)
            if "tp_multiplier" in outVars:
                byft("tp_multiplier", [getMultiplier(m, px, "tp_multiplier", ft) for ft in O.fTypes])
            if "mortCoef_multiplier" in outVars:
                byft("mortCoef_multiplier", [getMultiplier(m, px, "mortCoef_multiplier", ft) for ft in O.fTypes])
            if "vMort" in outVars:
                full("vMort", px.vMort)
            if "vMortAdd" in outVars:
                full("vMortAdd", px.vMortAdd)
            if "optDc" in outVars:
                byft("optDc", px.optDc)
            if "expectedAnnualIncome_timber" in outVars:
                byft("expectedAnnualIncome_timber", px.expectedAnnualIncome_timber)
            if "expectedAnnualIncome_carbon" in outVars:
                byft("expectedAnnualIncome_carbon", px.expectedAnnualIncome_carbon)
            if "expectedReturnsNotCorrByRa" in outVars:
                byft("expectedReturnsNotCorrByRa", px.expectedReturnsNotCorrByRa)
            if "expectedReturns" in outVars:
                byft("expectedReturns", px.expectedReturns)
            if "optFtChosen" in outVars:
                byft("optFtChosen", px.optFtChosen)
            if "optDcChosen" in outVars:
                byft("optDcChosen", px.optDcChosen)
            if "deltaAreas" in outVars:
                w(common + "deltaAreas" + d)
                for _ in range(nft):
                    w(d * nDc)
                    w(d)
                for f in range(nft + 1):
                    for f2 in range(nft + 1):
                        w(fmtg(px.deltaArea[f][f2]) + d)
                w("\n")
    _append(O.debugPxValuesFilename, "".join(out))


# --- maps ---

def printMaps(m):
    O = m.DO
    if O.oLevel < OUTVL_MAPS:
        return
    cYear = m.SCD.year
    if cYear in O.mapsOYears:
        printLayers(m)
        if O.oLevel < OUTVL_BINMAPS:
            return
        printBinMaps(m)


def printLayers(m):
    msgOut(MSG_DEBUG, "Printing the layers")
    it = m.SCD.iteration
    for L in m.GIS.layerVector:
        if it == 0 or L.dynamicContent:
            printLayer(m, L)


def printBinMaps(m):
    msgOut(MSG_DEBUG, "Printing the maps as images")
    it = m.SCD.iteration
    for L in m.GIS.layerVector:
        if it == 0 or L.dynamicContent:
            printBinMap(m, L)


def printLayer(m, L):
    """Layers::print(): the layer as GRASS (or ArcInfo) ASCII grid, plus the cats and colours files"""
    if getIntSetting(m, "outputLevel", DATA_NOW) < OUTVL_MAPS:
        return
    G = m.GIS
    base = m.MD.baseDirectory + m.MD.outputDirname + "maps/"
    suffix = L.name + "_" + i2s(m.SCD.year) + "_" + m.scenarioName
    fmt = getIntSetting(m, "mapOutputFormat", DATA_NOW)
    if fmt == 1:
        header = (f"north: {d2s(G.geoTopY)}\nsouth: {d2s(G.geoBottomY)}\neast: {d2s(G.geoRightX)}\n"
                  f"west: {d2s(G.geoLeftX)}\nrows: {i2s(G.yNPixels)}\ncols: {i2s(G.xNPixels)}\nnull: {d2s(G.noValue)}\n")
    elif fmt == 2:
        header = (f"ncols: {i2s(G.xNPixels)}\nlrows: {i2s(G.yNPixels)}\nxllcornel: {d2s(G.geoLeftX)}\n"
                  f"yllcorner: {d2s(G.geoBottomY)}\ncellsize: {d2s(G.xMetersByPixel)}\nnodata_value: {d2s(G.noValue)}\n")
        if G.xMetersByPixel != G.yMetersByPixel:
            msgOut(MSG_ERROR, "The X resolution is different to the Y resolution. I am exporting the map in ArcInfo ASCII Grid format using the X resolution, but be aware that it is incorrect, as this format doesn't support different X-Y resolutions.")
    else:
        header = ""
        msgOut(MSG_ERROR, "Map not print for unknow output type.")
    vals = G.values[L.name]
    out = [header, "\n"]
    nx = G.xNPixels
    for i in range(G.yNPixels):
        out.append(" ".join(fmtg(vals[j + i * nx]) for j in range(nx)))
        out.append(" \n")
    _write(base + "asciiGrids/" + suffix, "".join(out))
    cats = [f"# {L.name}_-_{i2s(m.SCD.year)}\n\n\n", "0.00 0.00 0.00 0.00\n"]
    for li in L.legendItems:
        if L.isInteger:
            cats.append(f"{li.ID}:{li.label}\n")
        else:
            cats.append(f"{fmtg(li.minValue)}:{fmtg(li.maxValue)}:{li.label}\n")
    _write(base + "cats/" + suffix, "".join(cats))
    colr = [f"% {L.name}_-_{i2s(m.SCD.year)}\n\n\n"]
    for li in L.legendItems:
        if L.isInteger:
            colr.append(f"{li.ID}:{li.rColor}:{li.gColor}:{li.bColor}\n")
        else:
            colr.append(f"{fmtg(li.minValue)}:{li.rColor}:{li.gColor}:{li.bColor} {fmtg(li.maxValue)}:{li.rColor}:{li.gColor}:{li.bColor}\n")
    _write(base + "colr/" + suffix, "".join(colr))
    listFile = base + ("integerListLayers/" if L.isInteger else "floatListLayers/") + m.scenarioName
    _append(listFile, f"{L.name}_{m.SCD.year}_{m.scenarioName}\n")


def printBinMap(m, L):
    """Layers::printBinMap(): the layer as png image coloured using the legend"""
    if not L.display or not L.dynamicContent:
        return
    w, h, rows = layerImage(m.GIS, L)
    fn = m.MD.baseDirectory + m.MD.outputDirname + "maps/bitmaps/" + L.name + "_" + i2s(m.SCD.year) + "_" + m.scenarioName + ".png"
    writePNG(fn, w, h, rows)


def layerImage(G, L):
    """RGB image of a layer, coloured using its legend (white for no-value): (width, height, rows), each row
    being a bytearray of 3*width bytes. The whole map is painted, including the last row and column."""
    w = G.subXR - G.subXL + 1
    h = G.subYB - G.subYT + 1
    vals = G.values[L.name]
    rows = []
    for row in range(G.subYT, G.subYB + 1):
        b = bytearray()
        for col in range(G.subXL, G.subXR + 1):
            b.extend(getColor(G, L, vals[row * G.xNPixels + col]))
        rows.append(b)
    return w, h, rows


def writePNG(fn, w, h, rows):
    """Minimal RGB png writer"""
    raw = bytearray()
    for r in rows:
        raw.append(0)
        raw.extend(r)

    def chunk(typ, data):
        td = typ + data
        return struct.pack(">I", len(data)) + td + struct.pack(">I", zlib.crc32(td) & 0xFFFFFFFF)
    with open(fn, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))
