"""Shared helpers of the test suite (data preparation, cached scenario runs, output readers and comparison).

The test input (ffsm_tests.ods, scenario csv files, synthetic GIS layers) and the C++ reference outputs are
language independent: they are those of the Julia port, in ../ffsmj/test/ (see doc/port_to_julia/tests.md).
"""

import contextlib
import gzip
import io
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, os.path.join(ROOT, "src"))

import ffsmp  # noqa: E402
from ffsmp.odsreader import loadInput  # noqa: E402
from ffsmp.modeldata import getForData, getProdData, getRegion, getForTypeIds  # noqa: E402
from ffsmp.gis import regPixels  # noqa: E402
from ffsmp.utils import vSum, FT_ALL, DIAM_ALL  # noqa: E402

SHARED_TESTS = os.path.normpath(os.path.join(ROOT, "..", "ffsmj", "test"))
TEST_SRC = os.path.join(SHARED_TESTS, "data")             # ffsm_tests.ods, scenarios/*.csv, gis/tests/
REFERENCE_DIR = os.path.join(SHARED_TESTS, "reference")   # C++ reference outputs (gzipped)
MODEL_DATA = os.path.join(ROOT, "data")                   # model data folder (gis, output skeleton)
TEST_INPUT = "ffsm_tests.ods"

# scenarios whose outputs are not reproducible between C++ and the port by construction
NOT_EXACT = ["t_newSeed", "t_shuffle"]
# test years
Y_START = 2013
Y_END = 2020
Y_SHOCK = 2015    # most scenario changes start this year
REGIONS = (11001, 11002)


def copyDataTree(src, dst, skeletonOnly=False):
    """Copy a folder tree; with `skeletonOnly` only the folders and their placeholder files are copied"""
    for root, dirs, files in os.walk(src):
        rel = os.path.relpath(root, src)
        d = dst if rel == "." else os.path.join(dst, rel)
        os.makedirs(d, exist_ok=True)
        for f in files:
            if skeletonOnly and f != "00_doNotRemove.txt" and not f.endswith(".tar.gz"):
                continue
            shutil.copy2(os.path.join(root, f), os.path.join(d, f))


def prepareTestDataDir(dest):
    """Create in `dest` a complete data folder for the test scenarios: the model gis data and output skeleton,
    the test input file, the scenario csv settings and the synthetic test layers. Returns the input file path."""
    copyDataTree(os.path.join(MODEL_DATA, "gis"), os.path.join(dest, "gis"))
    copyDataTree(os.path.join(MODEL_DATA, "output"), os.path.join(dest, "output"), skeletonOnly=True)
    os.makedirs(os.path.join(dest, "cachedInput"), exist_ok=True)
    shutil.copy2(os.path.join(TEST_SRC, TEST_INPUT), os.path.join(dest, TEST_INPUT))
    shutil.copytree(os.path.join(TEST_SRC, "scenarios"), os.path.join(dest, "scenarios"), dirs_exist_ok=True)
    shutil.copytree(os.path.join(TEST_SRC, "gis", "tests"), os.path.join(dest, "gis", "tests"), dirs_exist_ok=True)
    return os.path.join(dest, TEST_INPUT)


def prepareRobustnessDataDir(dest):
    """As prepareTestDataDir, plus the forData and prodData tables given as cached csv files and a file without
    extension in the spatial data folder (to be ignored). A run must give the same results as the normal one."""
    input_ = prepareTestDataDir(dest)
    with contextlib.redirect_stdout(io.StringIO()):
        tables = loadInput(input_, dest + "/")
    for t in tables:
        if t.tableName not in ("forData", "prodData"):
            continue
        with open(os.path.join(dest, "cachedInput", t.tableName + ".csv"), "w") as f:
            f.write(";".join(t.headers) + "\n")
            for r in t.records:
                f.write(";".join(r) + "\n")
    with open(os.path.join(dest, "gis", "france", "spatialParameters", "README"), "w") as f:
        f.write("a file without extension\n")
    return input_


_SCENARIOS = None


def testScenarios():
    """The t_* scenarios of the test input file"""
    global _SCENARIOS
    if _SCENARIOS is None:
        with contextlib.redirect_stdout(io.StringIO()):
            t = loadInput(os.path.join(TEST_SRC, TEST_INPUT), TEST_SRC + "/")
        sc = [x for x in t if x.tableName == "scenarios"][0]
        _SCENARIOS = [r[0] for r in sc.records if r[0].startswith("t_")]
    return _SCENARIOS


def selectedScenarios():
    """Scenarios selected with FFSMP_TESTS=t_a,t_b,... (all by default)"""
    sel = os.environ.get("FFSMP_TESTS")
    return sel.split(",") if sel else testScenarios()


# --- cached runs (each scenario is run once per test session) ---

_TEST_DATA_DIR = None
_RUNS = {}


def testDataDir():
    global _TEST_DATA_DIR
    if _TEST_DATA_DIR is None:
        d = tempfile.mkdtemp(prefix="ffsmp_tests_")
        prepareTestDataDir(d)
        _TEST_DATA_DIR = d
    return _TEST_DATA_DIR


