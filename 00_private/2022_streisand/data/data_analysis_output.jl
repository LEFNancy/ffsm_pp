cd(@__DIR__)
using Pkg
Pkg.activate(".")
Pkg.update()
using DataFrames
using Statistics,  DataFrames, DataFramesMeta,  LAJuliaUtils, OdsIO, NamedColors,  StatsPlots,  CSV
using StatsPlots.PlotMeasures
#using PlotlyJS


##########################################################
### Settings and definitions
##########################################################

ffsmscen = "bicaff_supply_6"
ffsmscen_symbol = Symbol(ffsmscen)
scenarios = ["default", ffsmscen]

allScenariosInAFile = true # set to false for some scnearios

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


baseIDir        = joinpath(@__DIR__,"output","results")
odsOut          = joinpath(@__DIR__,"output","juliaOut","formattedOutput.ods")
odsOut_template = joinpath(@__DIR__,"output","juliaOut","formattedOutput_template.ods")
chartOutDir     = joinpath(@__DIR__,"output","juliaOut")

years_ffsm       = collect(2013:2112) # years to consider for FFSM (and also BICAFF as 2015 and 2110 are the 5-years centroids)


problematicCols = Dict("freeDim" => Union{Missing,Int64})
oldRegions = ods_read("ffsmInput_streisand.ods",sheetName="regions",range=((9,1),(20,2)),retType="Matrix")
newRegions = ods_read("ffsmInput_streisand.ods",sheetName="regions_final",range=((9,1),(21,3)),retType="Matrix")
ffsm_species = ods_read("ffsmInput_streisand.ods",sheetName="forTypes",range=((5,1),(10,2)),retType="Matrix")


newRegionsIdToString = Dict([r[1] => r[3] for r in eachrow(newRegions)])

# bicaff outputs
bicaff_filename = joinpath(@__DIR__,"raw","20220926carbon_balance_BICAFF.csv")
bicaff =  CSV.read(bicaff_filename,DataFrame)

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

nregions_filename = joinpath(@__DIR__,"gis","france","nut_l2_2016_final.grd")
oregions_filename = joinpath(@__DIR__,"gis","france","nut_l2_inra2016.grd")
clc311_filename = joinpath(@__DIR__,"gis","france","clc_311.grd")
clc312_filename = joinpath(@__DIR__,"gis","france","clc_312.grd")
clc313_filename = joinpath(@__DIR__,"gis","france","clc_312.grd")

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

forVarsSubset = @subset(forVars,:parName .== "hV" .|| :parName .== "vol" .|| :parName .== "area")

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


# FFSM specific issues.......................


# FFSM harvest
hvffsm = @subset(ffsmout,:parName .== "hV")
hvffsm = hvffsm[:,Not("parName")]


## - FFSM harvest by year
groups = groupby(hvffsm,["scen","year"])

harvest_yearly_ffsm  = combine(groups, "valuenew" => sum => "yearly_harvested")
harvest_yearly_ffsm  = unstack(harvest_yearly_ffsm,"scen","yearly_harvested")
harvest_yearly_ffsm.diff = harvest_yearly_ffsm[:,ffsmscen] .- harvest_yearly_ffsm.default
total_hv_diff = sum(harvest_yearly_ffsm.diff)
plot(harvest_yearly_ffsm.year[5:end], harvest_yearly_ffsm.default[5:end], legend=:topleft, colour=:green, label="BAU", title="Yearly harvesting (FFSM)", ylabel="Mm³",ylims=[40,60])
plot!(harvest_yearly_ffsm[5:end,:year], harvest_yearly_ffsm[5:end,ffsmscen], legend=:topleft, colour=:red, label="bicaff scen in FFSM")

## - FFSM harvest by region
groups = groupby(hvffsm,["scen","region"])
harvest_by_region_ffsm = combine(groups, "valuenew" => sum => "total_harvested")
harvest_by_region_ffsm = unstack(harvest_by_region_ffsm,"scen","total_harvested")
harvest_by_region_ffsm.additionalHarvested = harvest_by_region_ffsm[:,ffsmscen]  - harvest_by_region_ffsm.default
@df harvest_by_region_ffsm bar(:region, :additionalHarvested, legend=:topleft, label=nothing, title="Additional harvest required by region (ffsm)", xrotation=45, bottom_margin=10mm)

