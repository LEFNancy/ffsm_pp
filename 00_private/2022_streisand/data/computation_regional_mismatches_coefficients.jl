# todelete 
using Pkg
Pkg.activate(@__DIR__)

ROOT = @__DIR__
nregions_filename = joinpath(@__DIR__,"gis","france","nut_l2_2016_final.grd")
oregions_filename = joinpath(@__DIR__,"gis","france","nut_l2_inra2016.grd")
clc311_filename = joinpath(@__DIR__,"gis","france","clc_311.grd")
clc312_filename = joinpath(@__DIR__,"gis","france","clc_312.grd")
clc313_filename = joinpath(@__DIR__,"gis","france","clc_312.grd")

new_regions = Matrix(CSV.read(nregions_filename, delim=' ', datarow=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))
old_regions = Matrix(CSV.read(oregions_filename, delim=' ', datarow=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))
clc311   = Matrix(CSV.read(clc311_filename, delim=' ', datarow=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))
clc312   = Matrix(CSV.read(clc312_filename, delim=' ', datarow=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))
clc313   = Matrix(CSV.read(clc313_filename, delim=' ', datarow=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))

forestcover = clc311+clc312+clc313

"""
These seems to be the "Modified Areal Weighting - Control Zones" in 
Plumejeaud et al. (2010), "Transferring Indicators into Different Partitions of
Geographic Space"
https://lig-membres.imag.fr/plumejeaud/publis/geog-an-mod_13-01-2010_CP-v8.pdf
"""
function compute_partitioning_factors(from_layer, dest_layer,part_layer=ones(size(from_layer));from_list=sort(unique(skipmissing(from_layer))),dest_list = sort(unique(skipmissing(dest_layer))))
    partitioned_area     = [sum(skipmissing(part_layer[isequal.(from_layer,fr_el) .&& isequal.(dest_layer,dest_el)])) for fr_el in from_list, dest_el in dest_list]
    partitioning_factors = partitioned_area ./ sum(partitioned_area,dims=2)
    inverted_partitioning_factors = partitioned_area ./ sum(partitioned_area,dims=1)
    return partitioning_factors, inverted_partitioning_factors
end
partitioning_factors, inverted_partitioning_factors = compute_partitioning_factors(old_regions, new_regions, forestcover) # considering the forest cover
partitioning_factors, inverted_partitioning_factors  = compute_partitioning_factors(old_regions, new_regions) # without considering the forest cover, just the regional area

old_regions = [1 1 1; 1 2 2]
new_regions = [3 4 5; 3 4 5]
partitioning_factors, inverted_partitioning_factors  = compute_partitioning_factors(old_regions, new_regions)
partitioning_factors
inverted_partitioning_factors

orig = [16,4]
dest = (orig'*partitioning_factors)'
orig2 = inverted_partitioning_factors * dest

