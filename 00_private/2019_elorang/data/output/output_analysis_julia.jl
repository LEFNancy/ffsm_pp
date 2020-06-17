
using DataFrames, DataFramesMeta, LAJuliaUtils, OdsIO, NamedColors,  StatsPlots, Plots, CSV, LaTeXStrings

##########################################################
### Settings and definitions
##########################################################

cd(@__DIR__)

#crossed elasticities (baseline scenarios)
#scenarios = ["es_d_0", "es_d_1", "es_d_03", "es_d_m1", "es_d_m03","es_d_change"]

# fixed parameter : es_d_02, es_d_m02, ed_d_m1, es_d_0, m_shock, m_50
parameter = "es_d_m02"

scenarios = ["m_rpW_10_es_d_m1", "m_rpW_70_es_d_m1"]
#all scenarii
scenarios = ["m_rpW_10_es_d_02", "m_rpW_30_es_d_02", "m_rpW_50_es_d_02", "m_rpW_70_es_d_02", "m_rpW_shock_es_d_02", "m_rpW_10_es_d_m02", "m_rpW_30_es_d_m02", "m_rpW_50_es_d_m02", "m_rpW_70_es_d_m02", "m_rpW_shock_es_d_m02", "m_rpW_10_es_d_m1", "m_rpW_30_es_d_m1", "m_rpW_50_es_d_m1", "m_rpW_70_es_d_m1", "m_rpW_shock_es_d_m1", "m_rpW_10_es_d_0", "m_rpW_30_es_d_0", "m_rpW_50_es_d_0",  "m_rpW_70_es_d_0", "m_rpW_shock_es_d_0"]

#transformation costs analysis (i.e. cost of recycling)
if parameter == "es_d_02"
    #low complementarity 0,2
    scenarios = ["m_rpW_10_es_d_02", "m_rpW_30_es_d_02", "m_rpW_50_es_d_02","m_rpW_70_es_d_02"]
elseif parameter == "es_d_m02"
    #low substitution -0,2
    scenarios = ["m_rpW_10_es_d_m02", "m_rpW_30_es_d_m02", "m_rpW_50_es_d_m02","m_rpW_70_es_d_m02"]
elseif parameter == "es_d_m1"
    #high substitution -1
    scenarios = ["m_rpW_10_es_d_m1", "m_rpW_30_es_d_m1", "m_rpW_50_es_d_m1","m_rpW_70_es_d_m1"]
elseif parameter == "es_d_0"
    #independance 0
    scenarios = ["m_rpW_10_es_d_0", "m_rpW_30_es_d_0", "m_rpW_50_es_d_0","m_rpW_70_es_d_0"]
elseif parameter == "m_50"
    #by substitution hypothesis
    #standard costs (50)
    scenarios = ["m_rpW_50_es_d_02", "m_rpW_50_es_d_m02", "m_rpW_50_es_d_m1","m_rpW_50_es_d_0"]
elseif parameter == "m_shock"
    #shocks of costs
    scenarios = ["m_rpW_shock_es_d_02", "m_rpW_shock_es_d_m02", "m_rpW_shock_es_d_m1","m_rpW_shock_es_d_0"]
elseif parameter == "es_d_neg"
    scenarios = ["m_rpW_10_es_d_m02", "m_rpW_30_es_d_m02", "m_rpW_50_es_d_m02","m_rpW_70_es_d_m02", "m_rpW_10_es_d_m1", "m_rpW_30_es_d_m1", "m_rpW_50_es_d_m1","m_rpW_70_es_d_m1"]
else
    print("no specific set of scenarii loaded")
end

# Set the final year for results displayed. In order to avoid long-term trends, use 2032 max (before change of curve for international prices)
endyear = 2092
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
 "hardWRoundW"       => "priProd",
 "softWRoundW"       => "priProd",
 "pulpWFuelW"        => "priProd",
 "fuelW"             => "secProd",
 "hardWSawnW"        => "secProd",
 "softWSawnW"        => "secProd",
 "plyW"              => "secProd",
 "pulpW"             => "secProd",
 "pannels"           => "secProd",
 "sortedPulpWFuelW"  => "priProd",
 "recycledPulpW"     => "secProd",
)