## - FFSM harvest per species
groups = groupby(hvffsm,["scen","region","year"])
harvest_by_species_ffsm = combine(groups) do subdf
    Broadl_hv         = @subset(subdf,:forGroup .== "Broadl").valuenew[1]
    Coniferous_hv     = @subset(subdf,:forGroup .== "Coniferous").valuenew[1]
    Mixed_hv          = @subset(subdf,:forGroup .== "Coniferous").valuenew[1]
    Broadl_hv_new     = Broadl_hv+Mixed_hv*(Broadl_hv / (Broadl_hv+Coniferous_hv))
    Coniferous_hv_new = Coniferous_hv+Mixed_hv*(Coniferous_hv / (Broadl_hv+Coniferous_hv))
    Broadl_hv_new     = isnan(Broadl_hv_new) ? 0.0 : Broadl_hv_new
    Coniferous_hv_new = isnan(Coniferous_hv_new) ? 0.0 : Coniferous_hv_new
    (forGroup2 = ["Broadl","Coniferous"], value = [Broadl_hv_new, Coniferous_hv_new])
end

groups = groupby(harvest_by_species_ffsm,["scen","forGroup2"])
harvest_by_species_ffsm = combine(groups) do subdf
    (hv = sum(subdf.value),)
end
harvest_by_species_ffsm = unstack(harvest_by_species_ffsm,"scen","hv")
harvest_by_species_ffsm.additionalHarvested = harvest_by_species_ffsm[:,ffsmscen] - harvest_by_species_ffsm.default
@df harvest_by_species_ffsm bar(:forGroup2, :additionalHarvested, legend=:topleft, label=nothing, title="Additional harvest required by species group (FFSM)")

## - FFSM carbon balance
# em are already stacked !!!
# unit is Mm^3 Co2 eq

cbal = @subset(cVars, :variable .== "STOCKS" .|| :variable .== "EM_NET" )
cbal = combine(groupby(cbal,["scen","year","variable"]), "value" => sum => "value")
cbal = unstack(cbal,"variable","value")
cbal.tot = cbal.STOCKS + cbal.EM_NET

cbal_bau = @subset(cbal, :scen .== "default")
cbal_scen = @subset(cbal, :scen .== ffsmscen)


@df cbal_bau plot(:year, :STOCKS, legend=:topleft, colour=:green, linestyle=:dot, label="bau - stocks", title="Yearly carbon balance (strockage + cum em) FFSM", ylabel="Mm³ CO₂ eq")
@df cbal_bau plot!(:year, :tot, legend=:topleft, colour=:green, label="bau - tot")
@df cbal_scen plot!(:year, :STOCKS, legend=:topleft, colour=:red, linestyle=:dot, label=string(ffsmscen," - stocks"))
@df cbal_scen plot!(:year, :tot, legend=:topleft, colour=:red, label=string(ffsmscen," - tot"))

carbon_gain = cbal_scen.tot[end] - cbal_bau.tot[end]



## - FFSM cost analysis
groups = groupby(prodVars,["scen","year"])
costs = combine(groups) do subdf
    polBal_mktDirInt_s = sum( @subset(subdf, :parName .== "polBal_mktDirInt_s").value)
    polBal_mktDirInt_d = sum( @subset(subdf, :parName .== "polBal_mktDirInt_d").value)
    polBal_trSub = sum( @subset(subdf, :parName .== "polBal_trSub").value)
    polBal_tcSub = sum( @subset(subdf, :parName .== "polBal_tcSub").value)

    surplus_prod = sum( @subset(subdf, :parName .== "surplus_prod").value)
    surplus_cons = sum( @subset(subdf, :parName .== "surplus_cons").value)
    policy_cost_part = polBal_mktDirInt_s  + polBal_mktDirInt_d + polBal_trSub + polBal_tcSub
    (policy_cost_part = policy_cost_part,surplus_prod=surplus_prod, surplus_cons=surplus_cons)
end
groups = groupby(forVars,["scen","year"])
costs2 = combine(groups) do subdf
    polBal_fiSub = sum( @subset(subdf, :parName .== "polBal_fiSub").value)
    (polBal_fiSub = polBal_fiSub,)
end

sort!(costs,["scen","year"])
sort!(costs2,["scen","year"])
costs.polBal_fiSub = costs2.polBal_fiSub

costs.policy_cost = costs.policy_cost_part + costs.polBal_fiSub
costs.gross_welfare = costs.surplus_prod + costs.surplus_cons
costs.net_welfare = costs.gross_welfare + costs.policy_cost

scenarios = unique(costs.scen) 
costs_bau = @subset(costs, :scen .== "default")
costs_alt = @subset(costs, :scen .== ffsmscen)

