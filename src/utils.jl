# Constants, messages and C/C++-compatible helpers (port of BaseClass)

# ---- enums (BaseClass.h) ----
const MSG_NO_MSG         = 0
const MSG_DEBUG          = 1
const MSG_INFO           = 2
const MSG_WARNING        = 3
const MSG_ERROR          = 4
const MSG_CRITICAL_ERROR = 5

const TYPE_INT    = 0
const TYPE_DOUBLE = 1
const TYPE_STRING = 2
const TYPE_BOOL   = 3

const DATA_NOW  = -1
const DATA_INIT = -2
const OP_SUM    = 1
const OP_AVG    = 5
const RETNA     = -9999
const WORLD     = -99999

const OUTVL_NONE       = 0
const OUTVL_AGGREGATED = 10
const OUTVL_DETAILED   = 15
const OUTVL_MAPS       = 18
const OUTVL_BINMAPS    = 20
const OUTVL_ALL        = 25

const DOM_PRI_PR          = 1
const DOM_SEC_PR          = 2
const DOM_ALL_PR          = 3
const DOM_R2_PRI_PR       = 4
const DOM_R2_SEC_PR       = 5
const DOM_R2_ALL_PR       = 6
const DOM_SCALAR          = 7
const DOM_PRI_PR_ALLCOMBS = 8

const STOCK_INV      = 1
const STOCK_EXTRA    = 2
const STOCK_PRODUCTS = 3
const EM_ENSUB       = 4
const EM_MATSUB      = 5
const EM_FOROP       = 6

const CONSTR_EQ  = 1
const CONSTR_LE0 = 2
const CONSTR_GE0 = 3

const VAR_VOL  = 1
const VAR_AREA = 2
const VAR_IN   = 3

const PROD_ALL   = "PROD_ALL"
const PROD_PRI   = "PROD_PRI"
const PROD_SEC   = "PROD_SEC"
const DIAM_ALL   = "DIAM_ALL"
const DIAM_PROD  = "DIAM_PROD"
const DIAM_FIRST = "DIAM_FIRST_CLASS"
const FT_ALL     = "FT_ALL"

# ---- messages ----

"Raised by `msgOut(MSG_CRITICAL_ERROR, ...)`, like the `throw(msgCode_h)` of the C++ code."
struct FFSMCriticalError <: Exception
    msg::String
end
Base.showerror(io::IO, e::FFSMCriticalError) = print(io, "FFSM critical error: ", e.msg)

const MSG_PREFIX = Dict(MSG_DEBUG => "*DEBUG: ", MSG_INFO => "**INFO: ", MSG_WARNING => "**WARNING: ",
                        MSG_ERROR => "***ERROR: ", MSG_CRITICAL_ERROR => "****CRITICAL ERROR: ")

function msgOut(code::Integer, msg::AbstractString)
    code == MSG_NO_MSG && return nothing
    haskey(MSG_PREFIX, code) || error("I got an unknow error code: $code ($msg)")
    println(stdout, MSG_PREFIX[code], msg)
    code == MSG_CRITICAL_ERROR && throw(FFSMCriticalError(String(msg)))
    return nothing
end
msgOut(code::Integer, msg) = msgOut(code, string(msg))

# ---- string <-> value conversions, mimicking stringstream/strtod semantics ----

"Like `stringstream >> int`: parse the leading integer, 0 on failure."
function s2i(s::AbstractString)::Int
    isempty(s) && return 0
    m = match(r"^\s*([+-]?\d+)", s)
    m === nothing && return 0
    v = tryparse(Int, m.captures[1])
    return v === nothing ? 0 : v
end

"Like `istringstream >> double`: parse the leading float (strtod), 0.0 on failure."
function s2d(s::AbstractString)::Float64
    isempty(s) && return 0.0
    v = tryparse(Float64, s)
    v !== nothing && return v
    ref = Ref{Ptr{UInt8}}()
    GC.@preserve s begin
        cs = Base.unsafe_convert(Cstring, s)
        r = ccall(:strtod, Cdouble, (Cstring, Ptr{Ptr{UInt8}}), cs, ref)
    end
    return r
end
s2d(s::AbstractString, replaceComma::Bool) = replaceComma ? s2d(replace(s, ',' => '.')) : s2d(s)

const TRUE_STRINGS  = Set(["true", "vero", "TRUE", "VRAI", "1", "True"])
const FALSE_STRINGS = Set(["false", "falso", "FALSE", "FAUX", "0", "", "False"])

function s2b(s::AbstractString)::Bool
    s in TRUE_STRINGS && return true
    s in FALSE_STRINGS && return false
    msgOut(MSG_CRITICAL_ERROR, "Conversion string to bool failed. Some problems with the data? (got\"$s\")")
    return true
end

i2s(i::Integer) = string(i)
i2s(x::Real) = string(trunc(Int, x))   # C++ implicit double->int conversion when calling i2s with a double

"`snprintf(\"%f\")` as in BaseClass::d2s"
d2s(x::Real) = @sprintf("%f", x)

function getType(type_h::AbstractString)
    type_h == "int"    && return TYPE_INT
    type_h == "double" && return TYPE_DOUBLE
    type_h == "string" && return TYPE_STRING
    type_h == "bool"   && return TYPE_BOOL
    msgOut(MSG_CRITICAL_ERROR, "Unknow type $type_h.")
    return 0