cbal_map = Dict(
    "EM_ENSUB"       => "1Emissions",
    "EM_FOROP"       => "1Emissions",
    "EM_MATSUB"      => "1Emissions",
    "EM_TRANS"       => "1Emissions",
    "STOCK_EXTRA"    => "0Pools",
    "STOCK_INV"      => "0Pools",
    "STOCK_PRODUCTS" => "0Pools",
)


baseIDir        = "results";
odsOut          = "juliaOut/formattedOutput.ods"
odsOut_template = "juliaOut/formattedOutput_template.ods"
chartOutDir     = "juliaOut"

years       = collect(2011:endyear)
years5      = collect(2015:5:endyear)
years10     = collect(2020:10:endyear)
years20     = collect(2020:20:endyear)

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
#[delete!(dfi, [:country,]) for dfi in [forVars,prodVars,cVars]] # delete columns country
[select!(dfi, Not([:country,])) for dfi in [forVars,prodVars,cVars]]

print("Data loaded\n")

##########################################################
### Data clean-up and initialisation
##########################################################

#Filtering out junk (keeping only scenarios of interest)
forVars = forVars[ [i in scenarios for i in forVars.scen],:]
prodVars = prodVars[ [i in scenarios for i in prodVars.scen],:]
cVars = cVars[ [i in scenarios for i in cVars.scen],:]

forVars.forGroup = map(k->fgroup_map[k], forVars.forType)
#[df[:region] = map(k->reg_map[k], df[:region]) for df in [forVars,cVars]]
prodVars.prodLev = map(k->pgroup_map[k], prodVars.prod)

# Adding totals to carbon variables
cVars_piv = pivot(cVars, [:scen,:year,:region], :balItem, :value, sort = [:scen,:year],)
cVars_piv.STOCKS = cVars_piv.STOCK_INV + cVars_piv.STOCK_EXTRA + cVars_piv.STOCK_PRODUCTS
cVars_piv.EM_NET = cVars_piv.EM_ENSUB + cVars_piv.EM_FOROP + cVars_piv.EM_MATSUB + cVars_piv.EM_TRANS
cVars_piv.TOT_BAL =  cVars_piv.STOCKS + cVars_piv.EM_NET
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
forAreaNat.region = "France"
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
forVolNat.region = "France"
forVolNat = forVolNat[vcat(forVolNatNames[1:1],:region,forVolNatNames[2:end])]
forVol = vcat(forVolReg,forVolNat)
ods_write(odsOut,Dict(("forVol",1,1) => forVol )  )

##### Mkt & Policy Variables

# Product balance
prodBalance = pivot(prodVars,[:year,:scen,:prodLev,:prod],:parName,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa","dl","sl"],
                        :year    => years),
                    sort = [:year,:scen,:prodLev,:prod],
              )
ods_write(odsOut,Dict(("prodBalance",1,1) => prodBalance )  )

# Product balance all 5 years
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


rateOfIn = pivot(prodVars,[:scen,:prod],:parName,:value,
            filter = Dict(
                :parName => ["in_recycled","st"],
                :year    => [2015],
                :prod    => ["sortedPulpWFuelW"],
            )
)

# Alternative pivot for carbon
cBalCumReg2 = pivot(cVars, [:scen,:region,:year], :variable, :value, sort = [:scen],)
cBalCumNat2 = pivot(cVars,[:scen,:year], :variable, :value, sort = [:scen],)
cBalCumNatNames2 = names(cBalCumNat2)
cBalCumNat2[:region] = "France"
cBalCumNat2 = cBalCumNat2[vcat(cBalCumNatNames2[1:1],:region,cBalCumNatNames2[2:end])]
cBalCum2 = vcat(cBalCumReg2,cBalCumNat2)
# Missing : the reporting of stocks

print("Data formatted\n")

# PLOTS
#
# if a year limit is needed :
# .&(prodVars.year .< XXXX)

names(prodVars)
names(prodBalance)

#using TableView
#showtable(prodBalance)

# Change backend for plot instruments
#pyplot()
#gr()
#plotlyjs()

#using LaTeXStrings

#test_prodBalance = @where(prodBalance, :prod .== "pulpW", :scen .!= "es_d_01", :scen .!= "es_d_m01", :scen .!= "es_d_02", :scen .!= "es_d_m02")
#showtable(test_prodBalance)

if parameter == "es_d_02"

# TESTS OF THE SENSITIVITY TO TRANSFORMATION COSTS

