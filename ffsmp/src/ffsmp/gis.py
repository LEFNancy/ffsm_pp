"""Port of Gis, Layers and Pixel.

In the C++ code every pixel owns a std::map<string,double> with the value of each layer. Here the values
are stored by layer (one array over all pixels); the sorted list of all the layer keys is kept to replicate
the upper_bound based look-ups of Pixel::getSTData().
"""

import bisect
import math
import os
import sys
import xml.etree.ElementTree as ET
from array import array

from .utils import (msgOut, s2i, s2d, i2s, cmin, cmax, cpow, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR,
                    MSG_CRITICAL_ERROR, RETNA, DATA_NOW)
from .rng import NormalDist, normSample
from .modeldata import (getForTypeIds, getForType, getForData, getDoubleSetting, getIntSetting, getBoolSetting,
                        getStringSetting, getPathMortalityRule, getRegionIds, getRegion, getAllRegions,
                        getFilenamesByDir)

_MISSING = object()


class LegendItem:
    __slots__ = ("ID", "label", "rColor", "gColor", "bColor", "minValue", "maxValue")

    def __init__(self, ID, label, r, g, b, minValue, maxValue):
        self.ID = ID
        self.label = label
        self.rColor = r
        self.gColor = g
        self.bColor = b
        self.minValue = minValue
        self.maxValue = maxValue


class Layer:
    __slots__ = ("name", "label", "isInteger", "dynamicContent", "fullFileName", "display", "legendItems")

    def __init__(self, name, label, isInteger, dynamicContent, fullFileName, display, legendItems):
        self.name = name
        self.label = label
        self.isInteger = isInteger
        self.dynamicContent = dynamicContent
        self.fullFileName = fullFileName
        self.display = display
        self.legendItems = legendItems


class Pixel:
    __slots__ = ("ID", "pxX", "pxY", "spMods", "avalCoef", "l2region", "vol", "area", "deltaArea", "initialDc0Area",
                 "hArea", "hVol", "hProductivity", "hVol_byPrd", "regArea", "vReg", "vMort", "vMortAdd",
                 "expectedReturns", "optDc", "optFtChosen", "optDcChosen", "expectedAnnualIncome_carbon",
                 "expectedAnnualIncome_timber", "expectedReturnsNotCorrByRa", "vol_l", "area_l", "beta", "mort",
                 "addMort", "tp", "cumTp", "vHa", "cumAlive", "cumTp_exp", "vHa_exp", "cumAlive_exp",
                 "portfolioVarRa", "expType", "expTypePrices", "usePortfolio")

    def __init__(self, ID, nft):
        self.ID = ID
        self.pxX = 0
        self.pxY = 0
        self.spMods = [1.0] * nft
        self.avalCoef = [1.0] * nft
        self.l2region = None
        self.vol = []
        self.area = []
        self.deltaArea = []
        self.initialDc0Area = []
        self.hArea = []
        self.hVol = []
        self.hProductivity = []
        self.hVol_byPrd = []
        self.regArea = {}
        self.vReg = []
        self.vMort = []
        self.vMortAdd = []
        self.expectedReturns = []
        self.optDc = []
        self.optFtChosen = []
        self.optDcChosen = []
        self.expectedAnnualIncome_carbon = []
        self.expectedAnnualIncome_timber = []
        self.expectedReturnsNotCorrByRa = []
        self.vol_l = []
        self.area_l = []
        self.beta = []
        self.mort = []
        self.addMort = []
        self.tp = []
        self.cumTp = []
        self.vHa = []
        self.cumAlive = []
        self.cumTp_exp = []
        self.vHa_exp = []
        self.cumAlive_exp = []
        self.portfolioVarRa = 0.0
        self.expType = 0.0
        self.expTypePrices = 0.0
        self.usePortfolio = False