@df costs plot(:year, :policy_cost, group=:scen)
@df costs plot(:year, :gross_welfare, group=:scen)
@df costs plot!(:year, :net_welfare, group=:scen)

@df costs_bau plot(:year, :gross_welfare, colour="green", label="Welfare - bau", legend=:bottomright)
@df costs_alt plot!(:year, :gross_welfare, colour="red", label="Gross welfare - $ffsmscen ")
@df costs_alt plot!(:year, :net_welfare, colour="red", linestyle=:dot, label="Net welfare - $ffsmscen ")

avg_yearly_policy_cost = -mean(costs_alt.policy_cost) # M€
avg_yearly_welfare_loss = mean(costs_bau.net_welfare - costs_alt.net_welfare) # M€




#=
groups = groupby(cbal,["scen"])
cbaltot = combine(groups) do subdf
    #(year = subdf.year, stocks = subdf.STOCKS, cumem = cumsum(subdf.EM_NET),totalbal = subdf.STOCKS .+ cumsum(subdf.EM_NET))
    (year = subdf.year,totalbal = subdf.STOCKS .+ cumsum(subdf.EM_NET))
end
cbalstocks = combine(groups) do subdf
    (year = subdf.year, stocks = subdf.STOCKS)
end
cbalem = combine(groups) do subdf
    (year = subdf.year, cumem = cumsum(subdf.EM_NET))
end
cbaltot = unstack(cbaltot,"scen","totalbal")
cbalstocks = unstack(cbalstocks,"scen","stocks")
cbalem = unstack(cbalem,"scen","cumem")

@df cbalstocks plot(:year, :default, legend=:topleft, colour=:green,label="bau", title="Yearly carbon balance (strockage + cum em) FFSM")
plot!(cbalstocks.year, cbalstocks[:,ffsmscen], legend=:topleft, colour=:red,label=ffsmscen)

@df cbalem plot(:year, :default, legend=:topleft, colour=:green,label="bau", title="Yearly carbon balance (strockage + cum em) FFSM")
plot!(cbalem.year, cbalem[:,ffsmscen], legend=:topleft, colour=:red,label=ffsmscen)

@df cbaltot plot(:year, :default, legend=:topleft, colour=:green,label="bau", title="Yearly carbon balance (strockage + cum em) FFSM")
plot!(cbaltot.year, cbaltot[:,ffsmscen], legend=:topleft, colour=:red,label=ffsmscen)
=#

# ------------------------------------------------------------------------------
#
#### BICAFF DATA...........
#


regions_bicaff = unique(bicaff.Region)
years_bicaff = unique(bicaff.year)
year_start_bicaff = minimum(years_bicaff)
year_ends_bicaff  = maximum(years_bicaff)
cut_options = unique(bicaff.plots_cut)

# Looking at the total harvesting by scenario/intensity combination
scenario_groups      = groupby(bicaff,["plots_cut","intensity"])
scenario_year_groups = groupby(bicaff,["plots_cut","intensity","year"])
harvest_yearly  = combine(scenario_year_groups, "yearly_harvested_m3" => sum => "yearly_harvested")
harvest_century = combine(scenario_groups,      "yearly_harvested_m3" => sum => "total_harvest_century")
harvest_yearly_bau = @subset(harvest_yearly, :plots_cut .== "BAU" ) #"scenario" .== "BAU")
harvest_century_bau = @subset(harvest_century, :plots_cut .== "BAU" )[1,"total_harvest_century"] #"scenario" .== "BAU")

carbon_balance_yearly  = combine(scenario_year_groups, "carbon_balance" => sum => "carbon_balance")

initial_carbon_balance = @subset(carbon_balance_yearly , :plots_cut .== "BAU", :year .== year_start_bicaff)[:,"carbon_balance"][1]
final_carbon_balance   = @subset(carbon_balance_yearly , :year .== year_ends_bicaff )
sort!(final_carbon_balance, ["carbon_balance"], rev=true)

topscenario = final_carbon_balance[1,"plots_cut"],final_carbon_balance[1,"intensity"]
carbon_balance_yearly_top = carbon_balance_yearly[carbon_balance_yearly.plots_cut .== topscenario[1] .&& carbon_balance_yearly.intensity .== topscenario[2],:]
harvest_yearly_top = harvest_yearly[harvest_yearly.plots_cut .== topscenario[1] .&& harvest_yearly.intensity .== topscenario[2],:]

carbon_balance_yearly_bau = carbon_balance_yearly[carbon_balance_yearly.plots_cut .== "BAU",:]




