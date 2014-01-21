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
   'random_958031':   '#d9d9d9',
   'random_816881':   '#d9d9d9',
   'random_589866':   '#d9d9d9',
   'random_251807':   '#d9d9d9',
   'random_177382':   '#d9d9d9',
   'random_173935':   '#d9d9d9',
   'random_266700':   '#d9d9d9',
   'random_956967':   '#d9d9d9',
   'random_376875':   '#d9d9d9',
   'random_226236':   '#d9d9d9',
   'random_974424':   '#d9d9d9',
   'random_329197':   '#d9d9d9',
   'random_969368':   '#d9d9d9',
   'random_502630':   '#d9d9d9',
   'random_904379':   '#d9d9d9',
   'random_94508' :   '#d9d9d9',
   'random_666933':   '#d9d9d9',
   'random_325051':   '#d9d9d9',
   'random_456691':   '#d9d9d9',
   'random_376237':   '#d9d9d9',
   'random_286113':   '#d9d9d9',
   'random_83519' :   '#d9d9d9',
   'random_437521':   '#d9d9d9',
   'random_841772':   '#d9d9d9',
   'random_262868':   '#d9d9d9',
   'random_481944':   '#d9d9d9',
   'random_715469':   '#d9d9d9',
   'random_192731':   '#d9d9d9',
   'random_703980':   '#d9d9d9',
   'random_458520':   '#d9d9d9',  
   
   #randomSpaceAndRisk scenarios
   'randomSpaceAndRisk_748791':   '#d9d9d9',
   'randomSpaceAndRisk_886729':   '#d9d9d9',
   'randomSpaceAndRisk_73572' :   '#d9d9d9',
   'randomSpaceAndRisk_768359':   '#d9d9d9',
   'randomSpaceAndRisk_560664':   '#d9d9d9',
   'randomSpaceAndRisk_952153':   '#d9d9d9',
   'randomSpaceAndRisk_495136':   '#d9d9d9',
   'randomSpaceAndRisk_221736':   '#d9d9d9',
   'randomSpaceAndRisk_683095':   '#d9d9d9',
   'randomSpaceAndRisk_526347':   '#d9d9d9',
   'randomSpaceAndRisk_80455' :   '#d9d9d9',
   'randomSpaceAndRisk_424553':   '#d9d9d9',
   'randomSpaceAndRisk_26531' :   '#d9d9d9',
   'randomSpaceAndRisk_481320':   '#d9d9d9',
   'randomSpaceAndRisk_306821':   '#d9d9d9',
   'randomSpaceAndRisk_886213':   '#d9d9d9',
   'randomSpaceAndRisk_556453':   '#d9d9d9',
   'randomSpaceAndRisk_568097':   '#d9d9d9',
   'randomSpaceAndRisk_258188':   '#d9d9d9',
   'randomSpaceAndRisk_40865' :   '#d9d9d9',
   'randomSpaceAndRisk_894191':   '#d9d9d9',
   'randomSpaceAndRisk_643361':   '#d9d9d9',
   'randomSpaceAndRisk_390167':   '#d9d9d9',
   'randomSpaceAndRisk_21655' :   '#d9d9d9',
   'randomSpaceAndRisk_837385':   '#d9d9d9',
   'randomSpaceAndRisk_927497':   '#d9d9d9',
   'randomSpaceAndRisk_120276':   '#d9d9d9',
   'randomSpaceAndRisk_236802':   '#d9d9d9',
   'randomSpaceAndRisk_811457':   '#d9d9d9',
   'randomSpaceAndRisk_428987':   '#d9d9d9'  
}

years = ['2007','2008','2009','2010','2011','2012','2013','2014','2015','2020','2030','2040','2050','2060','2070','2080','2090','2100']

ofilename = 'output_oracle.csv'
chartoutdir = 'output/charts'
tableoutdir = 'output/tables'
charttype = 'pdf'
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
             

