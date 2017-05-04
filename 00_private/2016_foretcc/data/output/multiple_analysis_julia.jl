using DataFrames, DataFramesMeta,  LAJuliaUtils, OdsIO, NamedColors,  StatPlots, Plots #  PlotlyJS  Plots

##########################################################
### Settings and definitions
##########################################################
scenarios = [
"RCP00_BAS_EXT",
"RCP00_BAS_ADAPT",
"RCP00_BAS_ADAPT_BASE",
"RCP00_BAS_ADAPT_ALLPOLICIES",
"RCP00_BAS_ADAPT_ACTIVEMGM",
"RCP00_BAS_ADAPT_SUBSIDES",
"RCP00_BAS_ADAPT_STRUCTURALPOLICIES",
"RCP85_BAS_EXT", # correct to 85
"RCP85_BAS_ADAPT"
]
scenarios = [
"RCP00_BAS_EXT",
"RCP00_BAS_ADAPT",
"RCP85_BAS_EXT",
"RCP85_BAS_ADAPT"
]

mainScenarios = [
"RCP00_BAS_EXT",
"RCP00_BAS_ADAPT",
"RCP85_BAS_EXT",
"RCP85_BAS_ADAPT"
]

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

baseIDir = "results";
odsOut          = "juliaOut/formattedOutput.ods"
odsOut_template = "juliaOut/formattedOutput_template.ods"
chartOutDir     = "juliaOut"

years = [2012,2013,2014,2015,2019,2020,2025,2030,2035,2040,2045,2050]
years5 = collect(2015:5:2050)

##########################################################
### Data import
##########################################################
forestData_byscenario = [readtable(baseIDir*"/forestData_"*scenario*".csv", separator = ';') for scenario in scenarios];
productData_byscenario = [readtable(baseIDir*"/productData_"*scenario*".csv", separator = ';') for scenario in scenarios];
carbonData_byscenario = [readtable(baseIDir*"/carbonBalance_"*scenario*".csv", separator = ';') for scenario in scenarios];
forVars = vcat(forestData_byscenario)
prodVars = vcat(productData_byscenario)
cVars = vcat(carbonData_byscenario)
[delete!(dfi, [:x,:country,]) for dfi in [cVars, prodVars, forVars]] # delete columns X (empty at the end) and country

#hVDet_byscenario = [readtable(baseIDir*"/detailedHV_"*scenario*".csv", separator = ';') for scenario in scenarios];
#hVDet = vcat(hVDet_byscenario)
#[delete!(dfi, [:x,:country,]) for dfi in [hVDet]] # delete columns X (empty at the end) and country
#forVars[:prod] = ""
delete!(forVars, [:freeDim])

print("Data loaded")

##########################################################
### Data clean-up and initialisation
##########################################################
forVars[:forGroup] = map(k->fgroup_map[k], forVars[:forType])
forVars[:ref]      = map(k->refor_map[k],  forVars[:forType])
cVars[:totItem]    = map(k->cbal_map[k],   cVars[:balItem])
cVars[:y2] = ""
[r[:y2] = "y" * string(r[:year])  for r in eachrow(cVars)] # Creating a column of years starting with a letter

#hVDet[:forGroup] = map(k->fgroup_map[k], hVDet[:forType])
#hVDet[:ref]      = map(k->refor_map[k],  hVDet[:forType])

prodVars[:prodLev] = map(k->pgroup_map[k], prodVars[:prod])

function applyScenType(dfs)
  for df in dfs
    df[:scenTypeCC]   = ""
    df[:scenTypeEcon] = ""
    df[:scenLevel] = ""
      for r in eachrow(df)
        r[:scenTypeCC] = r[:scen][1:9]
        if contains(r[:scen],"ADAPT")
            r[:scenTypeEcon] = "ADAPT"
        elseif contains(r[:scen],"EXT")
            r[:scenTypeEcon] = "EXT"
        end
        r[:scenLevel] =  r[:scen] in mainScenarios ? "main" : "sa"
      end
  end
end
applyScenType([forVars,prodVars,cVars])

# Creating pivoted sum variants (without forest type or product information)
forVar_piv = pivot(forVars,[:scen,:scenLevel,:scenTypeCC,:scenTypeEcon,:year],:parName,:value,
          filter = Dict(:year => collect(2015:1:2050),
          ),
)
prodVar_piv = pivot(prodVars,[:scen,:scenLevel,:scenTypeCC,:scenTypeEcon,:year],:parName,:value,
          filter = Dict(:year => collect(2015:1:2050),
          ),
)
cVars_piv = pivot(cVars,[:scen,:scenLevel,:scenTypeCC,:scenTypeEcon,:year],:balItem,:value,
          filter = Dict(:year => collect(2015:1:2050),
          ),
)
vars_piv = join(forVar_piv, prodVar_piv, on = [:scen,:scenLevel,:scenTypeCC,:scenTypeEcon,:year])
vars_piv = join(vars_piv, cVars_piv, on = [:scen,:scenLevel,:scenTypeCC,:scenTypeEcon,:year])

