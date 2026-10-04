"""Port of the Carbon class (carbon stocks and substitution/emission accounting)"""

from .utils import (msgOut, cexp, clog, cdiv, MSG_CRITICAL_ERROR, DATA_NOW, DIAM_ALL, STOCK_INV, STOCK_EXTRA,
                    STOCK_PRODUCTS, EM_ENSUB, EM_MATSUB, EM_FOROP)
from .modeldata import (getIntSetting, getDoubleSetting, getStringVectorSetting, getProdData, getForData,
                        getForTypeIds, getRegionIds)


class Carbon:
    def __init__(self):
        self.deathBiomassInventory = {}
        self.deathBiomassExtra = {}
        self.products = {}
        self.cumSubstitutedEnergy = {}
        self.cumSubstitutedMaterial = {}
        self.cumEmittedForOper = {}


def _mapinsert(d, k, v):
    if k not in d:
        d[k] = v


def _incrOrAdd(d, k, v):
    d[k] = d.get(k, 0.0) + v


def _incr(d, k, v):
    if k in d:
        d[k] += v
    else:
        msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")


def _findMap(d, k):
    if k not in d:
        msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
    return d[k]


def getRemainingStock(initialValue, halfLife, years):
    return initialValue * cexp(-cdiv(clog(2.0), halfLife) * years)


def _allProducts(m):
    return getStringVectorSetting(m, "priProducts") + getStringVectorSetting(m, "secProducts")


def getStock(m, regId, stock_type):
    C = m.CBAL
    MD = m.MD
    toReturn = 0.0
    currentYear = m.SCD.year
    initialYear = getIntSetting(m, "initialYear")
    if stock_type == STOCK_PRODUCTS:
        for pr in _allProducts(m):
            coeff = getProdData(m, "co2content_products", regId, pr, DATA_NOW, "")
            life = getProdData(m, "avgLife_products", regId, pr, DATA_NOW, "")
            for y in range(initialYear - 100, currentYear + 1):
                originalStock = C.products.get((y, regId, pr), 0.0)
                toReturn += getRemainingStock(originalStock, life, currentYear - y) * coeff / 1000
    elif stock_type == STOCK_INV:
        for ft in getForTypeIds(MD):
            coeff = getForData(m, "co2content_inventory", regId, ft, "", DATA_NOW)
            life = getForData(m, "avgLive_deathBiomass_inventory", regId, ft, "", DATA_NOW)
            for y in range(initialYear - 100, currentYear + 1):
                originalStock = C.deathBiomassInventory.get((y, regId, ft), 0.0)
                toReturn += getRemainingStock(originalStock, life, currentYear - y) * coeff / 1000
            toReturn += getForData(m, "vol", regId, ft, DIAM_ALL, DATA_NOW) * coeff / 1000
    elif stock_type == STOCK_EXTRA:
        for ft in getForTypeIds(MD):
            coeff = getForData(m, "co2content_extra", regId, ft, "", DATA_NOW)
            life = getForData(m, "avgLive_deathBiomass_extra", regId, ft, "", DATA_NOW)
            for y in range(initialYear - 100, currentYear + 1):
                originalStock = C.deathBiomassExtra.get((y, regId, ft), 0.0)
                toReturn += getRemainingStock(originalStock, life, currentYear - y) * coeff / 1000
            extraBiomass_ratio = getForData(m, "extraBiomass_ratio", regId, ft, "", DATA_NOW)
            toReturn += getForData(m, "vol", regId, ft, DIAM_ALL, DATA_NOW) * extraBiomass_ratio * coeff / 1000
    else:
        msgOut(MSG_CRITICAL_ERROR, "Unexpected stock_type in function getStock")
    return toReturn


def getCumSavedEmissions(m, regId, em_type):
    C = m.CBAL
    if em_type == EM_ENSUB:
        return _findMap(C.cumSubstitutedEnergy, regId)
    if em_type == EM_MATSUB:
        return _findMap(C.cumSubstitutedMaterial, regId)
    if em_type == EM_FOROP:
        return -_findMap(C.cumEmittedForOper, regId)
    msgOut(MSG_CRITICAL_ERROR, "Unexpected em_type in function getCumSavedEmissions")
    return 0.0


def initialiseEmissionCounters(m):
    C = m.CBAL
    for r in getRegionIds(m.MD, 2):
        _mapinsert(C.cumSubstitutedEnergy, r, 0.0)
        _mapinsert(C.cumSubstitutedMaterial, r, 0.0)
        _mapinsert(C.cumEmittedForOper, r, 0.0)


