"""Port of ModelData (settings, regions, forest/product data and their regional overriding), ModelRegion and
the small data structs."""

import math
import os

from .utils import (msgOut, s2i, s2d, s2b, getType, cpow, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR,
                    MSG_CRITICAL_ERROR, MSG_NO_MSG, WORLD, DATA_NOW, TYPE_INT, TYPE_DOUBLE, TYPE_STRING, TYPE_BOOL,
                    PROD_PRI, PROD_SEC, PROD_ALL, DIAM_ALL, DIAM_PROD, DIAM_FIRST, FT_ALL)
from .odsreader import LLData, getData, nrecords, nheaders, getTableFromFile
from .datamap import DataMap, mapinsert, mapreplace, mapdelete, sortedkeys, exactValues, prefixValues


class ScenarioData:
    __slots__ = ("id", "shortDesc", "longDesc", "settingTable", "forDataTable", "prodDataTable", "forToProdTable",
                 "pathTable", "settingFile")

    def __init__(self, *args):
        args = args or ("",) * 9
        for k, v in zip(self.__slots__, args):
            setattr(self, k, v)


class BasicData:
    __slots__ = ("name", "values", "type", "regId", "comment")

    def __init__(self, name, values, type_, regId, comment):
        self.name = name
        self.values = values
        self.type = type_
        self.regId = regId
        self.comment = comment


class ForToProd:
    __slots__ = ("product", "forType", "dClass", "maxYears")

    def __init__(self, product, forType, dClass, maxYears):
        self.product = product
        self.forType = forType
        self.dClass = dClass
        self.maxYears = maxYears


class ForType:
    __slots__ = ("forTypeId", "forLabel", "memType", "forLayer", "ereditatedFrom")

    def __init__(self, forTypeId, forLabel, memType, forLayer, ereditatedFrom):
        self.forTypeId = forTypeId
        self.forLabel = forLabel
        self.memType = memType
        self.forLayer = forLayer
        self.ereditatedFrom = ereditatedFrom


class ReclRule:
    __slots__ = ("regId", "forTypeIn", "forTypeOut", "coeff")

    def __init__(self, regId, forTypeIn, forTypeOut, coeff):
        self.regId = regId
        self.forTypeIn = forTypeIn
        self.forTypeOut = forTypeOut
        self.coeff = coeff


class PathRule:
    __slots__ = ("forType", "dClass", "pathId", "pres_min", "mortCoefficents")

    def __init__(self, forType, dClass, pathId, pres_min, mortCoefficents):
        self.forType = forType
        self.dClass = dClass
        self.pathId = pathId
        self.pres_min = pres_min
        self.mortCoefficents = mortCoefficents


class ModelRegion:
    __slots__ = ("regId", "regSName", "regLName", "regLevel", "parRegId", "isResidual", "parent", "chRegions",
                 "myPixels", "inResByAnyCombination")

    def __init__(self, regId, regSName, regLName, regLevel, parRegId, isResidual, nInBounds):
        self.regId = regId
        self.regSName = regSName
        self.regLName = regLName
        self.regLevel = regLevel
        self.parRegId = parRegId
        self.isResidual = isResidual
        self.parent = None
        self.chRegions = []
        self.myPixels = []                       # pixel IDs (0-based indices in the pixel vector)
        self.inResByAnyCombination = [0.0] * nInBounds


def getChildren(r, excludeResidual=True):
    return [c for c in r.chRegions if not c.isResidual] if excludeResidual else r.chRegions


def getNChildren(r, excludeResidual=True):
    return len(getChildren(r, excludeResidual))


class ModelData:
    def __init__(self, baseDirectory):
        self.baseDirectory = baseDirectory
        self.outputDirname = ""
        self.tables = []
        self.scenario = ScenarioData()
        self.settings = []
        self.settingsIndex = {}
        self.forDataMap = DataMap()
        self.prodDataMap = DataMap()
        self.forToProdVector = []
        self.forToProdIndex = {}
        self.regionsVector = []
        self.regionIndex = {}
        self.forTypes = []
        self.reclRules = []
        self.pathRules = []
        self.deathTimberInventory = {}
        self.diamClasses = []
        self.cached_initialYear = 0
        self.priProducts = []
        self.secProducts = []
        self.allProducts = []
        self.tempBool = True
        self.valueFoundBool = True
        self.errorLevel = MSG_ERROR
        # caches (not in the C++ code, for performance only)
        self.forTypeIdsCache = {}


# ------------------------------------------------------------------------------
# Tables & scenarios
# ------------------------------------------------------------------------------

