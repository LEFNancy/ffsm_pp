
using DataFrames, DataFramesMeta,  LAJuliaUtils, OdsIO, NamedColors,  StatPlots, Plots, CSV #  PlotlyJS  Plots

##########################################################
### Settings and definitions
##########################################################
scenarios = [
    "mr0.1-ec0.0-ep0.0-ra0.0",
    "mr0.1-ec0.0-ep0.0-ra1.5",
    "mr0.1-ec0.0-ep0.0-ra3.0",
    "mr0.1-ec0.0-ep0.5-ra0.0",
    "mr0.1-ec0.0-ep0.5-ra1.5",
    "mr0.1-ec0.0-ep0.5-ra3.0",
    "mr0.1-ec0.0-ep1.0-ra0.0",
    "mr0.1-ec0.0-ep1.0-ra1.5",
    "mr0.1-ec0.0-ep1.0-ra3.0",
    "mr0.1-ec0.5-ep0.0-ra0.0",
    "mr0.1-ec0.5-ep0.0-ra1.5",
    "mr0.1-ec0.5-ep0.0-ra3.0",
    "mr0.1-ec0.5-ep0.5-ra0.0",
    "mr0.1-ec0.5-ep0.5-ra1.5",
    "mr0.1-ec0.5-ep0.5-ra3.0",
    "mr0.1-ec0.5-ep1.0-ra0.0",
    "mr0.1-ec0.5-ep1.0-ra1.5",
    "mr0.1-ec0.5-ep1.0-ra3.0",
    "mr0.1-ec1.0-ep0.0-ra0.0",
    "mr0.1-ec1.0-ep0.0-ra1.5",
    "mr0.1-ec1.0-ep0.0-ra3.0",
    "mr0.1-ec1.0-ep0.5-ra0.0",
    "mr0.1-ec1.0-ep0.5-ra1.5",
    "mr0.1-ec1.0-ep0.5-ra3.0",
    "mr0.1-ec1.0-ep1.0-ra0.0",
    "mr0.1-ec1.0-ep1.0-ra1.5",
    "mr0.1-ec1.0-ep1.0-ra3.0",
    "mr0.5-ec0.0-ep0.0-ra0.0",
    "mr0.5-ec0.0-ep0.0-ra1.5",
    "mr0.5-ec0.0-ep0.0-ra3.0",
    "mr0.5-ec0.0-ep0.5-ra0.0",
    "mr0.5-ec0.0-ep0.5-ra1.5",
    "mr0.5-ec0.0-ep0.5-ra3.0",
    "mr0.5-ec0.0-ep1.0-ra0.0",
    "mr0.5-ec0.0-ep1.0-ra1.5",
    "mr0.5-ec0.0-ep1.0-ra3.0",
    "mr0.5-ec0.5-ep0.0-ra0.0",
    "mr0.5-ec0.5-ep0.0-ra1.5",
    "mr0.5-ec0.5-ep0.0-ra3.0",
    "mr0.5-ec0.5-ep0.5-ra0.0",
    "mr0.5-ec0.5-ep0.5-ra1.5",
    "mr0.5-ec0.5-ep0.5-ra3.0",
    "mr0.5-ec0.5-ep1.0-ra0.0",
    "mr0.5-ec0.5-ep1.0-ra1.5",
    "mr0.5-ec0.5-ep1.0-ra3.0",
    "mr0.5-ec1.0-ep0.0-ra0.0",
    "mr0.5-ec1.0-ep0.0-ra1.5",
    "mr0.5-ec1.0-ep0.0-ra3.0",
    "mr0.5-ec1.0-ep0.5-ra0.0",
    "mr0.5-ec1.0-ep0.5-ra1.5",
    "mr0.5-ec1.0-ep0.5-ra3.0",
    "mr0.5-ec1.0-ep1.0-ra0.0",
    "mr0.5-ec1.0-ep1.0-ra1.5",
    "mr0.5-ec1.0-ep1.0-ra3.0",
    "mr0.9-ec0.0-ep0.0-ra0.0",
    "mr0.9-ec0.0-ep0.0-ra1.5",
    "mr0.9-ec0.0-ep0.0-ra3.0",
    "mr0.9-ec0.0-ep0.5-ra0.0",
    "mr0.9-ec0.0-ep0.5-ra1.5",
    "mr0.9-ec0.0-ep0.5-ra3.0",
    "mr0.9-ec0.0-ep1.0-ra0.0",
    "mr0.9-ec0.0-ep1.0-ra1.5",
    "mr0.9-ec0.0-ep1.0-ra3.0",
    "mr0.9-ec0.5-ep0.0-ra0.0",
    "mr0.9-ec0.5-ep0.0-ra1.5",
    "mr0.9-ec0.5-ep0.0-ra3.0",
    "mr0.9-ec0.5-ep0.5-ra0.0",
    "mr0.9-ec0.5-ep0.5-ra1.5",
    "mr0.9-ec0.5-ep0.5-ra3.0",
    "mr0.9-ec0.5-ep1.0-ra0.0",
    "mr0.9-ec0.5-ep1.0-ra1.5",
    "mr0.9-ec0.5-ep1.0-ra3.0",
    "mr0.9-ec1.0-ep0.0-ra0.0",
    "mr0.9-ec1.0-ep0.0-ra1.5",
    "mr0.9-ec1.0-ep0.0-ra3.0",
    "mr0.9-ec1.0-ep0.5-ra0.0",
    "mr0.9-ec1.0-ep0.5-ra1.5",
    "mr0.9-ec1.0-ep0.5-ra3.0",
    "mr0.9-ec1.0-ep1.0-ra0.0",
    "mr0.9-ec1.0-ep1.0-ra1.5",
    "mr0.9-ec1.0-ep1.0-ra3.0",
]

