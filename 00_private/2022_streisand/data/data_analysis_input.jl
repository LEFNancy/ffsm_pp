# todelete

using Pkg
Pkg.activate(".")
using DataFrames, DataFramesMeta,  LAJuliaUtils, OdsIO, CSV, StatsPlots
#using NamedColors,  StatsPlots
ROOT = @__DIR__
rawi_filename = joinpath(ROOT,"raw","20220926carbon_balance_BICAFF.csv")
rawi =  CSV.read(rawi_filename,DataFrame)
fields = names(rawi)
unique(rawi.type)
a=1
#=
"Auvergne-Rhone-Alpes"
"Bourgogne-Franche-Comte"
"Bretagne"
"Centre-Val_de_Loire" => "Cen",
"Corse" => "Cor",
"Grand_Est" => "AlsChaLor",
"Hauts-de-France"
"ile-de-France"
"Normandie"
"Nouvelle-Aquitaine" => "AquPoi",
"Occitanie"
"Pays_de_la_Loire"
"Provence-Alpes-Cote_d_Azur" => "LanPro",

=> "IleNorPic",

 => "AuvLim",
 => "BasHau",
 => "BouFra",
 => "BrePay",



 
 => "Mid",
 => "Rho",
=#




regionsi = unique(rawi.Region)
yearsi = unique(rawi.year)
year_start = minimum(yearsi)
year_ends  = maximum(yearsi)
cut_options = unique(rawi.plots_cut)

# Looking at the total harvesting by scenario/intensity combination
scenario_groups      = groupby(rawi,["plots_cut","intensity"])
scenario_year_groups = groupby(rawi,["plots_cut","intensity","year"])
harvest_yearly  = combine(scenario_year_groups, "yearly_harvested_m3" => sum => "yearly_harvested")
harvest_century = combine(scenario_groups,      "yearly_harvested_m3" => sum => "total_harvest_century")
harvest_yearly_bau = @subset(harvest_yearly, :plots_cut .== "BAU" ) #"scenario" .== "BAU")
harvest_century_bau = @subset(harvest_century, :plots_cut .== "BAU" )[1,"total_harvest_century"] #"scenario" .== "BAU")

carbon_balance_yearly  = combine(scenario_year_groups, "carbon_balance" => sum => "carbon_balance")

initial_carbon_balance = @subset(carbon_balance_yearly , :plots_cut .== "BAU", :year .== year_start)[:,"carbon_balance"][1]
final_carbon_balance   = @subset(carbon_balance_yearly , :year .== year_ends )
sort!(final_carbon_balance, ["carbon_balance"], rev=true)

topscenario = final_carbon_balance[1,"plots_cut"],final_carbon_balance[1,"intensity"]
carbon_balance_yearly_top = carbon_balance_yearly[carbon_balance_yearly.plots_cut .== topscenario[1] .&& carbon_balance_yearly.intensity .== topscenario[2],:]
harvest_yearly_top = harvest_yearly[harvest_yearly.plots_cut .== topscenario[1] .&& harvest_yearly.intensity .== topscenario[2],:]

carbon_balance_yearly_bau = carbon_balance_yearly[carbon_balance_yearly.plots_cut .== "BAU",:]


top_carbon_balance = final_carbon_balance[1,"carbon_balance"]
carbon_gain = top_carbon_balance - initial_carbon_balance

harvest_century_top = @subset(harvest_century, :plots_cut .== topscenario[1], :intensity .== topscenario[2] )[1,"total_harvest_century"] #"scenario" .== "BAU")
harvest_needed_century  = harvest_century_top - harvest_century_bau

harvest_yearly_top = @subset(rawi, :plots_cut .== topscenario[1], :intensity .== topscenario[2] ) 

@df carbon_balance_yearly plot(:year, :carbon_balance, legend=:topleft, colour=:grey, label=nothing, title="Yearly carbon balance (stockage + cumulative subst.)")
@df carbon_balance_yearly_top plot!(:year, :carbon_balance, legend=:topleft, colour=:red,label="best scenario - $(topscenario[1]) $(topscenario[2])")
@df carbon_balance_yearly_bau plot!(:year, :carbon_balance, legend=:topleft, colour=:green,label="BAU")

@df harvest_yearly plot(:year, :yearly_harvested, legend=:topleft, colour=:grey, label=nothing, title="Yearly harvesting", ylabel="m³")
@df harvest_yearly_top plot!(:year, :yearly_harvested, legend=:topleft, colour=:red,label="best scenario - $(topscenario[1]) $(topscenario[2])")
@df harvest_yearly_bau plot!(:year, :yearly_harvested, legend=:topleft, colour=:green,label="BAU")


oldRegions = ods_read("ffsmInput_streisand.ods",sheetName="regions",range=((9,1),(20,2)),retType="Matrix")
newRegions = ods_read("ffsmInput_streisand.ods",sheetName="regions",range=((9,1),(21,3)),retType="Matrix")
ffsm_species = ods_read("ffsmInput_streisand.ods",sheetName="forTypes",range=((5,1),(10,2)),retType="Matrix")

"Coniferous"
"Broadl" 

"Fut_Feu" => "Broadl"
"Fut_Mix" => "Mixed"
"Fut_Con" => "Coniferous"
"Mel_Feu" => "Broadl"
"Mel_Mix"  => "Mixed"
"Tail_Feu" => "Broadl"


