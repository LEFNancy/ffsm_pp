"""Model state (the C++ ThreadManager/MainProgram role), initialisation levels (Init), the yearly scheduler
(Scheduler) and the command line entry point (main.cpp, console mode only)."""

import datetime
import os
import random
import sys
import time

from .utils import msgOut, i2s, FFSMCriticalError, MSG_DEBUG, MSG_INFO, MSG_CRITICAL_ERROR, WORLD
from .rng import MT19937, uniform_int
from .odsreader import loadInput
from .modeldata import (ModelData, setScenarioData, setDefaultSettings, setScenarioSettings, getBoolSetting,
                        getIntSetting, getDoubleSetting, getStringVectorSetting, cacheSettings, createRegions,
                        setDefaultForData, setScenarioForData, setDefaultProdData, setScenarioProdData, setForestTypes,
                        setReclassificationRules, applyOverrides, setDefaultPathogenRules, setScenarioPathogenRules,
                        setDefaultProductResourceMatrixLink, setScenarioProductResourceMatrixLink, applyDebugMode,
                        getScenarios)
from .gis import Gis, setSpace, applyForestReclassification
from .carbon import Carbon
from .output import OutputState, initOutput, printFinalOutput
from .core import CoreState, runInitPeriod, runSimulationYear


class SCDState:
    def __init__(self):
        self.year = 0
        self.iteration = 0


class RunCache:
    def __init__(self):
        self.noValue = -9999.0
        self.usePixelData = True
        self.usePathogenModule = False
        self.modifiersFromTable = []
        self.ftPos = None          # forest type id -> position (getSpModifier)
        self.multCache = {}        # regional multipliers from the forData table (getMultiplier)
        self.multCacheYear = None


class FFSMModel:
    def __init__(self, inputFileName, scenarioName="", hsllib=None, settings=None, ipoptOptions=None):
        inputFileName = os.path.abspath(inputFileName)
        self.inputFileName = inputFileName
        self.baseDirectory = os.path.dirname(inputFileName) + "/"
        self.scenarioName = scenarioName
        self.MD = ModelData(self.baseDirectory)
        self.GIS = Gis()
        self.SCD = SCDState()
        self.CBAL = Carbon()
        self.DO = OutputState()
        self.core = CoreState()
        self.market = None
        self.gen = MT19937(0)
        self.shuffleRng = random.Random(1)     # used only by initialRandomShuffle (rand() in the C++ code)
        self.cache = RunCache()
        self.hsllib = hsllib
        self.settingsOverrides = dict(settings or {})
        self.ipoptOptions = dict(ipoptOptions or {})


def applySettingsOverrides(m):
    """Apply the settings given as keyword argument to runModel (world level, after the scenario settings)"""
    MD = m.MD
    for name, value in m.settingsOverrides.items():
        if isinstance(value, (list, tuple)):
            values = [_settingStr(v) for v in value]
        else:
            values = [_settingStr(value)]
        i = MD.settingsIndex.get((name, WORLD))
        if i is None:
            msgOut(MSG_CRITICAL_ERROR, f"Setting {name} to override not found.")
        MD.settings[i].values = values
        msgOut(MSG_INFO, f"Setting {name} overridden with: {' '.join(values)}")


def _settingStr(v):
    if isinstance(v, bool):
        return "true" if v else "false"
    return str(v)


def _localTimeMessage():
    t = datetime.datetime.now()
    return f"Local time is {t.hour}:{t.minute}:{t.second}"


def initLevel1(m):
    """Init level 1: environment (settings, data, regions, space)"""
    MD = m.MD
    msgOut(MSG_DEBUG, "Entering Init state 1")
    msgOut(MSG_INFO, _localTimeMessage())
    setScenarioData(m, MD)
    setDefaultSettings(m, MD)
    setScenarioSettings(m, MD)
    applySettingsOverrides(m)
    if getBoolSetting(m, "newRandomSeed"):
        seed = (round(time.time()) + os.getpid()) % (2 ** 32)
        m.gen = MT19937(seed)
        m.shuffleRng = random.Random(seed)   # srand(time(NULL)+getpid()) in the C++ code
        m.scenarioName = m.scenarioName + "_" + i2s(uniform_int(m.gen, 1, 1000000))
    else:
        m.gen = MT19937(0)                   # std::mt19937(NULL)
        m.shuffleRng = random.Random(1)      # rand() is never seeded in the C++ code: same shuffle at every run
    m.SCD.year = getIntSetting(m, "initialYear")
    cacheSettings(m, MD)
    createRegions(m, MD)
    setDefaultForData(m, MD)
    setScenarioForData(m, MD)
    setDefaultProdData(m, MD)
    setScenarioProdData(m, MD)
    setForestTypes(m, MD)
    setReclassificationRules(m, MD)
    applyOverrides(m, MD)
    setDefaultPathogenRules(m, MD)
    setScenarioPathogenRules(m, MD)
    setDefaultProductResourceMatrixLink(m, MD)
    setScenarioProductResourceMatrixLink(m, MD)
    applyDebugMode(m, MD)
    # values that are asked very often and never change during the simulation
    m.cache.noValue = getDoubleSetting(m, "noValue")
    m.cache.usePixelData = getBoolSetting(m, "usePixelData")
    m.cache.usePathogenModule = getBoolSetting(m, "usePathogenModule")
    m.cache.modifiersFromTable = getStringVectorSetting(m, "modifiersFromTable")
    setSpace(m)
    applyForestReclassification(m)