def initialiseDeathBiomassStocks(m, deathByFt, regId):
    C = m.CBAL
    fTypes = getForTypeIds(m.MD)
    if len(fTypes) != len(deathByFt):
        msgOut(MSG_CRITICAL_ERROR, "deathByFt and fTypes have different lenght!")
    currentYear = m.SCD.year
    for i, ft in enumerate(fTypes):
        extraBiomass_ratio = getForData(m, "extraBiomass_ratio", regId, ft, "", DATA_NOW)
        for y in range(currentYear, currentYear - 100, -1):
            _mapinsert(C.deathBiomassInventory, (y, regId, ft), deathByFt[i])
            _mapinsert(C.deathBiomassExtra, (y, regId, ft), deathByFt[i] * extraBiomass_ratio)


def initialiseProductsStocks(m, qByProduct, regId):
    C = m.CBAL
    allProducts = _allProducts(m)
    if len(allProducts) != len(qByProduct):
        msgOut(MSG_CRITICAL_ERROR, "allProducts and qByProduct have different lenght!")
    currentYear = m.SCD.year
    for i, pr in enumerate(allProducts):
        getProdData(m, "avgLife_products", regId, pr, DATA_NOW)
        for y in range(currentYear, currentYear - 100, -1):
            _mapinsert(C.products, (y, regId, pr), qByProduct[i])


def addSavedEmissions(m, value, regId, em_type):
    C = m.CBAL
    if em_type == EM_ENSUB:
        _incr(C.cumSubstitutedEnergy, regId, value)
    elif em_type == EM_MATSUB:
        _incr(C.cumSubstitutedMaterial, regId, value)
    elif em_type == EM_FOROP:
        _incr(C.cumEmittedForOper, regId, -value)
    else:
        msgOut(MSG_CRITICAL_ERROR, "Unexpected em_type in function getCumSavedEmissions")


def registerHarvesting(m, value, regId, fType):
    convCoeff = getForData(m, "forOperEmissions", regId, fType, "")
    addSavedEmissions(m, -convCoeff * value / 1000, regId, EM_FOROP)
    extraBiomass_ratio = getForData(m, "extraBiomass_ratio", regId, fType, "", DATA_NOW)
    _incrOrAdd(m.CBAL.deathBiomassExtra, (m.SCD.year, regId, fType), value * extraBiomass_ratio)


def registerDeathBiomass(m, value, regId, fType):
    key = (m.SCD.year, regId, fType)
    extraBiomass_ratio = getForData(m, "extraBiomass_ratio", regId, fType, "", DATA_NOW)
    _incrOrAdd(m.CBAL.deathBiomassInventory, key, value)
    _incrOrAdd(m.CBAL.deathBiomassExtra, key, value * extraBiomass_ratio)


def registerProducts(m, value, regId, productName):
    _mapinsert(m.CBAL.products, (m.SCD.year, regId, productName), value)
    getProdData(m, "co2sub_energy", regId, productName, DATA_NOW, "")
    subMaterialCoeff = getProdData(m, "co2sub_material", regId, productName, DATA_NOW, "")
    addSavedEmissions(m, subMaterialCoeff * value / 1000, regId, EM_MATSUB)


def registerTransports(m, distQ, regId):
    transportEmissionsCoeff = getDoubleSetting(m, "transportEmissionsCoeff", DATA_NOW)
    addSavedEmissions(m, -transportEmissionsCoeff * distQ / 1000, regId, EM_FOROP)


def HWP_eol2energy(m):
    C = m.CBAL
    currentYear = m.SCD.year
    initialYear = getIntSetting(m, "initialYear")
    allProducts = _allProducts(m)
    for regId in getRegionIds(m.MD, 2):
        for pr in allProducts:
            life = getProdData(m, "avgLife_products", regId, pr, DATA_NOW, "")
            eol2e_share = getProdData(m, "eol2e_share", regId, pr, DATA_NOW, "")
            subEnergyCoeff = getProdData(m, "co2sub_energy", regId, pr, DATA_NOW, "")
            if eol2e_share > 0 and subEnergyCoeff > 0:
                for y in range(initialYear - 100, currentYear):
                    originalStock = C.products.get((y, regId, pr), 0.0)
                    lastYear = getRemainingStock(originalStock, life, currentYear - y - 1)
                    thisYear = getRemainingStock(originalStock, life, currentYear - y)
                    addSavedEmissions(m, subEnergyCoeff * (lastYear - thisYear) * eol2e_share / 1000, regId, EM_ENSUB)