class Gis:
    def __init__(self):
        self.pxVector = []
        self.layerVector = []
        self.layerIndex = {}
        self.values = {}          # pixel values by key (layer name): array('d') over all pixels
        self.present = {}         # for keys set only on some pixels (e.g. "ra"): bytearray mask
        self.sortedKeys = []
        self.keysSorted = True
        self.stCache = {}
        self.stQueryCache = {}
        self.xNPixels = 0
        self.yNPixels = 0
        self.xyNPixels = 0
        self.xMetersByPixel = 0.0
        self.yMetersByPixel = 0.0
        self.geoLeftX = 0.0
        self.geoTopY = 0.0
        self.geoRightX = 0.0
        self.geoBottomY = 0.0
        self.noValue = -9999.0
        self.subXL = 0
        self.subXR = 0
        self.subYT = 0
        self.subYB = 0


def getPixel(G, x, y):
    return G.pxVector[x + y * G.xNPixels]


def getHaByPixel(G):
    return (G.xMetersByPixel * G.yMetersByPixel) / 10000


# --- pixel values ---

def _filled(G, value):
    return array("d", [value]) * G.xyNPixels


def _addKey(G, key, v):
    G.values[key] = v
    G.sortedKeys.append(key)
    G.keysSorted = False
    G.stCache.clear()
    G.stQueryCache.clear()


def setValue(G, px, key, value):
    """Pixel::setValue (std::map insert: no change if the key already exists for that pixel)"""
    v = G.values.get(key)
    if v is None:
        v = _filled(G, G.noValue)
        _addKey(G, key, v)
        G.present[key] = bytearray(G.xyNPixels)
    pr = G.present.get(key)
    if pr is None:
        return            # a full layer: all pixels already have the key
    if not pr[px.ID]:
        pr[px.ID] = 1
        v[px.ID] = value


def _hasKey(G, px, key):
    if key not in G.values:
        return False
    pr = G.present.get(key)
    return pr is None or bool(pr[px.ID])


def getDoubleValue(G, px, key, returnZeroForNoValue=False):
    if _hasKey(G, px, key):
        v = G.values[key][px.ID]
        return 0.0 if (returnZeroForNoValue and v == G.noValue) else v
    msgOut(MSG_WARNING, f'No layer "{key}" found on pixel ({px.pxX},{px.pxY}). Sure you didn\'t mispelled it?')
    return 0.0 if returnZeroForNoValue else G.noValue


def changeValue(G, px, key, value, setNoValueForZero=False):
    if _hasKey(G, px, key):
        G.values[key][px.ID] = G.noValue if (setNoValueForZero and value == 0) else float(value)
    else:
        msgOut(MSG_ERROR, f"Coud not change pixel value for layer {key}. Layer don't found.")


def _sortedValueKeys(G):
    if not G.keysSorted:
        G.sortedKeys.sort()
        G.keysSorted = True
    return G.sortedKeys


def _stResolve(G, search):
    """Replicates `map::upper_bound(search); if(i!=begin) i--;` followed by the prefix check on the search
    string without the year: returns the matching key or None."""
    c = G.stCache.get(search, _MISSING)
    if c is not _MISSING:
        return c
    ks = _sortedValueKeys(G)
    i = bisect.bisect_right(ks, search) - 1   # last key <= search (-1: none, i.e. no layer of this series up to this year)
    base = search[:-4]
    res = ks[i] if (i >= 0 and ks[i].startswith(base)) else None
    G.stCache[search] = res
    return res


def stKey(m, parName, forName, year, d2=""):
    """The layer key that Pixel::getSTData() uses for parName#fType#d2#year (None if no layer matches)"""
    G = m.GIS
    if year == DATA_NOW:
        year = m.SCD.year
    qk = (parName, forName, d2, year)
    key = G.stQueryCache.get(qk, _MISSING)
    if key is _MISSING:
        key = None
        ys = i2s(year)
        for f, d in ((forName, d2), ("", d2), (forName, ""), ("", "")):
            key = _stResolve(G, f"{parName}#{f}#{d}#{ys}")
            if key is not None:
                break
        G.stQueryCache[qk] = key
    return key