tvalue001  = [63.6567411629, 9.9248432009, 5.8409093097, 4.6040948714, 4.0321429836, 3.7074280213, 3.4994832974, 3.3553873313, 3.2498355416, 3.1692726726, 3.1058065155, 3.0545395894, 3.0122758387, 2.9768427344, 2.9467128835, 2.9207816224, 2.8982305197, 2.8784404727, 2.8609346065, 2.8453397098, 2.831359558, 2.8187560606, 2.8073356838, 2.7969395048, 2.7874358137, 2.7787145333, 2.7706829571, 2.7632624555, 2.7563859037, 2.7499956536, 2.7440419193, 2.738481482, 2.7332766424, 2.7283943671, 2.7238055892, 2.7194846305, 2.7154087215, 2.7115576019, 2.7079131835, 2.7044592674, 2.7011813036, 2.6980661862, 2.6951020792, 2.6922782657, 2.6895850194, 2.6870134922, 2.6845556179, 2.682204027, 2.6799519736, 2.6777932709]    # invt for alpha=0.01
tvalue0001 = [636.6192487687, 31.5990545764, 12.9239786367, 8.6103015814, 6.8688266259, 5.9588161788, 5.4078825209, 5.0413054334, 4.7809125859, 4.5868938587, 4.4369793382, 4.3177912836, 4.2208317277, 4.1404541127, 4.0727651959, 4.0149963272, 3.9651262721, 3.9216458251, 3.8834058526, 3.8495162749, 3.8192771643, 3.7921306717, 3.7676268043, 3.7453986193, 3.7251439497, 3.7066117435, 3.6895917135, 3.6739064007, 3.6594050195, 3.645958635, 3.6334563498, 3.6218022599, 3.6109130077, 3.6007157974, 3.5911467758, 3.5821497015, 3.5736748444, 3.5656780716, 3.5581200813, 3.5509657609, 3.544183643, 3.5377454453, 3.5316256778, 3.5258013065, 3.520251465, 3.5149572055, 3.5099012834, 3.5050679705, 3.5004428914, 3.4960128818] # invt for alpha=0.001




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
 # printCharts()
  
  # H - Print tables
  printTables()

  print "Done!"

# =============================================================================
def plotMultivariable(scenarios_h, variables_h, region, title, filename, printLegend=True):
 
  #key_reference =  '11023', 'vReg', 'reference', 'broadL_highF', '2040'
  #key_nonspatial =  '11023', 'vReg', 'nonspatial', 'broadL_highF', '2040'
  #key_withVariance =  '11023', 'vReg', 'withVariance', 'broadL_highF', '2040'
  #print "test: " + str(idata[key_reference]) + "   " + str(idata[key_nonspatial]) + "   " + str(idata[key_withVariance])
  #params = {'legend.fontsize': 10.5,
  #          'legend.linewidth': 1}
  #plt.rcParams.update(params)

  nvar = len(variables_h)
  nscen = len(scenarios_h)
  #plt.figure(1)
  fig = plt.gcf()
  fig.set_size_inches(10,15)
  maintitle = unicode(title, 'utf_8')
  handles =[]
  labels = []
  #plt.suptitle(maintitle, fontsize=16, ha='center')
  for i in range(nvar):    
    #plt.subplot(nvar,1,i+1)
    ax =fig.add_subplot(nvar,1,i+1)
    subplotTitle = unicode(variables[variables_h[i]][0], 'utf_8')
    ylabel = unicode(variables[variables_h[i]][1], 'utf_8')
    plt.title(subplotTitle)
    plt.ylabel(ylabel)
    for spGroup in sorted(spAggregates.keys()):
      for scenario in scenarios_h:
        serieName = unicode(spGroup + " - " + scenario, 'utf_8')
        serieColor = scenarios[scenario]
        serieLineType = spAggregates[spGroup][1]
        serieWidth = spAggregates[spGroup][2]
        #print serieName+ " - " + serieLineType + " - " + str(serieWidth)
        key = region, variables_h[i], scenario, spGroup
	y = odata[key]
	plt.plot(x, y, serieLineType, label=serieName, linewidth=serieWidth, color=serieColor)
    handles, labels = ax.get_legend_handles_labels()
  #plt.subplots_adjust(hspace=0.6)
  #handles, labels = ax.get_legend_handles_labels()
  #ax.legend(handles, labels, ncol=3, shadow=False, title="Legend")
  if printLegend:
    plt.figlegend(handles, labels, loc = 'lower center', ncol=3, shadow=False, labelspacing=0., prop={'size':12})
  #plt.savefig(chartoutdir+"/"+filename+"_"+region+"."+charttype, bbox_inches='tight', dpi=300)
  plt.savefig(chartoutdir+"/"+filename+"_"+region+"."+charttype, dpi=300)
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
  plt.savefig(chartoutdir+"/"+filename+"."+charttype, bbox_inches='tight', pad_inches=0.1, dpi=300) 
  #plt.show()
  plt.close()
  
#import matplotlib.pyplot as plt
#ax = plt.subplot()  #create the axes
#ax.set_axis_off()  #turn off the axis
#....  #do patches and labels
#ax.legend(patches, labels, ...)  #legend alone in the figure
#plt.show() 


