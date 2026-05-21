cd(@__DIR__)
using Pkg
Pkg.activate(".")
#Pkg.update()
using DataFrames
using Statistics,  DataFrames, DataFramesMeta,  LAJuliaUtils, OdsIO, NamedColors,  StatsPlots,  CSV
using StatsPlots.PlotMeasures
#using PlotlyJS


##########################################################
### Settings and definitions
##########################################################

scenarios       = ["default", "decreased_elasticity", "increased_elasticity"]

allScenariosInAFile = false # set to true for some scenarios

fgroup_map = Dict(
 "Fut_Feu"         => "Broadl",
 "Fut_Mix"         => "Mixed",
 "Fut_Con"         => "Coniferous",
 "Mel_Feu"         => "Broadl",
 "Mel_Mix"         => "Mixed",
 "Tail_Feu"        => "Broadl",
  missing          => "All forests",
 "broadL"          => "Broadl",
 "mixedBC"         => "Mixed",
 "con"             => "Coniferous",
 "Reb_Fut_Con"     => "Coniferous",
 "Reb_Fut_Feu"     => "Broadl",
 "Reb_conShortRot" => "Coniferous",
 "Reb_Tail_Feu"    => "Broadleaved",
)

pgroup_map = Dict(
 "hardWRoundW" => "priProd",
 "softWRoundW" => "priProd",
 "pulpWFuelW"  => "priProd",
 "fuelW"       => "secProd",
 "hardWSawnW"  => "secProd",
 "softWSawnW"  => "secProd",
 "plyW"        => "secProd",
 "pulpW"       => "secProd",
 "pannels"     => "secProd",
)

cbal_map = Dict(
    "EM_ENSUB"       => "1Emissions",
    "EM_FOROP"       => "1Emissions",
    "EM_MATSUB"      => "1Emissions",
    "STOCK_EXTRA"    => "0Pools",
    "STOCK_INV"      => "0Pools",
    "STOCK_PRODUCTS" => "0Pools",
)


baseIDir        = joinpath(@__DIR__,"results")
odsOut          = joinpath(@__DIR__,"juliaOut","output.ods")
odsOut_template = joinpath(@__DIR__,"juliaOut","output_template.ods")
chartOutDir     = joinpath(@__DIR__,"juliaCharts")

years_ffsm       = collect(2013:2050) 


problematicCols = Dict("freeDim" => Union{Missing,Int64})
oldRegions = ods_read("../ffsmInput_elasticities.ods",sheetName="regions",range=((9,1),(20,2)),retType="Matrix")
newRegions = ods_read("../ffsmInput_elasticities.ods",sheetName="regions_final",range=((9,1),(21,3)),retType="Matrix")
ffsm_species = ods_read("../ffsmInput_elasticities.ods",sheetName="forTypes",range=((5,1),(10,2)),retType="Matrix")


newRegionsIdToString = Dict([r[1] => r[3] for r in eachrow(newRegions)])


# Preparing the reallocation with the new regions
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

nregions_filename = joinpath(@__DIR__,"../gis","france","nut_l2_2016_final.grd")
oregions_filename = joinpath(@__DIR__,"../gis","france","nut_l2_inra2016.grd")
clc311_filename = joinpath(@__DIR__,"../gis","france","clc_311.grd")
clc312_filename = joinpath(@__DIR__,"../gis","france","clc_312.grd")
clc313_filename = joinpath(@__DIR__,"../gis","france","clc_312.grd")

new_regions = Matrix(CSV.read(nregions_filename, delim=' ', skipto=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))
old_regions = Matrix(CSV.read(oregions_filename, delim=' ', skipto=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))
clc311   = Matrix(CSV.read(clc311_filename, delim=' ', skipto=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))
clc312   = Matrix(CSV.read(clc312_filename, delim=' ', skipto=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))
clc313   = Matrix(CSV.read(clc313_filename, delim=' ', skipto=7, header=false, ignorerepeated=true, missingstring="-9999",DataFrame))
forestcover = clc311+clc312+clc313
partitioning_factors, inverted_partitioning_factors = compute_partitioning_factors(old_regions, new_regions, forestcover) # considering the forest cover

##########################################################
### Data import
##########################################################

