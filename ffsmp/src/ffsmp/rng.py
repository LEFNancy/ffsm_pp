"""Bit-exact replica of the C++ random machinery used by FFSM:
- std::mt19937 (32-bit Mersenne Twister)
- std::generate_canonical<double,53>(mt19937) as used by libstdc++ distributions
- std::normal_distribution<double> (libstdc++ Marsaglia polar method, with the cached second value)
This allows the stochastic per-pixel values (risk aversion, expectation types, spatial modifiers)
to be identical to those of the C++ model for a given seed.
"""

import math

from .utils import clog, msgOut, MSG_CRITICAL_ERROR

_M32 = 0xFFFFFFFF
_ONE_MINUS = math.nextafter(1.0, 0.0)


class MT19937:
    __slots__ = ("mt", "mti")

    def __init__(self, seed=5489):
        mt = [0] * 624
        mt[0] = seed & _M32
        for i in range(1, 624):
            prev = mt[i - 1]
            mt[i] = (1812433253 * (prev ^ (prev >> 30)) + i) & _M32
        self.mt = mt
        self.mti = 624

    def _twist(self):
        mt = self.mt
        for i in range(624):
            y = (mt[i] & 0x80000000) | (mt[(i + 1) % 624] & 0x7FFFFFFF)
            v = mt[(i + 397) % 624] ^ (y >> 1)
            if y & 1:
                v ^= 0x9908B0DF
            mt[i] = v
        self.mti = 0

    def next_u32(self):
        """Next 32-bit output of std::mt19937"""
        if self.mti >= 624:
            self._twist()
        y = self.mt[self.mti]
        self.mti += 1
        y ^= y >> 11
        y ^= (y << 7) & 0x9D2C5680
        y ^= (y << 15) & 0xEFC60000
        y ^= y >> 18
        return y & _M32


def next_u32(g):
    return g.next_u32()


def generate_canonical(g):
    """std::generate_canonical<double, 53>(mt19937) as in libstdc++ (two 32-bit draws, low part first)"""
    s = float(g.next_u32())
    s += float(g.next_u32()) * 4294967296.0
    r = s / 18446744073709551616.0
    if r >= 1.0:
        r = _ONE_MINUS
    return r


class NormalDist:
    """std::normal_distribution<double>: it keeps the second value of each polar pair for the next call."""
    __slots__ = ("mean", "stddev", "saved", "saved_available")

    def __init__(self, mean, stddev):
        self.mean = float(mean)
        self.stddev = float(stddev)
        self.saved = 0.0
        self.saved_available = False

    def __call__(self, g):
        if self.saved_available:
            self.saved_available = False
            ret = self.saved
        else:
            while True:
                x = 2.0 * generate_canonical(g) - 1.0
                y = 2.0 * generate_canonical(g) - 1.0
                r2 = x * x + y * y
                if not (r2 > 1.0 or r2 == 0.0):
                    break
            mult = math.sqrt(-2 * clog(r2) / r2)
            self.saved = x * mult
            self.saved_available = True
            ret = y * mult
        return ret * self.stddev + self.mean


def normSample(d, g, minval=0.0, maxval=0.0):
    """Port of the templated BaseClass::normSample. NOTE: as in the C++ code a bound equal to 0.0
    (compared there against NULL) is treated as "no bound"."""
    if minval != 0.0 and maxval != 0.0 and maxval <= minval:
        msgOut(MSG_CRITICAL_ERROR, "Error in normSample: the maxvalue is lower than the minvalue")
    while True:
        c = d(g)
        if (minval == 0.0 or c >= minval) and (maxval == 0.0 or c <= maxval):
            return c


def uniform_int(g, a, b):
    """std::uniform_int_distribution<int>(a,b) as in recent libstdc++ (Lemire's method; used only for the
    random scenario suffix)"""
    urange = (b - a) & _M32
    if urange == _M32:
        return g.next_u32() + a
    rng = urange + 1
    product = g.next_u32() * rng
    low = product & _M32
    if low < rng:
        threshold = ((1 << 32) - rng) % rng
        while low < threshold:
            product = g.next_u32() * rng
            low = product & _M32
    return (product >> 32) + a
