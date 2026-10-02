"""
    FFSMj

Julia port of FFSM++ (Forest Sector Simulator), console version.

Run a scenario with `FFSMj.runModel("data/ffsmInput.ods", "scenarioName")` or from the command line
with `julia --project=. ffsmj.jl -s scenarioName`.
"""
module FFSMj

using Printf, Dates, Libdl, Random
using EzXML, ZipArchives
using JuMP, Ipopt
import Zlib_jll

export runModel

include("utils.jl")
include("rng.jl")
include("odsreader.jl")
include("datamap.jl")
include("modeldata.jl")
include("gis.jl")
include("carbon.jl")
include("market.jl")
include("core.jl")
include("output.jl")
include("run.jl")

__init__() = _init_libm()

end # module
