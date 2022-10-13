cd(@__DIR__)
using Pkg
Pkg.activate(".")
using DataFrames, DataFramesMeta,  LAJuliaUtils, OdsIO, NamedColors,  StatsPlots,  CSV

##########################################################
### Settings and definitions
##########################################################

scenarios = ["default", "increased_demand_fuelwood"]

allScenariosInAFile = true

fgroup_map = Dict(
 "Fut_Feu"         => "Broadleaved",
 "Fut_Mix"         => "Mixed",
 "Fut_Con"         => "Coniferous",
 "Mel_Feu"         => "Broadleaved",
 "Mel_Mix"         => "Mixed",
 "Tail_Feu"        => "Broadleaved",
  missing          => "All forests",
 "broadL"          => "Broadleaved",
 "mixedBC"         => "Mixed",
 "con"             => "Coniferous",
 "Reb_Fut_Con"     => "Coniferous",
 "Reb_Fut_Feu"     => "Broadleaved",
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
odsOut          = joinpath(@__DIR__,"juliaOut/formattedOutput.ods")
odsOut_template = joinpath(@__DIR__,"juliaOut/formattedOutput_template.ods")
chartOutDir     = joinpath(@__DIR__,"juliaOut")

years       = collect(2011:2100)
years5      = collect(2015:5:2100)
years10     = collect(2020:10:2100)

problematicCols = Dict("freeDim" => Union{Missing,Int64})

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
    cVarsVars = cVars[in.(cVars[:,"scen"],Ref(scenarios)),:] 
else
    forestData_byscenario = [CSV.read(baseIDir*"/forestData_"*scenario*".csv", DataFrame; delim=';', missingstring="") for scenario in scenarios];
    productData_byscenario = [CSV.read(baseIDir*"/productData_"*scenario*".csv", DataFrame; delim=';', types=problematicCols) for scenario in scenarios];
    carbonData_byscenario = [CSV.read(baseIDir*"/carbonBalance_"*scenario*".csv", DataFrame; delim=';', missingstring="") for scenario in scenarios];
    forVars = vcat(forestData_byscenario...)
    prodVars = vcat(productData_byscenario...)
    cVars = vcat(carbonData_byscenario...)
end


[select!(dfi, Not(["country"])) for dfi in [forVars,prodVars,cVars]] # delete columns country

print("Data loaded")

##########################################################
### Data clean-up and initialisation
##########################################################
forVars.forGroup = map(k->fgroup_map[k], forVars.forType)
#[df[:region] = map(k->reg_map[k], df[:region]) for df in [forVars,cVars]]
prodVars.prodLev = map(k->pgroup_map[k], prodVars.prod)

# Adding totals to carbon variables
cVars_piv = pivot(cVars,[:scen,:year,:region],:balItem,:value,
                sort = [:scen,:year],
            )
cVars_piv.STOCKS = cVars_piv.STOCK_INV + cVars_piv.STOCK_EXTRA + cVars_piv.STOCK_PRODUCTS
cVars_piv.EM_NET = cVars_piv.EM_ENSUB + cVars_piv.EM_FOROP + cVars_piv.EM_MATSUB
cVars_piv.TOT_BAL =  cVars_piv.STOCKS + cVars_piv.EM_NET
cVars = stack(cVars_piv) # setting it back in long format but now with totals

#cp(odsOut_template, odsOut; force=true) # copy template odsFile. Don't forget CTRL+Shift+F9 to update the spreadsheet

##########################################################
### Elaborations
##########################################################

################ Tables..

##### Forest Variables

# Forest area and forest volumes
forAreaReg = pivot(forVars,[:scen, :region, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["forArea"],
               :year    => years
               )
            )
forAreaNat = pivot(forVars,[:scen, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["forArea"],
               :year    => years
               )
            )
#forAreaNatNames = names(forAreaNat)
#forAreaNat[:region] = "France"
#forAreaNat = forAreaNat[vcat(forAreaNatNames[1:1],:region,forAreaNatNames[2:end])]
insertcols!(forAreaNat, 2, :region => "France" )
forArea = vcat(forAreaReg,forAreaNat)
ods_write(odsOut,Dict(("forArea",1,1) => forArea )  )

forVolReg = pivot(forVars,[:scen, :region, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["vol"],
               :year    => years,
               )
            )
forVolNat = pivot(forVars,[:scen, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["vol"],
               :year    => years,
               )
            )
#forVolNatNames = names(forVolNat)
#forVolNat[:region] = "France"
#forVolNat = forVolNat[vcat(forVolNatNames[1:1],:region,forVolNatNames[2:end])]
insertcols!(forVolNat, 2, :region => "France" )
forVol = vcat(forVolReg,forVolNat)
ods_write(odsOut,Dict(("forVol",1,1) => forVol )  )

hVReg = pivot(forVars,[:scen, :region, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["hV"],
               :year    => years,
               )
            )
hVNat = pivot(forVars,[:scen, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["hV"],
               :year    => years,
               )
            )
#forVolNatNames = names(forVolNat)
#forVolNat[:region] = "France"
#forVolNat = forVolNat[vcat(forVolNatNames[1:1],:region,forVolNatNames[2:end])]
insertcols!(hVNat, 2, :region => "France" )
hVol = vcat(hVReg,hVNat)
ods_write(odsOut,Dict(("hVol",1,1) => hVol )  )






##### Mkt & Policy Variables

# Product balance
prodBalance = pivot(prodVars,[:year,:scen,:prodLev,:prod],:parName,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa"],
                        :year    => [2013,2014,2015,2016,2017,2018]),
                    sort = [:year,:scen,:prodLev,:prod],
              )
