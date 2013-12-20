#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os, sys
import csv, math
import matplotlib.pyplot as plt

''' Scope of this script
- parse the pythia output to produce nice summarized tables
'''

# 0 - parameters

# input data filenames
# input data expected format
# - scen;parName;country;region;forType;diamClass;year;value;
forIFiles = [  
  'output_reference/results/forestData.csv',
  'output_vRegEnd070/results/forestData.csv',
  'output_vRegFixed/results/forestData.csv',
  'output_vRegFromHr/results/forestData.csv',
  'output_nonspatial/results/forestData.csv',
  'output_withRisk06/results/forestData.csv',
  'output_withRisk08/results/forestData.csv', 
  'output_withRisk10/results/forestData.csv',
  'output_withVariance/results/forestData.csv',
  'output_withSpVarianceAndRisk/results/forestData.csv',
  'output_randomSpace/results/forestData.csv',
  'output_randomSpaceAndRisk/results/forestData.csv',
]

scenarios = {
   'nonspatial':   '#b7b7b7',
   'reference':    '#f80000',
   'vRegEnd070':   '#ff7200',
   'vRegFixed':    '#416c47',
   'vRegFromHr':   '#a1a612',
   'withRisk06':   '#6dcaf1',
   'withRisk08':   '#6c77e7',
   'withRisk10':   '#0015db',
   'withVariance': '#d4a4d7',
   'withSpVarianceAndRisk': '#dbda6a',
  #randomSpace scenarios
   'random_117558':   '#d9d9d9',
   'random_151813':   '#d9d9d9',
   'random_152293':   '#d9d9d9',
   'random_186692':   '#d9d9d9',
   'random_190673':   '#d9d9d9',
   'random_305289':   '#d9d9d9',
   'random_317517':   '#d9d9d9',
   'random_336080':   '#d9d9d9',
   'random_349314':   '#d9d9d9',
   'random_486049':   '#d9d9d9',
   'random_505139':   '#d9d9d9',
   'random_507065':   '#d9d9d9',
   'random_514968':   '#d9d9d9',
   'random_529216':   '#d9d9d9',
   'random_663867':   '#d9d9d9',
   'random_709506':   '#d9d9d9',
   'random_767506':   '#d9d9d9',
   'random_861718':   '#d9d9d9',
   'random_888399':   '#d9d9d9',
   'random_985589':   '#d9d9d9', 
   #randomSpaceAndRisk scenarios
   'random_10225':    '#d9d9d9',
   'random_113183':   '#d9d9d9',
   'random_124177':   '#d9d9d9',
   'random_138725':   '#d9d9d9',
   'random_144223':   '#d9d9d9',
   'random_17935':    '#d9d9d9',
   'random_221906':   '#d9d9d9',
   'random_232901':   '#d9d9d9',
   'random_269220':   '#d9d9d9',
   'random_292783':   '#d9d9d9',
   'random_465259':   '#d9d9d9',
   'random_465275':   '#d9d9d9',
   'random_481647':   '#d9d9d9',
   'random_499587':   '#d9d9d9',
   'random_517382':   '#d9d9d9',
   'random_571690':   '#d9d9d9',
   'random_596099':   '#d9d9d9',
   'random_640941':   '#d9d9d9',
   'random_758961':   '#d9d9d9',
   'random_950809':   '#d9d9d9',
}

years = ['2007','2008','2009','2010','2011','2012','2013','2014','2015','2020','2030','2040','2050','2060','2070','2080','2090','2100']

ofilename = 'output_oracle.csv'
chartoutdir = 'output/charts'
sep = ';'

countries = {'11000': [['11042', '11061', '11072', '11025', '11026', '11052', '11024', '11021',
  '11083', '11043', '11023', '11010', '11081', '11063', '11041', '11062',
  '11030', '11051', '11022', '11053', '11082', '11071',],'France']}