# LOW COMPLEMENTARITY : es_d = 0,2

    #supply of waste
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "ST spWfW, es_d = 0,2")
    savefig("sl_spwfw_es_d_02_by_m.pdf")

    #demand of recycled pulp
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT rpW, es_d = 0,2")
    savefig("dt_rpw_es_d_02_by_m.pdf")

    DLrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dl, group=:scen, title = "DL rpW")
    DArpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :da, group=:scen, title = "DA rpW")

    #full recycling industry (supply waste and demand recycled pulp)
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, legend = false, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years20, title = "DT rpW, es_d = 0,2")
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years20, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "ST spWfW, es_d = 0,2")
    plot(DTrpW, SLspWfW,layout=(1,2))
    savefig("fullrecy_es_d_02_by_m.pdf")

    #demand of wood-pulp
    DTpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW") ,:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of wood-pulp (Mm³)",  xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT pW, es_d = 0,2")
    savefig("dt_pw_es_d_02_by_m.pdf")
    DTpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2031) .& (prodBalance.prod .== "pulpW") ,:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of wood-pulp (Mm³)",  xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"])#, title = "DT pW, es_d = 0,2")
    savefig("dt_pw_es_d_02_by_m_short.pdf")

    DLpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :dl, group=:scen, title = "DL pW")
    DApW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :da, group=:scen, title = "DA pW")

    #demand for other wood product
    DTfuelW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "fuelW"),:] plot(:year, :dt, group=:scen, xticks = years20, legend = false, xlabel = "year", ylabel = "total demand for fuelwood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT fuelW, es_d = 0,2")
    DTplyW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "plyW"),:] plot(:year, :dt, group=:scen, xticks = years20, xlabel = "year", ylabel = "total demand for plywood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT plyW, es_d = 0,2")
    plot(DTfuelW,DTplyW,layout=(1,2))
    savefig("dt_plyw_fw_es_d_02_by_m.pdf")

    #Effects on prices
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "sortedPulpWFuelW"), :] plot(:year, :value, group=:scen, title = "PL spWfW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "recycledPulpW"), :] plot(:year, :value, group=:scen, title = "PL rpW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "pulpW"), :] plot(:year, :value, group=:scen, title = "PL pW")

    print("figures with es_d = 0,2 saved")

###################################################################
###################################################################

elseif parameter == "es_d_m02"

# LOW SUBSTITUTION : es_d = -0,2

    #supply of waste
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "ST spWfW, es_d = -0,2")
    savefig("sl_spwfw_es_d_m02_by_m.pdf")
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2031) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years5, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"])#, title = "ST spWfW, es_d = -0,2")
    savefig("sl_spwfw_es_d_m02_by_m_short.pdf")

    #demand of recycled pulp
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT rpW, es_d = -0,2")
    savefig("dt_rpw_es_d_m02_by_m.pdf")
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2031) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years5, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"])#, title = "DT rpW, es_d = -0,2")
    savefig("dt_rpw_es_d_m02_by_m_short.pdf")

    DLrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dl, group=:scen, title = "DL rpW")
    DArpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :da, group=:scen, title = "DA rpW")

    #full recycling industry (supply waste and demand recycled pulp)
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, legend = false, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years20, title = "DT rpW, es_d = -0,2")
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years20, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "ST spWfW, es_d = -0,2")
    plot(DTrpW, SLspWfW,layout=(1,2))
    savefig("fullrecy_es_d_m02_by_m.pdf")
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2031) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, legend = false, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years5, title = "DT rpW, es_d = -0,2")
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2031) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years5, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "ST spWfW, es_d = -0,2")
    plot(DTrpW, SLspWfW,layout=(1,2))
    savefig("fullrecy_es_d_m02_by_m_short.pdf")

    #demand of wood-pulp
    DTpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW") ,:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of wood-pulp (Mm³)",  xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT pW, es_d = -0,2")
    savefig("dt_pw_es_d_m02_by_m.pdf")
    DTpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2031) .& (prodBalance.prod .== "pulpW") ,:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of wood-pulp (Mm³)",  xticks = years5, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"])#, title = "DT pW, es_d = -0,2")
    savefig("dt_pw_es_d_m02_by_m_short.pdf")

    DLpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :dl, group=:scen, title = "DL pW")
    DApW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :da, group=:scen, title = "DA pW")

    #demand for other wood product
    DTfuelW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "fuelW"),:] plot(:year, :dt, group=:scen, xticks = years20, legend = false, xlabel = "year", ylabel = "total demand for fuelwood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT fuelW, es_d = -0,2")
    DTplyW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "plyW"),:] plot(:year, :dt, group=:scen, xticks = years20, xlabel = "year", ylabel = "total demand for plywood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT plyW, es_d = -0,2")
    plot(DTfuelW,DTplyW,layout=(1,2))
    savefig("dt_plyw_fw_es_d_m02_by_m.pdf")
    DTfuelW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2031) .& (prodBalance.prod .== "fuelW"),:] plot(:year, :dt, group=:scen, xticks = years5, legend = false, xlabel = "year", ylabel = "total demand for fuelwood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"])#, title = "DT fuelW, es_d = -0,2")
    DTplyW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2031) .& (prodBalance.prod .== "plyW"),:] plot(:year, :dt, group=:scen, xticks = years5, xlabel = "year", ylabel = "total demand for plywood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"])#, title = "DT plyW, es_d = -0,2")
    plot(DTfuelW,DTplyW,layout=(1,2))
    savefig("dt_plyw_fw_es_d_m02_by_m_short.pdf")

    #Effects on prices
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "sortedPulpWFuelW"), :] plot(:year, :value, group=:scen, title = "PL spWfW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "recycledPulpW"), :] plot(:year, :value, group=:scen, title = "PL rpW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "pulpW"), :] plot(:year, :value, group=:scen, title = "PL pW")

    print("figures with es_d = -0,2 saved")