def getTable(MD, tableName, debugLevel=MSG_CRITICAL_ERROR):
    for t in MD.tables:
        if t.tableName == tableName:
            return t
    msgOut(debugLevel, f"No table found with name {tableName}")
    return LLData("", [], [])


def getScenarios(MD):
    table = getTable(MD, "scenarios")
    return [getData(table, i, "id") for i in range(nrecords(table))]


def setScenarioData(m, MD):
    table = getTable(MD, "scenarios")
    for i in range(nrecords(table)):
        if getData(table, i, "id") == m.scenarioName:
            MD.scenario = ScenarioData(*(getData(table, i, h) for h in ScenarioData.__slots__))
            return
    msgOut(MSG_CRITICAL_ERROR, "Scenario not found!")


# ------------------------------------------------------------------------------
# Settings
# ------------------------------------------------------------------------------

def _readSettingRecord(table, i):
    name = getData(table, i, "name")
    type_ = getType(getData(table, i, "type"))
    region_s = getData(table, i, "region")
    regId = WORLD if region_s == "" else s2i(region_s)
    comment = getData(table, i, "comment")
    values = []
    for z in range(nheaders(table) - 4):   # don't consider name, type, comment and region headers
        v = getData(table, i, f"value_{z}")
        if v != "":
            values.append(v)
    return BasicData(name, values, type_, regId, comment)


def _reindexSettings(MD):
    MD.settingsIndex.clear()
    for i, s in enumerate(MD.settings):
        MD.settingsIndex.setdefault((s.name, s.regId), i)


def setOutputDirectory(MD, dirname):
    MD.outputDirname = MD.baseDirectory + "output/" if not dirname else dirname


def setDefaultSettings(m, MD):
    table = getTable(MD, "settings")
    for i in range(nrecords(table)):
        MD.settings.append(_readSettingRecord(table, i))
    _reindexSettings(MD)
    msgOut(MSG_INFO, f"### USING SCENARIO: {m.scenarioName} ###")
    setOutputDirectory(MD, getStringSetting(m, "outputDirname"))


def setScenarioSettings(m, MD):
    sc = MD.scenario
    if sc.settingTable == "" and sc.settingFile == "":
        return
    if sc.settingTable != "" and sc.settingFile != "":
        msgOut(MSG_CRITICAL_ERROR, "Scenario-specific settings has been indicated to be loaded from BOTH from the spreadsheet and a file. Choose one of them.")
    table = getTable(MD, sc.settingTable) if sc.settingTable != "" else getTableFromFile("settings", MD.baseDirectory + sc.settingFile)
    for i in range(nrecords(table)):
        s = _readSettingRecord(table, i)
        for existing in MD.settings:
            if existing.name == s.name and existing.regId == s.regId:
                existing.values = s.values
                existing.type = s.type
                existing.comment = s.comment
                break
        else:
            MD.settings.append(s)
    _reindexSettings(MD)
    setOutputDirectory(MD, getStringSetting(m, "outputDirname"))


def _settingRegIds(MD, regId_h):
    """The region chain WORLD / L1 / L2 used to look up a setting"""
    if regId_h == WORLD:
        return (WORLD,)
    reg = getRegion(MD, regId_h)
    if reg.regLevel == 1:
        return (regId_h, WORLD)
    if reg.regLevel == 2:
        return (regId_h, reg.parRegId, WORLD)
    msgOut(MSG_CRITICAL_ERROR, "Error in getBaseData(). Data requested for a unknown region level.")


def getBaseData(m, name, type_h, position=0, regId_h=WORLD):
    MD = m.MD
    if position == DATA_NOW:
        position = m.SCD.iteration
    for regId in _settingRegIds(MD, regId_h):
        i = MD.settingsIndex.get((name, regId))
        if i is not None:
            s = MD.settings[i]
            if s.type != type_h:
                msgOut(MSG_CRITICAL_ERROR, f"mismatching type in calling getBaseData() for {name}")
            if len(s.values) > position:
                return s.values[position]
            if s.values:
                return s.values[-1]
            msgOut(MSG_CRITICAL_ERROR, f"Error: {name} doesn't have any value, even on the first position(year)!")
    msgOut(MSG_CRITICAL_ERROR, f"Error calling getBaseData() for {name}. No setting option or macro data found with this name.")
    return ""