regions = {'11042': 'Alsace', '11061': 'Aquitaine', '11072': 'Auvergne', '11025': 'Basse-Normandie', '11026': 'Bourgogne',
	  '11052': 'Bretagne', '11024': 'Centre', '11021': 'Champagne-Ardenne', '11083': 'Corse', '11043': 'Franche-Comté',
	  '11023': 'Haute-Normandie', '11010': 'Île de France', '11081': 'Languedoc-Roussillon', '11063': 'Limousin',
	  '11041': 'Lorraine', '11062': 'Midi-Pyrénées', '11030': 'Nord - Pas-de-Calais', '11051': 'Pays de la Loire',
	  '11022': 'Picardie', '11053': 'Poitou-Charentes', '11082': 'Provence-Alpes-Côte d\'Azur', '11071': 'Rhône-Alpes',} 

# key: var short name
# value: turple with long name, unit and optionally variable to act for ponderation and name of aggregated variable
variables = {'hV': ['Harvested Volumes', r"$Mm^3$"],
	     'vReg': ['Regeneration Volumes', r"$Mm^3$"],
	     'vol': ['Forest Volumes', r"$Mm^3$"],
	     'expReturns': ['Expected returns','€/ha','forArea', 'totalExpReturns'],
	     'forArea': ['Forest area','M ha ??']}


spGroups = ['broadL_highF','broadL_mixedF','broadL_copp','con_highF']

#key: human name
#value[0]: list of sp groups
#value[1]: chart line type
#value[2]: chart line width
spAggregates = {'00_Total': [['broadL_highF','broadL_mixedF','broadL_copp','con_highF'],'-',4],
                '01_Broadleaved': [['broadL_highF','broadL_mixedF','broadL_copp'],'--',3],
                '02_Coniferous': [['con_highF'],':',3]}
             


# -----------------------------------------------------------------------------
# global containers, don't touch
idata = {}
odata = {}
x = []





