
using DataFrames, DataFramesMeta,  LAJuliaUtils, OdsIO, NamedColors,  StatPlots, Plots, CSV

##########################################################
### Settings and definitions
##########################################################

scenarios = ["default"]

allScenariosInAFile = false

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


baseIDir        = "results_temp";
odsOut          = "juliaOut/formattedOutput_temp.ods"
odsOut_template = "juliaOut/formattedOutput_template.ods"
chartOutDir     = "juliaOut"

years       = collect(2011:2100)
years5      = collect(2015:5:2100)
years10     = collect(2020:10:2100)

problematicCols = Dict("freeDim" => Union{Missing,Int64})

##########################################################
### Data import
##########################################################

forestData_byscenario  = []
productData_byscenario = []
carbonData_byscenario  = []


if allScenariosInAFile
    push!(forestData_byscenario,CSV.read(baseIDir*"/forestData.csv"; delim=';', missingstring=""));
    push!(productData_byscenario,CSV.read(baseIDir*"/productData.csv"; delim=';', missingstring="", types=problematicCols));
    push!(carbonData_byscenario,CSV.read(baseIDir*"/carbonBalance.csv"; delim=';', missingstring=""));
else
    forestData_byscenario = [CSV.read(baseIDir*"/forestData_"*scenario*".csv"; delim=';', missingstring="") for scenario in scenarios];
    productData_byscenario = [CSV.read(baseIDir*"/productData_"*scenario*".csv"; delim=';', types=problematicCols) for scenario in scenarios];
    carbonData_byscenario = [CSV.read(baseIDir*"/carbonBalance_"*scenario*".csv"; delim=';', missingstring="") for scenario in scenarios];
end

forVars = vcat(forestData_byscenario...)
prodVars = vcat(productData_byscenario...)
cVars = vcat(carbonData_byscenario...)
[delete!(dfi, [:country,]) for dfi in [forVars,prodVars,cVars]] # delete columns country

print("Data loaded")

##########################################################
### Data clean-up and initialisation
##########################################################
forVars[:forGroup] = map(k->fgroup_map[k], forVars[:forType])
#[df[:region] = map(k->reg_map[k], df[:region]) for df in [forVars,cVars]]
prodVars[:prodLev] = map(k->pgroup_map[k], prodVars[:prod])

# Adding totals to carbon variables
cVars_piv = pivot(cVars,[:scen,:year,:region],:balItem,:value,
                sort = [:scen,:year],
            )
cVars_piv[:STOCKS] = cVars_piv[:STOCK_INV] + cVars_piv[:STOCK_EXTRA] + cVars_piv[:STOCK_PRODUCTS]
cVars_piv[:EM_NET] = cVars_piv[:EM_ENSUB] + cVars_piv[:EM_FOROP] + cVars_piv[:EM_MATSUB]
cVars_piv[:TOT_BAL] =  cVars_piv[:STOCKS] + cVars_piv[:EM_NET]
cVars = stack(cVars_piv) # setting it back in long format but now with totals

#cp(odsOut_template, odsOut; remove_destination=true) # copy template odsFile. Don't forget CTRL+Shift+F9 to update the spreadsheet

##########################################################
### Elaborations
##########################################################

################ Tables..

##### Forest Variables

# Forest area and forest volumes
forAreaReg = pivot(forVars,[:scen, :region, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["forArea"],
               #:year    => years
               )
            )
forAreaNat = pivot(forVars,[:scen, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["forArea"],
               #:year    => years
               )
            )
forAreaNatNames = names(forAreaNat)
forAreaNat[:region] = "France"
forAreaNat = forAreaNat[vcat(forAreaNatNames[1:1],:region,forAreaNatNames[2:end])]
forArea = vcat(forAreaReg,forAreaNat)
ods_write(odsOut,Dict(("forArea",1,1) => forArea )  )

forVolReg = pivot(forVars,[:scen, :region, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["vol"],
               #:year    => years,
               )
            )
forVolNat = pivot(forVars,[:scen, :forGroup,],:year,:value,
             filter = Dict(
               :parName => ["vol"],
               #:year    => years,
               )
            )
forVolNatNames = names(forVolNat)
forVolNat[:region] = "France"
forVolNat = forVolNat[vcat(forVolNatNames[1:1],:region,forVolNatNames[2:end])]
forVol = vcat(forVolReg,forVolNat)
ods_write(odsOut,Dict(("forVol",1,1) => forVol )  )

##### Mkt & Policy Variables

# Product balance
prodBalance = pivot(prodVars,[:year,:scen,:prodLev,:prod],:parName,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa"],
                        :year    => [2010,2015]),
                    sort = [:year,:scen,:prodLev,:prod],
              )
ods_write(odsOut,Dict(("prodBalance",1,1) => prodBalance )  )

# Product balance all years
prodBalanceAll5Y = pivot(prodVars,[:parName,:prodLev,:prod,:scen,],:year,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa"],
                        :year    => years5),
                    sort = [:parName,:prodLev,:prod,:scen,],
              )
ods_write(odsOut,Dict(("prodBalanceAll5Y",1,1) => prodBalanceAll5Y )  )

# Preparation of policy variables
pol1 = by(forVars, [:scen,:parName,:region,:year]) do df
   DataFrame(value = sum(df[:value]))
end
pol2 = by(prodVars, [:scen,:parName,:region,:year]) do df
   DataFrame(value = sum(df[:value]))
end
pol=vcat(pol1,pol2)
policy_vars = ["polBal_mktDirInt_s", "polBal_mktDirInt_d", "polBal_trSub", "polBal_tcSub", "polBal_fiSub", "surplus_prod", "surplus_cons"]
pol = pol[ [i in policy_vars for i in pol[:parName]] ,:]

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
cBalCumNatNames = names(cBalCumNat)
cBalCumNat[:region] = "France"
cBalCumNat = cBalCumNat[vcat(cBalCumNatNames[1:1],:region,cBalCumNatNames[2:end])]
cBalCum = vcat(cBalCumReg,cBalCumNat)

# reporting stocks in comparision to the first year
ymin = minimum(cVars[:year])
ymax = maximum(cVars[:year])
[cBalCum[Symbol(string(i))] = cBalCum[Symbol(string(i))] - cBalCum[Symbol(ymin)]  for i = collect(ymax:-1:ymin+1)]
delete!(cBalCum, [Symbol(ymin)])
ods_write(odsOut,Dict(("cBalCum",1,1) => cBalCum )  )

# Carbon balance yearly
cBal = cBalCum
[cBal[Symbol(string(i))] = cBal[Symbol(string(i))] - cBal[Symbol(string(i-1))]  for i = collect(ymax:-1:ymin+2)] # computing diff
ods_write(odsOut,Dict(("cBal",1,1) => cBal )  )