def initLevel3(m):
    """Init level 3: output initialisation and pre-optimisation years"""
    initOutput(m)
    if getBoolSetting(m, "usePixelData"):
        runInitPeriod(m)
    else:
        msgOut(MSG_CRITICAL_ERROR, "The non-spatial version of the model (ModelCore, usePixelData=false) has not been ported to Python.")


def runScheduler(m):
    """Scheduler::run(): the yearly loop"""
    initialYear = getIntSetting(m, "initialYear")
    initialSimulationYear = getIntSetting(m, "initialOptYear")
    preSimulationYears = initialSimulationYear - initialYear
    for it in range(preSimulationYears, getIntSetting(m, "simulationYears") + preSimulationYears):
        m.SCD.iteration = it
        m.SCD.year = it + m.MD.cached_initialYear
        msgOut(MSG_INFO, f"### {m.SCD.year} year started.. ####")
        runSimulationYear(m)


def initLevel6(m):
    printFinalOutput(m)
    msgOut(MSG_INFO, "Model has ended scheduled simulation in a regular way.")
    msgOut(MSG_INFO, _localTimeMessage())


def runModel(inputFile, scenarioName="", hsllib=None, settings=None, ipoptOptions=None):
    """Run a whole FFSM scenario (console mode). If the scenario name is empty the first scenario of the input
    file is used. Returns the model object.

    `settings` can override (world level) settings after the scenario-specific ones have been applied,
    e.g. settings={"simulationYears": 5, "outputLevel": 10}.
    `ipoptOptions` are passed to Ipopt after the default ones (e.g. {"tol": 1e-9}).
    """
    m = FFSMModel(inputFile, scenarioName, hsllib=hsllib, settings=settings, ipoptOptions=ipoptOptions)
    print("Using base directory: ", m.baseDirectory, sep="")
    m.MD.tables = loadInput(m.inputFileName, m.baseDirectory)
    if m.scenarioName == "":
        m.scenarioName = getScenarios(m.MD)[0]
    initLevel1(m)
    initLevel3(m)
    runScheduler(m)
    initLevel6(m)
    return m


USAGE = """
*** FFSMp - Forest Sector Simulator (Python port) ***
Usage:

 -h  --help                          Prints this help
 -c  --console                       Run in console mode (the only mode available in the Python port, accepted for compatibility)
 -i  --input_file [input_file_name]  Input file (relative path, default: 'data/ffsmInput.ods')
 -s  --scenario [scenario_name]      Scenario name (default: the first defined in the input file)
     --hsllib [path]                 Path to the HSL library (to use the ma27/ma57 linear solvers)

Notes:
  - the working directory is the base path relative to the input file.
"""


def main(args=None):
    args = sys.argv[1:] if args is None else args
    print()
    print("*******************************************************************")
    print("***   !! Welcome to FFSMp - The Forest Sector Simulator !!     ***")
    print("***        (Python port of FFSM++, console version)            ***")
    print("*******************************************************************")
    print()
    inputFile = os.path.join(os.getcwd(), "data", "ffsmInput.ods")
    scenario = ""
    hsllib = None
    i = 0
    while i < len(args):
        a = args[i]
        if a in ("-h", "--help"):
            print(USAGE, end="")
            return 1
        elif a in ("-c", "--console"):
            pass
        elif a in ("-i", "--input_file") and i < len(args) - 1:
            inputFile = args[i + 1] if os.path.isabs(args[i + 1]) else os.path.join(os.getcwd(), args[i + 1])
            i += 1
        elif a in ("-s", "--scenario") and i < len(args) - 1:
            scenario = args[i + 1]
            i += 1
        elif a == "--hsllib" and i < len(args) - 1:
            hsllib = args[i + 1]
            i += 1
        else:
            print(USAGE, end="")
            return 1
        i += 1
    try:
        runModel(inputFile, scenario, hsllib=hsllib)
    except FFSMCriticalError:
        return 1
    return 0