# =============================================================================
def main():  
  
  
  print "Creating nicely formatted output and charts.."
  
  # A - creating empty dictionaries with just the keys..   
  for country, data in countries.iteritems():
    regions[country] = data[1] # add 11000: 'France' to regions
    
  specieswithAggregates = spGroups
  specieswithAggregates.extend(spAggregates.keys())  
  
  variablesWithAggregates = variables.keys()
  for variable in variables.keys():
    #'expReturns': ['Expected returns','€/ha','forArea','totalExpReturns'],
    if len(variables[variable]) == 4:
      variablesWithAggregates.append(variables[variable][3])
  
  for region in regions.keys():
    for variable in variablesWithAggregates:
      for scenario in scenarios.keys():
	for spGroup in specieswithAggregates:
	  for year in years:
            key = region, variable, scenario, spGroup, year
	    idata[key] = 0.0
  for region in regions.keys():
    for variable in variablesWithAggregates:
      for scenario in scenarios.keys():
	for spGroup in specieswithAggregates:
          key = region, variable, scenario, spGroup
	  odata[key] = []	    
  for year in years:
    x.append(int(year))
    
	    
  # B - loading data..	    
  for ifile in forIFiles:	    
    idata_raw = csv.DictReader(open(ifile, 'rb'), delimiter=sep)
    for rec in idata_raw:
      # scen;parName;country;region;forType;diamClass;year;value;
      key = rec['region'],rec['parName'],rec['scen'],rec['forType'],rec['year']
      if key in idata:
        idata[key] += float (rec['value'])

  # C - creating aggregated data for variables that need to be pondered
  for variable in variables.keys():
    #'expReturns': ['Expected returns','€/ha','forArea', 'totalExpReturns'],
    if len(variables[variable]) == 4:
      pondVariable = variables[variable][2]
      totalVariable = variables[variable][3]
      for region in regions.keys():
        for scenario in scenarios.keys():
	  for spGroup in specieswithAggregates:
	    for year in years:
              key = region, variable, scenario, spGroup, year
              key_pvar = region, pondVariable, scenario, spGroup, year
              key_tvar = region, totalVariable, scenario, spGroup, year
	      idata[key_tvar] = idata[key] * idata[key_pvar]
  
  # D - performing various summing up..
   
  # summing up to the country level..
  for country, regionsInTheCountry in countries.iteritems():
    for variable in variablesWithAggregates:
      for scenario in scenarios.keys():
	for spGroup in spGroups:
	  for year in years:
	    destKey = country, variable, scenario, spGroup, year
	    idata[destKey] = 0.0
	    for regionInTheCountry in regionsInTheCountry[0]:
	      varToBeSumKey = regionInTheCountry, variable, scenario, spGroup, year
	      idata[destKey] +=  idata[varToBeSumKey]
	
  # checking country aggregation, ok
  #for country, regionsInTheCountry in countries.iteritems():
    #print "country: " + country + " " + str(idata[country,'vol', 'vRegFixed', 'broadL_highF', '2006'])
    #for regionInTheCountry in regionsInTheCountry[0]:
      #print "region: " + regionInTheCountry + " " + str(idata[regionInTheCountry,'vol', 'vRegFixed', 'broadL_highF', '2006'])
 
 # summing up the specie aggregation
  for spAggregate, species in spAggregates.iteritems():
    for region in regions.keys():
      for variable in variablesWithAggregates:
        for scenario in scenarios.keys():
	  for year in years:
	    destKey = region, variable, scenario, spAggregate, year
	    idata[destKey] = 0.0
	    for specie in species[0]:
              varToBeSumKey = region, variable, scenario, specie, year
	      idata[destKey] +=  idata[varToBeSumKey]
	      
  # testing specie aggregating
  #for spAggregate, species in spAggregates.iteritems():
    #print "aggregate: "+ spAggregate + " " + str(idata['11042','vol', 'vRegFixed', spAggregate, '2006'])
    #for specie in species[0]:
      #print "specieGroup: " + specie + " " + str(idata['11042','vol', 'vRegFixed', specie, '2006'])
   
  # E - after all the summing up let's compute the poundered value  
  for variable in variables.keys():
    #'expReturns': ['Expected returns','€/ha','forArea', 'totalExpReturns'],
    if len(variables[variable]) == 4:
      pondVariable = variables[variable][2]
      totalVariable = variables[variable][3]
      for region in regions.keys():
        for scenario in scenarios.keys():
	  for spGroup in specieswithAggregates:
	    for year in years:
              key = region, variable, scenario, spGroup, year
              key_pvar = region, pondVariable, scenario, spGroup, year
              key_tvar = region, totalVariable, scenario, spGroup, year
	      idata[key] =  (idata[key_tvar] / idata[key_pvar]) if  idata[key_pvar] != 0 else 0
	      
  # testing ponderation variables
  #for variable in variables.keys():
    #'expReturns': ['Expected returns','€/ha','forArea', 'totalExpReturns'],
    #if len(variables[variable]) == 4:
      #pondVariable = variables[variable][2]
      #totalVariable = variables[variable][3]
      #print "Orig variable: " + variable + " " + str(idata['11000', variable, 'vRegFixed','Total', '2006'])
      #print "Pond variable: " + pondVariable + " " + str(idata['11000', pondVariable, 'vRegFixed', 'Total', '2006'])
      #print "Total variable: "  + totalVariable + " " + str(idata['11000', totalVariable, 'vRegFixed', 'Total', '2006'])
  
  # F - converting everything in years array
  for region in regions.keys():
    for variable in variablesWithAggregates:
      for scenario in scenarios.keys():
	for spGroup in specieswithAggregates:
	  key = region, variable, scenario, spGroup
	  for year in years:
            key_year = region, variable, scenario, spGroup, year
	    odata[key].append(idata[key_year])
  
  # testing odata
  #print "idata[2005]: " + str(idata['11000', 'vol', 'vRegFixed','Total', '2005'])
  #print "idata[2006]: " + str(idata['11000', 'vol', 'vRegFixed','Total', '2006'])
  #print "odata: " + str(odata['11000', 'vol', 'vRegFixed','Total'])
        
  # G - Printing charts
  printCharts()
  
  # H - Print tables
  printTables()

  print "Done!"