def getVectorBaseData(m, name, type_h, regId_h=WORLD):
    MD = m.MD
    for regId in _settingRegIds(MD, regId_h):
        i = MD.settingsIndex.get((name, regId))
        if i is not None:
            s = MD.settings[i]
            if s.type != type_h:
                msgOut(MSG_CRITICAL_ERROR, f"mismatching type in calling getVectorBaseData() for {name}")
            return list(s.values)
    msgOut(MSG_CRITICAL_ERROR, f"Error calling getVectorBaseData() for {name}. No setting option or macro data found with this name.")
    return []


def getIntSetting(m, name, position=0, reg=WORLD):
    return s2i(getBaseData(m, name, TYPE_INT, position, reg))


def getDoubleSetting(m, name, position=0, reg=WORLD):
    return s2d(getBaseData(m, name, TYPE_DOUBLE, position, reg))


def getStringSetting(m, name, position=0, reg=WORLD):
    return getBaseData(m, name, TYPE_STRING, position, reg)


def getBoolSetting(m, name, position=0, reg=WORLD):
    return s2b(getBaseData(m, name, TYPE_BOOL, position, reg))


def getIntVectorSetting(m, name, reg=WORLD):
    return [s2i(x) for x in getVectorBaseData(m, name, TYPE_INT, reg)]


def getDoubleVectorSetting(m, name, reg=WORLD):
    return [s2d(x) for x in getVectorBaseData(m, name, TYPE_DOUBLE, reg)]


def getStringVectorSetting(m, name, reg=WORLD):
    return getVectorBaseData(m, name, TYPE_STRING, reg)


def getBoolVectorSetting(m, name, reg=WORLD):
    return [s2b(x) for x in getVectorBaseData(m, name, TYPE_BOOL, reg)]


def cacheSettings(m, MD):
    MD.cached_initialYear = getIntSetting(m, "initialYear")
    MD.diamClasses = getStringVectorSetting(m, "dClasses")
    MD.priProducts = getStringVectorSetting(m, "priProducts")
    MD.secProducts = getStringVectorSetting(m, "secProducts")
    MD.allProducts = MD.priProducts + MD.secProducts


# ------------------------------------------------------------------------------
# Regions
# ------------------------------------------------------------------------------

def createRegions(m, MD):
    table = getTable(MD, "regions")
    nInBounds = 2 ** len(getStringVectorSetting(m, "priProducts"))
    for i in range(nrecords(table)):
        reg = ModelRegion(s2i(getData(table, i, "regId")), getData(table, i, "regSName"), getData(table, i, "regLName"),
                          s2i(getData(table, i, "regLevel")), s2i(getData(table, i, "parRegId")),
                          s2b(getData(table, i, "isResidual")), nInBounds)
        MD.regionsVector.append(reg)
        MD.regionIndex.setdefault(reg.regId, len(MD.regionsVector) - 1)
    for r in MD.regionsVector:
        r.parent = getRegion(MD, r.parRegId)
        r.chRegions = [c for c in MD.regionsVector if c.parRegId == r.regId]


def getRegion(MD, regId):
    i = MD.regionIndex.get(regId)
    if i is None:
        msgOut(MSG_CRITICAL_ERROR, f"Region id {regId} not found, check your input data. Aborting simulation.")
    return MD.regionsVector[i]


def regionExist(MD, regId):
    return regId in MD.regionIndex


def getRegionIds(MD, level, excludeResidual=True):
    return [r.regId for r in MD.regionsVector if r.regLevel == level and (not excludeResidual or not r.isResidual)]


def getAllRegions(MD, excludeResidual=True):
    return [r for r in MD.regionsVector if not excludeResidual or not r.isResidual]


def getL2r(MD, excludeResidual=True):
    """The l2r structure: for each (non residual) level-1 region, its (non residual) level-2 children ids"""
    out = []
    for l1 in getRegionIds(MD, 1, excludeResidual):
        ch = [c.regId for c in getChildren(getRegion(MD, l1), excludeResidual)]
        if ch:
            out.append(ch)
    return out


def regId2RegSName(MD, regId):
    return getRegion(MD, regId).regSName


def getSiblings(MD, reg, excludeResidual=True):
    return [r for r in getAllRegions(MD, excludeResidual) if r.parRegId == reg.parRegId and r.regId != reg.regId]


# ------------------------------------------------------------------------------
# Forest types, io matrix, reclassification and pathogen rules
# ------------------------------------------------------------------------------

def getForTypeIds(MD, all_=False):
    c = MD.forTypeIdsCache.get(all_)
    if c is not None:
        return c
    v = [ft.forTypeId for ft in MD.forTypes if ft.memType != 1 or all_]
    MD.forTypeIdsCache[all_] = v
    return v