if allScenariosInAFile
    forVars  = CSV.read(baseIDir*"/forestData.csv", DataFrame; delim=';', missingstring="");
    prodVars = CSV.read(baseIDir*"/productData.csv", DataFrame; delim=';', missingstring="", types=problematicCols);
    cVars    = CSV.read(baseIDir*"/carbonBalance.csv", DataFrame; delim=';', missingstring="");
    # cleaning to desired scenarios only...
    forVars = forVars[in.(forVars[:,"scen"],Ref(scenarios)),:]
    prodVars = prodVars[in.(prodVars[:,"scen"],Ref(scenarios)),:]
    cVars = cVars[in.(cVars[:,"scen"],Ref(scenarios)),:] 
else
    forestData_byscenario = [CSV.read(baseIDir*"/forestData_"*scenario*".csv", DataFrame; delim=';', missingstring="") for scenario in scenarios];
    productData_byscenario = [CSV.read(baseIDir*"/productData_"*scenario*".csv", DataFrame; delim=';', types=problematicCols) for scenario in scenarios];
    carbonData_byscenario = [CSV.read(baseIDir*"/carbonBalance_"*scenario*".csv", DataFrame; delim=';', missingstring="") for scenario in scenarios];
    forVars = vcat(forestData_byscenario...)
    prodVars = vcat(productData_byscenario...)
    cVars = vcat(carbonData_byscenario...)
end

print("Data loaded")

##########################################################
### Data clean-up and initialisation
##########################################################

forVars  = forVars[in.(forVars.year, Ref(years_ffsm) ),:]
prodVars = prodVars[in.(prodVars.year, Ref(years_ffsm) ),:]
cVars    = cVars[in.(cVars.year, Ref(years_ffsm) ),:]

[select!(dfi, Not(["country"])) for dfi in [forVars,prodVars,cVars]] # delete columns country


forVars.forGroup = map(k->fgroup_map[k], forVars.forType)
#[df[:region] = map(k->reg_map[k], df[:region]) for df in [forVars,cVars]]
prodVars.prodLev = map(k->pgroup_map[k], prodVars.prod)

oldregions_toid_map = Dict(i[2]=>i[1] for i in eachrow(oldRegions) )
# 

forVarsSubset = @subset(forVars,:parName .== "hV" .|| :parName .== "vol" .|| :parName .== "forArea")

forVarsSubset.regionidold = map(k->oldregions_toid_map[k], forVarsSubset.region)
unique(forVarsSubset.regionidold)



sort!(forVarsSubset,[["scen","parName","year","forGroup"]])

subgroups1 = groupby(forVarsSubset,["scen","parName","year","forGroup","regionidold"])
forVarsGrouped1 = combine(subgroups1) do subdf # slower
    (value=sum(subdf.value),)
end

subgroups2 = groupby(forVarsGrouped1,["scen","parName","year","forGroup"])