# =============================================================================
def plotMultivariable(scenarios_h, variables_h, region, title, filename):
 
  #key_reference =  '11023', 'vReg', 'reference', 'broadL_highF', '2040'
  #key_nonspatial =  '11023', 'vReg', 'nonspatial', 'broadL_highF', '2040'
  #key_withVariance =  '11023', 'vReg', 'withVariance', 'broadL_highF', '2040'
  #print "test: " + str(idata[key_reference]) + "   " + str(idata[key_nonspatial]) + "   " + str(idata[key_withVariance])
  
  nvar = len(variables_h)
  nscen = len(scenarios_h)
  #plt.figure(1)
  fig = plt.gcf()
  fig.set_size_inches(10,14)
  maintitle = unicode(title, 'utf_8')
  #plt.suptitle(maintitle, fontsize=16, ha='center')
  for i in range(nvar):    
    plt.subplot(nvar,1,i+1)
    subplotTitle = unicode(variables[variables_h[i]][0], 'utf_8')
    ylabel = unicode(variables[variables_h[i]][1], 'utf_8')
    plt.title(subplotTitle)
    plt.ylabel(ylabel)
    for scenario in scenarios_h:
      for spGroup in spAggregates.keys():
        serieName = unicode(spGroup + " - " + scenario, 'utf_8')
        serieColor = scenarios[scenario]
        serieLineType = spAggregates[spGroup][1]
        serieWidth = spAggregates[spGroup][2]
        #print serieName+ " - " + serieLineType + " - " + str(serieWidth)
        key = region, variables_h[i], scenario, spGroup
	y = odata[key]
	plt.plot(x, y, serieLineType, label=serieName, linewidth=serieWidth, color=serieColor)
  #plt.subplots_adjust(hspace=0.6)
  plt.savefig(chartoutdir+"/"+filename+"_"+region+".pdf", bbox_inches='tight', dpi=300)
  #plt.show()
  plt.close()
        
# P = ggplot(dataset) + aes(x=sdfsd, y=sdfasd, color=product) + facet_wrap(region)
# p + facet

##ax = plt.axes()       
##ax.yaxis.grid() #orizzontal lines

##handles, labels = ax.get_legend_handles_labels()
##ax.legend(handles, labels, ncol=2, shadow=True, title="Legend")
##ax.get_legend().get_title().set_color("green")
#plt.savefig('foo.png', bbox_inches='tight')
#plt.show()
   
# =============================================================================
def plotLegend(scenarios_h, title, filename):
  nscen = len(scenarios_h)
  fig = plt.gcf()
  fig.set_size_inches(10,14)
  #ax = plt.axes()
  #ax.set_axis_off()
  
  #fig = plt.figure()
  ax =fig.add_subplot(111)
  ax.set_axis_off()
  
  for spGroup in sorted(spAggregates.keys()):
    for scenario in scenarios_h:
      serieName = unicode(spGroup + " - " + scenario, 'utf_8')
      serieColor = scenarios[scenario]
      serieLineType = spAggregates[spGroup][1]
      serieWidth = spAggregates[spGroup][2]
      #print serieName+ " - " + serieLineType + " - " + str(serieWidth)
      dummyx = [1]
      dummyy = [1]
      plt.plot(dummyx, dummyy, serieLineType, label=serieName, linewidth=serieWidth, color=serieColor)
  handles, labels = ax.get_legend_handles_labels()
  ax.legend(handles, labels, ncol=3, shadow=False, title="Legend")
  plt.savefig(chartoutdir+"/"+filename+".pdf", bbox_inches='tight', pad_inches=0.1, dpi=300) 
  #plt.show()
  plt.close()
  
