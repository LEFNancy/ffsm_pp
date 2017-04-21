using DataFrames, DataFramesMeta, Plots, StatPlots, LAJuliaUtils, OdsIO

scenarios = [
"RCP00_BAS_EXT",
"RCP00_BAS_ADAPT",
"RCP00_BAS_ADAPT_BASE",
"RCP00_BAS_ADAPT_ALLPOLICIES",
"RCP00_BAS_ADAPT_ACTIVEMGM",
"RCP00_BAS_ADAPT_SUBSIDES",
"RCP00_BAS_ADAPT_STRUCTURALPOLICIES",
"RCP86_BAS_EXT",
"RCP86_BAS_ADAPT"
]

mainScenarios = [
"RCP00_BAS_EXT",
"RCP00_BAS_ADAPT",
"RCP86_BAS_EXT",
"RCP86_BAS_ADAPT"
]

baseIDir = "results";

fgroup_map = Dict(
 "Fut_Feu"         => "Broadleaved",
 "Fut_Mix"         => "Mixed",
 "Fut_Con"         => "Coniferous",
 "Mel_Feu"         => "Broadleaved",
 "Mel_Mix"         => "Mixed",
 "Tail_Feu"        => "Broadleaved",
 NA                => "All forests",
 "broadL"          => "Broadleaved",
 "mixedBC"         => "Mixed",
 "con"             => "Coniferous",
 "Reb_Fut_Con"     => "Coniferous",
 "Reb_Fut_Feu"     => "Broadleaved",
 "Reb_conShortRot" => "Coniferous",
 "Reb_Tail_Feu"    => "Broadleaved",
)

refor_map = Dict( #reforestation
 "Fut_Feu"         => "TradFT",
 "Fut_Mix"         => "TradFT",
 "Fut_Con"         => "TradFT",
 "Mel_Feu"         => "TradFT",
 "Mel_Mix"         => "TradFT",
 "Tail_Feu"        => "TradFT",
 NA                => "FT",
 "broadL"          => "TradFT",
 "mixedBC"         => "TradFT",
 "con"             => "TradFT",
 "Reb_Fut_Con"     => "Refor",
 "Reb_Fut_Feu"     => "Refor",
 "Reb_conShortRot" => "Refor",
 "Reb_Tail_Feu"    => "Refor",

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

# Data import
forestData_byscenario = [readtable(baseIDir*"/forestData_"*scenario*".csv", separator = ';') for scenario in scenarios];
productData_byscenario = [readtable(baseIDir*"/productData_"*scenario*".csv", separator = ';') for scenario in scenarios];
carbonData_byscenario = [readtable(baseIDir*"/carbonBalance_"*scenario*".csv", separator = ';') for scenario in scenarios];
#hVDet_byscenario = [readtable(baseIDir*"/detailedHV_"*scenario*".csv", separator = ';') for scenario in scenarios];
forVars = vcat(forestData_byscenario)
prodVars = vcat(productData_byscenario)
cVars = vcat(carbonData_byscenario)
#hVDet = vcat(hVDet_byscenario)
[delete!(dfi, [:x,:country,]) for dfi in [cVars, prodVars, forVars]] # delete columns X (empty at the end) and country
delete!(forVars, [:freeDim])
#[delete!(dfi, [:x,:country,]) for dfi in [hVDet]] # delete columns X (empty at the end) and country
#forVars[:prod] = ""

print("Data loaded")

#head(forVars)  # scen	parName	region	forType	year	value
#head(prodVars) # scen	parName	region	prod	freeDim	year	value
#head(cVars)    # scen	region	balItem	year	value
#unique(prodVars[:prod])

# Data clean-up
forVars[:forGroup] = map(k->fgroup_map[k], forVars[:forType])
forVars[:ref]      = map(k->refor_map[k],  forVars[:forType])
cVars[:totItem]    = map(k->cbal_map[k],   cVars[:balItem])
cVars[:y2] = ""
[r[:y2] = "y" * string(r[:year])  for r in eachrow(cVars)] # Creating a column of years starting with a letter

#hVDet[:forGroup] = map(k->fgroup_map[k], hVDet[:forType])
#hVDet[:ref]      = map(k->refor_map[k],  hVDet[:forType])

prodVars[:prodLev] = map(k->pgroup_map[k], prodVars[:prod])

years = [2012,2013,2014,2015,2019,2020,2025,2030,2035,2040,2045,2050]
years5 = collect(2015:5:2050)


#ign = vcat([forVars,hVDet])
#ign = ign[[i in ["vol","hVbyPrd","regArea"] for i in ign[:parName] ], :]
#writetable("ign.csv", ign, separator = ';', header = true)

#names(ign)

# Forest dynamics
forDyn = pivot(forVars,[:parName,:scen],:year,:value,
             filter = Dict(
               :parName => ["hV","vol"],
               :scen    => mainScenarios,
               :year    => years5)
            )
ods_write("formattedOutput.ods",Dict(("forDyn",1,1) => forDyn )  )

# Forest dynamics by typology (traditional vf reforested plantation)
forDynByType = pivot(forVars,[:parName,:ref,:scen],:year,:value,
               filter = Dict(
                 :parName => ["hV","vol","regArea"],
                 :scen    => mainScenarios,
                 :year    => years5)
               )
ods_write("formattedOutput.ods",Dict(("forDynByType",1,1) => forDynByType )  )

# Forest dynamic by spGroup (con, broad..)
forDynBySpGroup = pivot(forVars,[:parName,:forGroup,:scen],:year,:value,
               filter = Dict(
                 :parName => ["hV","vol","regArea"],
                 :scen    => mainScenarios,
                 :year    => years5)
               )
ods_write("formattedOutput.ods",Dict(("forDynBySpGroup",1,1) => forDynBySpGroup )  )

# Product balance
prodBalance = pivot(prodVars,[:year,:scen,:prodLev,:prod],:parName,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa"],
                        :scen    => mainScenarios,
                        :year    => [2015,2050]),
                    sort = [:year,:scen,:prodLev,:prod],
              )
