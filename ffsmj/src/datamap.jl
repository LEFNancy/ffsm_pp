# DataMap: the std::map<string, vector<double>> of the C++ code (forDataMap / prodDataMap).
#
# Exact-key access uses a Dict. Prefix queries (used by getProdData when freeDim is not given) need
# the keys in the (byte-lexicographic) std::map order: a sorted key vector is (re)built lazily and the
# result of each prefix query is cached until a new key is inserted.

mutable struct DataMap
    d::Dict{String,Vector{Float64}}
    sortedKeys::Vector{String}
    sorted::Bool
    prefixCache::Dict{String,Vector{Vector{Float64}}}
    # look-up caches by key components, to avoid building the key strings (cleared when keys are added/removed)
    exactTCache::Dict{Tuple{String,Int,String,String},Union{Nothing,Vector{Float64}}}
    prefixTCache::Dict{Tuple{String,Int,String},Vector{Vector{Float64}}}
end
DataMap() = DataMap(Dict{String,Vector{Float64}}(), String[], true, Dict{String,Vector{Vector{Float64}}}(),
    Dict{Tuple{String,Int,String,String},Union{Nothing,Vector{Float64}}}(), Dict{Tuple{String,Int,String},Vector{Vector{Float64}}}())

function _clearCaches!(m::DataMap)
    empty!(m.prefixCache)
    empty!(m.exactTCache)
    empty!(m.prefixTCache)
end

Base.haskey(m::DataMap, k::AbstractString) = haskey(m.d, k)
Base.length(m::DataMap) = length(m.d)

"std::map::insert semantics: do nothing if the key already exists"
function mapinsert!(m::DataMap, k::String, v::Vector{Float64})
    haskey(m.d, k) && return false
    m.d[k] = v
    push!(m.sortedKeys, k)
    m.sorted = false
    _clearCaches!(m)
    return true
end

"Replace the values of an existing key, keeping the vector identity (prefix caches hold references)"
function mapreplace!(m::DataMap, k::String, v::Vector{Float64})
    old = m.d[k]
    resize!(old, length(v))
    copyto!(old, v)
    return old
end

function mapdelete!(m::DataMap, k::String)
    haskey(m.d, k) || return false
    delete!(m.d, k)
    m.sortedKeys = collect(keys(m.d))
    m.sorted = false
    _clearCaches!(m)
    return true
end

function _ensureSorted!(m::DataMap)
    if !m.sorted
        sort!(m.sortedKeys)   # Julia String ordering is by code units, like std::string::compare
        m.sorted = true
    end
    return m
end

"Keys in map (sorted) order"
sortedkeys(m::DataMap) = (_ensureSorted!(m); m.sortedKeys)

"All the value vectors whose key starts with `prefix`, in map order"
function prefixValues(m::DataMap, prefix::String)
    c = get(m.prefixCache, prefix, nothing)
    c === nothing || return c
    _ensureSorted!(m)
    ks = m.sortedKeys
    out = Vector{Vector{Float64}}()
    i = searchsortedfirst(ks, prefix)
    while i <= length(ks) && startswith(ks[i], prefix)
        push!(out, m.d[ks[i]])
        i += 1
    end
    m.prefixCache[prefix] = out
    return out
end

"First key >= prefix (lower_bound) starts with prefix ? (dataMapCheckExist with exactMatch=false)"
function prefixExists(m::DataMap, prefix::String)
    _ensureSorted!(m)
    ks = m.sortedKeys
    i = searchsortedfirst(ks, prefix)
    return i <= length(ks) && startswith(ks[i], prefix)
end

"Value vector of the key par#reg#d1#d2# (or nothing), cached by components"
function exactValues(m::DataMap, par::AbstractString, reg::Integer, d1::AbstractString, d2::AbstractString)
    k = (String(par), Int(reg), String(d1), String(d2))
    v = get(m.exactTCache, k, missing)
    if v === missing
        v = get(m.d, string(par, '#', reg, '#', d1, '#', d2, '#'), nothing)
        m.exactTCache[k] = v
    end
    return v
end

"Value vectors whose key starts with par#reg#d1#, cached by components"
function prefixValues(m::DataMap, par::AbstractString, reg::Integer, d1::AbstractString)
    k = (String(par), Int(reg), String(d1))
    v = get(m.prefixTCache, k, nothing)
    if v === nothing
        v = prefixValues(m, string(par, '#', reg, '#', d1, '#'))
        m.prefixTCache[k] = v
    end
    return v
end