# =============================================================================
def checkScenarioDifferences(ref_scenario, comparing_scenarios, variables_h, regions_h, years_h, title, filename):
  d = " & "
  el = " \\\\"
  label_comparing_scenario = "comparing scenarios"
  nvar = len(variables_h)
  nscen = len(comparing_scenarios)
  nyears = len(years_h)
  nregions = len(regions_h)
  if nscen == 1:
    label_comparing_scenario = comparing_scenarios[0]
    
    
  oString = ""
  oString += "\\hline\n"
  oString += d+ref_scenario+d+label_comparing_scenario+d+"difference"+d+"cv"+el+"\n"
  
  for region in regions_h:
    oString += "\\hline\n"
    oString += "\\multicolumn{5}{l}{"+regions[region]+"}"+el+'\n'
  
    for variable in variables_h:
      oString += "\\multicolumn{5}{l}{"+variables[variable][0]+" (\\textit{"+variables[variable][1]+"})}"+el+'\n'
      for spGroup in sorted(spAggregates.keys()):
        sumRScenario =   0
        sumCScenarios = [0] * nscen
        valRScenario = 0
        valCScenarios = [0] * nscen
        avgCScenarios = 0
        scenarioDiff = 0
        scenarioRelativeDiff = 0
        qdiffCScenarios = [0] * nscen
        sumqdiffCScenarios = 0
        sd = 0
        significance = ""
        cv= 0
        for year in years_h:
          keyr = region, variable, ref_scenario, spGroup, year
          sumRScenario += idata[keyr]
          for s in range(nscen):
            keyc = region, variable, comparing_scenarios[s], spGroup, year
            sumCScenarios[s] += idata[keyc]
        valRScenario = sumRScenario/nyears
        for s in range(nscen):
          valCScenarios[s] = sumCScenarios[s]/nyears
        avgCScenarios = sum(valCScenarios) / float(nscen)
        scenarioDiff = avgCScenarios-valRScenario
        scenarioRelativeDiff = 100 * scenarioDiff/valRScenario
        if nscen > 1:
          for s in range(nscen):
            qdiffCScenarios[s] = (valCScenarios[s] - avgCScenarios)**2.0
            sumqdiffCScenarios += qdiffCScenarios[s]
          sd = (sumqdiffCScenarios/(nscen-1))**0.5
          t = abs(scenarioDiff)*nscen**0.5/sd
          cv = 100 * sd/abs(avgCScenarios)
          if t >= tvalue001[nscen-1-1]:
            significance = '$^a$'
          if t >= tvalue0001[nscen-1-1]:
            significance = '$^b$'
        oString += '- '+spGroup+d+"%0.3f"%(valRScenario)+d+"%0.3f"%(avgCScenarios)+d+"%0.3f"%(scenarioDiff)+significance+' ('+"%0.3f"%(scenarioRelativeDiff)+'\\%)'+d+"%0.2f"%(cv)+' \\%'+el+'\n'
  oString += "\\hline\n"
  ofilename = tableoutdir+'/'+filename+'.txt'
  ofile = open(ofilename,'w')
  ofile.write(oString)
  ofile.close()
  

# =============================================================================   
def printCharts():
  print "Printing charts.."
  
  #plotLegend(['nonspatial','reference','vRegEnd070','vRegFixed','vRegFromHr','withRisk06','withRisk08','withRisk10','withVariance','withSpVarianceAndRisk'],'Scenarios Legend','legend')
  
  for region in regions.keys():
    plotMultivariable(['vRegFixed', 'vRegFromHr', 'reference','vRegEnd070'],['expReturns','vReg','vol','hV'],region,'Management effect','management')
    plotMultivariable(['reference','withRisk06','withRisk08','withRisk10'],['expReturns','vReg','vol','hV'],region,'Risk effect','risk')
    plotMultivariable(['nonspatial','reference','withVariance'],['expReturns','vReg','vol','hV'],region,'Spatial explicit simulations','space')
    plotMultivariable(['reference','withSpVarianceAndRisk'],['expReturns','vReg','vol','hV'],region,'Combined effect of risk and spatial heterogeneity','spaceandrisk')
    plotMultivariable([
      'random_958031', 'random_816881', 'random_589866', 'random_251807', 'random_177382', 'random_173935', 'random_266700', 'random_956967', 'random_376875', 'random_226236', 'random_974424', 'random_329197', 'random_969368', 'random_502630', 'random_904379', 'random_94508' , 'random_666933', 'random_325051', 'random_456691', 'random_376237', 'random_286113', 'random_83519' , 'random_437521', 'random_841772', 'random_262868', 'random_481944', 'random_715469', 'random_192731', 'random_703980', 'random_458520',
      'reference','withVariance'],['expReturns','vReg','vol','hV'],region,'Random space','randomSpace', False)
    plotMultivariable([
      'randomSpaceAndRisk_748791', 'randomSpaceAndRisk_886729', 'randomSpaceAndRisk_73572' , 'randomSpaceAndRisk_768359', 'randomSpaceAndRisk_560664', 'randomSpaceAndRisk_952153', 'randomSpaceAndRisk_495136', 'randomSpaceAndRisk_221736', 'randomSpaceAndRisk_683095', 'randomSpaceAndRisk_526347', 'randomSpaceAndRisk_80455' , 'randomSpaceAndRisk_424553', 'randomSpaceAndRisk_26531' , 'randomSpaceAndRisk_481320', 'randomSpaceAndRisk_306821', 'randomSpaceAndRisk_886213', 'randomSpaceAndRisk_556453', 'randomSpaceAndRisk_568097', 'randomSpaceAndRisk_258188', 'randomSpaceAndRisk_40865' , 'randomSpaceAndRisk_894191', 'randomSpaceAndRisk_643361', 'randomSpaceAndRisk_390167', 'randomSpaceAndRisk_21655' , 'randomSpaceAndRisk_837385', 'randomSpaceAndRisk_927497', 'randomSpaceAndRisk_120276', 'randomSpaceAndRisk_236802', 'randomSpaceAndRisk_811457', 'randomSpaceAndRisk_428987',
      'reference','withSpVarianceAndRisk'],['expReturns','vReg','vol','hV'],region,'Random space and risk','randomSpaceAndRisk', False)
    
    
  
