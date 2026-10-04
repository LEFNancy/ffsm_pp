"""Constants, messages and C/C++-compatible helpers (port of BaseClass).

Python differs from C on a few floating point operations that the model relies on:
math.pow/exp/log raise exceptions where C returns inf/nan, `x / 0.0` raises ZeroDivisionError and the
built-in `sum()` of floats is compensated (not a plain left-to-right sum). The helpers below restore the
C semantics; CPython's math module calls the system libm (glibc), as the C++ binary does.
"""

import math
import re
import sys

# ---- enums (BaseClass.h) ----
MSG_NO_MSG = 0
MSG_DEBUG = 1
MSG_INFO = 2
MSG_WARNING = 3
MSG_ERROR = 4
MSG_CRITICAL_ERROR = 5

TYPE_INT = 0
TYPE_DOUBLE = 1
TYPE_STRING = 2
TYPE_BOOL = 3

DATA_NOW = -1
DATA_INIT = -2
OP_SUM = 1
OP_AVG = 5
RETNA = -9999
WORLD = -99999

OUTVL_NONE = 0
OUTVL_AGGREGATED = 10
OUTVL_DETAILED = 15
OUTVL_MAPS = 18
OUTVL_BINMAPS = 20
OUTVL_ALL = 25

DOM_PRI_PR = 1
DOM_SEC_PR = 2
DOM_ALL_PR = 3
DOM_R2_PRI_PR = 4
DOM_R2_SEC_PR = 5
DOM_R2_ALL_PR = 6
DOM_SCALAR = 7
DOM_PRI_PR_ALLCOMBS = 8

STOCK_INV = 1
STOCK_EXTRA = 2
STOCK_PRODUCTS = 3
EM_ENSUB = 4
EM_MATSUB = 5
EM_FOROP = 6

CONSTR_EQ = 1
CONSTR_LE0 = 2
CONSTR_GE0 = 3

VAR_VOL = 1
VAR_AREA = 2
VAR_IN = 3

PROD_ALL = "PROD_ALL"
PROD_PRI = "PROD_PRI"
PROD_SEC = "PROD_SEC"
DIAM_ALL = "DIAM_ALL"
DIAM_PROD = "DIAM_PROD"
DIAM_FIRST = "DIAM_FIRST_CLASS"
FT_ALL = "FT_ALL"

NAN = float("nan")
INF = float("inf")


# ---- messages ----

class FFSMCriticalError(Exception):
    """Raised by msgOut(MSG_CRITICAL_ERROR, ...), like the throw(msgCode_h) of the C++ code."""

    def __str__(self):
        return "FFSM critical error: " + self.args[0]


MSG_PREFIX = {MSG_DEBUG: "*DEBUG: ", MSG_INFO: "**INFO: ", MSG_WARNING: "**WARNING: ",
              MSG_ERROR: "***ERROR: ", MSG_CRITICAL_ERROR: "****CRITICAL ERROR: "}


def msgOut(code, msg):
    if code == MSG_NO_MSG:
        return
    prefix = MSG_PREFIX.get(code)
    if prefix is None:
        raise ValueError(f"I got an unknow error code: {code} ({msg})")
    sys.stdout.write(prefix + str(msg) + "\n")
    if code == MSG_CRITICAL_ERROR:
        raise FFSMCriticalError(str(msg))


# ---- string <-> value conversions, mimicking stringstream/strtod semantics ----

_INT_RE = re.compile(r"^\s*([+-]?\d+)")
_FLOAT_RE = re.compile(r"^\s*([+-]?(?:\d+\.?\d*(?:[eE][+-]?\d+)?|\.\d+(?:[eE][+-]?\d+)?|inf(?:inity)?|nan))", re.I)


def s2i(s):
    """Like `stringstream >> int`: parse the leading integer, 0 on failure."""
    if not s:
        return 0
    m = _INT_RE.match(s)
    return int(m.group(1)) if m else 0


def s2d(s, replaceComma=False):
    """Like `istringstream >> double` / strtod: parse the leading float, 0.0 on failure."""
    if replaceComma:
        s = s.replace(",", ".")
    if not s:
        return 0.0
    try:
        if "_" not in s:
            return float(s)
    except ValueError:
        pass
    m = _FLOAT_RE.match(s)
    return float(m.group(1)) if m else 0.0


TRUE_STRINGS = {"true", "vero", "TRUE", "VRAI", "1", "True"}
FALSE_STRINGS = {"false", "falso", "FALSE", "FAUX", "0", "", "False"}


def s2b(s):
    if s in TRUE_STRINGS:
        return True
    if s in FALSE_STRINGS:
        return False
    msgOut(MSG_CRITICAL_ERROR, f'Conversion string to bool failed. Some problems with the data? (got"{s}")')
    return True


def i2s(x):
    """BaseClass::i2s (a double argument is truncated, as the implicit C++ conversion)."""
    return str(int(x))


def d2s(x):
    """snprintf("%f") as in BaseClass::d2s"""
    return cfmt("%f", x)