ods_write("formattedOutput.ods",Dict(("prodBalance",1,1) => prodBalance )  )

# Product balance all years
prodBalanceAll5Y = pivot(prodVars,[:parName,:prodLev,:prod,:scen,],:year,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa"],
                        :scen    => mainScenarios,
                        :year    => years5),
                    sort = [:parName,:prodLev,:prod,:scen,],
              )
ods_write("formattedOutput.ods",Dict(("prodBalanceAll5Y",1,1) => prodBalanceAll5Y )  )

# Preparation of policy variables
pol1 = by(forVars, [:scen,:parName,:region,:year]) do df
   DataFrame(value = sum(df[:value]))
end
pol2 = by(prodVars, [:scen,:parName,:region,:year]) do df
   DataFrame(value = sum(df[:value]))
end
pol=vcat([pol1,pol2])
policy_vars = ["polBal_mktDirInt_s", "polBal_mktDirInt_d", "polBal_trSub", "polBal_tcSub", "polBal_fiSub", "surplus_prod", "surplus_cons"]
pol = pol[ [i in policy_vars for i in pol[:parName]] ,:]

# Policy cost analysis (national)
polCosts_nat = pivot(pol,[:scen,:parName],:year,:value,
            filter = Dict(
                :scen    => mainScenarios,
                :year    => years5
            ),
            sort = [:scen, :parName]
)
ods_write("formattedOutput.ods",Dict(("polCosts_nat",1,1) => polCosts_nat )  )

# Policy cost analysis (regional)
polCosts_reg = pivot(pol,[:scen,:parName],:region,:value,
            filter = Dict(
                :scen    => mainScenarios,
                :year    => 2050
            ),
            sort = [:scen, :parName]
)
ods_write("formattedOutput.ods",Dict(("polCosts_reg",1,1) => polCosts_reg )  )