def stValue(m, px, key, naToReturn=RETNA):
    """Value of the layer `key` (from stKey) on a pixel, as returned by Pixel::getSTData()"""
    G = m.GIS
    defaultNoValue = m.cache.noValue
    if naToReturn == RETNA:
        naToReturn = defaultNoValue
    if key is None:
        return float(naToReturn)
    pr = G.present.get(key)
    v = G.values[key][px.ID] if (pr is None or pr[px.ID]) else G.noValue
    return float(naToReturn) if v == defaultNoValue else v


def getSTData(m, px, parName, forName, year, d2="", naToReturn=RETNA):
    """Pixel::getSTData() (spatio-temporal layer data: parName#fType#d2#year)"""
    if not m.cache.usePixelData:
        return float(m.cache.noValue if naToReturn == RETNA else naToReturn)
    return stValue(m, px, stKey(m, parName, forName, year, d2), naToReturn)


def getSpModifier(m, px, ft):
    pos = m.cache.ftPos
    if pos is None:
        pos = m.cache.ftPos = {f: i for i, f in reversed(list(enumerate(getForTypeIds(m.MD))))}
    i = pos.get(ft)
    if i is None:
        msgOut(MSG_CRITICAL_ERROR, "Asked spatial modifier for a forest type that doesn't exist")
    return px.spMods[i]


def getMultiplier(m, px, multiplierName, forName, year=DATA_NOW):
    """Pixel::getMultiplier(). The regional factors given in the forData table (pure look-ups) are cached for
    the current simulation year."""
    if year == DATA_NOW:
        year = m.SCD.year
    multiplierSpVar = getSpModifier(m, px, forName) if multiplierName == "tp_multiplier" else 1.0
    cache = m.cache
    if multiplierName in cache.modifiersFromTable:
        if cache.multCacheYear != m.SCD.year:
            cache.multCache.clear()
            cache.multCacheYear = m.SCD.year
        regId = px.l2region.regId
        k = (multiplierName, regId, forName, year)
        v = cache.multCache.get(k)
        if v is None:
            multiplier = getForData(m, multiplierName, regId, forName, "", year)
            if multiplierName == "mortCoef_multiplier":
                multiplier = cpow(multiplier, getDoubleSetting(m, "mortMultiplier_link", DATA_NOW))
            v = cache.multCache[k] = multiplier
        return v * multiplierSpVar
    return getSTData(m, px, multiplierName, forName, year, "", 1.0)


def getPathMortality(m, px, forType, dC, year=DATA_NOW):
    """Pixel::getPathMortality(): increased mortality due to the pathogens present in the pixel"""
    if not m.cache.usePathogenModule:
        return 0.0
    initialOptYear = getIntSetting(m, "initialOptYear")
    simulationYears = getIntSetting(m, "simulationYears")
    maxYear = initialOptYear + simulationYears
    rules = getPathMortalityRule(m.MD, forType, dC)
    pathMort = 0.0
    if year == DATA_NOW:
        year = m.SCD.year
    for r in rules:
        pathMort_thispath = 0.0
        nc = len(r.mortCoefficents)
        for y in range(year, year - nc, -1):
            i = year - y
            y2 = maxYear - 1 if y >= maxYear else y
            layerName = f"pathogen_pp#{r.pathId}#{y2}"
            if layerExist(m.GIS, layerName):
                if getDoubleValue(m.GIS, px, layerName, True) >= r.pres_min:
                    pathMort_thispath = r.mortCoefficents[i]
        pathMort += pathMort_thispath
    return pathMort


def getPastRegArea(px, ft_idx, year):
    """ft_idx is 0-based"""
    v = px.regArea.get(year)
    if v is None:
        msgOut(MSG_ERROR, f"Asking for a pastRegArea of a not-registered year. I don't have year {year}!")
        return 0.0
    return v[ft_idx]


# --- layers ---

def layerExist(G, name):
    return name in G.layerIndex