def runScenario(name, settings=None):
    """Run (once) a test scenario with the Python model; returns the model object"""
    key = name if not settings else name + repr(sorted(settings.items()))
    if key not in _RUNS:
        input_ = os.path.join(testDataDir(), TEST_INPUT)
        with contextlib.redirect_stdout(io.StringIO()):
            _RUNS[key] = ffsmp.runModel(input_, name, settings=settings)
    return _RUNS[key]


def outputDir():
    """Output folder of the test runs"""
    return os.path.join(testDataDir(), "output")


def cleanup():
    if _TEST_DATA_DIR is not None:
        shutil.rmtree(_TEST_DATA_DIR, ignore_errors=True)


# --- readers ---

def readText(path):
    """Read a (possibly gzipped) text file"""
    if path.endswith(".gz"):
        with gzip.open(path, "rt", encoding="utf-8") as f:
            return f.read()
    with open(path, encoding="utf-8") as f:
        return f.read()


# --- comparison of output files (exact tests) ---

RTOL = 1e-4
ATOL = 1e-8
RT_RTOL = 1e-2
RT_ATOL = 1e-5


def _num(s):
    if s in ("nan", "-nan"):
        return float("nan")
    try:
        return float(s)
    except ValueError:
        return None


def compareOutputText(ref, new, delim=";", optLog=False):
    """Compare two output texts; returns (number of differences, description of the first ones)"""
    lr = ref.rstrip("\n").split("\n")
    ln = new.rstrip("\n").split("\n")
    msgs = []
    nbad = 0
    if len(lr) != len(ln):
        return 1, [f"number of lines: reference {len(lr)}, Python {len(ln)}"]
    for i, (a, b) in enumerate(zip(lr, ln), start=1):
        fa = a.split() if delim == " " else a.split(delim)
        fb = b.split() if delim == " " else b.split(delim)
        if len(fa) != len(fb):
            nbad += 1
            if len(msgs) < 5:
                msgs.append(f"line {i}: number of fields differ")
            continue
        isrt = len(fa) >= 2 and fa[1] == "rt"
        rtol, atol = (RT_RTOL, RT_ATOL) if isrt else (RTOL, ATOL)
        for j, (x, y) in enumerate(zip(fa, fb), start=1):
            if optLog and i > 1 and j in (3, 5):    # time stamp and iteration count
                continue
            nx, ny = _num(x), _num(y)
            if nx is not None and ny is not None:
                ok = (nx != nx and ny != ny) or abs(nx - ny) <= atol + rtol * max(abs(nx), abs(ny))
            else:
                ok = x == y
            if not ok:
                nbad += 1
                if len(msgs) < 5:
                    msgs.append(f"line {i} field {j}: reference '{x}' vs Python '{y}'   [{a[:200]}]")
    return nbad, msgs


def referenceFiles(sc):
    """Reference files (relative paths, without .gz) of a scenario"""
    d = os.path.join(REFERENCE_DIR, sc)
    files = []
    for root, _, fs in os.walk(d):
        for f in fs:
            if f.endswith(".gz") and f != "log.txt.gz":
                files.append(os.path.relpath(os.path.join(root, f[:-3]), d))
    return sorted(files)


def compareScenario(sc):
    """Compare all the reference files of a scenario with the Python outputs: list of (file, nbad, msgs)"""
    runScenario(sc)
    out = []
    for f in referenceFiles(sc):
        new = os.path.join(outputDir(), f)
        if not os.path.isfile(new):
            out.append((f, 1, ["missing output file"]))
            continue
        ref = readText(os.path.join(REFERENCE_DIR, sc, f + ".gz"))
        delim = " " if f.startswith(os.path.join("maps", "asciiGrids")) else ";"
        nbad, msgs = compareOutputText(ref, readText(new), delim=delim, optLog=f.startswith("optimisationLogs"))
        out.append((f, nbad, msgs))
    return out


# --- model quantities used by the tests ---

def forTot(m, var, year, regions=REGIONS, ft=FT_ALL):
    """Sum over the test regions of a forest variable (all forest types and diameter classes)"""
    s = 0.0
    for r in regions:
        s += getForData(m, var, r, ft, DIAM_ALL, year)
    return s


def prodTot(m, var, prod, year, regions=REGIONS):
    """Sum over the test regions of a product variable"""
    s = 0.0
    for r in regions:
        s += getProdData(m, var, r, prod, year)
    return s


def pxArea(m, reg):
    """Total forest area of the pixels of a region [ha]"""
    s = 0.0
    for px in regPixels(m, getRegion(m.MD, reg)):
        s += vSum(px.area)
    return s


def sameTypeRegShare(m, reg):
    """Share of the regeneration of the last year that stays on the same forest type (diagonal of deltaArea)"""
    d = t = 0.0
    for px in regPixels(m, getRegion(m.MD, reg)):
        n = len(px.deltaArea) - 1
        for i in range(n):
            for j in range(n):
                t += px.deltaArea[i][j]
                if i == j:
                    d += px.deltaArea[i][j]
    return d / t if t > 0 else float("nan")


def relchange(new, old):
    return (new - old) / abs(old)