ods_write(odsOut,Dict(("prodBalance",1,1) => prodBalance )  )

# fuelW regional product balance
prodBalanceFuelW = pivot(prodVars,[:year,:scen,:region,:prodLev,:prod],:parName,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa"],
                        :year    => [2013,2014,2015,2016,2017,2018,2019,2020,2021,2022],
                        :prod    => ["fuelW"]),
                    sort = [:year,:scen,:prodLev,:prod],
              )
ods_write(odsOut,Dict(("prodBalanceFuelW",1,1) => prodBalanceFuelW )  )

# Product balance all years
prodBalanceAll5Y = pivot(prodVars,[:parName,:prodLev,:prod,:scen,],:year,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa"],
                        :year    => years5),
                    sort = [:parName,:prodLev,:prod,:scen,],
              )
ods_write(odsOut,Dict(("prodBalanceAll5Y",1,1) => prodBalanceAll5Y )  )

# Preparation of policy variables
pol1 = combine(groupby(forVars,[:scen,:parName,:region,:year]) ,  "value" => sum => "value")
pol2 = combine(groupby(prodVars,[:scen,:parName,:region,:year]) ,  "value" => sum => "value")

pol=vcat(pol1,pol2)
policy_vars = ["polBal_mktDirInt_s", "polBal_mktDirInt_d", "polBal_trSub", "polBal_tcSub", "polBal_fiSub", "surplus_prod", "surplus_cons"]
pol = pol[ [i in policy_vars for i in pol.parName] ,:]

# Policy cost analysis (national)
polCosts_nat = pivot(pol,[:scen,:parName],:year,:value,
            filter = Dict(
                :year    => years5
            ),
            sort = [:scen, :parName]
)
ods_write(odsOut,Dict(("polCosts_nat",1,1) => polCosts_nat )  )

# Policy cost analysis (regional)
polCosts_reg = pivot(pol,[:scen,:parName],:region,:value,
            filter = Dict(
                :year    => 2030
            ),
            sort = [:scen, :parName]
)
ods_write(odsOut,Dict(("polCosts_reg",1,1) => polCosts_reg )  )

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



# For vol analysis

forVolRatio = forVol[:,["scen","region","forGroup","2011","2100"]]
forVolRatio.ratio = forVolRatio[!,"2100"] ./ forVolRatio[!,"2011"]
forVolRatioDefault = forVolRatio[forVolRatio.scen .== "default",:]
forVolRatioIncreasedDemand = forVolRatio[forVolRatio.scen .== "increased_demand_fuelwood",:]
plotColors = [:green :blue :orange]
@df forVolRatioDefault density(:ratio, group=(:forGroup), title="Distribution of regional vol ratios 2100 vs 2011", linestyle = :solid, color=plotColors, label = reshape(sort(unique(:forGroup)) .* " (bau)" , (1,:) ) , linewidth=3)
@df forVolRatioIncreasedDemand density!(:ratio, group=(:forGroup), title="Distribution of regional vol ratios 2100 vs 2011", linestyle = :dot, color=plotColors, label = reshape(sort(unique(:forGroup)) .* " (increased demand)" , (1,:) ), linewidth=3 )

savefig("volRatioFuelWoodShock.svg")

forVolRatios = forVolRatio[forVolRatio.scen .== "increased_demand_fuelwood",:]