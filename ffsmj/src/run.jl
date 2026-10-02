# Model state (the C++ ThreadManager/MainProgram role), initialisation levels (Init), the yearly
# scheduler (Scheduler) and the command line entry point (main.cpp, console mode only).

mutable struct SCDState
    year::Int
    iteration::Int
end

mutable struct RunCache
    noValue::Float64
    usePixelData::Bool
    usePathogenModule::Bool
    modifiersFromTable::Vector{String}
end
RunCache() = RunCache(-9999.0, true, false, String[])

mutable struct FFSMModel
    inputFileName::String
    baseDirectory::String
    scenarioName::String
    MD::ModelData
    GIS::Gis
    SCD::SCDState
    CBAL::Carbon
    DO::OutputState
    core::CoreState
    market::Union{Nothing,MarketLayout}
    gen::MT19937
    shuffleRng::Random.Xoshiro     # used only by initialRandomShuffle (rand() in the C++ code)
    cache::RunCache
    hsllib::Union{Nothing,String}
    settingsOverrides::Dict{String,Any}
    ipoptOptions::Dict{String,Any}
end

function FFSMModel(inputFileName::AbstractString, scenarioName::AbstractString = ""; hsllib = nothing, settings = Dict{String,Any}(),
                   ipoptOptions = Dict{String,Any}())
    inputFileName = abspath(inputFileName)
    baseDirectory = dirname(inputFileName) * "/"
    return FFSMModel(inputFileName, baseDirectory, String(scenarioName), ModelData(baseDirectory), Gis(), SCDState(0, 0),
        Carbon(), OutputState(), CoreState(), nothing, MT19937(0), Random.Xoshiro(1), RunCache(), hsllib, Dict{String,Any}(settings),
        Dict{String,Any}(ipoptOptions))
end

"Apply the settings given as keyword argument to runModel (world level, after the scenario settings)"
function applySettingsOverrides!(m::FFSMModel)
    MD = m.MD
    for (name, value) in m.settingsOverrides
        values = value isa AbstractVector ? [string(v) for v in value] : [string(value)]
        i = get(MD.settingsIndex, (name, WORLD), 0)
        i == 0 && msgOut(MSG_CRITICAL_ERROR, "Setting $name to override not found.")
        MD.settings[i].values = values
        msgOut(MSG_INFO, "Setting $name overridden with: $(join(values, " "))")
    end
end

_localTimeMessage() = (t = Dates.now(); "Local time is $(Dates.hour(t)):$(Dates.minute(t)):$(Dates.second(t))")

"Init level 1: environment (settings, data, regions, space)"
function initLevel1!(m::FFSMModel)
    MD = m.MD
    msgOut(MSG_DEBUG, "Entering Init state 1")
    msgOut(MSG_INFO, _localTimeMessage())
    setScenarioData!(m, MD)
    setDefaultSettings!(m, MD)
    setScenarioSettings!(m, MD)
    applySettingsOverrides!(m)
    if getBoolSetting(m, "newRandomSeed")
        seed = (round(Int, time()) + getpid()) % UInt32
        m.gen = MT19937(seed)
        m.shuffleRng = Random.Xoshiro(seed)   # srand(time(NULL)+getpid()) in the C++ code
        m.scenarioName = m.scenarioName * "_" * i2s(uniform_int!(m.gen, 1, 1000000))
    else
        m.gen = MT19937(0)    # std::mt19937(NULL)
        m.shuffleRng = Random.Xoshiro(1)      # rand() is never seeded in the C++ code: same shuffle at every run
    end
    m.SCD.year = getIntSetting(m, "initialYear")
    cacheSettings!(m, MD)
    createRegions!(m, MD)
    setDefaultForData!(m, MD)
    setScenarioForData!(m, MD)
    setDefaultProdData!(m, MD)
    setScenarioProdData!(m, MD)
    setForestTypes!(m, MD)
    setReclassificationRules!(m, MD)
    applyOverrides!(m, MD)
    setDefaultPathogenRules!(m, MD)
    setScenarioPathogenRules!(m, MD)
    setDefaultProductResourceMatrixLink!(m, MD)
    setScenarioProductResourceMatrixLink!(m, MD)
    applyDebugMode!(m, MD)
    # values that are asked very often and never change during the simulation
    m.cache.noValue = getDoubleSetting(m, "noValue")
    m.cache.usePixelData = getBoolSetting(m, "usePixelData")
    m.cache.usePathogenModule = getBoolSetting(m, "usePathogenModule")
    m.cache.modifiersFromTable = getStringVectorSetting(m, "modifiersFromTable")
    setSpace!(m)
    applyForestReclassification!(m)