#Scenario component analysis
adaptScenarios = [
  "RCP00_BAS_ADAPT",
  "RCP00_BAS_ADAPT_BASE",
  "RCP00_BAS_ADAPT_ACTIVEMGM",
  "RCP00_BAS_ADAPT_ALLPOLICIES",
  "RCP00_BAS_ADAPT_SUBSIDES",
  "RCP00_BAS_ADAPT_STRUCTURALPOLICIES",
]
polSA_1 = pivot(forVars,:scen,:parName,:value,
                filter = Dict(
                    :scen    => adaptScenarios,
                    :year    => 2050,
                    :parName => ["hV","vol"]
                ),
                sort = :scen,
           )
polSA_2 = pivot(forVars,:scen,:parName,:value,
               filter = Dict(
                   :scen    => adaptScenarios,
                   :year    => 2050,
                   :parName => ["regArea"],
                   :ref     => ["Refor"]
               ),
               sort = :scen,
          )
polSA_3 = pivot(pol,:scen,:parName,:value,
               filter = Dict(
                   :scen    => adaptScenarios,
                   :year    => 2050,
               ),
               sort = :scen,
          )
polSA = hcat(polSA_1,polSA_2,polSA_3)
polSA[:pol_costs] = polSA[:polBal_mktDirInt_s] + polSA[:polBal_mktDirInt_d] + polSA[:polBal_trSub] + polSA[:polBal_tcSub] + polSA[:polBal_fiSub]
polSA[:welf_gross] = polSA[:surplus_prod] + polSA[:surplus_cons]
polSA[:welf_net]   = polSA[:pol_costs] + polSA[:welf_gross]
delete!(polSA, [:scen_1, :scen_2, :polBal_mktDirInt_s, :polBal_mktDirInt_d, :polBal_trSub, :polBal_tcSub, :polBal_fiSub, :surplus_prod, :surplus_cons])
rename!(polSA, Dict(:regArea => :reforRegArea))
ods_write("formattedOutput.ods",Dict(("polSA",1,1) => polSA )  )

# Carbon balance
ymin = minimum(cVars[:year])
ymax = maximum(cVars[:year])
cBal = pivot(cVars,[:scen,:totItem,:balItem],:y2,:value,
                filter = Dict(:scen => mainScenarios),
                sort = [:scen,:totItem,:balItem],
           )
cBal = by(cBal, :scen) do sub1
    t = DataFrame(totItem=NA, balItem=NA)
    [t[Symbol("y"*string(i))] = sum(sub1[Symbol("y"*string(i))]) for i = collect(ymin:ymax)]
    sub1mod = by(sub1, [:scen,:totItem]) do sub2
       t2 = DataFrame(balItem=NA)
       [t2[Symbol("y"*string(i))] = sum(sub2[Symbol("y"*string(i))]) for i = collect(ymin:ymax)]
       t3 = vcat(t2,sub2)
     end
     t2 = vcat(t,sub1mod)
end
delete!(cBal, [:scen_2,  :scen_1,  :totItem_1])


[cBal[Symbol("y"*string(i))] = cBal[Symbol("y"*string(i))] - cBal[Symbol("y"*string(i-1))]  for i = collect(ymax:-1:ymin+1)] # computing diff
delete!(cBal, :y2012)
#[cBal[isna(cBal[Symbol("y"*string(i))]), Symbol("y"*string(i))] = 0 for i = collect(ymin+1:ymax)] # removing na
cBal[isna(cBal[:totItem]), :totItem] = ""
cBal[isna(cBal[:balItem]), :balItem] = ""
ods_write("formattedOutput.ods",Dict(("cBal",1,1) => cBal )  )

# Carbon balance just every 5 year
cBal5y = DataFrame()
cBal5y[:scen] = cBal[:scen]
cBal5y[:totItem] = cBal[:totItem]
cBal5y[:balItem] = cBal[:balItem]
[cBal5y[Symbol("y"*string(i))] = cBal[Symbol("y"*string(i))] for i = collect(2015:5:ymax)]
ods_write("formattedOutput.ods",Dict(("cBal5y",1,1) => cBal5y )  )