def getLayer(G, name):
    i = G.layerIndex.get(name)
    if i is None:
        msgOut(MSG_CRITICAL_ERROR, f"Layer {name} not found. Aborting.")
    return G.layerVector[i]


def addLayer(G, name, label, isInteger, dynamicContent, fullFileName="", display=True):
    if layerExist(G, name):
        msgOut(MSG_ERROR, "Layer already exist with that name")
        return
    G.layerVector.append(Layer(name, label, isInteger, dynamicContent, fullFileName, display, []))
    G.layerIndex[name] = len(G.layerVector) - 1
    # every pixel gets the noValue (map insert: existing keys are not changed)
    v = G.values.get(name)
    if v is None:
        _addKey(G, name, _filled(G, G.noValue))
    else:
        pr = G.present.get(name)
        if pr is not None:
            nv = G.noValue
            for i in range(G.xyNPixels):
                if not pr[i]:
                    v[i] = nv
            del G.present[name]


def resetLayer(G, name):
    if layerExist(G, name):
        G.values[name][:] = _filled(G, G.noValue)
        return
    msgOut(MSG_ERROR, f"I could not reset layer {name} as it doesn't exist!")


def addLegendItem(G, name, ID, label, r, g, b, minV, maxV):
    if not layerExist(G, name):
        msgOut(MSG_ERROR, "Trying to add a legend item to a layer that doesn't exist.")
        return
    L = getLayer(G, name)
    for li in L.legendItems:
        if li.ID == ID:
            msgOut(MSG_ERROR, f"Trying to add a legend item that already exist on this layer (layer: {L.label} - legend label: {label})")
            return
    L.legendItems.append(LegendItem(ID, label, r, g, b, float(minV), float(maxV)))


def addLegendItems(L, items):
    toAdd = []
    for it in items:
        if any(li.ID == it.ID for li in L.legendItems):
            msgOut(MSG_WARNING, f"Legend item {it.ID} non added on layer {L.name} as already existing.")
        else:
            toAdd.append(it)
    L.legendItems.extend(toAdd)


def _truncInt(v):
    """C++ (int) conversion of a double (inf/nan give INT_MIN on x86)"""
    return int(v) if math.isfinite(v) else -2147483648


def getColor(G, L, v):
    nocolor = (255, 255, 255)
    if v == G.noValue:
        return nocolor
    if L.isInteger:
        iv = _truncInt(v)
        for li in L.legendItems:
            if li.ID == iv:
                return (li.rColor, li.gColor, li.bColor)
    else:
        for li in L.legendItems:
            if li.minValue <= v < li.maxValue:
                return (li.rColor, li.gColor, li.bColor)
    return nocolor


# --- space initialisation ---

def _local(tag):
    return tag.rsplit("}", 1)[-1] if isinstance(tag, str) else ""


def _descendants(el, name):
    return [d for d in el.iter() if d is not el and _local(d.tag) == name]


def _xmlText(el, name):
    for d in el.iter():
        if d is not el and _local(d.tag) == name:
            return "".join(d.itertext())
    return ""


def _xmlBool(el, name):
    c = _xmlText(el, name)
    if c in ("false", "falso", "FALSE", "0"):
        return False
    if c in ("true", "vero", "TRUE", "1"):
        return True
    msgOut(MSG_WARNING, f"Sorry, I don't know how to convert {c} to a bool value. I return true... hope for the best")
    return True


def _xmlAttr(el, name):
    for k, v in el.attrib.items():
        if _local(k) == name:
            return v
    return None


