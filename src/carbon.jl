# Port of the Carbon class (carbon stocks and substitution/emission accounting)

mutable struct Carbon
    deathBiomassInventory::Dict{Tuple{Int,Int,String},Float64}
    deathBiomassExtra::Dict{Tuple{Int,Int,String},Float64}
    products::Dict{Tuple{Int,Int,String},Float64}
    cumSubstitutedEnergy::Dict{Int,Float64}
    cumSubstitutedMaterial::Dict{Int,Float64}
    cumEmittedForOper::Dict{Int,Float64}
end
Carbon() = Carbon(Dict{Tuple{Int,Int,String},Float64}(), Dict{Tuple{Int,Int,String},Float64}(), Dict{Tuple{Int,Int,String},Float64}(),
    Dict{Int,Float64}(), Dict{Int,Float64}(), Dict{Int,Float64}())

_mapinsert!(d::Dict, k, v) = (haskey(d, k) || (d[k] = v); nothing)
_incrOrAdd!(d::Dict, k, v) = (d[k] = get(d, k, 0.0) + v; nothing)
function _incr!(d::Dict, k, v)
    if haskey(d, k)
        d[k] += v
    else
        msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
    end
end
function _findMap(d::Dict, k)
    haskey(d, k) || msgOut(MSG_CRITICAL_ERROR, "Error in finding a value in a map (no value found)")
    return d[k]
end

getRemainingStock(initialValue::Float64, halfLife::Float64, years::Real) = initialValue * cexp(-(clog(2.0) / halfLife) * years)

function getStock(m, regId::Integer, stock_type::Integer)
    C = m.CBAL
    MD = m.MD
    toReturn = 0.0
    currentYear = m.SCD.year
    initialYear = getIntSetting(m, "initialYear")
    if stock_type == STOCK_PRODUCTS
        allProducts = vcat(getStringVectorSetting(m, "priProducts"), getStringVectorSetting(m, "secProducts"))
        for pr in allProducts
            coeff = getProdData(m, "co2content_products", regId, pr, DATA_NOW, "")
            life = getProdData(m, "avgLife_products", regId, pr, DATA_NOW, "")
            for y in (initialYear-100):currentYear
                originalStock = get(C.products, (y, regId, pr), 0.0)
                toReturn += getRemainingStock(originalStock, life, currentYear - y) * coeff / 1000
            end
        end
    elseif stock_type == STOCK_INV
        for ft in getForTypeIds(MD)
            coeff = getForData(m, "co2content_inventory", regId, ft, "", DATA_NOW)
            life = getForData(m, "avgLive_deathBiomass_inventory", regId, ft, "", DATA_NOW)
            for y in (initialYear-100):currentYear
                originalStock = get(C.deathBiomassInventory, (y, regId, ft), 0.0)
                toReturn += getRemainingStock(originalStock, life, currentYear - y) * coeff / 1000
            end
            toReturn += getForData(m, "vol", regId, ft, DIAM_ALL, DATA_NOW) * coeff / 1000
        end
    elseif stock_type == STOCK_EXTRA
        for ft in getForTypeIds(MD)
            coeff = getForData(m, "co2content_extra", regId, ft, "", DATA_NOW)
            life = getForData(m, "avgLive_deathBiomass_extra", regId, ft, "", DATA_NOW)
            for y in (initialYear-100):currentYear
                originalStock = get(C.deathBiomassExtra, (y, regId, ft), 0.0)
                toReturn += getRemainingStock(originalStock, life, currentYear - y) * coeff / 1000
            end
            extraBiomass_ratio = getForData(m, "extraBiomass_ratio", regId, ft, "", DATA_NOW)
            toReturn += getForData(m, "vol", regId, ft, DIAM_ALL, DATA_NOW) * extraBiomass_ratio * coeff / 1000
        end
    else
        msgOut(MSG_CRITICAL_ERROR, "Unexpected stock_type in function getStock")
    end
    return toReturn
end

function getCumSavedEmissions(m, regId::Integer, em_type::Integer)
    C = m.CBAL
    em_type == EM_ENSUB && return _findMap(C.cumSubstitutedEnergy, regId)
    em_type == EM_MATSUB && return _findMap(C.cumSubstitutedMaterial, regId)
    em_type == EM_FOROP && return -_findMap(C.cumEmittedForOper, regId)
    msgOut(MSG_CRITICAL_ERROR, "Unexpected em_type in function getCumSavedEmissions")
    return 0.0
end

function initialiseEmissionCounters!(m)
    C = m.CBAL
    for r in getRegionIds(m.MD, 2)
        _mapinsert!(C.cumSubstitutedEnergy, r, 0.0)
        _mapinsert!(C.cumSubstitutedMaterial, r, 0.0)
        _mapinsert!(C.cumEmittedForOper, r, 0.0)
    end
end

