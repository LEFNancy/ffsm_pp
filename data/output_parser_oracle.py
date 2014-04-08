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
  'output_bau/results/forestData_bau.csv',  
  'output_oracle/results/forestData_arpege_a1b.csv',
  'output_oracle/results/forestData_arpege_a2.csv',
  'output_oracle/results/forestData_arpege_b1.csv',
  'output_oracle/results/forestData_CNCM33_1.csv',
  'output_oracle/results/forestData_DMIEH5C_1.csv',
  'output_oracle/results/forestData_DMIEH5C_2.csv',
  'output_oracle/results/forestData_DMIEH5C_3.csv',
  'output_oracle/results/forestData_EGMAM2_3.csv',
  'output_oracle/results/forestData_HADGEM2_1.csv',
  'output_oracle/results/forestData_IPCM4_1.csv',
  'output_oracle/results/forestData_IPCM4_2.csv',
  'output_oracle/results/forestData_IPCM4_3.csv',
  'output_oracle/results/forestData_MPEH5C_1.csv',
  'output_oracle/results/forestData_MPEH5C_2.csv',
  'output_oracle/results/forestData_MPEH5C_3.csv',
  
]

scenarios = {
  'bau':                 '#610e0e',
  'arpege_a1b':          '#6dcaf1',
  'arpege_a2':           '#6c77e7',
  'arpege_b1':           '#0015db',
  'CNCM33_1':            '#6dcaf1',
  'DMIEH5C_1':           '#6dcaf1',
  'DMIEH5C_2':           '#6c77e7',
  'DMIEH5C_3':           '#0015db',
  'EGMAM2_3':            '#6dcaf1',
  'HADGEM2_1':           '#6dcaf1',
  'IPCM4_1':             '#6dcaf1',
  'IPCM4_2':             '#6c77e7',
  'IPCM4_3':             '#0015db',
  'MPEH5C_1':            '#6dcaf1',
  'MPEH5C_2':            '#6c77e7',
  'MPEH5C_3':            '#0015db', 
}

years = ['2015','2016','2017','2018','2019','2020','2021','2022','2023','2024','2025','2026','2027','2028','2029','2030','2031','2032','2033','2034','2035','2036','2037','2038','2039','2040','2041','2042','2043','2044','2045','2046','2047','2048','2049','2050','2051','2052','2053','2054','2055','2056','2057','2058','2059','2060','2061','2062','2063','2064','2065','2066','2067','2068','2069','2070','2071','2072','2073','2074','2075','2076','2077','2078','2079','2080','2081','2082','2083','2084','2085']

printCharts = True
printTablae = True
ofilename = 'output_cc.csv'
chartoutdir = 'output_oracle/charts'
tableoutdir = 'output_oracle/tables'
tablesmaster = '00_master_tables'
chartsmaster = '00_master_charts'
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
	     'forArea': ['Forest area','ha'],
	     'harvestedArea': ['Harvested area','ha'],
	     'regArea': ['Regeneration area','ha'],
	     }


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
sortedregions = []




# =============================================================================
def main():  
  global sortedregions
  
  print "Creating nicely formatted output and charts.."
  
  # A - creating empty dictionaries with just the keys..   
  for country, data in countries.iteritems():
    regions[country] = data[1] # add 11000: 'France' to regions
  sortedregions = regions.keys()
  sortedregions.sort()
   
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
  
  # G - Reset latex files
  filename_t = tableoutdir+'/'+tablesmaster+'.tex'
  filename_c = chartoutdir+'/'+chartsmaster+'.tex'
  file_t = open(filename_t,'w')
  file_c = open(filename_c,'w')
  file_t.close()
  file_c.close()
  
  # H - Printing charts
  if printCharts:
    printCharts()
  
  # I - Print tables
  if printTables:
    printTables()
  
  print "Done!"