def initLayers(m):
    G = m.GIS
    MD = m.MD
    filename = m.baseDirectory + getStringSetting(m, "gisFilename")
    if not os.path.isfile(filename):
        msgOut(MSG_CRITICAL_ERROR, f"Error opening the gis file {filename}.")
    root = ET.parse(filename).getroot()
    ftIds = getForTypeIds(MD)
    for layerNode in [d for d in root.iter() if _local(d.tag) == "layer"]:
        nameOrig = _xmlText(layerNode, "name")
        labelOrig = _xmlText(layerNode, "label")
        isInteger = _xmlBool(layerNode, "isInteger")
        dynamicContent = _xmlBool(layerNode, "dynamicContent")
        expandByFt = _xmlBool(layerNode, "expandByFt")
        if _xmlText(layerNode, "readAtStart") != "true":
            continue
        dirName = _xmlText(layerNode, "dirName")
        fileName = _xmlText(layerNode, "fileName")
        nLoop = len(ftIds) if expandByFt else 1
        for z in range(nLoop):
            ftExt = "_" + ftIds[z] if expandByFt else ""
            labelExt = " (" + ftIds[z] + ")" if expandByFt else ""
            name = nameOrig + ftExt
            label = labelOrig + labelExt
            fullFileName = "" if (dirName == "" or fileName == "") else MD.baseDirectory + dirName + fileName + ftExt
            addLayer(G, name, label, isInteger, dynamicContent, fullFileName)
            for li in _descendants(layerNode, "legendItem"):
                lID = s2i("".join(li.itertext()).strip())

                def at(n):
                    a = _xmlAttr(li, n)
                    if a is None:
                        msgOut(MSG_ERROR, f"Element doens't have attribute {n}")
                        return ""
                    return a
                llabel = at("label")
                r, g, b = s2i(at("rColor")), s2i(at("gColor")), s2i(at("bColor"))
                if isInteger:
                    minV = maxV = float(lID)
                else:
                    minV, maxV = s2d(at("minValue")), s2d(at("maxValue"))
                addLegendItem(G, name, lID, llabel, r, g, b, minV, maxV)
    initLayersPixelData(m)


def initLayersPixelData(m):
    G = m.GIS
    MD = m.MD
    if not getBoolSetting(m, "usePixelData"):
        return

    def addDirLayers(dir_, ext):
        for f in getFilenamesByDir(dir_, ext):
            name = f[:f.rfind(".")]
            addLayer(G, name, name, False, False, dir_ + "/" + f, False)
    addDirLayers(MD.baseDirectory + getStringSetting(m, "spatialDataSubfolder"), getStringSetting(m, "spatialDataFileExtension"))
    if getBoolSetting(m, "useSpExplicitForestTypes"):
        addDirLayers(MD.baseDirectory + getStringSetting(m, "spExplicitForTypesInputDir"), getStringSetting(m, "spExplicitForTypesFileExtension"))
    if getBoolSetting(m, "usePathogenModule"):
        addDirLayers(MD.baseDirectory + getStringSetting(m, "pathogenExogenousSpreadModelFolder"), getStringSetting(m, "pathogenExogenousSpreadModelFileExtension"))
    addLayer(G, "pixelIds", "Pixel IDs", True, False, "", True)
    # In the C++ code getAllPlots(false) is called here, but as the pixels are not yet assigned to the regions
    # (and because of a "masked = false" assignment in the condition) it returns an empty vector, so the
    # pixelIds layer remains filled with noValue.
    msgOut(MSG_WARNING, "We can't find any free plot.")


def randomShuffle(m, L):
    """Layers::randomShuffle(): randomly reassign the values of a layer among its pixels with data (the no-value
    mask is kept). Used, for sensitivity analysis only, when `initialRandomShuffle` is true.

    As in the C++ code it applies to every layer read from a file, including the region layers. It does not
    replicate the C++ draws: it uses Python's random.shuffle with a dedicated generator (fixed seed, or a
    random one with `newRandomSeed`)."""
    G = m.GIS
    vals = G.values[L.name]
    idx = [i for i, v in enumerate(vals) if v != G.noValue]
    sub = [vals[i] for i in idx]
    m.shuffleRng.shuffle(sub)
    for i, v in zip(idx, sub):
        vals[i] = v


def _qdouble(s):
    try:
        return float(s) if "_" not in s else 0.0
    except ValueError:
        return 0.0