ffsmout = combine(subgroups2) do subdf # slower
    (newregid = convert(Vector{Int64},newRegions[:,1]), valuenew = (subdf.value' * partitioning_factors)')
end


ffsmout.region = [newRegionsIdToString[i] for i in ffsmout.newregid]

# Adding totals to carbon variables
cVars_piv = pivot(cVars,[:scen,:year,:region],:balItem,:value,
                sort = [:scen,:year],
            )
cVars_piv.STOCKS = cVars_piv.STOCK_INV + cVars_piv.STOCK_EXTRA + cVars_piv.STOCK_PRODUCTS
cVars_piv.EM_NET = cVars_piv.EM_ENSUB + cVars_piv.EM_FOROP + cVars_piv.EM_MATSUB
cVars_piv.TOT_BAL =  cVars_piv.STOCKS + cVars_piv.EM_NET
cVars = stack(cVars_piv) # setting it back in long format but now with totals

# Copying the template
cp(odsOut_template, odsOut; force=true) # copy template odsFile. Don't forget CTRL+Shift+F9 to update the spreadsheet

##########################################################
### Elaborations
##########################################################

##### Carbon Variables

# Carbon balance cumulative
cBalCumReg = pivot(cVars,[:scen,:region,:variable],:year,:value,
                #filter = Dict(
                  #:variable => [:TOT_BAL, :STOCKS, :STOCK_EXTRA, :STOCK_INV, :STOCK_PRODUCTS, :EM_NET, :EM_ENSUB, :EM_FOROP, :EM_MATSUB],
                  #:year => years,
                 # ),
                sort = [:scen],
           )
cBalCumNat = pivot(cVars,[:scen,:variable],:year,:value,
                #filter = Dict(
                  #:variable => [:TOT_BAL, :STOCKS, :STOCK_EXTRA, :STOCK_INV, :STOCK_PRODUCTS, :EM_NET, :EM_ENSUB, :EM_FOROP, :EM_MATSUB],
                  #:year => years,
                 # ),
                sort = [:scen],
           )
#cBalCumNatNames = names(cBalCumNat)
#cBalCumNat[:region] = "France"
#cBalCumNat = cBalCumNat[vcat(cBalCumNatNames[1:1],:region,cBalCumNatNames[2:end])]
insertcols!(cBalCumNat, 2, :region => "France" )
cBalCum = vcat(cBalCumReg,cBalCumNat)


CBalToPlot = @subset(stack(cBalCumNat;variable_name="years"),
                #:region .== "France",
                :variable .== "STOCK_INV",
)
chart_colours = ["#3498DB" "#E74C3C" "#2ECC71"] 
pp_plot = @df CBalToPlot plot(:years, :value, group=:scen, linestyle = :solid, linewidth=1, label=reshape((" " .* sort(unique(:scen))) ,(1,:)), color=chart_colours, title="Carbon stocks in inventoried forest resources", ylabel="10⁶ T CO₂eq", titlefontsize=13)
savefig(joinpath(chartOutDir,"stocks_co2.svg"))

# reporting stocks in comparision to the first year
ymin = minimum(cVars.year)
ymax = maximum(cVars.year)
[cBalCum[!,Symbol(string(i))] = cBalCum[!,Symbol(string(i))] - cBalCum[!,Symbol(ymin)]  for i = collect(ymax:-1:ymin+1)]
select!(cBalCum, Not([Symbol(ymin)]))
ods_write(odsOut,Dict(("cBalCum",1,1) => cBalCum )  )

# Carbon balance yearly
cBal = cBalCum
[cBal[!,Symbol(string(i))] = cBal[!,Symbol(string(i))] - cBal[!,Symbol(string(i-1))]  for i = collect(ymax:-1:ymin+2)] # computing diff
ods_write(odsOut,Dict(("cBal",1,1) => cBal )  )


# Forest area (constant)

farea_temp = @subset(forVars,
    #in.(:scen, Ref(["bau",ffsmscen,rebscen])),
    :parName .== "forArea",
)[:,[:scen,:year,:value]]
groups = groupby(farea_temp,["scen","year"])
farea = combine(groups) do subdf # slower
    (forArea=sum(subdf.value),)
end

farea_w = unstack(farea, :year, :forArea)
ods_write(odsOut,Dict(("farea",1,1) => farea_w )  )


# Primary products supply
prodPP = @subset(prodVars,
    #in.(:scen, Ref(["bau",ffsmscen,rebscen])),
    :parName .== "st",
    :prodLev .== "priProd"
)[:,[:scen,:prod,:year,:value]]
groups = groupby(prodPP,["scen","prod","year"])
prodPP_nat = combine(groups) do subdf # slower
    (value=sum(subdf.value),)
end
prodPPw = unstack(prodPP_nat, "prod", "value")

chart_colours = ["#3498DB" "#E74C3C" "#2ECC71"] 
pp_plot = @df prodPPw plot(:year, :hardWRoundW, group=:scen, linestyle = :solid, linewidth=1, label=reshape(("Hardwood RoundWood " .* sort(unique(:scen))) ,(1,:)), color=chart_colours, title="Total primary products supply", ylabel="10⁶ m³", legend=:outertopright, titlefontsize=13, size=(900,400));
@df prodPPw plot!(:year, :softWRoundW, group=:scen, linestyle = :dot, linewidth=1, label=reshape(("Softwood RoundWood " .* sort(unique(:scen))) ,(1,:)), color=chart_colours);
@df prodPPw plot!(:year, :pulpWFuelW, group=:scen, linestyle = :dash, linewidth=1, label=reshape(("Industrial Wood " .* sort(unique(:scen))) ,(1,:)), color=chart_colours)
savefig(joinpath(chartOutDir,"ppst.svg"))

# Forest density in 2050 by (new) regions

den1 = @subset(ffsmout,
    in.(:parName, Ref(["vol","forArea"])),
    :year .== 2050
)[:,[:scen,:parName, :newregid, :forGroup, :valuenew]]
den2 = unstack(den1, :parName, :valuenew)
den2.density = 1_000_000 * den2.vol ./ den2.forArea
groups = groupby(den2,["scen","newregid"])
den3 = combine(groups) do subdf # slower
    (forArea=sum(subdf.forArea), vol= sum(subdf.vol), density = sum(subdf.density))
end

den3_decreased = den3[den3.scen .== "decreased_elasticity", :]
CSV.write(joinpath("juliaOut","den3_decreased.csv"), den3_decreased)
den3_default = den3[den3.scen .== "default", :]
CSV.write(joinpath("juliaOut","den3_default.csv"), den3_default)
den3_increased_elasticity = den3[den3.scen .== "increased_elasticity", :]
CSV.write(joinpath("juliaOut","den3_increased_elasticity.csv"), den3_increased_elasticity)

# OLD.....
################################################################################
# D4.1: Quantity of wood energy consumed with rebound effect
# D4.3: Quantities of wood energy imported into each region

# Secondary product "fuelW"

# All detailed data
SprodVar_d41_d43 = @where(prodVars,
    #in.(:scen, Ref(["bau",ffsmscen,rebscen])),
    in.(:parName, Ref(["dl","da","dt"])),
    :prod .== "fuelW",
)[:,[:scen,:parName,:region,:year,:value]]

d41_d43 = unstack(prodVar_d41_d43, :year, :value)

ods_write(odsOut,Dict(("d41_d43",1,1) => d41_d43))

# Simplified, for chart - D41
prodVar_d41 = @subset(prodVars,
    #in.(:scen, Ref(["bau",ffsmscen,rebscen])),
    :parName .== "dt",
    :prod .== "fuelW",
)[:,[:scen,:region,:year,:value]]
groups = groupby(prodVar_d41,["scen","year"])
prodVar_d41_nat = combine(groups) do subdf # slower
    (value=sum(subdf.value),)
end
@df prodVar_d41_nat plot(:year, :value, group=:scen, label =["alt" "alt_reb" "bau" "bau_reb"], title="D 4.1 - Quantity of wood energy consumed with rebound effect", ylab="1O³ m³", titlefontsize=11)
savefig(joinpath(chartOutDir,"d41.svg"))

plot(prodVar_d41_nat[prodVar_d41_nat.scen .== "alt", "value"], label="alt");
plot!(prodVar_d41_nat[prodVar_d41_nat.scen .== "alt_reb", "value"], label="alt_reb");
plot!(prodVar_d41_nat[prodVar_d41_nat.scen .== "bau", "value"], label="bau");
plot!(prodVar_d41_nat[prodVar_d41_nat.scen .== "bau_reb", "value"], label="bau_reb")


# Simplified, for chart - D43
prodVar_d43 = @subset(prodVars,
    #in.(:scen, Ref(["bau",ffsmscen,rebscen])),
    :parName .== "da",
    :prod .== "fuelW",
)[:,[:scen,:region,:year,:value]]
groups = groupby(prodVar_d43,["scen","year"])
prodVar_d43_nat = combine(groups) do subdf # slower
    (value=sum(subdf.value),)
end
@df prodVar_d43_nat plot(:year, :value, group=:scen, label =["alt" "rebound" "bau" "rebonly"], title="D 4.3 - Quantities of wood energy imported", ylab="1O³ m³", titlefontsize=11)
savefig(joinpath(chartOutDir,"d43.svg"))

################################################################################
# D4.2: Spatialised harvest levels at administrative region level

# All detailed data
forVars_d42 = @subset(forVars,
    #in.(:scen, Ref(["bau",ffsmscen, rebscen])),
    in.(:parName, Ref(["hV","harvestedArea"])),
)[:,[:scen,:parName,:region,:year,:value]]
groups = groupby(forVars_d42,["scen","parName","region","year"])
forVars_d42_reg = combine(groups) do subdf # slower
    (value=sum(subdf.value),)
end
d42 = unstack(forVars_d42_reg, :year, :value)
ods_write(odsOut,Dict(("d42",1,1) => d42))

# Simplified, for charts

# Time trend nat chart
groups = groupby(forVars_d42,["scen","parName","year"])
forVars_d42_nat = combine(groups) do subdf # slower
    (value=sum(subdf.value),)
end
@df forVars_d42_nat[forVars_d42_nat.parName .== "harvestedArea", :] plot(:year, :value, group=:scen, label = ["alt" "rebound" "bau" "rebonly"], title="D 4.2 - Harvest levels (area)", titlefontsize=11, ylabel="ha")
savefig(joinpath(chartOutDir,"d42_natTrend_area.svg"))
@df forVars_d42_nat[forVars_d42_nat.parName .== "hV", :] plot(:year, :value, group=:scen, label = ["alt" "rebound" "bau" "rebonly"], title="D 4.2 - Harvest levels (vols)", titlefontsize=11, ylabel="10³ m³")
savefig(joinpath(chartOutDir,"d42_natTrend_vols.svg"))

# By region bar chart (todo: map instead)
forVars_d42 = @subset(forVars,
    #in.(:scen, Ref(["bau",ffsmscen,rebscen])),
    in.(:parName, Ref(["hV","harvestedArea"])),
)[:,[:scen,:parName,:region,:year, :value]]

nyears = maximum(forVars_d42.year) - minimum(forVars_d42.year) + 1 

groups = groupby(forVars_d42,["scen","parName","region"])
forVars_d42_reg_allyears = combine(groups) do subdf # slower
    (value=sum(subdf.value) / nyears,)
end

@df forVars_d42_reg_allyears[forVars_d42_reg_allyears.parName .== "harvestedArea", :] groupedbar(:region, :value, group=:scen, xrotation=45, bottom_margin=10mm, label = ["alt" "rebound" "bau" "rebonly"], title="D 4.2 - Regionalised harvest levels (area)", titlefontsize=11, ylabel="ha y⁻¹")
savefig(joinpath(chartOutDir,"d42_byRegion_area.svg"))

@df forVars_d42_reg_allyears[forVars_d42_reg_allyears.parName .== "hV", :] groupedbar(:region, :value, group=:scen, xrotation=45, bottom_margin=10mm, label = ["alt" "rebound" "bau" "rebonly"], title="D 4.2 - Regionalised harvest levels (vols)", titlefontsize=11, ylabel="10³ m³ y⁻¹")
savefig(joinpath(chartOutDir,"d42_byRegion_vols.svg"))

################################################################################
# D4.4: Projected forest rent for each pixel

# All detailed data
forVars_d44 = @subset(forVars,
    #in.(:scen, Ref(["bau",ffsmscen, rebscen])),
    in.(:parName, Ref(["expReturns"])),
    :forGroup .== "All forests"
)[:,[:scen,:region,:year,:value]]

groups = groupby(forVars_d44,["scen","region","year"])
forVars_d44_reg = combine(groups) do subdf # slower
    (value=sum(subdf.value),)
end
d44 = unstack(forVars_d44_reg, :year, :value)
ods_write(odsOut,Dict(("d44",1,1) => d44))

# Simplified, for charts



# Time trend nat chart
# Getting area by region

d44_area_temp = @subset(forVars,
    #in.(:scen, Ref(["bau",ffsmscen,rebscen])),
    :parName .== "forArea",
)[:,[:scen,:region,:year,:value]]
groups = groupby(d44_area_temp,["scen","region","year"])
d44_area = combine(groups) do subdf # slower
    (forArea=sum(subdf.value),)
end
d44_expr_temp = @subset(forVars,
    #in.(:scen, Ref(["bau",ffsmscen,rebscen])),
    :parName .== "sumExpReturns",
    :forGroup .== "All forests"
)[:,[:scen,:region,:year,:value]]
d44_expr = rename!(d44_expr_temp, :value => :sumExpReturns)
d44_nat_temp = innerjoin(d44_area,d44_expr,on=[:scen,:region,:year])

groups = groupby(d44_nat_temp,["scen","year"])
d44_nat = combine(groups) do subdf # slower
    (forArea=sum(subdf.forArea),sumExpReturns=sum(subdf.sumExpReturns))
end
d44_nat.expReturns =  d44_nat.sumExpReturns ./ d44_nat.forArea
@df d44_nat plot(:year, :expReturns, group=:scen, label = ["alt" "rebound" "bau" "rebonly"], title="D 4.4 - Expected revenues", titlefontsize=11, ylabel="Eur ha⁻¹")
savefig(joinpath(chartOutDir,"d44_natTrend.svg"))

# By region bar chart (todo: map instead)
d44_expr_reg_temp = @subset(forVars,
    in.(:scen, Ref(["bau",ffsmscen,"alt2"])),
    :parName .== "expReturns",
    :forGroup .== "All forests"
)[:,[:scen,:region,:year,:value]]

groups = groupby(d44_expr_reg_temp,["scen","region"])
d44_expr_reg_allyears = combine(groups) do subdf # slower
    (value=mean(subdf.value))
end
rename!(d44_expr_reg_allyears, :x1 => :value)

@df d44_expr_reg_allyears groupedbar(:region, :value, group=:scen, xrotation=45, bottom_margin=10mm, label = ["alt" "rebound" "bau" "rebonly"], title="D 4.4 - Regionalised expected returns [avg]", titlefontsize=11, ylabel="ha y⁻¹")
savefig(joinpath(chartOutDir,"d44_byRegion.svg"))