##################################################################
###################################################################

elseif parameter == "es_d_m1"

# HIGH SUBSTITUTION : es_d = -1

    #supply of waste
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "ST spWfW, es_d = -1")
    savefig("sl_spwfw_es_d_m1_by_m.pdf")

    #demand of recycled pulp
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT rpW, es_d = -1")
    savefig("dt_rpw_es_d_m1_by_m.pdf")

    DLrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dl, group=:scen, title = "DL rpW")
    DArpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :da, group=:scen, title = "DA rpW")

    #full recycling industry (supply waste and demand recycled pulp)
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, legend = false, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years20, title = "DT rpW, es_d = -1")
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years20, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "ST spWfW, es_d = -1")
    plot(DTrpW, SLspWfW,layout=(1,2))
    savefig("fullrecy_es_d_m1_by_m.pdf")

    #demand of wood-pulp
    DTpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW") ,:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of wood-pulp (Mm³)",  xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT pW, es_d = -1")
    savefig("dt_pw_es_d_m1_by_m.pdf")

    DLpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :dl, group=:scen, title = "DL pW")
    DApW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :da, group=:scen, title = "DA pW")

    #demand for other wood product
    DTfuelW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "fuelW"),:] plot(:year, :dt, group=:scen, xticks = years20, legend = false, xlabel = "year", ylabel = "total demand for fuelwood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT fuelW, es_d = -1")
    DTplyW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "plyW"),:] plot(:year, :dt, group=:scen, xticks = years20, xlabel = "year", ylabel = "total demand for plywood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT plyW, es_d = -1")
    plot(DTfuelW,DTplyW,layout=(1,2))
    savefig("dt_plyw_fw_es_d_m1_by_m.pdf")

    #Effects on prices
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "sortedPulpWFuelW"), :] plot(:year, :value, group=:scen, title = "PL spWfW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "recycledPulpW"), :] plot(:year, :value, group=:scen, title = "PL rpW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "pulpW"), :] plot(:year, :value, group=:scen, title = "PL pW")

    print("figures with es_d = -1 saved")

##################################################################
###################################################################

elseif parameter == "es_d_0"