def getForType(MD, id_):
    for ft in MD.forTypes:
        if ft.forTypeId == id_:
            return ft
    msgOut(MSG_CRITICAL_ERROR, f"forTypeId {id_} not found. Aborting.")


def getForTypeParentId(MD, id_):
    for ft in MD.forTypes:
        if ft.forTypeId == id_:
            return ft.ereditatedFrom
    msgOut(MSG_CRITICAL_ERROR, f"forTypeId {id_} not found. Aborting.")


def getForTypeChilds(MD, id_):
    return [ft.forTypeId for ft in MD.forTypes if ft.ereditatedFrom == id_]


def getForTypeChilds_pos(MD, id_, all_=False):
    """0-based positions in getForTypeIds(all) of the children of a forest type"""
    ids = getForTypeIds(MD, all_)
    return [i for i in range(len(ids)) if getForType(MD, ids[i]).ereditatedFrom == id_]


def getForTypeParents(MD):
    parents = []
    for ft in MD.forTypes:
        p = ft.ereditatedFrom
        if p not in parents and p != "":
            parents.append(p)
    return parents


def getNForTypesChilds(MD, id_):
    return sum(1 for ft in MD.forTypes if ft.ereditatedFrom == id_)


def setForestTypes(m, MD):
    table = getTable(MD, "forTypes")
    for i in range(nrecords(table)):
        ft = ForType(getData(table, i, "forTypeId"), getData(table, i, "forLabel"), s2i(getData(table, i, "memType")),
                     getData(table, i, "forLayer"), getData(table, i, "ereditatedFrom"))
        if ft.memType == 3 and not getBoolSetting(m, "useSpExplicitForestTypes"):
            continue
        MD.forTypes.append(ft)
    MD.forTypeIdsCache.clear()


def setReclassificationRules(m, MD):
    msgOut(MSG_DEBUG, "Loading (but not yet applying) reclassification rules..")
    table = getTable(MD, "reclRules")
    for i in range(nrecords(table)):
        MD.reclRules.append(ReclRule(s2i(getData(table, i, "regID")), getData(table, i, "forTypeIn"),
                                     getData(table, i, "forTypeOut"), s2d(getData(table, i, "coeff"))))


def _readPathRule(table, i):
    values = []
    for z in range(nheaders(table) - 4):
        v = getData(table, i, f"year_{z}")
        if v != "":
            values.append(s2d(v))
    return PathRule(getData(table, i, "forType"), getData(table, i, "dClass"), getData(table, i, "path_name"),
                    s2d(getData(table, i, "pres_min")), values)


def setDefaultPathogenRules(m, MD):
    if not getBoolSetting(m, "usePathogenModule"):
        return
    msgOut(MSG_DEBUG, "Loading pathogen rules..")
    table = getTable(MD, "pathRules")
    for i in range(nrecords(table)):
        MD.pathRules.append(_readPathRule(table, i))


def setScenarioPathogenRules(m, MD):
    if MD.scenario.pathTable == "":
        return
    table = getTable(MD, MD.scenario.pathTable)
    for i in range(nrecords(table)):
        PR = _readPathRule(table, i)
        for r in MD.pathRules:
            if r.forType == PR.forType and r.dClass == PR.dClass and r.pathId == PR.pathId:
                r.pres_min = PR.pres_min
                r.mortCoefficents = PR.mortCoefficents
                break
        else:
            MD.pathRules.append(PR)


def getPathMortalityRule(MD, ft, dc):
    return [r for r in MD.pathRules if r.forType == ft and r.dClass == dc]


def _setF2P(MD, table):
    MD.forToProdVector = []
    MD.forToProdIndex = {}
    for i in range(nrecords(table)):
        f = ForToProd(getData(table, i, "product"), getData(table, i, "forType"), getData(table, i, "dClass"),
                      s2i(getData(table, i, "maxYears")))
        MD.forToProdVector.append(f)
        MD.forToProdIndex.setdefault((f.product, f.forType, f.dClass), len(MD.forToProdVector) - 1)


def setDefaultProductResourceMatrixLink(m, MD):
    msgOut(MSG_DEBUG, "Loading forest resource to primary products io matrix..")
    _setF2P(MD, getTable(MD, "forToProd"))


def setScenarioProductResourceMatrixLink(m, MD):
    if MD.scenario.forToProdTable == "":
        return
    _setF2P(MD, getTable(MD, MD.scenario.forToProdTable))