end

"Init level 3: output initialisation and pre-optimisation years"
function initLevel3!(m::FFSMModel)
    initOutput!(m)
    if getBoolSetting(m, "usePixelData")
        runInitPeriod!(m)
    else
        msgOut(MSG_CRITICAL_ERROR, "The non-spatial version of the model (ModelCore, usePixelData=false) has not been ported to Julia.")
    end
end

"Scheduler::run(): the yearly loop"
function runScheduler!(m::FFSMModel)
    initialYear = getIntSetting(m, "initialYear")
    initialSimulationYear = getIntSetting(m, "initialOptYear")
    preSimulationYears = initialSimulationYear - initialYear
    for it in preSimulationYears:(getIntSetting(m, "simulationYears") + preSimulationYears - 1)
        m.SCD.iteration = it
        m.SCD.year = it + m.MD.cached_initialYear
        msgOut(MSG_INFO, "### $(m.SCD.year) year started.. ####")
        runSimulationYear!(m)
    end
end

function initLevel6!(m::FFSMModel)
    printFinalOutput(m)
    msgOut(MSG_INFO, "Model has ended scheduled simulation in a regular way.")
    msgOut(MSG_INFO, _localTimeMessage())
end

"""
    runModel(inputFile, scenarioName=""; hsllib=nothing, settings=Dict())

Run a whole FFSM scenario (console mode). If the scenario name is empty the first scenario of the
input file is used. Returns the model object.

`settings` can override (world level) settings after the scenario-specific ones have been applied,
e.g. `settings = Dict("simulationYears" => 5, "outputLevel" => 10)`.
`ipoptOptions` are passed to Ipopt after the default ones (e.g. `Dict("tol" => 1e-9)`).
"""
function runModel(inputFile::AbstractString, scenarioName::AbstractString = ""; hsllib = nothing, settings = Dict{String,Any}(),
                  ipoptOptions = Dict{String,Any}())
    _init_libm()
    m = FFSMModel(inputFile, scenarioName; hsllib = hsllib, settings = settings, ipoptOptions = ipoptOptions)
    println("Using base directory: ", m.baseDirectory)
    m.MD.tables = loadInput(m.inputFileName, m.baseDirectory)
    if m.scenarioName == ""
        m.scenarioName = getScenarios(m.MD)[1]
    end
    initLevel1!(m)
    initLevel3!(m)
    runScheduler!(m)
    initLevel6!(m)
    return m
end

const USAGE = """
*** FFSMj - Forest Sector Simulator (Julia port) ***
Usage:

 -h  --help                          Prints this help
 -c  --console                       Run in console mode (the only mode available in the Julia port, accepted for compatibility)
 -i  --input_file [input_file_name]  Input file (relative path, default: 'data/ffsmInput.ods')
 -s  --scenario [scenario_name]      Scenario name (default: the first defined in the input file)
     --hsllib [path]                 Path to the HSL library (to use the ma27/ma57 linear solvers)

Notes:
  - the working directory is the base path relative to the input file.
"""

function main(args::Vector{String} = ARGS)
    println()
    println("*******************************************************************")
    println("***   !! Welcome to FFSMj - The Forest Sector Simulator !!     ***")
    println("***        (Julia port of FFSM++, console version)             ***")
    println("*******************************************************************")
    println()
    inputFile = joinpath(pwd(), "data", "ffsmInput.ods")
    scenario = ""
    hsllib = nothing
    i = 1
    while i <= length(args)
        a = args[i]
        if a in ("-h", "--help")
            print(USAGE)
            return 1
        elseif a in ("-c", "--console")
        elseif a in ("-i", "--input_file") && i < length(args)
            inputFile = isabspath(args[i+1]) ? args[i+1] : joinpath(pwd(), args[i+1])
            i += 1
        elseif a in ("-s", "--scenario") && i < length(args)
            scenario = args[i+1]
            i += 1
        elseif a == "--hsllib" && i < length(args)
            hsllib = args[i+1]
            i += 1
        else
            print(USAGE)
            return 1
        end
        i += 1
    end
    try
        runModel(inputFile, scenario; hsllib = hsllib)
    catch e
        if e isa FFSMCriticalError
            return 1
        end
        rethrow()
    end
    return 0
end