def _qint(s):
    try:
        return int(s) if "_" not in s else 0
    except ValueError:
        return 0


def loadLayersDataFromFile(m):
    """Read the GRASS/ArcInfo ASCII grids associated to the layers (Gis::loadLayersDataFromFile)"""
    G = m.GIS
    initialRandomShuffle = getBoolSetting(m, "initialRandomShuffle")
    localNoValue = G.noValue  # NOTE: as in the C++ code it persists across the files
    nx = G.xNPixels
    for L in G.layerVector:
        if L.fullFileName == "":
            continue
        if not os.path.isfile(L.fullFileName):
            print("Cannot open file for reading: No such file or directory", file=sys.stderr)
            msgOut(MSG_ERROR, f"Cannot open map file {L.fullFileName} for reading.")
            continue
        vals = G.values[L.name]
        noValue = G.noValue
        countRow = 0
        with open(L.fullFileName, encoding="latin-1") as f:
            for line in f:
                fields = line.rstrip("\n").split(" ")   # QString::split keeps the empty parts
                f0 = fields[0]
                f1 = fields[1] if len(fields) > 1 else ""
                if ((f0 == "north:" and _qdouble(f1) != G.geoTopY) or
                        (f0 in ("south:", "yllcorner") and _qdouble(f1) != G.geoBottomY) or
                        (f0 == "east:" and _qdouble(f1) != G.geoRightX) or
                        (f0 in ("west:", "xllcorner") and _qdouble(f1) != G.geoLeftX) or
                        (f0 in ("rows:", "nrows") and _qint(f1) != G.yNPixels) or
                        (f0 in ("cols:", "ncols") and _qint(f1) != G.xNPixels)):
                    msgOut(MSG_ERROR, f"Layer {L.name} has different coordinates. Aborting reading.")
                    break
                elif f0 in ("null:", "NODATA_value", "nodata_value"):
                    localNoValue = _qdouble(f1)
                elif len(fields) > 5:
                    base = countRow * nx
                    for c in range(nx):
                        try:
                            v = float(fields[c])
                        except ValueError:
                            v = 0.0
                        vals[base + c] = noValue if v == localNoValue else v  # filterExogenousDataset() is the identity
                    countRow += 1
        if initialRandomShuffle:
            randomShuffle(m, L)
        # filterSubRegion()
        G.subXL, G.subYT, G.subXR, G.subYB = 0, 0, G.xNPixels - 1, G.yNPixels - 1


def setMyPixels(m, reg):
    G = m.GIS
    l1 = G.values["regLev_1"]
    l2 = G.values["regLev_2"]
    rid = reg.regId
    for i in range(G.xyNPixels):
        if l1[i] == rid or l2[i] == rid:
            reg.myPixels.append(i)
            if reg.regLevel == 2:
                G.pxVector[i].l2region = reg


def regPixels(m, reg):
    pv = m.GIS.pxVector
    return [pv[i] for i in reg.myPixels]