cp(odsOut_template, odsOut; remove_destination=true) # copy template odsFile. Don't forget CTRL+Shift+F9 to update the spreadsheet

##########################################################
### Elaborations
##########################################################

# ################ Ign out..
# ign = vcat([forVars,hVDet])
# ign = ign[[i in ["vol","hVbyPrd","regArea"] for i in ign[:parName] ], :]
# delete!(ign, :scenLevel )
# rename!(ign, Dict(:freeDim => :dc, :ref => :reforestationType ))
# writetable("ign_v3_calibratedHarvestingRates.csv", ign, separator = ';', header = true)
# ############### Ign in testing
# ignIn = readtable("ign_v3_calibratedHarvestingRates.csv", separator = ';')
# ignTest = pivot(ignIn,[:scen,:parName],:year,:value,
#                 filter=Dict(
#                   :year => collect(2012:2050),
#                 ),
# )
# ods_write("ignTest.ods",Dict(("ignTestV3",1,1) => ignTest )  )

################ Tables..

# Forest dynamics
forDyn = pivot(forVars,[:parName,:scen],:year,:value,
             filter = Dict(
               :parName => ["hV","vol"],
               :scen    => mainScenarios,
               :year    => years5)
            )
ods_write(odsOut,Dict(("forDyn",1,1) => forDyn )  )

# Forest dynamics by typology (traditional vf reforested plantation)
forDynByType = pivot(forVars,[:parName,:ref,:scen],:year,:value,
               filter = Dict(
                 :parName => ["hV","vol","regArea"],
                 :scen    => mainScenarios,
                 :year    => years5)
               )
ods_write(odsOut,Dict(("forDynByType",1,1) => forDynByType )  )

# Forest dynamic by spGroup (con, broad..)
forDynBySpGroup = pivot(forVars,[:parName,:forGroup,:scen],:year,:value,
               filter = Dict(
                 :parName => ["hV","vol","regArea"],
                 :scen    => mainScenarios,
                 :year    => years5)
               )
ods_write(odsOut,Dict(("forDynBySpGroup",1,1) => forDynBySpGroup )  )

# Regional forest harvesting
hVReg = pivot(forVars,[:scen,:region],:year,:value,
                filter = Dict(
                  :parName => ["hV"],
                  :scen    => mainScenarios,
                  :year    => years5
                ),
            )
ods_write(odsOut,Dict(("hVReg",1,1) => hVReg )  )

# Product balance
prodBalance = pivot(prodVars,[:year,:scen,:prodLev,:prod],:parName,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa"],
                        :scen    => mainScenarios,
                        :year    => [2015,2050]),
                    sort = [:year,:scen,:prodLev,:prod],
              )
ods_write(odsOut,Dict(("prodBalance",1,1) => prodBalance )  )

