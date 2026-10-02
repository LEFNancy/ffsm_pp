# Bit-exact replica of the C++ random machinery used by FFSM:
# - std::mt19937 (32-bit Mersenne Twister)
# - std::generate_canonical<double,53>(mt19937) as used by libstdc++ distributions
# - std::normal_distribution<double> (libstdc++ Marsaglia polar method, with the cached second value)
# This allows the stochastic per-pixel values (risk aversion, expectation types, spatial modifiers)
# to be identical to those of the C++ model for a given seed.

mutable struct MT19937
    mt::Vector{UInt32}
    mti::Int
end

function MT19937(seed::Integer = 5489)
    mt = Vector{UInt32}(undef, 624)
    mt[1] = UInt32(seed % UInt32)
    for i in 2:624
        prev = mt[i-1]
        mt[i] = UInt32(1812433253) * (prev ⊻ (prev >> 30)) + UInt32(i - 1)
    end
    return MT19937(mt, 625)
end

function _mt_twist!(g::MT19937)
    mt = g.mt
    @inbounds for i in 1:624
        y = (mt[i] & 0x80000000) | (mt[mod1(i + 1, 624)] & 0x7fffffff)
        v = mt[mod1(i + 397, 624)] ⊻ (y >> 1)
        if (y & 0x00000001) != 0
            v ⊻= 0x9908b0df
        end
        mt[i] = v
    end
    g.mti = 1
    return g
end

"Next 32-bit output of std::mt19937"
function next_u32!(g::MT19937)::UInt32
    g.mti > 624 && _mt_twist!(g)
    y = g.mt[g.mti]
    g.mti += 1
    y ⊻= (y >> 11)
    y ⊻= (y << 7) & 0x9d2c5680
    y ⊻= (y << 15) & 0xefc60000
    y ⊻= (y >> 18)
    return y
end

"std::generate_canonical<double, 53>(mt19937) as in libstdc++ (two 32-bit draws, low part first)"
function generate_canonical!(g::MT19937)::Float64
    s = Float64(next_u32!(g))
    s += Float64(next_u32!(g)) * 4294967296.0
    r = s / 18446744073709551616.0
    if r >= 1.0
        r = prevfloat(1.0)
    end
    return r
end

# glibc libm log, to stay as close as possible to the C++ binary
_clog(x::Float64) = clog(x)

"std::normal_distribution<double>: it keeps the second value of each polar pair for the next call."
mutable struct NormalDist
    mean::Float64
    stddev::Float64
    saved::Float64
    saved_available::Bool
end
NormalDist(mean, stddev) = NormalDist(Float64(mean), Float64(stddev), 0.0, false)

function (d::NormalDist)(g::MT19937)::Float64
    local ret::Float64
    if d.saved_available
        d.saved_available = false
        ret = d.saved
    else
        local x::Float64, y::Float64, r2::Float64
        while true
            x = 2.0 * generate_canonical!(g) - 1.0
            y = 2.0 * generate_canonical!(g) - 1.0
            r2 = x * x + y * y
            (r2 > 1.0 || r2 == 0.0) || break
        end
        mult = sqrt(-2 * _clog(r2) / r2)
        d.saved = x * mult
        d.saved_available = true
        ret = y * mult
    end
    return ret * d.stddev + d.mean
end

"""
    normSample(d, gen, minval, maxval)

Port of the templated BaseClass::normSample. NOTE: as in the C++ code a bound equal to 0.0
(compared there against NULL) is treated as "no bound".
"""
function normSample(d::NormalDist, g::MT19937, minval::Float64 = 0.0, maxval::Float64 = 0.0)
    if minval != 0.0 && maxval != 0.0 && maxval <= minval
        msgOut(MSG_CRITICAL_ERROR, "Error in normSample: the maxvalue is lower than the minvalue")
    end
    while true
        c = d(g)
        if (minval == 0.0 || c >= minval) && (maxval == 0.0 || c <= maxval)
            return c
        end
    end
end

"std::uniform_int_distribution<int>(a,b) as in recent libstdc++ (Lemire's method; used only for the random scenario suffix)"
function uniform_int!(g::MT19937, a::Integer, b::Integer)
    urange = UInt32(b - a)
    urange == typemax(UInt32) && return Int(next_u32!(g)) + a
    range = urange + UInt32(1)
    product = UInt64(next_u32!(g)) * UInt64(range)
    low = product % UInt32
    if low < range
        threshold = (-range) % range
        while low < threshold
            product = UInt64(next_u32!(g)) * UInt64(range)
            low = product % UInt32
        end
    end
    return Int(product >> 32) + a
end