def applySpatialStochasticValues(m):
    MD = m.MD
    if not getBoolSetting(m, "usePixelData"):
        return
    fTypes = getForTypeIds(MD)
    for regId in getRegionIds(MD, 2):
        rpx = regPixels(m, getRegion(MD, regId))
        if getBoolSetting(m, "useSpatialRegionalVariance"):
            for j, ft in enumerate(fTypes):
                sStDev = getForData(m, "sStDev", regId, ft, "")
                agr = getForData(m, "agr", regId, ft, "")
                d = NormalDist(1.0, sStDev / agr)
                for px in rpx:
                    c = d(m.gen)
                    px.spMods[j] = cmax(0.4, cmin(1.6, c))
        avgExpTypes = getDoubleSetting(m, "expType", DATA_NOW, regId)
        avgExpTypesPrices = getDoubleSetting(m, "expTypePrices", DATA_NOW, regId)
        expTypes_cv = getDoubleSetting(m, "expType_cv", DATA_NOW, regId)
        expTypesPrices_cv = getDoubleSetting(m, "expTypePrices_cv", DATA_NOW, regId)
        if (avgExpTypes < 0 or avgExpTypes > 1) and avgExpTypes != -1:
            msgOut(MSG_CRITICAL_ERROR, "expType parameter must be between 1 (expectations) and 0 (adaptative) or -1 (fixed).")
        if avgExpTypesPrices < 0 or avgExpTypesPrices > 1:
            msgOut(MSG_CRITICAL_ERROR, "vgExpTypesPrices parameter must be between 1 (expectations) and 0 (adaptative).")
        exp_distr = NormalDist(avgExpTypes, expTypes_cv * avgExpTypes)
        expPrices_distr = NormalDist(avgExpTypesPrices, expTypesPrices_cv * avgExpTypesPrices)
        for px in rpx:
            if avgExpTypes == -1:
                px.expType = -1.0
            else:
                px.expType = normSample(exp_distr, m.gen, 0.0, 1.0)
            px.expTypePrices = normSample(expPrices_distr, m.gen, 0.0, 1.0)


def applyStochasticRiskAdversion(m):
    MD = m.MD
    if not getBoolSetting(m, "usePixelData"):
        return
    raEnabled = getBoolSetting(m, "heterogeneousRiskAversion")
    for regId in getRegionIds(MD, 2):
        for px in regPixels(m, getRegion(MD, regId)):
            if raEnabled:
                raStDev = getDoubleSetting(m, "riskAversionAgentSd", DATA_NOW, regId)
                avg = getDoubleSetting(m, "riskAversionAgentAverage", DATA_NOW, regId)
                d = NormalDist(avg, raStDev)
                setValue(m.GIS, px, "ra", d(m.gen))
            else:
                setValue(m.GIS, px, "ra", 0.0)


def setSpace(m):
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
    nft = len(getForTypeIds(m.MD))
    for i in range(G.yNPixels):
        for j in range(G.xNPixels):
            px = Pixel(i * G.xNPixels + j, nft)
            px.pxX = j
            px.pxY = i
            G.pxVector.append(px)
    initLayers(m)
    loadLayersDataFromFile(m)
    for reg in getAllRegions(m.MD):
        setMyPixels(m, reg)
    applySpatialStochasticValues(m)
    applyStochasticRiskAdversion(m)


def updateImage(m, layerName):
    # no GUI: we only keep the debug message of the C++ code
    msgOut(MSG_DEBUG, f"Update image {layerName}...")
    if not layerExist(m.GIS, layerName):
        msgOut(MSG_CRITICAL_ERROR, "Layer not found in Gis::updateImage()")


def applyForestReclassification(m):
    G = m.GIS
    MD = m.MD
    RRs = MD.reclRules
    for FT in MD.forTypes:
        if not layerExist(G, FT.forLayer):
            addLayer(G, FT.forLayer, "Are layer for forest type " + FT.forTypeId, False, True)
            resetLayer(G, FT.forLayer)
            newLayer = getLayer(G, FT.forLayer)
            ereditated = getLayer(G, getForType(MD, FT.ereditatedFrom).forLayer)
            addLegendItems(newLayer, list(ereditated.legendItems))
    regL2v = G.values["regLev_2"]
    nv = G.noValue
    for FT in MD.forTypes:
        target = G.values[FT.forLayer]
        rules = [(RR.regId, G.values[getForType(MD, RR.forTypeIn).forLayer], RR.coeff) for RR in RRs if RR.forTypeOut == FT.forTypeId]
        for j in range(G.xyNPixels):
            regL2 = _truncInt(regL2v[j])
            v0 = target[j]
            value = 0.0 if v0 == nv else v0
            for rid, vinArr, coeff in rules:
                if rid == regL2:
                    vin = vinArr[j]
                    value += (0.0 if vin == nv else vin) * coeff
            target[j] = nv if value == 0 else value
        updateImage(m, FT.forLayer)