# Product balance all years
prodBalanceAll5Y = pivot(prodVars,[:parName,:prodLev,:prod,:scen,],:year,:value,
                    filter = Dict(
                        :parName => ["st","da","dt","sa"],
                        :scen    => mainScenarios,
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
ods_write(odsOut,Dict(("polCosts_nat",1,1) => polCosts_nat )  )

# Policy cost analysis (regional)
polCosts_reg = pivot(pol,[:scen,:parName],:region,:value,
            filter = Dict(
                :scen    => mainScenarios,
                :year    => 2050
            ),
            sort = [:scen, :parName]
)
ods_write(odsOut,Dict(("polCosts_reg",1,1) => polCosts_reg )  )

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
ods_write(odsOut,Dict(("polSA",1,1) => polSA )  )

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
ods_write(odsOut,Dict(("cBal",1,1) => cBal )  )

# Carbon balance just every 5 year
cBal5y = DataFrame()
cBal5y[:scen] = cBal[:scen]
cBal5y[:totItem] = cBal[:totItem]
cBal5y[:balItem] = cBal[:balItem]
[cBal5y[Symbol("y"*string(i))] = cBal[Symbol("y"*string(i))] for i = collect(2015:5:ymax)]
ods_write(odsOut,Dict(("cBal5y",1,1) => cBal5y )  )

# Forest density (reg)
dens = pivot(forVars,[:scen,:year,:parName],:region,:value,
             filter = Dict(
               :parName => ["vol","forArea"],
               :scen    => mainScenarios,
               :year    => [2015,2050])
            )
dens = by(dens, [:scen,:year]) do sub
    nreg = size(sub)[2]
    forArea = sub[1,:] #ha
    vol = sub[2,:] #M m^3
    d = deepcopy(vol)
    d[:parName] = "density"
    [d[1,i] =  (vol[1,i]*1000000) / forArea[1,i] for i in collect(4:nreg)] #mc/ha
    vcat(vol,forArea,d)
end
delete!(dens,[:scen_1, :year_1])
ods_write(odsOut,Dict(("dens",1,1) => dens )  )

# Coniferous share
conShare = pivot(forVars,[:scen,:year,:forGroup],:region,:value,
             filter = Dict(
               :parName => ["forArea"],
               :scen    => mainScenarios,
               :year    => [2015,2050])
            )

conShare = by(conShare, [:scen,:year]) do sub
    nreg = size(sub)[2]
    broadL = sub[1,:] #ha
    con = sub[2,:]
    mix = sub[3,:]
    conSh = deepcopy(con)
    conSh[:forGroup] = "conShare"
    [conSh[1,i] =  con[1,i] / (broadL[1,i]+mix[1,i]+con[1,i]) for i in collect(4:nreg)] #mc/ha
    vcat(broadL,mix,con,conSh)
end

delete!(conShare,[:scen_1, :year_1])
ods_write(odsOut,Dict(("conShare",1,1) => conShare )  )

################ Plots..
plotlyjs() # For displaying
#pyplot() # For printing

mycolors = [colorant"olive green" colorant"rust orange" colorant"Gold Fusion" colorant"stormy blue"]

vars_00base_main = vars_piv[((vars_piv[:scenTypeCC] .== "RCP00_BAS") & (vars_piv[:scenLevel] .== "main")) , :]
vars_85base_main = vars_piv[((vars_piv[:scenTypeCC] .== "RCP85_BAS") & (vars_piv[:scenLevel] .== "main")) , :]

# Volumes
vol = plot(vars_00base_main, :year, :vol, group=:scen, linestyle = :solid, linewidth=3, color = mycolors, label  = (sort(unique(vars_00base_main[:scen]))') )
plot!(vars_85base_main, :year, :vol, group=:scen, linestyle = :dot, linewidth=3,  color = mycolors,  label  = (sort(unique(vars_85base_main[:scen]))')    )
savefig(chartOutDir*"/vol.png")

# Harvested Volumes
hV = plot(vars_00base_main, :year, :hV, group=:scen, linestyle = :solid, linewidth=3, color = mycolors, label  = (sort(unique(vars_00base_main[:scen]))') )
plot!(vars_85base_main, :year, :hV, group=:scen, linestyle = :dot, linewidth=3,  color = mycolors,  label  = (sort(unique(vars_85base_main[:scen]))')    )
savefig(chartOutDir*"/hV.png")

# Volumes of Reforestations
refor_00base= pivot(forVars,[:scen,:scenLevel,:scenTypeCC,:scenTypeEcon,:year],:parName,:value,
              filter = Dict(
                :year       => collect(2015:1:2050),
                :ref        => ["Refor"],
                :scenTypeCC => ["RCP00_BAS"],
                :scenLevel  => ["main"]
          ),
)
refor_85base= pivot(forVars,[:scen,:scenLevel,:scenTypeCC,:scenTypeEcon,:year],:parName,:value,
              filter = Dict(
                :year       => collect(2015:1:2050),
                :ref        => ["Refor"],
                :scenTypeCC => ["RCP85_BAS"],
                :scenLevel  => ["main"]
          ),
)

vol = plot(refor_00base, :year, :vol, group=:scen, linestyle = :solid, linewidth=3, color = mycolors, label  = (sort(unique(refor_00base[:scen]))') )
plot!(refor_85base, :year, :vol, group=:scen, linestyle = :dot, linewidth=3,  color = mycolors,  label  = (sort(unique(refor_85base[:scen]))')    )
savefig(chartOutDir*"/vol_refor.png")
# Harvested Volumes
hV = plot(refor_00base, :year, :hV, group=:scen, linestyle = :solid, linewidth=3, color = mycolors, label  = (sort(unique(refor_00base[:scen]))') )
plot!(refor_85base, :year, :hV, group=:scen, linestyle = :dot, linewidth=3,  color = mycolors,  label  = (sort(unique(refor_85base[:scen]))')    )
savefig(chartOutDir*"/hV_refor.png")

#=
# Carbon balance
cBal2 = pivot(cVars,[:scen,:scenTypeCC,:scenTypeEcon,:year],:balItem,:value,
             filter=Dict(
               :scenLevel => ["main"],
             ),
)
cBal2[:Pools]
cBal_rcp00base_ext = cBal2[cBal2[:scen] .== "RCP00_BAS_EXT",:]
cBal_rcp00base_ext_plot = plot(cBal_rcp00base_ext, :year, :hV, group=:scen, linestyle = :solid, linewidth=3, color = mycolors, label  = (sort(unique(refor_85base[:scen]))') )
plot!(refor_85base, :year, :hV, group=:scen, linestyle = :dot, linewidth=3,  color = mycolors,  label  = (sort(unique(refor_85base[:scen]))')    )
savefig(chartOutDir*"/hV_refor.png")

cBal2 = by(cVars, [:scen,:year]) do sub1
    println(sub1)
    #t = DataFrame(totItem=NA, balItem="Total")
    #[t[Symbol("y"*string(i))] = sum(sub1[Symbol("y"*string(i))]) for i = collect(ymin:ymax)]
    #sub1mod = by(sub1, [:scen,:totItem]) do sub2
    #   t2 = DataFrame(balItem=NA)
    #   [t2[Symbol("y"*string(i))] = sum(sub2[Symbol("y"*string(i))]) for i = collect(ymin:ymax)]
    #   t3 = vcat(t2,sub2)
    # end
    # t2 = vcat(t,sub1mod)
end
=#