def assessProdPossibility(MD, prod, ft, dc):
    return (prod, ft, dc) in MD.forToProdIndex


def getMaxYearUsableDeathTimberAll(MD):
    maxMaxYears = 0
    for f in MD.forToProdVector:
        if f.maxYears > maxMaxYears:
            maxMaxYears = f.maxYears
    return maxMaxYears


def getMaxYearUsableDeathTimber(MD, prod, ft, dc):
    i = MD.forToProdIndex.get((prod, ft, dc))
    if i is None:
        msgOut(MSG_CRITICAL_ERROR, "In getMaxYearUsableDeathTimber() I has been asked of a combination that I don't know how to handle.")
    return MD.forToProdVector[i].maxYears


# ------------------------------------------------------------------------------
# Forest and product data
# ------------------------------------------------------------------------------

def makeKeyProdData(parName, regId, prod, freeDim=""):
    return f"{parName}#{regId}#{prod}#{freeDim}#"


def makeKeyForData(parName, regId, forType, diamClass):
    return f"{parName}#{regId}#{forType}#{diamClass}#"


def _readDataValues(table, i):
    values = []
    rec = table.records[i]
    for z in range(nheaders(table) - 4):
        j = table.hIndex.get(f"value_{z}")
        if j is None:
            msgOut(MSG_CRITICAL_ERROR, f"Header string value_{z} not found!")
        v = rec[j]
        if v != "":
            values.append(s2d(v))
    return values


def setDefaultForData(m, MD):
    msgOut(MSG_DEBUG, "Loading forest sector data..")
    table = getTable(MD, "forData")
    for i in range(nrecords(table)):
        values = _readDataValues(table, i)
        key = makeKeyForData(getData(table, i, "parName"), getData(table, i, "region"), getData(table, i, "forType"), getData(table, i, "freeDim"))
        mapinsert(MD.forDataMap, key, values)


def setScenarioForData(m, MD):
    if MD.scenario.forDataTable == "":
        return
    table = getTable(MD, MD.scenario.forDataTable)
    for i in range(nrecords(table)):
        key = makeKeyForData(getData(table, i, "parName"), getData(table, i, "region"), getData(table, i, "forType"), getData(table, i, "freeDim"))
        values = _readDataValues(table, i)
        if key in MD.forDataMap:
            mapreplace(MD.forDataMap, key, values)
        else:
            mapinsert(MD.forDataMap, key, values)


def setDefaultProdData(m, MD):
    msgOut(MSG_DEBUG, "Loading products data..")
    table = getTable(MD, "prodData")
    for i in range(nrecords(table)):
        values = _readDataValues(table, i)
        key = makeKeyProdData(getData(table, i, "parName"), getData(table, i, "region"), getData(table, i, "prod"), getData(table, i, "freeDim"))
        mapinsert(MD.prodDataMap, key, values)


def setScenarioProdData(m, MD):
    if MD.scenario.prodDataTable == "":
        return
    table = getTable(MD, MD.scenario.prodDataTable)
    for i in range(nrecords(table)):
        key = makeKeyProdData(getData(table, i, "parName"), getData(table, i, "region"), getData(table, i, "prod"), getData(table, i, "freeDim"))
        values = _readDataValues(table, i)
        if key in MD.prodDataMap:
            mapreplace(MD.prodDataMap, key, values)
        else:
            mapinsert(MD.prodDataMap, key, values)


def _unpackKey(key):
    parts = key.split("#")
    if len(parts) < 5:
        msgOut(MSG_CRITICAL_ERROR, "Error in unpacking a key in the map of production data.")
    return parts[0], s2i(parts[1]), parts[2], parts[3]


def _overrideLevel(MD, dm, level, makeKey, what):
    toBeAdded = {}
    toBeAddedOrder = []
    keysToRemove = []
    for k in list(sortedkeys(dm)):
        parName, regId, d1, d2 = _unpackKey(k)
        if not regionExist(MD, regId):
            msgOut(MSG_CRITICAL_ERROR, f"{what} variable has a unknown region code If you want not to model a region, set residual=true.: {parName}\t{regId}")
            continue
        if getRegion(MD, regId).regLevel == level:
            for ch in getChildren(getRegion(MD, regId), False):
                key = makeKey(parName, ch.regId, d1, d2)
                if key not in dm and key not in toBeAdded:
                    toBeAdded[key] = dm.d[k]
                    toBeAddedOrder.append(key)
            keysToRemove.append(k)
    for key in toBeAddedOrder:
        mapinsert(dm, key, list(toBeAdded[key]))
    for k in keysToRemove:
        mapdelete(dm, k)


