"""Unit tests of the C++ compatible helpers, map images, spatio-temporal look-ups and a short debug run."""

import contextlib
import io
import os

import pytest

from common import ROOT
import ffsmp
from ffsmp.rng import MT19937, NormalDist, normSample
from ffsmp.utils import (getMaxPos, getMinPos, createCombinationsVector, fmtg, s2i, s2d, cpow, cdiv, vSum,
                         FFSMCriticalError, DIAM_ALL)
from ffsmp.datamap import DataMap, mapinsert, prefixValuesStr
from ffsmp.modeldata import getForData, getProdData, getTimedData, setTimedData
from ffsmp import gis as G_
from ffsmp.output import layerImage

INPUT = os.path.join(ROOT, "data", "ffsmInput.ods")


def test_cpp_compatible_helpers():
    # values produced by g++ (libstdc++) with std::mt19937(0) and std::normal_distribution
    g = MT19937(0)
    assert [g.next_u32() for _ in range(3)] == [2357136044, 2546248239, 3071714933]
    d = NormalDist(1.0, 0.3)
    assert [d(g) for _ in range(3)] == [1.2936213849866203, 1.6722679814376837, 1.5602673905076454]
    assert normSample(NormalDist(0.5, 10.0), MT19937(1), 0.0, 1.0) <= 1.0
    # std::minmax_element: max is the last of the largest elements, min the first of the smallest (0-based)
    assert getMaxPos([1.0, 3.0, 3.0, 2.0]) == 2
    assert getMinPos([2.0, 1.0, 1.0]) == 1
    assert createCombinationsVector(2) == [[], [0], [1], [0, 1]]
    assert fmtg(0.000123456789) == "0.000123457"
    assert fmtg(1.0e7) == "1e+07"
    assert fmtg(float("nan")) == "nan" and fmtg(float("-inf")) == "-inf"
    assert s2i("10.5") == 10 and s2i("abc") == 0
    assert s2d("2.5e3") == 2500.0
    # C semantics where Python differs
    assert cpow(0.0, -1.0) == float("inf")
    assert cpow(-8.0, 1.0 / 3.0) != cpow(-8.0, 1.0 / 3.0)   # nan
    assert cdiv(1.0, 0.0) == float("inf") and cdiv(-1.0, 0.0) == float("-inf")
    assert vSum([0.1] * 10) == sum_left_to_right([0.1] * 10) != sum([0.1] * 10)   # plain, not compensated
    dm = DataMap()
    mapinsert(dm, "a#1#p##", [1.0])
    mapinsert(dm, "a#1#p#x#", [2.0])
    mapinsert(dm, "a#1#p2##", [4.0])
    assert sum(v[0] for v in prefixValuesStr(dm, "a#1#p#")) == 3.0


def sum_left_to_right(v):
    s = 0.0
    for x in v:
        s += x
    return s


def test_map_images():
    # a 3x2 integer layer: every pixel, including the last row/column, is painted with its legend colour
    G = G_.Gis()
    G.xNPixels, G.yNPixels, G.xyNPixels = 3, 2, 6
    G.subXL, G.subXR, G.subYT, G.subYB = 0, 2, 0, 1
    L = G_.Layer("t", "t", True, True, "", True, [G_.LegendItem(1, "one", 10, 20, 30, 1, 1)])
    from array import array
    G.values["t"] = array("d", [1.0, 1.0, 1.0, 1.0, 1.0, G.noValue])
    w, h, rows = layerImage(G, L)
    assert (w, h) == (3, 2)
    assert list(rows[0][6:9]) == [10, 20, 30] and list(rows[1][0:3]) == [10, 20, 30]   # last column / last row
    assert list(rows[1][6:9]) == [255, 255, 255]                                       # no-value pixel stays white


def test_spatio_temporal_lookup():
    # the layer of a year is the last one of the series up to that year; none before the first year of the
    # series, even when that layer is the first key of the whole (sorted) layer map (bug fixed in C++ too)
    from array import array
    G = G_.Gis()
    for k in ("additionalMortality#Fut_Con#20#2016", "additionalMortality#Fut_Con#20#2017", "avalCoef###"):
        G.values[k] = array("d", [1.0])
        G.sortedKeys.append(k)
    G.keysSorted = False
    assert G_._stResolve(G, "additionalMortality#Fut_Con#20#2014") is None
    assert G_._stResolve(G, "additionalMortality#Fut_Con#20#2016") == "additionalMortality#Fut_Con#20#2016"
    assert G_._stResolve(G, "additionalMortality#Fut_Con#20#2030") == "additionalMortality#Fut_Con#20#2017"
    assert G_._stResolve(G, "additionalMortality#Fut_Con#30#2016") is None
    assert G_._stResolve(G_.Gis(), "avalCoef###2016") is None        # empty layer map


@pytest.fixture(scope="module")
def debugRun():
    with contextlib.redirect_stdout(io.StringIO()):
        return ffsmp.runModel(INPUT, "debug", settings={"simulationYears": 2, "outputLevel": 0})


def test_short_debug_run(debugRun):
    m = debugRun
    # C++ reference values (6 significant digits, as printed in the C++ output files)
    assert getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, 2012) == pytest.approx(174.062, rel=1e-5)
    assert getForData(m, "vol", 11001, "Fut_Feu", DIAM_ALL, 2014) == pytest.approx(179.6, rel=1e-5)
    assert getProdData(m, "pl", 11002, "hardWRoundW", 2013) == pytest.approx(109.325, rel=1e-5)
    assert getProdData(m, "pl", 11002, "softWSawnW", 2014) == pytest.approx(196.064, rel=1e-5)
    assert getProdData(m, "sl", 11002, "softWSawnW", 2014) == pytest.approx(2.32349, rel=1e-5)
    # time series bounds: setting a value before the initial year is an error (was an infinite loop in C++);
    # reading a year before the initial year gives the first value, after the last year the last one
    v = [1.0, 2.0]
    with contextlib.redirect_stdout(io.StringIO()):
        setTimedData(m, 3.0, v, 2013)
        assert v == [1.0, 2.0, 3.0]
        with pytest.raises(FFSMCriticalError):
            setTimedData(m, 3.0, v, 2010)
        assert getTimedData(m, v, 2005) == 1.0
        assert getTimedData(m, v, 2030) == 3.0


def test_initial_random_shuffle(debugRun):
    # every layer read from file is a permutation of the original one, with the same no-data mask
    m = debugRun
    with contextlib.redirect_stdout(io.StringIO()):
        ms = ffsmp.runModel(INPUT, "debug", settings={"simulationYears": 1, "outputLevel": 0, "initialRandomShuffle": True})
        ms2 = ffsmp.runModel(INPUT, "debug", settings={"simulationYears": 1, "outputLevel": 0, "initialRandomShuffle": True})
    for layer in ("dtm", "regLev_2", "avalCoef###"):
        a, b = list(m.GIS.values[layer]), list(ms.GIS.values[layer])
        assert [x == m.GIS.noValue for x in a] == [x == ms.GIS.noValue for x in b]
        assert sorted(a) == sorted(b)
        assert a != b
    # same seed (newRandomSeed = false): same permutation at every run
    assert list(ms.GIS.values["regLev_2"]) == list(ms2.GIS.values["regLev_2"])
