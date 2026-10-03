# Generator of the synthetic spatial layers used by some test scenarios of test/data/ffsm_tests.ods.
#
# The layers are derived (deterministically) from the master grids of the model data folder and are stored,
# committed, in test/data/gis/tests/<name>/: the tests and the C++ reference script copy them from there.
# Run this script only to change them (then regenerate the C++ references):
#
#   julia --project=. test/test_layers.jl

using Printf

const TEST_HIT_REGION = 11001.0   # the debug region hit by the synthetic disturbances

"Read a GRASS ASCII grid: (header lines, matrix of values rows x cols)"
function readGrid(fn)
    header = String[]
    rows = Vector{Vector{Float64}}()
    for line in eachline(fn)
        f = split(strip(line))
        isempty(f) && continue
        if occursin(':', f[1])
            push!(header, line)
        else
            push!(rows, parse.(Float64, f))
        end
    end
    return header, permutedims(reduce(hcat, rows))
end

function writeGrid(fn, header, vals::AbstractMatrix)
    open(fn, "w") do io
        for h in header
            println(io, h)
        end
        for i in axes(vals, 1)
            println(io, join((v == -9999 ? "-9999" : @sprintf("%.10g", v) for v in vals[i, :]), " "))
        end
    end
end

"""
    writeTestLayers(datadir, outdir)

Write the synthetic layers of the test scenarios in `outdir/<name>/`, using the region map and the
availability-coefficient map of `datadir/gis/france/`.
"""
function writeTestLayers(datadir::AbstractString, outdir::AbstractString)
    fr = joinpath(datadir, "gis", "france")
    hdr, reg = readGrid(joinpath(fr, "nut_l2_inra2016.grd"))
    _, aval = readGrid(joinpath(fr, "spatialParameters", "avalCoef###.grd"))
    inFrance = aval .!= -9999
    hit = reg .== TEST_HIT_REGION
    masked(v) = [inFrance[i] ? v : -9999.0 for i in CartesianIndices(aval)]
    hitLayer(v) = [inFrance[i] ? (hit[i] ? v : 0.0) : -9999.0 for i in CartesianIndices(aval)]
    out(name) = (d = joinpath(outdir, name); rm(d; force = true, recursive = true); mkpath(d); d)

    # climate change multipliers given as maps (instead of the forData table)
    d = out("ccMaps")
    writeGrid(joinpath(d, "avalCoef###.grd"), hdr, aval)
    writeGrid(joinpath(d, "tp_multiplier#Fut_Feu##2015.grd"), hdr, masked(0.8))
    writeGrid(joinpath(d, "mortCoef_multiplier#Fut_Con##2015.grd"), hdr, masked(1.5))

    # availability coefficient halved, and specific (lower) one for the coniferous high forest
    d = out("aval")
    writeGrid(joinpath(d, "avalCoef###.grd"), hdr, [v == -9999 ? v : 0.5 * v for v in aval])
    writeGrid(joinpath(d, "avalCoef#Fut_Con##.grd"), hdr, masked(0.2))

    # windstorm in 2016 hitting region 11001 (additional mortality rates), back to zero from 2017.
    # No layers before 2016: "additionalMortality#Fut_Con#20#2016" is the first key of the whole (sorted) layer
    # map, the case where Pixel::getSTData() used to return it also for the previous years (fixed)
    d = out("windstorm")
    writeGrid(joinpath(d, "avalCoef###.grd"), hdr, aval)
    for (ft, dcs, rate) in (("Fut_Con", 20:10:60, 0.4), ("Fut_Feu", 40:10:60, 0.2))
        for dc in dcs
            writeGrid(joinpath(d, "additionalMortality#$ft#$dc#2016.grd"), hdr, hitLayer(rate))
            writeGrid(joinpath(d, "additionalMortality#$ft#$dc#2017.grd"), hdr, hitLayer(0.0))
        end
    end

    # pathogen present in region 11001 from 2014
    d = out("pathogen")
    for y in 2014:2020
        writeGrid(joinpath(d, "pathogen_pp#testpath#$y.asc"), hdr, hitLayer(1.0))
    end

    # spatially explicit forest type: volumes moved from Fut_Feu to Fut_Hetre in region 11001 [m^3 per pixel]
    d = out("spExplicit")
    for k in 1:13
        writeGrid(joinpath(d, "spInput#vol#Fut_Hetre#Fut_Feu#$k.asc"), hdr, hitLayer(3000.0))
    end
    return outdir
end

if abspath(PROGRAM_FILE) == @__FILE__
    out = writeTestLayers(normpath(joinpath(@__DIR__, "..", "data")), joinpath(@__DIR__, "data", "gis", "tests"))
    println("Written the test layers in $out")
end