def _overrideReclRules(MD, isOverridable):
    i = 0
    while i < len(MD.reclRules):
        rr = MD.reclRules[i]
        if not regionExist(MD, rr.regId):
            msgOut(MSG_CRITICAL_ERROR, f"Reclassification rule has a unknown region code. If you want not to model a region, set residual=true.: {rr.regId}")
            i += 1
            continue
        if isOverridable(rr):
            reg = getRegion(MD, rr.regId)
            for ch in getChildren(reg, False):
                found = any(z.regId == ch.regId and z.forTypeIn == rr.forTypeIn and z.forTypeOut == rr.forTypeOut for z in MD.reclRules)
                if not found:
                    MD.reclRules.append(ReclRule(ch.regId, rr.forTypeIn, rr.forTypeOut, rr.coeff))
            del MD.reclRules[i]
        else:
            i += 1


def applyOverrides(m, MD):
    """Cancel all reg0/reg1 level data and transform them in reg2 level data if not already existing"""
    if not getBoolSetting(m, "applyOverriding"):
        return
    msgOut(MSG_INFO, "Starting regional overriding analysis..")
    _overrideLevel(MD, MD.forDataMap, 0, makeKeyForData, "Forest")
    _overrideLevel(MD, MD.forDataMap, 1, makeKeyForData, "Forest")
    _overrideLevel(MD, MD.prodDataMap, 0, makeKeyProdData, "Product")
    _overrideLevel(MD, MD.prodDataMap, 1, makeKeyProdData, "Product")
    _overrideReclRules(MD, lambda rr: rr.regId == 0)
    _overrideReclRules(MD, lambda rr: getRegion(MD, rr.regId).regLevel == 1)


def applyDebugMode(m, MD):
    if not getBoolSetting(m, "debugFlag"):
        return
    debugRegions = getIntVectorSetting(m, "debugRegions")
    debugPriProducts = getStringVectorSetting(m, "debugPriProducts")
    debugSecProducts = getStringVectorSetting(m, "debugSecProducts")
    for r in MD.regionsVector:
        if r.regLevel == 2 and r.regId not in debugRegions:
            r.isResidual = True
    for s in MD.settings:
        if s.name == "priProducts":
            s.values = list(debugPriProducts)
        elif s.name == "secProducts":
            s.values = list(debugSecProducts)


# --- timed data ---

def getTimedData(m, v, year_h):
    position = (m.SCD.year if year_h == DATA_NOW else year_h) - m.MD.cached_initialYear
    n = len(v)
    if position < 0 and n > 0:
        msgOut(MSG_WARNING, f"getTimedData: year {position + m.MD.cached_initialYear} is before the initial year, returning the value of the initial year.")
        return v[0]
    if n > position:
        return v[position]
    if n > 0:
        return v[-1]       # after the last year: the last available value
    msgOut(MSG_CRITICAL_ERROR, "Error in getTimedData: requested value doesn't have any value, even on the first position(year)!")
    return 0.0


def setTimedData(m, value, v, year_h, MSG_LEVEL=MSG_WARNING):
    position = (m.SCD.year if year_h == DATA_NOW else year_h) - m.MD.cached_initialYear
    if position < 0:
        msgOut(MSG_CRITICAL_ERROR, f"setTimedData: year {position + m.MD.cached_initialYear} is before the initial year.")
    orig = len(v)
    if orig > position:
        v[position] = value
    else:
        v.extend([value] * (position - orig + 1))
        if position - orig > 0:
            msgOut(MSG_LEVEL, f"setTimedData: a dated vector has been filled several years ({1 + position - orig}) with incoming values to reach desidered position in time.")


def _dataRegIds(MD, regId_h, fname):
    """Level-2 regions for a data request on a level 1 or 2 region"""
    reg = getRegion(MD, regId_h)
    if reg.regLevel == 2:
        return [regId_h]
    if reg.regLevel == 1:
        return [c.regId for c in getChildren(reg)]
    msgOut(MSG_CRITICAL_ERROR, f"Error in {fname}. Setting a value for the whole World is not supported.")
    return []


def _productsFor(MD, prodId_h, withEmpty):
    if prodId_h == PROD_PRI:
        return MD.priProducts
    if prodId_h == PROD_SEC:
        return MD.secProducts
    if prodId_h == PROD_ALL or (withEmpty and prodId_h == ""):
        return MD.allProducts + [""] if withEmpty else MD.allProducts
    return [prodId_h]