#import matplotlib.pyplot as plt
#ax = plt.subplot()  #create the axes
#ax.set_axis_off()  #turn off the axis
#....  #do patches and labels
#ax.legend(patches, labels, ...)  #legend alone in the figure
#plt.show() 

# =============================================================================   
def printCharts():
  print "Printing charts.."
  
  #plotLegend(['nonspatial','reference','vRegEnd070','vRegFixed','vRegFromHr','withRisk06','withRisk08','withRisk10','withVariance'],'Scenarios Legend','legend')
  
  #for region in regions.keys():
    #plotMultivariable(['reference','vRegEnd070','vRegFixed','vRegFromHr'],['expReturns','vReg','vol','hV'],region,'Management effect','management')
    #plotMultivariable(['reference','withRisk06','withRisk08','withRisk10'],['expReturns','vReg','vol','hV'],region,'Risk effect','risk')
    #plotMultivariable(['nonspatial','reference','withVariance'],['expReturns','vReg','vol','hV'],region,'Spatial explicit simulations','space')
    #plotMultivariable(['reference','withSpVarianceAndRisk'],['expReturns','vReg','vol','hV'],region,'Combined effect of risk and spatial heterogeneity','spaceandrisk')
    
  plotMultivariable(['random_117558', 'random_151813', 'random_152293', 'random_186692', 'random_190673', 'random_305289', 'random_317517', 'random_336080', 'random_349314', 'random_486049', 'random_505139', 'random_507065', 'random_514968', 'random_529216', 'random_663867', 'random_709506', 'random_767506', 'random_861718', 'random_888399', 'random_985589','reference','withVariance'],['expReturns','vReg','vol','hV'],'11041','Random space','randomSpace')
  plotMultivariable(['random_10225', 'random_113183', 'random_124177', 'random_138725', 'random_144223', 'random_17935', 'random_221906', 'random_232901', 'random_269220', 'random_292783', 'random_465259', 'random_465275', 'random_481647', 'random_499587', 'random_517382', 'random_571690', 'random_596099', 'random_640941', 'random_758961', 'random_950809','reference','withSpVarianceAndRisk'],['expReturns','vReg','vol','hV'],'11041','Random space and risk','randomSpaceAndRisk')  
  
  
# =============================================================================  
def printTables():
  print "Printing tables.."

# =============================================================================
# EXECUTION ACTUALLY STARTS HERE.....
main()


# old code, it can be used when printing tables
  ## printing results ready..
  #print "Printing results to "+ofilename+" .."
  #ofile = open(ofilename,'w')
  #nyears = len(years)
  #header1 = 'region'+sep+'variable'+sep+'scenario'+sep
  ##for spGroup in spGroups:
  #for spAggregate, species in sorted(spAggregates.iteritems()):
   #for year in years:
     #header1 += (spAggregate+sep)
  #header1 += '\n'
  #header2 = sep+sep+sep
  ##for spGroup in spGroups:
  #for spAggregate, species in spAggregates.iteritems():
   #for year in years:
     #header2 += (year+sep)
  #header2 += '\n'

  #ofile.write(header1)
  #ofile.write(header2)
  
  #for region in regions:
    #for variable in variables:
      #for scenario in scenarios:
        #outputRow = region+sep+variable+sep+scenario+sep
        ##for spGroup in spGroups:
	#for spAggregate, species in sorted(spAggregates.iteritems()):  
          #for year in years:
            #value = 0;
            #for specie in species:
	      #key = region, variable, scenario, specie, year
	      #value += odata[key]
            #outputRow += str(value)+sep
        #outputRow += '\n'
        #ofile.write(outputRow)
  #ofile.close()