# =============================================================================
def plotMultivariable(scenarios_h, variables_h, region, title, filename, printLegend=True):
 
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
        
  omasterfilename = chartoutdir+'/'+chartsmaster+'.tex'
  omfile = open(omasterfilename,'a')
  omfile.write("\\begin{figure}[htbp]\n")
  omfile.write("  \\centering\n")
  omfile.write("  \\caption{"+title+"}\n")
  omfile.write("  \\includegraphics[width=0.8\\textwidth]{"+chartoutdir+"/"+filename+"_"+region+"}\n")
  omfile.write("  \\label{fig:"+filename+"}\n")
  omfile.write("\\end{figure}\n")
  omfile.close()
   
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
def printTable(ref_scenario, comparing_scenarios, variables_h, regions_h, years_h, title, filename):
  d = " & "
  el = " \\\\"
  label_comparing_scenario = "comparing scenarios"
  nvar = len(variables_h)
  nscen = len(comparing_scenarios)
  nyears = len(years_h)
  nregions = len(regions_h)
  nmulticol = "4"
  label_ref_scenario = ref_scenario.replace("_", "\\_")
  if nscen == 1:
    label_comparing_scenario = comparing_scenarios[0].replace("_", "\\_")
  if nscen > 5:
    nmulticol = "5"
    
    
  oString = ""
  oString += "\\begin{table}[htbp]\n"
  oString += "\\begin{center}\n"
  oString += "\\begin{threeparttable}\n"
  oString += "\\centering\n"
  oString += "\\caption{"+title+"}\n"
  oString += "\\begin{footnotesize}\n"
  if nscen > 5:
    oString += "\\begin{tabularx}{\\textwidth}{l r r r r}\n"
  else:
    oString += "\\begin{tabularx}{\\textwidth}{l r r r}\n"
  oString += "\\hline\n"
  if nscen > 5:
    oString += d+label_ref_scenario+d+label_comparing_scenario+d+"difference"+d+"cv"+el+"\n"  
  else:
    oString += d+label_ref_scenario+d+label_comparing_scenario+d+"difference"+el+"\n"
     
  for region in regions_h:
    oString += "\\hline\n"
    if nregions > 1:
      oString += "\\multicolumn{"+nmulticol+"}{l}{"+regions[region]+"}"+el+'\n'
  
    for variable in variables_h:
      oString += "\\multicolumn{"+nmulticol+"}{l}{"+variables[variable][0]+" (\\textit{"+variables[variable][1]+"})}"+el+'\n'
      for spGroup in sorted(spAggregates.keys()):
	outSpGroup = spGroup.replace("_", "\\_")
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
        if nscen > 5:
          for s in range(nscen):
            qdiffCScenarios[s] = (valCScenarios[s] - avgCScenarios)**2.0
            sumqdiffCScenarios += qdiffCScenarios[s]
          sd = (sumqdiffCScenarios/(nscen-1))**0.5
          t = abs(scenarioDiff)*nscen**0.5/sd if sd>0.0 else 0.0 
          cv = 100.0 * sd/abs(avgCScenarios) if abs(avgCScenarios)> 0.0 else 0.0
          if t >= tvalue001[nscen-1-1]:
            significance = '$^a$'
          if t >= tvalue0001[nscen-1-1]:
            significance = '$^b$'
          oString += '- '+outSpGroup+d+"%0.3f"%(valRScenario)+d+"%0.3f"%(avgCScenarios)+d+"%0.3f"%(scenarioDiff)+significance+' ('+"%0.3f"%(scenarioRelativeDiff)+'\\%)'+d+"%0.2f"%(cv)+' \\%'+el+'\n'
        else:
	  oString += '- '+outSpGroup+d+"%0.3f"%(valRScenario)+d+"%0.3f"%(avgCScenarios)+d+"%0.3f"%(scenarioDiff)+' ('+"%0.3f"%(scenarioRelativeDiff)+'\\%)'+el+'\n'
          
  oString += "\\hline\n"
  oString += "\\end{tabularx}\n"
  oString += "\\end{footnotesize}\n"
  oString += "\\label{tab:"+filename+"}\n"
  if nscen > 5:
    oString += "\\begin{tablenotes}\n"
    oString += "\\begin{footnotesize}\n"
    oString += "\\item [a] Significantly different from 0 at $\\alpha=0.01$\n"
    oString += "\\item [b] Significantly different from 0 at $\\alpha=0.001$\n"
    oString += "\\end{footnotesize}\n"
    oString += "\\end{tablenotes}\n"
  oString += "\\end{threeparttable}\n"
  oString += "\\end{center}\n"
  oString += "\\end{table}\n" 
  
  ofilename = tableoutdir+'/'+filename+'.tex'
  ofile = open(ofilename,'w')
  ofile.write(oString)
  ofile.close()
  
  omasterfilename = tableoutdir+'/'+tablesmaster+'.tex'
  omfile = open(omasterfilename,'a')
  omfile.write("\\input{"+tableoutdir+'/'+filename+"}\n")
  omfile.close()



# =============================================================================
def title (cat, level, title):
  filename = ""
  if cat == 't':
    filename = tableoutdir+'/'+tablesmaster+'.tex'
  elif cat == 'c': 
    filename = chartoutdir+'/'+chartsmaster+'.tex'
  else:
    print "Error in printTable: not know where to print the title !"
    exit(1)
  file = open(filename,'a')
  
  file.write("\n\\clearpage\n")
  file.write("\\"+level+"{"+title+"}\n")
  file.close()
  
# =============================================================================
def text(cat, text_h):
  filename = ""
  if cat == 't':
    filename = tableoutdir+'/'+tablesmaster+'.tex'
  elif cat == 'c': 
    filename = chartoutdir+'/'+chartsmaster+'.tex'
  else:
    print "Error in text: not know where to print the title !"
    exit(1)
  file = open(filename,'a')
  file.write(text_h+"\n")
  file.close()