# INDEPENDANT PRODUCTS : es_d = 0

    #supply of waste
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "ST spWfW, es_d = 0")
    savefig("sl_spwfw_es_d_0_by_m.pdf")

    #demand of recycled pulp
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT rpW, es_d = 0")
    savefig("dt_rpw_es_d_0_by_m.pdf")

    DLrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dl, group=:scen, title = "DL rpW")
    DArpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :da, group=:scen, title = "DA rpW")

    #full recycling industry (supply waste and demand recycled pulp)
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, legend = false, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years20, title = "DT rpW, es_d = 0")
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years20, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "ST spWfW, es_d = 0")
    plot(DTrpW, SLspWfW,layout=(1,2))
    savefig("fullrecy_es_d_0_by_m.pdf")

    #demand of wood-pulp
    DTpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW") ,:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of wood-pulp (Mm³)",  xticks = years10, label=["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT pW, es_d = 0")
    savefig("dt_pw_es_d_0_by_m.pdf")

    DLpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :dl, group=:scen, title = "DL pW")
    DApW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :da, group=:scen, title = "DA pW")

    #demand for other wood product
    DTfuelW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "fuelW"),:] plot(:year, :dt, group=:scen, xticks = years20, legend = false, xlabel = "year", ylabel = "total demand for fuelwood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT fuelW, es_d = 0")
    DTplyW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "plyW"),:] plot(:year, :dt, group=:scen, xticks = years20, xlabel = "year", ylabel = "total demand for plywood (Mm³)", label = ["m = 10" "m = 30" "m = 50" "m = 70" "cost shock"], title = "DT plyW, es_d = 0")
    plot(DTfuelW,DTplyW,layout=(1,2))
    savefig("dt_plyw_fw_es_d_0_by_m.pdf")

    #Effects on prices
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "sortedPulpWFuelW"), :] plot(:year, :value, group=:scen, title = "PL spWfW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "recycledPulpW"), :] plot(:year, :value, group=:scen, title = "PL rpW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "pulpW"), :] plot(:year, :value, group=:scen, title = "PL pW")

    print("figures with es_d = 0 saved")

##################################################################
###################################################################

elseif parameter == "m_50"

# TESTS OF THE SENSITIVITY TO CROSSED ELASTICITIES

# STANDARD PRICE m = 50

    #supply of waste
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years10, title = "ST spWfW")
    savefig("sl_spwfw_by_es_d.pdf")

    #demand of recycled pulp
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years10, title = "DT rpW")
    savefig("dt_rpw_by_es_d.pdf")

    DLrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dl, group=:scen, title = "DL rpW")
    DArpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :da, group=:scen, title = "DA rpW")

    #full recycling industry (supply waste and demand recycled pulp)
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, legend = false, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years20, title = "DT rpW")
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years20, title = "ST spWfW")
    plot(DTrpW, SLspWfW,layout=(1,2))
    savefig("fullrecy_by_es_d.pdf")

    #demand of wood-pulp
    DTpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW") ,:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of wood-pulp (Mm³)",  xticks = years10, title = "DT pW")
    savefig("dt_pw_by_es_d.pdf")

    DLpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :dl, group=:scen, title = "DL pW")
    DApW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :da, group=:scen, title = "DA pW")

    #demand for other wood product
    DTfuelW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "fuelW"),:] plot(:year, :dt, group=:scen, xticks = years20, legend = false, xlabel = "year", ylabel = "total demand for fuelwood (Mm³)", title = "DT fuelW")
    DTplyW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "plyW"),:] plot(:year, :dt, group=:scen, xticks = years20, xlabel = "year", ylabel = "total demand for plywood (Mm³)", title = "DT plyW")
    plot(DTfuelW,DTplyW,layout=(1,2))
    savefig("dt_plyw_fw_by_es_d.pdf")

    #Effects on prices
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "sortedPulpWFuelW"), :] plot(:year, :value, group=:scen, title = "PL spWfW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "recycledPulpW"), :] plot(:year, :value, group=:scen, title = "PL rpW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "pulpW"), :] plot(:year, :value, group=:scen, title = "PL pW")

    print("figures with sensitivity on es_d saved")

##################################################################
###################################################################

elseif parameter == "m_shock"

