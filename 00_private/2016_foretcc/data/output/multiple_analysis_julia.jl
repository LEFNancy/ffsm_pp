using DataArrays, DataFrames, DataFramesMeta, Plots, StatPlots
include("pivot.jl")

scenarios = ["RCP00_BAS_EXT",            "RCP00_BAS_ADAPT_BASE",               "RCP00_BAS_ADAPT_REFORESTATION_ACTIVEMGM",
             "RCP00_BAS_ADAPT_SUBSIDES",       "RCP00_BAS_ADAPT_STRUCTURALPOLICIES", "RCP00_BAS_ADAPT"]
scenarios = ["RCP00_BAS_EXT_FINAL","RCP00_BAS_ADAPT_FINAL"]

scenarios = ["RCP00_BAS_ADAPT", "RCP00_BAS_ADAPT_FINAL"]

#scenarios = ["RCP00_BAS_ADAPT_SUBSIDES", "RCP00_BAS_ADAPT_SUBSIDES_GRAD"]
#scenarios = ["debug","debug_pol_mktDirInt_s"]
#scenarios = ["debug","debug_pol_mktDirInt_d"]
#scenarios = ["RCP00_BAS_EXT",            "RCP00_BAS_ADAPT_BASE",               "RCP00_BAS_ADAPT_REFORESTATION_ACTIVEMGM",
#             "RCP00_BAS_ADAPT_SUBSIDES", "RCP00_BAS_ADAPT_STRUCTURALPOLICIES", "RCP00_BAS_ADAPT"]


#scenarios = ["RCP00_BAS_EXT", "RCP00_BAS_EXT_09", "RCP00_BAS_EXT_2007v"]


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


# Data import
forestData_byscenario = [readtable(baseIDir*"/forestData_"*scenario*".csv", separator = ';') for scenario in scenarios];
productData_byscenario = [readtable(baseIDir*"/productData_"*scenario*".csv", separator = ';') for scenario in scenarios];
carbonData_byscenario = [readtable(baseIDir*"/carbonBalance_"*scenario*".csv", separator = ';') for scenario in scenarios];
hVDet_byscenario = [readtable(baseIDir*"/detailedHV_"*scenario*".csv", separator = ';') for scenario in scenarios];
forVars = vcat(forestData_byscenario)
prodVars = vcat(productData_byscenario)
cVars = vcat(carbonData_byscenario)
hVDet = vcat(hVDet_byscenario)
#delete!(forVars, [:x,:country,:freeDim]) # delete columns X (empty at the end), country and freeDim
[delete!(dfi, [:x,:country,]) for dfi in [cVars, prodVars, forVars,hVDet]] # delete columns X (empty at the end) and country
forVars[:prod] = ""

print("Data loaded")

head(forVars)


#head(forVars)  # scen	parName	region	forType	year	value
#head(prodVars) # scen	parName	region	prod	freeDim	year	value
#head(cVars)    # scen	region	balItem	year	value
#unique(prodVars[:prod])

# Data clean-up
forVars[:forGroup] = map(k->fgroup_map[k], forVars[:forType])
forVars[:ref]      = map(k->refor_map[k],  forVars[:forType])
hVDet[:forGroup] = map(k->fgroup_map[k], hVDet[:forType])
hVDet[:ref]      = map(k->refor_map[k],  hVDet[:forType])

prodVars[:prodLev] = map(k->pgroup_map[k], prodVars[:prod])

years = [2012,2013,2014,2015,2019,2020,2030,2035,2040,2050]

ign = vcat([forVars,hVDet])
ign = ign[[i in ["vol","hVbyPrd","regArea"] for i in ign[:parName] ], :]
writetable("ign.csv", ign, separator = ';', header = true)

names(ign)

debugtest = 1

debug = pivot(ign,[:parName,:scen],:year,:value

            )


forV = pivot(forVars,[:parName,:scen],:year,:value,
             filter = Dict(:parName=>["hV","vol","polBal_fiSub"], :year=>years)
            )
regArea = pivot(forVars,[:parName,:ref,:scen],:year,:value,
             filter = Dict(:parName=>["hV","regArea"], :year=>years)
            )
prodV = pivot(prodVars,[:parName,:prod,:scen],:year,:value,
             #filter = Dict(:parName=>["st","polBal_mktDirInt_s", "polBal_mktDirInt_d", "polBal_trSub", "polBal_tcSub",
            #             "surplus_prod", "surplus_cons"],
             filter = Dict(:parName=>["da","dl","dt","pl"],
               :prod => ["fuelW","hardWSawnW","softWSawnW", "plyW","pulpW","pannels"],
               :year=>years)
            )