top_carbon_balance = final_carbon_balance[1,"carbon_balance"]
carbon_gain = top_carbon_balance - initial_carbon_balance

harvest_century_top = @subset(harvest_century, :plots_cut .== topscenario[1], :intensity .== topscenario[2] )[1,"total_harvest_century"] #"scenario" .== "BAU")
harvest_needed_century  = (harvest_century_top - harvest_century_bau) * 5 # because bicaff report harvesting evey 5 years

harvest_yearly_top = @subset(harvest_yearly, :plots_cut .== topscenario[1], :intensity .== topscenario[2] ) 

@df carbon_balance_yearly plot(:year, :carbon_balance, legend=:topleft, colour=:grey, label=nothing, title="Yearly carbon balance (stockage + cumulative subst.)")
@df carbon_balance_yearly_top plot!(:year, :carbon_balance, legend=:topleft, colour=:red,label="best scenario - $(topscenario[1]) $(topscenario[2])")
@df carbon_balance_yearly_bau plot!(:year, :carbon_balance, legend=:topleft, colour=:green,label="BAU")

@df harvest_yearly plot(:year, :yearly_harvested, legend=:topright, colour=:grey, label=nothing, title="Yearly harvesting", ylabel="m³")
@df harvest_yearly_top plot!(:year, :yearly_harvested, legend=:topright, colour=:red,label="best scenario - $(topscenario[1]) $(topscenario[2])")
@df harvest_yearly_bau plot!(:year, :yearly_harvested, legend=:topright, colour=:green,label="BAU")

groups      = groupby(bicaff,["plots_cut","intensity","Region"])
harvest_by_region = combine(groups) do subdf # slower
    (total_harvested=sum(subdf.yearly_harvested_m3).*5,)
end
harvest_by_region = @subset(harvest_by_region, ((:plots_cut .== topscenario[1] .&& :intensity .== topscenario[2]) .|| :plots_cut .== "BAU") )
harvest_by_region = harvest_by_region[:,Not(:intensity)]
harvest_by_region = unstack(harvest_by_region,"plots_cut","total_harvested")
harvest_by_region.additionalHarvested = harvest_by_region.Managed-harvest_by_region.BAU
@df harvest_by_region bar(:Region, :additionalHarvested, legend=:topleft, label=nothing, title="Additional harvest required by region", xrotation=45, bottom_margin=10mm)


categories = harvest_by_region.Region
maximum(harvest_by_region.additionalHarvested)
fig = plot()
addtraces!(fig, 
           scatterpolar(r=harvest_by_region.additionalHarvested,
                        theta=categories,
                        fill="toself",
                        name="Additional Harvest",
                        #title="Additional harvest by region (BICAFF)"
                        ))
relayout!(fig,  polar=attr(radialaxis=attr(visible=true, range=[0, maximum(harvest_by_region.additionalHarvested)])),
          showlegend=false, margin=5mm)
display(fig)

# INPUT DATA:
n = length(harvest_by_region.Region)
R1 = copy(harvest_by_region.additionalHarvested)
push!(R1,R1[1])
labels = harvest_by_region.Region
upl = maximum(harvest_by_region.additionalHarvested)
# PLOT:
justifyme(θ) = (0≤θ<π/2 || 3π/2<θ≤2π) ? :left : (π/2<θ<3π/2) ? :right : :center
θ = LinRange(0, 2pi, n+1)
z = 1.15*exp.(im*2π*(0:n-1)/n)
plot(θ, R1, proj=:polar, ms=3, m=:o, c=:blue, fill=(true,:blues), fa=0.4, lims=(0,upl), xaxis=false,right_margin=40mm,label=nothing)
annotate!(real.(z), imag.(z), text.(labels,12,justifyme.(θ[1:n]),"Computer Modern"))



groups      = groupby(bicaff,["plots_cut","intensity","type"])
harvest_by_species = combine(groups) do subdf # slower
    (tot_harvested=sum(subdf.yearly_harvested_m3) .* 5,)
end
harvest_by_species = @subset(harvest_by_species, ((:plots_cut .== topscenario[1] .&& :intensity .== topscenario[2]) .|| :plots_cut .== "BAU") )
harvest_by_species = harvest_by_species[:,Not(:intensity)]
harvest_by_species = unstack(harvest_by_species,"plots_cut","tot_harvested")
harvest_by_species.additionalHarvested = harvest_by_species.Managed-harvest_by_species.BAU
@df harvest_by_species bar(:type, :additionalHarvested, legend=:topleft, label=nothing, title="Additional harvest required by species group", xrotation=45, bottom_margin=10mm)