# SHOCK OF PRICE m = 50 TO m = 30

    #supply of waste
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years10, title = "ST spWfW")
    savefig("sl_spwfw_shock_by_es_d.pdf")

    #demand of recycled pulp
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years10, title = "DT rpW")
    savefig("dt_rpw_shock_by_es_d.pdf")

    DLrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dl, group=:scen, title = "DL rpW")
    DArpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :da, group=:scen, title = "DA rpW")

    #full recycling industry (supply waste and demand recycled pulp)
    DTrpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "recycledPulpW"),:] plot(:year, :dt, group=:scen, legend = false, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years20, title = "DT rpW")
    SLspWfW = @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "sortedPulpWFuelW"),:] plot(:year, :sl, group=:scen, xlabel="year", ylabel="local supply of waste (Mm³)", xticks = years20, title = "ST spWfW")
    plot(DTrpW, SLspWfW,layout=(1,2))
    savefig("fullrecy_shock_by_es_d.pdf")

    #demand of wood-pulp
    DTpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW") ,:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of wood-pulp (Mm³)",  xticks = years10, title = "DT pW")
    savefig("dt_pw_shock_by_es_d.pdf")

    DLpW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :dl, group=:scen, title = "DL pW")
    DApW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "pulpW"),:] plot(:year, :da, group=:scen, title = "DA pW")

    #demand for other wood product
    DTfuelW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "fuelW"),:] plot(:year, :dt, group=:scen, xticks = years20, legend = false, xlabel = "year", ylabel = "total demand for fuelwood (Mm³)", title = "DT fuelW")
    DTplyW =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2092) .& (prodBalance.prod .== "plyW"),:] plot(:year, :dt, group=:scen, xticks = years20, xlabel = "year", ylabel = "total demand for plywood (Mm³)", title = "DT plyW")
    plot(DTfuelW,DTplyW,layout=(1,2))
    savefig("dt_plyw_fw_shock_by_es_d.pdf")

    #Effects on prices
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "sortedPulpWFuelW"), :] plot(:year, :value, group=:scen, title = "PL spWfW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "recycledPulpW"), :] plot(:year, :value, group=:scen, title = "PL rpW")
    @df prodVars[(prodVars.year .> 2014) .& (prodVars.year .< 2092) .& (prodVars.region .== "Cen") .& (prodVars.parName .== "pl") .& (prodVars.prod .== "pulpW"), :] plot(:year, :value, group=:scen, title = "PL pW")

    print("figures with cost shocks and sensitivity on es_d saved")

##################################################################
###################################################################

elseif parameter == "es_d_neg"

# COMPARISON OF DIFFERENT SUBSTITUTION LEVELS es_d = -0,2 AND es_d = -1

    #demand of recycled pulp
    DTrpW1 =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2032) .& (prodBalance.prod .== "recycledPulpW") .& (prodBalance.scen .!= "m_rpW_10_es_d_m1") .& (prodBalance.scen .!= "m_rpW_30_es_d_m1") .& (prodBalance.scen .!= "m_rpW_50_es_d_m1") .& (prodBalance.scen .!= "m_rpW_70_es_d_m1"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", legend =false, xticks = years5, title = "DT rpW")
    DTrpW2 =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2032) .& (prodBalance.prod .== "recycledPulpW") .& (prodBalance.scen .!= "m_rpW_10_es_d_m02") .& (prodBalance.scen .!= "m_rpW_30_es_d_m02") .& (prodBalance.scen .!= "m_rpW_50_es_d_m02") .& (prodBalance.scen .!= "m_rpW_70_es_d_m02"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of recycled pulp (Mm³)", xticks = years5, title = "DT rpW")
    plot(DTrpW1,DTrpW2,layout=(1,2))
    savefig("dt_rpw_by_m_by_es_d_short.pdf")

    #demand of wood pulp
    DTpW1 =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2032) .& (prodBalance.prod .== "pulpW") .& (prodBalance.scen .!= "m_rpW_10_es_d_m1") .& (prodBalance.scen .!= "m_rpW_30_es_d_m1") .& (prodBalance.scen .!= "m_rpW_50_es_d_m1") .& (prodBalance.scen .!= "m_rpW_70_es_d_m1"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of pulp (Mm³)", legend =false, xticks = years5, title = "DT pW")
    DTpW2 =  @df prodBalance[(prodBalance.year .> 2014) .& (prodBalance.year .< 2032) .& (prodBalance.prod .== "pulpW") .& (prodBalance.scen .!= "m_rpW_10_es_d_m02") .& (prodBalance.scen .!= "m_rpW_30_es_d_m02") .& (prodBalance.scen .!= "m_rpW_50_es_d_m02") .& (prodBalance.scen .!= "m_rpW_70_es_d_m02"),:] plot(:year, :dt, group=:scen, xlabel="year", ylabel="total demand of pulp (Mm³)", xticks = years5, title = "DT pW")
    plot(DTpW1,DTpW2,layout=(1,2))
    savefig("dt_pw_by_m_by_es_d_short.pdf")

    print("figures with on m and es_d saved")

else

    print("Plots to be defined")

end