debug = pivot(prodVars,[:parName,:prod],:year,:value,
             filter = Dict(
               :scen    => ["RCP00_BAS_EXT_2007V"],
               :year    => years,
               :parName => ["da"],
               #:prod    => ["fuelW"],
             )
)

pol1 = by(forVars, [:scen,:parName,:region,:year]) do df
   DataFrame(value = sum(df[:value]))
end

pol2 = by(prodVars, [:scen,:parName,:region,:year]) do df
   DataFrame(value = sum(df[:value]))
end

pol=vcat([pol1,pol2])

policy_vars = ["polBal_mktDirInt_s", "polBal_mktDirInt_d", "polBal_trSub", "polBal_tcSub", "pol_fiSub", "surplus_prod", "surplus_cons"]
pol = pol[ [i in policy_vars for i in pol[:parName]] ,:]

nat_pol = pivot(pol,[:scen,:parName],:year,:value,
                sort = :scen
)

reg_pol =  pivot(pol,[:scen,:year,:region],:parName,:value,
                 sort = [:scen,:year],
                 filter = Dict(:year => [2015,2020,2030,2050])
)

#writetable("debug.csv", debug, separator = ';', header = true)

#=
## Create winden versions of datasets..
# Taking ages because of the trade in prodVarsW
forVarsW = unstack(forVars, :parName, :value)
prodVarsW = unstack(prodVars, :parName, :value)
cVarsW = unstack(cVars, :balItem, :value)

# Adding cumulative harvesting
forVarsW[:cumHV] = 0.0
forVarsW[:cumRegArea] = 0.0
by(forVarsW, [:scen,:region,:forType]) do dd
    dd[:cumHV] = cumsum(dd[:hV])
    dd[:cumRegArea] = cumsum(dd[:regArea])
end
#forVarsW[9000:9020,:]

# Aggregating volumes at national level and for the whole forest
volHVTotal = by(forVarsW, [:scen,:year]) do df
    DataFrame(hV = sum(dropna(df[:hV])), vol = sum(dropna(df[:vol])), cumHV =sum(dropna(df[:cumHV])), hR = sum(dropna(df[:hV]))/sum(dropna(df[:vol])))
end
volHVTotal[:cumHV_stacked] = volHVTotal[:vol] + volHVTotal[:cumHV]
volHVTotal[:hR] = volHVTotal[:hV] ./ volHVTotal[:vol]
# Preparing data to plot
#=
years = unique(volHVTotal[:year])
vol_ext = volHVTotal[volHVTotal[:scen]   .== "RCP00_BAS_EXT" , :vol]
vol_adapt = volHVTotal[volHVTotal[:scen] .== "RCP00_BAS_ADAPT" , :vol]
cumHV_ext = volHVTotal[volHVTotal[:scen]   .== "RCP00_BAS_EXT" , :cumHV] +  vol_ext
cumHV_adapt = volHVTotal[volHVTotal[:scen]   .== "RCP00_BAS_ADAPT" , :cumHV] +  vol_adapt

p1=layer(x=years,y=vol_ext,Geom.line)
p2=layer(x=years,y=cumHV_ext,Geom.line)
plot(p1,p2,
     Guide.xlabel("Years"), Guide.ylabel("Mm^3"), Guide.title("Extensive scenario")
)

p3=layer(x=years,y=vol_adapt,Geom.line)
p4=layer(x=years,y=cumHV_adapt,Geom.line)
plot(p3,p4,
    Guide.xlabel("Years"), Guide.ylabel("Mm^3"), Guide.title("Adaptative scenario")
)


plot(volHVTotal, x="year", y="vol", color="scen", Geom.line)
=#

plotlyjs()
temp = volHVTotal[ [s in ["RCP00_BAS_EXT","RCP00_BAS_ADAPT_SUBSIDES","RCP00_BAS_ADAPT_SUBSIDES_GRAD"] for s in volHVTotal[:scen]], :]
# :blue, :yellow, :red
mycolors = [:blue :green :red]
volHVTotal_plot = plot(temp, :year, :vol, group=:scen, linestyle = :dot, linewidth=3, color = mycolors)
plot!(temp, :year, :cumHV_stacked, group=:scen, linestyle = :solid, linewidth=3,  color = mycolors)

# volHVTotal_plot = plot([1, 2, 3], [[4, 6, 8] [5, 6, 9]], linestyle = :solid, linewidth=3, linecolor=:red)
volHVTotal_plot = plot(volHVTotal[ [s in ["RCP00_BAS_EXT","RCP00_BAS_ADAPT","RCP00_BAS_ADAPT_BASE"] for s in volHVTotal[:scen]], :] ,
    layer(x="year", y="vol", color="scen", Geom.line, Theme(line_width=1.5pt)),
    layer(x="year", y="cumHV_stacked", color="scen", Geom.line, Theme(line_width=1pt))
)
draw(SVG("volHVTotal.svg", 15cm, 10cm), volHVTotal_plot)