function initialiseDeathBiomassStocks!(m, deathByFt::Vector{Float64}, regId::Integer)
    C = m.CBAL
    fTypes = getForTypeIds(m.MD)
    length(fTypes) != length(deathByFt) && msgOut(MSG_CRITICAL_ERROR, "deathByFt and fTypes have different lenght!")
    currentYear = m.SCD.year
    for (i, ft) in enumerate(fTypes)
        extraBiomass_ratio = getForData(m, "extraBiomass_ratio", regId, ft, "", DATA_NOW)
        for y in currentYear:-1:(currentYear-99)
            _mapinsert!(C.deathBiomassInventory, (y, regId, ft), deathByFt[i])
            _mapinsert!(C.deathBiomassExtra, (y, regId, ft), deathByFt[i] * extraBiomass_ratio)
        end
    end
end

function initialiseProductsStocks!(m, qByProduct::Vector{Float64}, regId::Integer)
    C = m.CBAL
    allProducts = vcat(getStringVectorSetting(m, "priProducts"), getStringVectorSetting(m, "secProducts"))
    length(allProducts) != length(qByProduct) && msgOut(MSG_CRITICAL_ERROR, "allProducts and qByProduct have different lenght!")
    currentYear = m.SCD.year
    for (i, pr) in enumerate(allProducts)
        getProdData(m, "avgLife_products", regId, pr, DATA_NOW)
        for y in currentYear:-1:(currentYear-99)
            _mapinsert!(C.products, (y, regId, pr), qByProduct[i])
        end
    end
end

function addSavedEmissions!(m, value::Float64, regId::Integer, em_type::Integer)
    C = m.CBAL
    if em_type == EM_ENSUB
        _incr!(C.cumSubstitutedEnergy, regId, value)
    elseif em_type == EM_MATSUB
        _incr!(C.cumSubstitutedMaterial, regId, value)
    elseif em_type == EM_FOROP
        _incr!(C.cumEmittedForOper, regId, -value)
    else
        msgOut(MSG_CRITICAL_ERROR, "Unexpected em_type in function getCumSavedEmissions")
    end
end

function registerHarvesting!(m, value::Float64, regId::Integer, fType::AbstractString)
    convCoeff = getForData(m, "forOperEmissions", regId, fType, "")
    addSavedEmissions!(m, -convCoeff * value / 1000, regId, EM_FOROP)
    extraBiomass_ratio = getForData(m, "extraBiomass_ratio", regId, fType, "", DATA_NOW)
    _incrOrAdd!(m.CBAL.deathBiomassExtra, (m.SCD.year, regId, String(fType)), value * extraBiomass_ratio)
end

function registerDeathBiomass!(m, value::Float64, regId::Integer, fType::AbstractString)
    key = (m.SCD.year, regId, String(fType))
    extraBiomass_ratio = getForData(m, "extraBiomass_ratio", regId, fType, "", DATA_NOW)
    _incrOrAdd!(m.CBAL.deathBiomassInventory, key, value)
    _incrOrAdd!(m.CBAL.deathBiomassExtra, key, value * extraBiomass_ratio)
end

function registerProducts!(m, value::Float64, regId::Integer, productName::AbstractString)
    _mapinsert!(m.CBAL.products, (m.SCD.year, regId, String(productName)), value)
    getProdData(m, "co2sub_energy", regId, productName, DATA_NOW, "")
    subMaterialCoeff = getProdData(m, "co2sub_material", regId, productName, DATA_NOW, "")
    addSavedEmissions!(m, subMaterialCoeff * value / 1000, regId, EM_MATSUB)
end

function registerTransports!(m, distQ::Float64, regId::Integer)
    transportEmissionsCoeff = getDoubleSetting(m, "transportEmissionsCoeff", DATA_NOW)
    addSavedEmissions!(m, -transportEmissionsCoeff * distQ / 1000, regId, EM_FOROP)
end

function HWP_eol2energy!(m)
    C = m.CBAL
    currentYear = m.SCD.year
    initialYear = getIntSetting(m, "initialYear")
    allProducts = vcat(getStringVectorSetting(m, "priProducts"), getStringVectorSetting(m, "secProducts"))
    for regId in getRegionIds(m.MD, 2)
        for pr in allProducts
            life = getProdData(m, "avgLife_products", regId, pr, DATA_NOW, "")
            eol2e_share = getProdData(m, "eol2e_share", regId, pr, DATA_NOW, "")
            subEnergyCoeff = getProdData(m, "co2sub_energy", regId, pr, DATA_NOW, "")
            if eol2e_share > 0 && subEnergyCoeff > 0
                for y in (initialYear-100):(currentYear-1)
                    originalStock = get(C.products, (y, regId, pr), 0.0)
                    lastYear = getRemainingStock(originalStock, life, currentYear - y - 1)
                    thisYear = getRemainingStock(originalStock, life, currentYear - y)
                    addSavedEmissions!(m, subEnergyCoeff * (lastYear - thisYear) * eol2e_share / 1000, regId, EM_ENSUB)
                end
            end
        end
    end
end