def _dClassesFor(MD, freeDim_h, withEmpty):
    if freeDim_h == DIAM_ALL:
        return MD.diamClasses + [""] if withEmpty else MD.diamClasses
    if freeDim_h == DIAM_PROD:
        return MD.diamClasses[1:]
    if freeDim_h == DIAM_FIRST:
        return [MD.diamClasses[0]]
    return [freeDim_h]


_PROD_KEYWORDS = {PROD_PRI, PROD_SEC, PROD_ALL, ""}
_DIAM_KEYWORDS = {DIAM_ALL, DIAM_PROD, DIAM_FIRST}


def _isProdKeyword(p):
    return p in _PROD_KEYWORDS


def _isDiamKeyword(d):
    return d in _DIAM_KEYWORDS


def getProdData(m, type_h, regId_h, prodId_h, year=DATA_NOW, freeDim_h=""):
    MD = m.MD
    exactMatch = freeDim_h != ""
    value = 0.0
    found = False
    if prodId_h not in _PROD_KEYWORDS and getRegion(MD, regId_h).regLevel == 2:
        # fast path: a single product in a single region
        if exactMatch:
            v = exactValues(MD.prodDataMap, type_h, regId_h, prodId_h, freeDim_h)
            MD.tempBool = v is not None
            if v is not None:
                value += getTimedData(m, v, year)
                found = True
        else:
            vs = prefixValues(MD.prodDataMap, type_h, regId_h, prodId_h)
            MD.tempBool = bool(vs)
            for v in vs:
                value += getTimedData(m, v, year)
            found = bool(vs)
    else:
        regIds = _dataRegIds(MD, regId_h, "setProdData()")
        products = _productsFor(MD, prodId_h, True)
        for r in regIds:
            for p in products:
                if exactMatch:
                    v = exactValues(MD.prodDataMap, type_h, r, p, freeDim_h)
                    MD.tempBool = v is not None
                    if v is not None:
                        value += getTimedData(m, v, year)
                        found = True
                else:
                    # the last '#' of the full key is removed (bug 20140402): prefix query on type#reg#prod#
                    vs = prefixValues(MD.prodDataMap, type_h, r, p)
                    MD.tempBool = bool(vs)
                    for v in vs:
                        value += getTimedData(m, v, year)
                    if vs:
                        found = True
    if not found:
        MD.valueFoundBool = False
        msgOut(MD.errorLevel, f"Error in getProdData: no combination found for {type_h}, {regId_h}, {prodId_h}, {year}, {freeDim_h}. Returning 0, but double check that this is ok for your model.")
    return value


def getForData(m, type_h, regId_h, forType_h, freeDim_h, year=DATA_NOW):
    MD = m.MD
    value = 0.0
    found = False
    if forType_h != FT_ALL and freeDim_h not in _DIAM_KEYWORDS and getRegion(MD, regId_h).regLevel == 2:
        # fast path: a single key
        v = exactValues(MD.forDataMap, type_h, regId_h, forType_h, freeDim_h)
        MD.tempBool = v is not None
        if v is not None:
            value += getTimedData(m, v, year)
            found = True
    else:
        fTypes = getForTypeIds(MD) + [""] if forType_h == FT_ALL else [forType_h]
        dClasses = _dClassesFor(MD, freeDim_h, True)
        regIds = _dataRegIds(MD, regId_h, "getProdData()")
        for r in regIds:
            for dc in dClasses:
                for ft in fTypes:
                    v = exactValues(MD.forDataMap, type_h, r, ft, dc)
                    MD.tempBool = v is not None
                    if v is not None:
                        value += getTimedData(m, v, year)
                        found = True
    if not found:
        MD.valueFoundBool = False
        msgOut(MD.errorLevel, f"Error in getForData(): no combination found for {type_h}, {regId_h}, {forType_h}, {year}, {freeDim_h}. Returning 0, but double check that this is ok for your model.")
    return value


def setProdData(m, value, type_h, regId_h, prodId_h, year=DATA_NOW, allowCreate=False, freeDim_h=""):
    MD = m.MD
    value = float(value)
    products = _productsFor(MD, prodId_h, False)
    regIds = _dataRegIds(MD, regId_h, "setProdData()")
    found = False
    for r in regIds:
        for p in products:
            v = exactValues(MD.prodDataMap, type_h, r, p, freeDim_h)
            if v is not None:
                found = True
                setTimedData(m, value, v, year, MD.errorLevel)
    if not found:
        if not allowCreate:
            msgOut(MSG_CRITICAL_ERROR, f'Error in setProdData: no combination found for {type_h}, {regId_h}, {prodId_h}, {year}, {freeDim_h}. You can allow new variables to be created using the "allowCreate" flag.')
        else:
            for r in regIds:
                for p in products:
                    values = []
                    setTimedData(m, value, values, year, MSG_NO_MSG)
                    mapinsert(MD.prodDataMap, makeKeyProdData(type_h, r, p, freeDim_h), values)