hV_plot = plot(volHVTotal[
      ([s in ["RCP00_BAS_EXT","RCP00_BAS_ADAPT","RCP00_BAS_ADAPT_BASE"] for s in volHVTotal[:scen]]) & (volHVTotal[:year] .> 2012), :] ,
    layer(x="year", y="hV", color="scen", Geom.line, Theme(line_width=1.5pt)),
    #layer(x="year", y="cumHV_stacked", color="scen", Geom.line, Theme(line_width=1pt))
)
draw(SVG("hV.svg", 15cm, 10cm), hV_plot)
hR_plot = plot(volHVTotal[
      ([s in ["RCP00_BAS_EXT","RCP00_BAS_ADAPT","RCP00_BAS_ADAPT_BASE"] for s in volHVTotal[:scen]]) & (volHVTotal[:year] .> 2012), :] ,
    layer(x="year", y="hR", color="scen", Geom.line, Theme(line_width=1.5pt)),
    #layer(x="year", y="cumHV_stacked", color="scen", Geom.line, Theme(line_width=1pt))
)
draw(SVG("hR.svg", 15cm, 10cm), hR_plot)


volHVTotal_plotsa = plot(volHVTotal[ [s in ["RCP00_BAS_ADAPT_BASE", "RCP00_BAS_ADAPT_REFORESTATION_ACTIVEMGM", "RCP00_BAS_ADAPT_SUBSIDES", "RCP00_BAS_ADAPT_OPENMARKETS", "RCP00_BAS_ADAPT_STUCTURALPOLICIES", "RCP00_BAS_ADAPT"] for s in volHVTotal[:scen]], :] ,
    layer(x="year", y="vol", color="scen", Geom.line, Theme(line_width=1.5pt)),
    layer(x="year", y="cumHV_stacked", color="scen", Geom.line, Theme(line_width=1pt))
)
draw(SVG("volHVTotal_sa.svg", 18cm, 10cm), volHVTotal_plotsa)


volHVTotal_mainyears=volHVTotal[[y in [2012,2020,2030,2040,2050] for y in volHVTotal[:year]],:]

volHVTotal_mainyears_long = melt(volHVTotal_mainyears, [:scen,:year])

volHVTotal_mainyears2 = unstack(volHVTotal_mainyears_long, :year, :value)

#sort!(volHVTotal_mainyears2, cols = (:scen, :variable), rev = (false, false))

writetable("volHVTotal_mainyears2.csv", volHVTotal_mainyears2, separator = ';', header = true)


# Regeneration area (to compute the amount of total area regenerated)
regArea = forVarsW[(forVarsW[:year] .>2020) & (forVarsW[:year] .<=2030), :]

regArea = by(regArea, [:scen]) do df
    DataFrame(regArea = sum(dropna(df[:regArea])) )
end
writetable("regArea.csv", regArea, separator = ';', header = true)

#################################
## Forest area
#################################
# Aggregating forest area at national level and for the whole forest



forArea = by(forVarsW, [:scen,:year,:forType,:forGroup]) do df
    DataFrame(forArea = sum(dropna(df[:forArea])), regArea = sum(dropna(df[:regArea])),  cumRegArea =  sum(dropna(df[:cumRegArea]))  )
end

# Filters..
forArea = @where(forArea, indexin(:forType, ["Fut_Feu", "Fut_Mix","Fut_Con","Mel_Feu","Mel_Mix","Tail_Feu"]) .> 0 ) # only atomic cathegories have forArea
forArea = vcat(
  @where(forArea, :scen .== "RCP00_BAS_EXT", :year .== 2012),
  @where(forArea, indexin(:scen, ["RCP00_BAS_EXT", "RCP00_BAS_ADAPT_BASE", "RCP00_BAS_ADAPT_REFORESTATION_ACTIVEMGM", "RCP00_BAS_ADAPT_SUBSIDES", "RCP00_BAS_ADAPT_OPENMARKETS", "RCP00_BAS_ADAPT_STUCTURALPOLICIES", "RCP00_BAS_ADAPT"]) .> 0, :year .== 2050)
)
# Create a new column with both scenario and year
forArea[:scenYear] = map((x,y) -> string(x, "_", y), forArea[:scen], forArea[:year])

forArea_plot = plot(forArea, x="scenYear", y="forArea", color="forType", Geom.bar)
draw(PNG("forArea_plot.png", 30cm, 20cm), forArea_plot)
draw(SVG("forArea_plot.svg", 30cm, 20cm), forArea_plot)
writetable("forArea.csv", forArea, separator = ';', header = true)

=#