end

"Format a Float64 like the default `std::ostream << double` (precision 6, %g), including glibc nan/inf spelling."
function fmtg(x::Real)
    x = Float64(x)
    if isnan(x)
        return signbit(x) ? "-nan" : "nan"
    elseif isinf(x)
        return x > 0 ? "inf" : "-inf"
    end
    return @sprintf("%g", x)
end
fmtg(x::Integer) = string(x)
fmtg(x::Bool) = x ? "1" : "0"

# ---- glibc math functions (the same used by the C++ binary), bound in __init__ ----
const LIBM_POW = Ref{Ptr{Cvoid}}(C_NULL)
const LIBM_EXP = Ref{Ptr{Cvoid}}(C_NULL)
const LIBM_LOG = Ref{Ptr{Cvoid}}(C_NULL)

function _init_libm()
    h = try
        Libdl.dlopen("libm.so.6")
    catch
        nothing
    end
    if h !== nothing
        LIBM_POW[] = Libdl.dlsym(h, :pow)
        LIBM_EXP[] = Libdl.dlsym(h, :exp)
        LIBM_LOG[] = Libdl.dlsym(h, :log)
    end
end

"C `pow` (glibc when available, otherwise Julia's ^)"
@inline cpow(x::Real, y::Real) = LIBM_POW[] == C_NULL ? Float64(x)^Float64(y) :
    ccall(LIBM_POW[], Cdouble, (Cdouble, Cdouble), Float64(x), Float64(y))
@inline cexp(x::Real) = LIBM_EXP[] == C_NULL ? exp(Float64(x)) : ccall(LIBM_EXP[], Cdouble, (Cdouble,), Float64(x))
@inline clog(x::Real) = LIBM_LOG[] == C_NULL ? log(Float64(x)) : ccall(LIBM_LOG[], Cdouble, (Cdouble,), Float64(x))

"std::min semantics: (b < a) ? b : a  (no NaN propagation, unlike Base.min)"
@inline cmin(a, b) = b < a ? b : a
"std::max semantics: (a < b) ? b : a  (no NaN propagation, unlike Base.max)"
@inline cmax(a, b) = a < b ? b : a

# ---- vector helpers ----

vSum(v::AbstractVector{<:Real}) = isempty(v) ? 0.0 : foldl(+, v; init = zero(eltype(v)) + 0.0)
vSum(v::AbstractVector{<:AbstractVector}) = foldl((acc, x) -> acc + vSum(x), v; init = 0.0)

"""
    minmax_element(v)

Exact port of libstdc++ `std::minmax_element` with `operator<` (returns 1-based indices).
The max is the LAST largest element, the min is the FIRST smallest one.
"""
function minmax_element(v::AbstractVector)
    n = length(v)
    n == 0 && return (0, 0)
    n == 1 && return (1, 1)
    first = 1
    nxt = 2
    if v[nxt] < v[first]
        mn = nxt; mx = first
    else
        mn = first; mx = nxt
    end
    first = nxt + 1
    while first <= n
        nxt = first + 1
        if nxt > n
            if v[first] < v[mn]
                mn = first
            elseif !(v[first] < v[mx])
                mx = first
            end
            break
        end
        if v[nxt] < v[first]
            if v[nxt] < v[mn]
                mn = nxt
            end
            if !(v[first] < v[mx])
                mx = first
            end
        else
            if v[first] < v[mn]
                mn = first
            end
            if !(v[nxt] < v[mx])
                mx = nxt
            end
        end
        first = nxt + 1
    end
    return (mn, mx)
end

"1-based position of the (last) maximum, as BaseClass::getMaxPos"
getMaxPos(v) = minmax_element(v)[2]
getMinPos(v) = minmax_element(v)[1]
getMax(v) = v[minmax_element(v)[2]]
getMin(v) = v[minmax_element(v)[1]]
getAvg(v) = isempty(v) ? 0.0 : vSum(v) / length(v)
function getSd(v; sample::Bool = true)
    isempty(v) && return 0.0
    corr = sample ? 1 : 0
    s = 0.0
    for x in v
        s += x
    end
    m = s / length(v)
    acc = 0.0
    for x in v
        acc += (x - m) * (x - m)
    end
    return sqrt(acc / (length(v) - corr))
end

"1-based position of `el` in `v` (critical error if not found), as BaseClass::getPos"
function getPos(el, v; msgCode = MSG_CRITICAL_ERROR)
    for i in eachindex(v)
        v[i] == el && return i
    end
    msgOut(msgCode, "Element not found in vector in getPos()")
    return 0
end

"BaseClass::tokenize: split skipping empty tokens"
tokenize(str::AbstractString, delim::AbstractString = " ") = [String(t) for t in split(str, collect(delim)) if !isempty(t)]

"All the subsets of `nItems` items, in the same order as ModelData::createCombinationsVector (0-based items converted to 1-based)."
function createCombinationsVector(nItems::Integer)
    toReturn = Vector{Vector{Int}}()
    for i in 0:(2^nItems - 1)
        items = Int[]
        for j in 0:(nItems - 1)
            if (i & (1 << j)) != 0
                push!(items, j + 1)
            end
        end
        push!(toReturn, items)
    end
    return toReturn
end