def setForData(m, value, type_h, regId_h, forType_h, freeDim_h, year=DATA_NOW, allowCreate=False):
    MD = m.MD
    value = float(value)
    fTypes = getForTypeIds(MD) if forType_h == FT_ALL else [forType_h]
    dClasses = _dClassesFor(MD, freeDim_h, False)
    regIds = _dataRegIds(MD, regId_h, "setProdData()")
    found = False
    for r in regIds:
        for dc in dClasses:
            for ft in fTypes:
                v = exactValues(MD.forDataMap, type_h, r, ft, dc)
                if v is not None:
                    found = True
                    setTimedData(m, value, v, year, MD.errorLevel)
    if not found:
        if not allowCreate:
            msgOut(MSG_CRITICAL_ERROR, f'Error in setForData: no combination found for {type_h}, {regId_h}, {forType_h}, {year}, {freeDim_h}. You can allow new variables to be created using the "allowCreate" flag.')
        else:
            for r in regIds:
                for dc in dClasses:
                    for ft in fTypes:
                        values = []
                        setTimedData(m, value, values, year, MSG_NO_MSG)
                        mapinsert(MD.forDataMap, makeKeyForData(type_h, r, ft, dc), values)


# ------------------------------------------------------------------------------
# Misc
# ------------------------------------------------------------------------------

def calculateAnnualisedEquivalent(amount, years, ir):
    """`years` given as a float is rounded up (ceil), as the C++ double overload does"""
    if isinstance(years, float):
        years = int(math.ceil(years)) if math.isfinite(years) else -(2 ** 63)
    if years < 0:
        return 0.0
    if years == 0:
        return amount
    return amount * ir / (cpow(1.0 + ir, years) - 1.0)


def getFilenamesByDir(dir_, filter_=""):
    """Files in a directory whose extension matches `filter` (ModelData::getFilenamesByDir)"""
    if not os.path.isdir(dir_):
        msgOut(MSG_ERROR, f"Error 2 opening the {dir_} directory.")
        return []
    files = []
    for f in sorted(os.listdir(dir_)):
        i = f.rfind(".")
        ext = "" if i < 0 else f[i:]
        if (filter_ != "" and ext == filter_) or (filter_ == "" and (i < 0 or len(f) > i + 1)):
            files.append(f)
    return files


# --- death timber ---

def getAvailableDeathTimber(m, primProd, regId, year):
    MD = m.MD
    if not getBoolSetting(m, "useDeathTimber", DATA_NOW):
        return 0.0
    toReturn = 0.0
    inv = MD.deathTimberInventory
    for ft in getForTypeIds(MD):
        for dc in MD.diamClasses:
            possible = False
            maxYears = 0
            for pp in primProd:
                if assessProdPossibility(MD, pp, ft, dc):
                    possible = True
                    maxYears = max(maxYears, getMaxYearUsableDeathTimber(MD, pp, ft, dc))
            if possible:
                for y in range(year, year - maxYears, -1):
                    toReturn += inv.get((y, regId, ft, dc), 0.0)
    return toReturn


def getAllocableProductIdsFromDeathTimber(m, regId, ft, dc, harvesting_year, request_year=DATA_NOW):
    """0-based ids of the primary products obtainable from the death timber of a given ft/dc/year"""
    MD = m.MD
    ids = []
    if not getBoolSetting(m, "useDeathTimber", DATA_NOW):
        return ids
    if request_year == DATA_NOW:
        request_year = m.SCD.year
    for p, pp in enumerate(MD.priProducts):
        if assessProdPossibility(MD, pp, ft, dc):
            if request_year - harvesting_year < getMaxYearUsableDeathTimber(MD, pp, ft, dc):
                ids.append(p)
    return ids


def deathTimberInventory_incrOrAdd(MD, key, value):
    MD.deathTimberInventory[key] = MD.deathTimberInventory.get(key, 0.0) + value


def deathTimberInventory_get(MD, key):
    v = MD.deathTimberInventory.get(key)
    if v is None:
        msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
        return 0.0
    return v
