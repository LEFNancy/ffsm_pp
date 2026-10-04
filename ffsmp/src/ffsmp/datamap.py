"""DataMap: the std::map<string, vector<double>> of the C++ code (forDataMap / prodDataMap).

Exact-key access uses a dict. Prefix queries (used by getProdData when freeDim is not given) need the keys in
the (lexicographic) std::map order: a sorted key list is (re)built lazily and the result of each prefix query
is cached until a new key is inserted.
"""

import bisect

_MISSING = object()


class DataMap:
    __slots__ = ("d", "sortedKeys", "sorted", "prefixCache", "exactTCache", "prefixTCache")

    def __init__(self):
        self.d = {}
        self.sortedKeys = []
        self.sorted = True
        self.prefixCache = {}
        # look-up caches by key components, to avoid building the key strings (cleared when keys are added/removed)
        self.exactTCache = {}
        self.prefixTCache = {}

    def __contains__(self, k):
        return k in self.d

    def __len__(self):
        return len(self.d)


def _clearCaches(m):
    m.prefixCache.clear()
    m.exactTCache.clear()
    m.prefixTCache.clear()


def mapinsert(m, k, v):
    """std::map::insert semantics: do nothing if the key already exists"""
    if k in m.d:
        return False
    m.d[k] = v
    m.sortedKeys.append(k)
    m.sorted = False
    _clearCaches(m)
    return True


def mapreplace(m, k, v):
    """Replace the values of an existing key, keeping the list identity (prefix caches hold references)"""
    old = m.d[k]
    old[:] = v
    return old


def mapdelete(m, k):
    if k not in m.d:
        return False
    del m.d[k]
    m.sortedKeys = list(m.d.keys())
    m.sorted = False
    _clearCaches(m)
    return True


def _ensureSorted(m):
    if not m.sorted:
        m.sortedKeys.sort()   # Python str ordering is by code point, like std::string::compare on UTF-8
        m.sorted = True
    return m


def sortedkeys(m):
    """Keys in map (sorted) order"""
    _ensureSorted(m)
    return m.sortedKeys


def prefixValuesStr(m, prefix):
    """All the value lists whose key starts with `prefix`, in map order"""
    c = m.prefixCache.get(prefix)
    if c is not None:
        return c
    _ensureSorted(m)
    ks = m.sortedKeys
    out = []
    i = bisect.bisect_left(ks, prefix)
    n = len(ks)
    d = m.d
    while i < n and ks[i].startswith(prefix):
        out.append(d[ks[i]])
        i += 1
    m.prefixCache[prefix] = out
    return out


def prefixExists(m, prefix):
    """First key >= prefix (lower_bound) starts with prefix ? (dataMapCheckExist with exactMatch=false)"""
    _ensureSorted(m)
    ks = m.sortedKeys
    i = bisect.bisect_left(ks, prefix)
    return i < len(ks) and ks[i].startswith(prefix)


def exactValues(m, par, reg, d1, d2):
    """Value list of the key par#reg#d1#d2# (or None), cached by components"""
    k = (par, reg, d1, d2)
    v = m.exactTCache.get(k, _MISSING)
    if v is _MISSING:
        v = m.d.get(f"{par}#{reg}#{d1}#{d2}#")
        m.exactTCache[k] = v
    return v


def prefixValues(m, par, reg, d1):
    """Value lists whose key starts with par#reg#d1#, cached by components"""
    k = (par, reg, d1)
    v = m.prefixTCache.get(k)
    if v is None:
        v = prefixValuesStr(m, f"{par}#{reg}#{d1}#")
        m.prefixTCache[k] = v
    return v