fgroup_map = Dict(
  "broadL" => "Broadleaved",
  "broadL_highF" => "Broadleaved",
  "broadL_mixedF" => "Broadleaved",
  "broadL_copp" => "Broadleaved",
  "con" => "Coniferous",
  "con_highF" => "Coniferous",
  missing => "All forest"
);

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

reg_map = Dict(
    "AL (FR42)" => "AL",
    "AQ (FR61)" => "AQ",
    "AU (FR72)" => "AU",
    "BN (FR25)" => "BN",
    "BO (FR26)" => "BO",
    "BR (FR52)" => "BR",
    "CE (FR24)" => "CE",
    "CA (FR21)" => "CA",
    "CO (FR83)" => "CO",
    "FC (FR43)" => "FC",
    "HN (FR23)" => "HN",
    "IF (FR10)" => "IF",
    "LR (FR81)" => "LR",
    "LI (FR63)" => "LI",
    "LO (FR41)" => "LO",
    "MP (FR62)" => "MP",
    "NP (FR30)" => "NP",
    "PL (FR51)" => "PL",
    "PI (FR22)" => "PI",
    "PC (FR53)" => "PC",
    "PA (FR82)" => "PA",
    "RA (FR71)" => "RA",
    "OTH FR"    => "OTHFR",
    "OTH (OTH)" => "OTH",
)

baseIDir        = "results";
odsOut          = "juliaOut/formattedOutput.ods"
odsOut_template = "juliaOut/formattedOutput_template.ods"
chartOutDir     = "juliaOut"

years       = collect(2007:2100)
years5      = collect(2010:5:2100)
years10      = collect(2010:10:2100)

problematicCols = Dict("freeDim" => Union{Missing,Int64})

##########################################################
### Data import
##########################################################

forestData_byscenario = [CSV.read(baseIDir*"/forestData_"*scenario*".csv"; delim=';', missingstring="") for scenario in scenarios];
carbonData_byscenario = [CSV.read(baseIDir*"/carbonBalance_"*scenario*".csv"; delim=';', missingstring="") for scenario in scenarios];

forVars = vcat(forestData_byscenario...)
cVars = vcat(carbonData_byscenario...)
[delete!(dfi, [:country,]) for dfi in [cVars, forVars]] # delete columns country

print("Data loaded")

##########################################################
### Data clean-up and initialisation
##########################################################
forVars[:forGroup] = map(k->fgroup_map[k], forVars[:forType])
[df[:region] = map(k->reg_map[k], df[:region]) for df in [forVars,cVars]]

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