# =============================================================================   
def printCharts():
  print "Printing charts.."
  
  #text('c',"Automatically generated charts.")  
  #plotLegend(['nonspatial','reference','vRegEnd070','vRegFixed','vRegFromHr','withRisk06','withRisk08','withRisk10','withVariance','withSpVarianceAndRisk'],'Scenarios Legend','legend')
  
  selectedregions=['11000', '11041']
  
  for region in sortedregions: #selectedregions:#sortedregions:
    title('c','subsection',regions[region])

    title('c','subsubsection',"IPCC scenarios (using arpege)")
    plotMultivariable(['bau','arpege_a1b', 'arpege_a2', 'arpege_b1'],['expReturns','vReg','vol','hV','forArea'],region,'IPCC Scenarios','ipcc_scenarios')
    
    title('c','subsubsection',"IPCC scenario a1b")
    plotMultivariable(['bau','arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea'],region,'All a1b scenarios','a1b',False)
    plotMultivariable(['bau','arpege_a1b'],['expReturns','vReg','vol','hV','forArea'],region,'Arpege a1b','arpege')
    plotMultivariable(['bau','CNCM33_1'],['expReturns','vReg','vol','hV','forArea'],region,'CNCM33','cncm33')
    plotMultivariable(['bau','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3'],['expReturns','vReg','vol','hV','forArea'],region,'DMIEH5C','dmieh5c')    
    plotMultivariable(['bau','EGMAM2_3'],['expReturns','vReg','vol','hV','forArea'],region,'EGMAM2','egmam2') 
    plotMultivariable(['bau','HADGEM2_1'],['expReturns','vReg','vol','hV','forArea'],region,'HADGEM2','hadgem2') 
    plotMultivariable(['bau','IPCM4_1','IPCM4_2','IPCM4_3'],['expReturns','vReg','vol','hV','forArea'],region,'IPCM4','ipcm4') 
    plotMultivariable(['bau','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea'],region,'MPEH5C','mpeh5c') 

# =============================================================================  
def printTables():
  print "Printing tables.."

  oyears = ['2030','2031','2032','2033','2034','2035','2036','2037','2038','2039','2040','2041','2042','2043','2044','2045','2046','2047','2048','2049','2050','2051','2052','2053','2054','2055','2056','2057','2058','2059','2060','2061','2062','2063','2064','2065','2066','2067','2068','2069','2070','2071','2072','2073','2074','2075','2076','2077','2078','2079','2080','2081','2082','2083','2084','2085']
  
  #text('t',"Automatically generated result tables.")
  
  selectedregions=['11000', '11041']
  
  for region in sortedregions: #selectedregions:#sortedregions:
    title('t','subsection',regions[region])

    title('c','subsubsection',"IPCC scenario a1b")
    printTable('bau',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'IPCC scenario a1b (average of all models) [avg. 2030-2085]','a1b_all_2030-2085_'+region)
    printTable('bau',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'IPCC scenario a1b (average of all models) [2085]','a1b_all_2085_'+region)
    printTable('bau',['arpege_a1b'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'arpege\_a1b [avg. 2030-2085]','arpege_a1b_2030-2085_'+region)
    printTable('bau',['arpege_a1b'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'arpege\_a1b [2085]','arpege_a1b_2085_'+region)
    printTable('bau',['CNCM33_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'CNCM33 [avg. 2030-2085]','cncm33_2030-2085_'+region)
    printTable('bau',['CNCM33_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'CNCM33 [2085]','cncm33_2085_'+region)
    printTable('bau',['DMIEH5C_1','DMIEH5C_2','DMIEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'DMIEH5C [avg. 2030-2085]','dmieh5c_2030-2085_'+region)
    printTable('bau',['DMIEH5C_1','DMIEH5C_2','DMIEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'DMIEH5C [2085]','dmieh5c_2085_'+region)
    printTable('bau',['EGMAM2_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'EGMAM2 [avg. 2030-2085]','egmam2_2030-2085_'+region)
    printTable('bau',['EGMAM2_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'EGMAM2 [2085]','egmam2_2085_'+region)
    printTable('bau',['HADGEM2_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'HADGEM2 [avg. 2030-2085]','hadgem2_2030-2085_'+region)
    printTable('bau',['HADGEM2_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'HADGEM2 [2085]','hadgem2_2085_'+region)
    printTable('bau',['IPCM4_1','IPCM4_2','IPCM4_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'IPCM4 [avg. 2030-2085]','ipcm4_2030-2085_'+region)
    printTable('bau',['IPCM4_1','IPCM4_2','IPCM4_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'IPCM4 [2085]','ipcm4_2085_'+region)
    printTable('bau',['MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'MPEH5C [avg. 2030-2085]','mpeh5c_2030-2085_'+region)
    printTable('bau',['MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'MPEH5C [2085]','mpeh5c_2085_'+region)
    
    text('t',"\clearpage")
    
    title('c','subsubsection',"IPCC scenario a2")
    printTable('bau',['arpege_a2'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'arpege\_a2 [avg. 2030-2085]','arpege_a2_2030-2085_'+region)
    printTable('bau',['arpege_a2'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'arpege\_a2 [2085]','arpege_a2_2085_'+region)
    title('c','subsubsection',"IPCC scenario b1")    
    printTable('bau',['arpege_b1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'arpege\_b1 [avg. 2030-2085]','arpege_b1_2030-2085_'+region)
    printTable('bau',['arpege_b1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'arpege\_b1 [2085]','arpege_b1_2085_'+region)
    
   
# =============================================================================
# EXECUTION ACTUALLY STARTS HERE.....
main()