def cfmt(fmt, x):
    """printf-like formatting of a float with glibc spelling of nan/inf."""
    x = float(x)
    if x != x:
        return "-nan" if math.copysign(1.0, x) < 0 else "nan"
    if x in (INF, -INF):
        return "inf" if x > 0 else "-inf"
    return fmt % x


def getType(type_h):
    t = {"int": TYPE_INT, "double": TYPE_DOUBLE, "string": TYPE_STRING, "bool": TYPE_BOOL}.get(type_h)
    if t is None:
        msgOut(MSG_CRITICAL_ERROR, f"Unknow type {type_h}.")
        return 0
    return t


def fmtg(x):
    """Format a value like the default `std::ostream << double` (precision 6, %g), with glibc nan/inf spelling."""
    if isinstance(x, bool):
        return "1" if x else "0"
    if isinstance(x, int):
        return str(x)
    return cfmt("%g", x)


# ---- C math semantics ----

def cpow(x, y):
    """C pow (glibc): inf/nan instead of Python exceptions."""
    try:
        return math.pow(x, y)
    except ValueError:            # negative base with non integer exponent, or 0 ** negative
        if x == 0.0:
            return INF
        return NAN
    except OverflowError:
        if x < 0 and float(y).is_integer() and int(y) % 2 == 1:
            return -INF
        return INF


def cexp(x):
    try:
        return math.exp(x)
    except OverflowError:
        return INF


def clog(x):
    try:
        return math.log(x)
    except ValueError:
        if x == 0.0:
            return -INF
        return NAN


def cdiv(a, b):
    """IEEE division (a / 0.0 is +-inf or nan, as in C)."""
    try:
        return a / b
    except ZeroDivisionError:
        if a != a or a == 0.0:
            return NAN
        neg = (a < 0) != (math.copysign(1.0, b) < 0)
        return -INF if neg else INF


def cmin(a, b):
    """std::min semantics: (b < a) ? b : a"""
    return b if b < a else a


def cmax(a, b):
    """std::max semantics: (a < b) ? b : a"""
    return b if a < b else a


def yint(x):
    """C++ implicit double -> int conversion (truncation)"""
    return int(x)


# ---- vector helpers ----

def vSum(v):
    """Plain left-to-right sum (not Python's compensated sum()), recursing into nested lists"""
    s = 0.0
    if v and isinstance(v[0], (list, tuple)):
        for x in v:
            s += vSum(x)
    else:
        for x in v:
            s += x
    return s


def minmax_element(v):
    """Exact port of libstdc++ std::minmax_element with operator< (0-based indices).
    The max is the LAST largest element, the min is the FIRST smallest one."""
    n = len(v)
    if n == 0:
        return (-1, -1)
    if n == 1:
        return (0, 0)
    first = 0
    nxt = 1
    if v[nxt] < v[first]:
        mn, mx = nxt, first
    else:
        mn, mx = first, nxt
    first = nxt + 1
    while first < n:
        nxt = first + 1
        if nxt >= n:
            if v[first] < v[mn]:
                mn = first
            elif not (v[first] < v[mx]):
                mx = first
            break
        if v[nxt] < v[first]:
            if v[nxt] < v[mn]:
                mn = nxt
            if not (v[first] < v[mx]):
                mx = first
        else:
            if v[first] < v[mn]:
                mn = first
            if not (v[nxt] < v[mx]):
                mx = nxt
        first = nxt + 1
    return (mn, mx)


def getMaxPos(v):
    """0-based position of the (last) maximum, as BaseClass::getMaxPos"""
    return minmax_element(v)[1]


def getMinPos(v):
    return minmax_element(v)[0]


def getMax(v):
    return v[minmax_element(v)[1]]


def getMin(v):
    return v[minmax_element(v)[0]]


def getAvg(v):
    return vSum(v) / len(v) if v else 0.0


def getSd(v, sample=True):
    if not v:
        return 0.0
    s = 0.0
    for x in v:
        s += x
    m = s / len(v)
    acc = 0.0
    for x in v:
        acc += (x - m) * (x - m)
    return math.sqrt(acc / (len(v) - (1 if sample else 0)))


def getPos(el, v, msgCode=MSG_CRITICAL_ERROR):
    """0-based position of `el` in `v` (critical error if not found), as BaseClass::getPos"""
    for i, x in enumerate(v):
        if x == el:
            return i
    msgOut(msgCode, "Element not found in vector in getPos()")
    return -1


def tokenize(s, delim=" "):
    """BaseClass::tokenize: split on any of the delimiter characters, skipping empty tokens"""
    out = []
    cur = []
    for ch in s:
        if ch in delim:
            if cur:
                out.append("".join(cur))
                cur = []
        else:
            cur.append(ch)
    if cur:
        out.append("".join(cur))
    return out


def createCombinationsVector(nItems):
    """All the subsets of `nItems` items (0-based), in the same order as ModelData::createCombinationsVector."""
    out = []
    for i in range(2 ** nItems):
        out.append([j for j in range(nItems) if i & (1 << j)])
    return out