# =============================================================================  
def printTables():
  print "Printing tables.."
  
  checkScenarioDifferences('vRegFromHr',['reference'],['expReturns','vReg','vol','hV'],['11000'],['2040','2050','2060','2070','2080','2090','2100'],'Reference from vRegFixed','endmanagement_11000')
  checkScenarioDifferences('vRegFromHr',['vRegEnd070'],['expReturns','vReg','vol','hV'],['11000'],['2040','2050','2060','2070','2080','2090','2100'],'Reference from vRegFixed','endmanagement_strong_11000')
  checkScenarioDifferences('vRegFromHr',['reference'],['expReturns','vReg','vol','hV'],['11000'],['2100'],'Reference from vRegFixed','endmanagement__2100_11000')
  checkScenarioDifferences('vRegFromHr',['vRegEnd070'],['expReturns','vReg','vol','hV'],['11000'],['2100'],'Reference from vRegFixed','endmanagement_strong__2100_11000')
  checkScenarioDifferences('reference',['withRisk08'],['expReturns','vReg','vol','hV'],['11000'],['2040','2050','2060','2070','2080','2090','2100'],'Reference from vRegFixed','risk_11000')
  checkScenarioDifferences('reference',['withRisk10'],['expReturns','vReg','vol','hV'],['11000'],['2040','2050','2060','2070','2080','2090','2100'],'Reference from vRegFixed','risk_strong_11000')
  checkScenarioDifferences('reference',['withSpVarianceAndRisk'],['expReturns','vReg','vol','hV'],['11082'],['2040','2050','2060','2070','2080','2090','2100'],'Reference from vRegFixed','spandrisk_11082')
  checkScenarioDifferences('reference',['withSpVarianceAndRisk'],['expReturns','vReg','vol','hV'],['11000'],['2040','2050','2060','2070','2080','2090','2100'],'Reference from vRegFixed','spandrisk_11000')
  checkScenarioDifferences('reference',['withSpVarianceAndRisk'],['expReturns','vReg','vol','hV'],['11082'],['2100'],'Reference from vRegFixed','spandrisk_2100_11082')
  checkScenarioDifferences('reference',['withSpVarianceAndRisk'],['expReturns','vReg','vol','hV'],['11000'],['2100'],'Reference from vRegFixed','spandrisk_2100_11000')
  checkScenarioDifferences('reference', [
      'random_958031', 'random_816881', 'random_589866', 'random_251807', 'random_177382', 'random_173935', 'random_266700', 'random_956967', 'random_376875', 'random_226236', 'random_974424', 'random_329197', 'random_969368', 'random_502630', 'random_904379', 'random_94508' , 'random_666933', 'random_325051', 'random_456691', 'random_376237', 'random_286113', 'random_83519' , 'random_437521', 'random_841772', 'random_262868', 'random_481944', 'random_715469', 'random_192731', 'random_703980', 'random_458520'
      ],['expReturns','vReg','vol','hV'], ['11000'], ['2040','2050','2060','2070','2080','2090','2100'],'Random space','randomspace_11000')
  
  for region in regions.keys():
    checkScenarioDifferences('reference', [
      'random_958031', 'random_816881', 'random_589866', 'random_251807', 'random_177382', 'random_173935', 'random_266700', 'random_956967', 'random_376875', 'random_226236', 'random_974424', 'random_329197', 'random_969368', 'random_502630', 'random_904379', 'random_94508' , 'random_666933', 'random_325051', 'random_456691', 'random_376237', 'random_286113', 'random_83519' , 'random_437521', 'random_841772', 'random_262868', 'random_481944', 'random_715469', 'random_192731', 'random_703980', 'random_458520'
      ],['expReturns','vReg','vol','hV'], [region], ['2100'],'Random space','randomspace_2100_'+region)


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