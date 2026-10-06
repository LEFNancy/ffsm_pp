"""
    FFSM

Julia port of FFSM++ (Forest Sector Simulator), console version.

Run a scenario with `FFSM.runModel("data/ffsmInput.ods", "scenarioName")` or from the command line
with `julia --project=. ffsm.jl -s scenarioName`.
"""
module FFSM

using Printf, Dates, Libdl, Random
using StableRNGs: StableRNG
using Distributions: Normal, truncated
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
