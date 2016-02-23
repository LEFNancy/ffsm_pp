#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os, sys
import csv, math
import matplotlib
import matplotlib.pyplot as plt
import output_parser_globals as g

''' Scope of this script
- parse the pythia output to produce nice summarized tables
'''


# =============================================================================
def prepare_data():  
  print "Loading and preparing the data.."
  
  # A - creating empty dictionaries with just the keys..   
  for country, data in g.countries.iteritems():
    g.regions[country] = data[1] # add 11000: 'France' to regions
  g.sortedregions = g.regions.keys()
  g.sortedregions.sort()
   
  specieswithAggregates = g.spGroups
  specieswithAggregates.extend(g.spAggregates.keys())
  tempSpecieswithAggregates = specieswithAggregates
  tempSpecieswithAggregates.append("")
  tempSpGroups = g.spGroups
  tempSpGroups.append("")

  
  variablesWithAggregates = g.variables.keys()
  for variable in g.variables.keys():
    #'expReturns': ['Expected returns','€/ha','forArea','totalExpReturns'],
    if len(g.variables[variable]) == 4:
      variablesWithAggregates.append(g.variables[variable][3])
  
  for region in g.regions.keys():
    for variable in variablesWithAggregates:
      for scenario in g.scenarios.keys():
	for spGroup in tempSpecieswithAggregates:
	  for year in g.years:
            key = region, variable, scenario, spGroup, year
	    g.idata[key] = 0.0
  for region in g.regions.keys():
    for variable in variablesWithAggregates:
      for scenario in g.scenarios.keys():
	for spGroup in tempSpecieswithAggregates:
          key = region, variable, scenario, spGroup
	  g.odata[key] = []	    
  for year in g.years:
    g.x.append(int(year))
    
	    
  # B - loading data..	    
  for ifile in g.forIFiles:	    
    idata_raw = csv.DictReader(open(ifile, 'rb'), delimiter=g.sep)
    for rec in idata_raw:
      # scen;parName;country;region;forType;diamClass;year;value;
      key = rec['region'],rec['parName'],rec['scen'],rec['forType'],rec['year']
      if key in g.idata:
        g.idata[key] += float (rec['value'])
        
  for ifile in g.carbonIFile:   
    #print (g.carbonIFile)  
    idata_raw = csv.DictReader(open(ifile, 'rb'), delimiter=g.sep)
    for rec in idata_raw:
      # scen;parName;country;region;forType;diamClass;year;value;
      key = rec['region'],rec['balItem'],rec['scen'],"",rec['year']
      #print key
      if key in g.idata:
        g.idata[key] += float (rec['value'])
        #print (key)
        #print (g.idata[key])

  #exit(1)
  
  # C - creating aggregated data for variables that need to be pondered
  for variable in g.variables.keys():
    #'expReturns': ['Expected returns','€/ha','forArea', 'totalExpReturns'],
    if len(g.variables[variable]) == 4:
      pondVariable = g.variables[variable][2]
      totalVariable = g.variables[variable][3]
      for region in g.regions.keys():
        for scenario in g.scenarios.keys():
	  for spGroup in specieswithAggregates:
	    for year in g.years:
              key = region, variable, scenario, spGroup, year
              key_pvar = region, pondVariable, scenario, spGroup, year
              key_tvar = region, totalVariable, scenario, spGroup, year
	      g.idata[key_tvar] = g.idata[key] * g.idata[key_pvar]
  
  # D - performing various summing up..
   
  # summing up to the country level..
  for country, regionsInTheCountry in g.countries.iteritems():
    for variable in variablesWithAggregates:
      for scenario in g.scenarios.keys():
        for spGroup in tempSpGroups:
          for year in g.years:
            destKey = country, variable, scenario, spGroup, year
            g.idata[destKey] = 0.0
            for regionInTheCountry in regionsInTheCountry[0]:
              varToBeSumKey = regionInTheCountry, variable, scenario, spGroup, year
              g.idata[destKey] +=  g.idata[varToBeSumKey]
	
  # checking country aggregation, ok
  #for country, regionsInTheCountry in countries.iteritems():
    #print "country: " + country + " " + str(idata[country,'vol', 'vRegFixed', 'broadL_highF', '2006'])
    #for regionInTheCountry in regionsInTheCountry[0]:
      #print "region: " + regionInTheCountry + " " + str(idata[regionInTheCountry,'vol', 'vRegFixed', 'broadL_highF', '2006'])
 
 # summing up the specie aggregation
  for spAggregate, species in g.spAggregates.iteritems():
    for region in g.regions.keys():
      for variable in variablesWithAggregates:
        for scenario in g.scenarios.keys():
          for year in g.years:
            destKey = region, variable, scenario, spAggregate, year
            g.idata[destKey] = 0.0
            for specie in species[0]:
              varToBeSumKey = region, variable, scenario, specie, year
              g.idata[destKey] +=  g.idata[varToBeSumKey]
          
  # testing specie aggregating
  #for spAggregate, species in spAggregates.iteritems():
    #print "aggregate: "+ spAggregate + " " + str(idata['11042','vol', 'vRegFixed', spAggregate, '2006'])
    #for specie in species[0]:
      #print "specieGroup: " + specie + " " + str(idata['11042','vol', 'vRegFixed', specie, '2006'])
   
  # E - after all the summing up let's compute the poundered value  
  for variable in g.variables.keys():
    #'expReturns': ['Expected returns','€/ha','forArea', 'totalExpReturns'],
    if len(g.variables[variable]) == 4:
      pondVariable = g.variables[variable][2]
      totalVariable = g.variables[variable][3]
      for region in g.regions.keys():
        for scenario in g.scenarios.keys():
	  for spGroup in specieswithAggregates:
	    for year in g.years:
              key = region, variable, scenario, spGroup, year
              key_pvar = region, pondVariable, scenario, spGroup, year
              key_tvar = region, totalVariable, scenario, spGroup, year
	      g.idata[key] =  (g.idata[key_tvar] / g.idata[key_pvar]) if  g.idata[key_pvar] != 0 else 0
	      
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
  for region in g.regions.keys():
    for variable in variablesWithAggregates:
      for scenario in g.scenarios.keys():
        for spGroup in tempSpecieswithAggregates:
          key = region, variable, scenario, spGroup
          for year in g.years:
            key_year = region, variable, scenario, spGroup, year
            g.odata[key].append(g.idata[key_year])
  
  # testing odata
  #print "idata[2005]: " + str(idata['11000', 'vol', 'vRegFixed','Total', '2005'])
  #print "idata[2006]: " + str(idata['11000', 'vol', 'vRegFixed','Total', '2006'])
  #print "odata: " + str(odata['11000', 'vol', 'vRegFixed','Total'])
  
# =============================================================================  
def reset_output():
  # G - Reset latex files
  filename_t = g.tableoutdir+'/'+g.tablesmaster+'.tex'
  filename_c = g.chartoutdir+'/'+g.chartsmaster+'.tex'
  file_t = open(filename_t,'w')
  file_c = open(filename_c,'w')
  file_t.close()
  file_c.close()

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
    subplotTitle = unicode(g.variables[variables_h[i]][0], 'utf_8')
    ylabel = unicode(g.variables[variables_h[i]][1], 'utf_8')
    plt.title(subplotTitle)
    plt.ylabel(ylabel)
    for spGroup in sorted(g.spAggregates.keys()):
      for scenario in scenarios_h:
        serieName = unicode(spGroup + " - " + scenario, 'utf_8')
        serieColor = g.scenarios[scenario]
        serieLineType = g.spAggregates[spGroup][1]
        serieWidth = g.spAggregates[spGroup][2]
        #print serieName+ " - " + serieLineType + " - " + str(serieWidth)
        key = region, variables_h[i], scenario, spGroup
        y = g.odata[key]
        plt.plot(g.x, y, serieLineType, label=serieName, linewidth=serieWidth, color=serieColor)
    handles, labels = ax.get_legend_handles_labels()
  #plt.subplots_adjust(hspace=0.6)
  #handles, labels = ax.get_legend_handles_labels()
  #ax.legend(handles, labels, ncol=3, shadow=False, title="Legend")
  if printLegend:
    plt.figlegend(handles, labels, loc = 'lower center', ncol=3, shadow=False, labelspacing=0., prop={'size':12})
  #plt.savefig(chartoutdir+"/"+filename+"_"+region+"."+charttype, bbox_inches='tight', dpi=300)
  plt.savefig(g.chartoutdir+"/"+filename+"_"+region+"."+g.charttype, dpi=300)
  #plt.show()
  plt.close()
        
  omasterfilename = g.chartoutdir+'/'+g.chartsmaster+'.tex'
  omfile = open(omasterfilename,'a')
  omfile.write("\\begin{figure}[htbp]\n")
  omfile.write("  \\centering\n")
  omfile.write("  \\caption{"+title+"}\n")
  omfile.write("  \\includegraphics[width=0.8\\textwidth]{"+g.chartoutdir+"/"+filename+"_"+region+"}\n")
  omfile.write("  \\label{fig:"+filename+"}\n")
  omfile.write("\\end{figure}\n")
  omfile.close()
 
# =============================================================================
def plotCarbonChart(scenarios_h,region,title, filename):
#def plotMultivariable(scenarios_h, variables_h, region, title, filename, printLegend=True):
  
 
  cVariables = [
      ['Forest pool', ['STOCK_INV','STOCK_EXTRA'],':',3],
      ['Wood products pool', ['STOCK_PRODUCTS'],'--',3],
      ['Net cumulative substitution effect', ['EM_ENSUB','EM_MATSUB','EM_FOROP'],'-',4],
  ]    
  
  nscen = len(scenarios_h)
  

  matplotlib.rcParams.update({'font.size': 22})


  fig = plt.gcf()
  fig.set_size_inches(12,10)
  ylabel = unicode("Gt CO2 eq", 'utf_8')
  plt.title(unicode(title, 'utf_8'))
  plt.ylabel(ylabel)
  
  totals = [[0]*len(g.x)]* nscen
  
  for idg, cGroup in enumerate(cVariables): 
    for ids, scenario in enumerate(scenarios_h):    
      grTotals =  [0]*len(g.x)
      #serieName = unicode(cGroup[0] + " - " + scenario, 'utf_8')
      serieName = "_"+unicode(scenario, 'utf_8') # not shown in legend    
      if idg==2:
         serieName = unicode(scenario, 'utf_8')         
      serieColor = g.scenarios[scenario]
      serieLineType = cGroup[2]
      serieWidth = cGroup[3]
      for var in cGroup[1]: # for idx, var in enumerate(cGroup[1]):
        key = region, var, scenario, ""
        varData = g.odata[key]
        grTotals = [x2+y for x2, y in zip(grTotals, varData)]
        
      totals[ids]  = [x3+y2 for x3, y2 in zip(totals[ids],grTotals)]
      y = [x4 / 1000 for x4 in totals[ids]]
      plt.plot(g.x, y, serieLineType, label=serieName, linewidth=serieWidth, color=serieColor)
  #plt.legend(loc='lower right', ncol=3, shadow=False, labelspacing=0., prop={'size':12})
  plt.legend(loc='lower right', ncol=1, shadow=False, labelspacing=0.)
  #plt.ylim([0,18]) # This would scale the plot y axis to the desired ranges
  plt.savefig(g.chartoutdir+"/"+filename+"_"+region+"."+g.charttype, dpi=300)
  #plt.show()
  plt.close()
        
  omasterfilename = g.chartoutdir+'/'+g.chartsmaster+'.tex'
  omfile = open(omasterfilename,'a')
  omfile.write("\\begin{figure}[htbp]\n")
  omfile.write("  \\centering\n")
  omfile.write("  \\caption{"+title+"}\n")
  omfile.write("  \\includegraphics[width=0.8\\textwidth]{"+g.chartoutdir+"/"+filename+"_"+region+"}\n")
  omfile.write("  \\label{fig:"+filename+"}\n")
  omfile.write("\\end{figure}\n")
  omfile.close() 
      
"""        
      scenTotals
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
  """
  
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
  
  for spGroup in sorted(g.spAggregates.keys()):
    for scenario in scenarios_h:
      serieName = unicode(spGroup + " - " + scenario, 'utf_8')
      serieColor = g.scenarios[scenario]
      serieLineType = g.spAggregates[spGroup][1]
      serieWidth = g.spAggregates[spGroup][2]
      #print serieName+ " - " + serieLineType + " - " + str(serieWidth)
      dummyx = [1]
      dummyy = [1]
      plt.plot(dummyx, dummyy, serieLineType, label=serieName, linewidth=serieWidth, color=serieColor)
  handles, labels = ax.get_legend_handles_labels()
  ax.legend(handles, labels, ncol=3, shadow=False, title="Legend")
  plt.savefig(g.chartoutdir+"/"+filename+"."+g.charttype, bbox_inches='tight', pad_inches=0.1, dpi=300) 
  #plt.show()
  plt.close()
  
#import matplotlib.pyplot as plt
#ax = plt.subplot()  #create the axes
#ax.set_axis_off()  #turn off the axis
#....  #do patches and labels
#ax.legend(patches, labels, ...)  #legend alone in the figure
#plt.show() 

# =============================================================================
def plotVectorChart_inner(origin,end1,endt,xlabel,ylabel,filename):
 a=plt.figure()
 ax = plt.gca()
 fig = plt.gcf()
 flag_2d = True
 if(origin[0] == end1[0] == endt[0]):
   flag_2d = False;
   fig.set_size_inches(6,10)
 else:
   fig.set_size_inches(10,10)
 end2 = (endt[0]-end1[0]+origin[0],endt[1]-end1[1]+origin[1])
 minx = min(origin[0],end1[0],end2[0],endt[0])
 maxx = max(origin[0],end1[0],end2[0],endt[0])
 miny = min(origin[1],end1[1],end2[1],endt[1])
 maxy = max(origin[1],end1[1],end2[1],endt[1])
 centre = (((maxx-minx)/2)+minx,((maxy-miny)/2)+miny)
 X  = (origin[0], origin[0], origin[0])
 Y  = (origin[1], origin[1], origin[1])
 X2 = (end1[0]-origin[0], endt[0]-origin[0], end2[0]-origin[0]) 
 Y2 = (end1[1]-origin[1], endt[1]-origin[1], end2[1]-origin[1])
 C  = (255,010,150) # ? colour codes, but didn't got it

 ax.quiver(X,Y,X2,Y2,C,angles='xy',scale_units='xy',scale=1, width=0.008)
 x  = (end1[0],end2[0])
 y  = (end1[1],end2[1])
 x2 = (endt[0]-end1[0], endt[0]-end2[0])
 y2 = (endt[1]-end1[1], endt[1]-end2[1])
 
 if(flag_2d):
   ax.quiver(x,y,x2,y2,angles='xy',scale_units='xy',scale=1, width=0.005, color='gray')
   ax.set_xlim([minx- (centre[0]-minx)*0.4, maxx + (maxx-centre[0])*0.4])

 ax.set_ylim([miny- (centre[1]-miny)*0.4, maxy + (maxy-centre[1])*0.4])

 plt.xlabel(unicode(xlabel, 'utf_8'))
 plt.ylabel(unicode(ylabel, 'utf_8'))
 # Uncomment the following lines if you want to display instead of save the figure..
 #plt.draw()
 #plt.show()
 plt.savefig(filename, dpi=300)
 
# =============================================================================
def printTable(ref_scenario, comparing_scenarios, variables_h, regions_h, years_h, title, filename, singleComparation=True, refYear=0):
  """Print a LaTeX Table for variables variable_h comparing ref_scenario scenario vs coparing_scenarios.
  @param singleComparation: if True multiple comparing scenarios are treated as multiple replications of the same scenario and
  some basic stats are computed; if False they are all represented as diff from the ref_scenario.
  @param refYear: if 0 reference vs comparing scenarios are compared on the same year (or average of years if years_h has length > 1.).
  Otherwise the comparing scneario at year(s) years_h is compared with reference scenario at year refYear (useful to see the dynamic
  effects within a single scenario)  
  """
  d = " & "
  el = " \\\\"
  label_comparing_scenario = "comparing scenarios"
  labels_comparing_scenarios = []
  nvar = len(variables_h)
  nscen = len(comparing_scenarios)
  nyears = len(years_h)
  nregions = len(regions_h)
  ncol = 4
  label_ref_scenario = ref_scenario.replace("_", "\\_")
  
  for comp_scenario in comparing_scenarios:
    labels_comparing_scenarios.append(comp_scenario.replace("_", "\\_"))
    
  if (singleComparation and nscen == 1):
    label_comparing_scenario = labels_comparing_scenarios[0]
  
  if (singleComparation):
    if nscen > 5:
      ncol = 5
  else:
    ncol = nscen+2 #+1 for the val label and +1 for the ref scenario
    
  oString = ""
  oString += "\\begin{table}[htbp]\n"
  oString += "\\begin{center}\n"
  oString += "\\begin{threeparttable}\n"
  oString += "\\centering\n"
  oString += "\\caption{"+title+"}\n"
  oString += "\\begin{footnotesize}\n"
  oString += "\\begin{tabularx}{\\textwidth}{l "
  for nc in range(1,ncol):
    oString += "r "
  oString += "}\n"
  oString += "\\hline\n"
  if (singleComparation):
    if nscen > 5:
      oString += d+label_ref_scenario+d+label_comparing_scenario+d+"difference"+d+"cv"+el+"\n"  
    else:
      oString += d+label_ref_scenario+d+label_comparing_scenario+d+"difference"+el+"\n"
  else:
    oString += d+label_ref_scenario
    for label_comparing_scenarios in labels_comparing_scenarios:
      oString += d+label_comparing_scenarios
    oString += el+'\n' 
    
  for region in regions_h:
    oString += "\\hline\n"
    if nregions > 1:
      oString += "\\multicolumn{"+str(ncol)+"}{l}{"+regions[region]+"}"+el+'\n'
  
    for variable in variables_h:
      oString += "\\multicolumn{"+str(ncol)+"}{l}{"+g.variables[variable][0]+" (\\textit{"+g.variables[variable][1]+"})}"+el+'\n'
      for spGroup in sorted(g.spAggregates.keys()):
        outSpGroup = spGroup.replace("_", "\\_")
        sumRScenario =   0
        sumCScenarios = [0] * nscen
        valRScenario = 0
        valCScenarios = [0] * nscen
        for year in years_h:
          rYear = str(refYear) if refYear else year # If we overrided the reference year we gonna pick it up here
          keyr = region, variable, ref_scenario, spGroup, rYear
          sumRScenario += g.idata[keyr]
          for s in range(nscen):
            keyc = region, variable, comparing_scenarios[s], spGroup, year
            sumCScenarios[s] += g.idata[keyc]
        valRScenario = sumRScenario/nyears
        for s in range(nscen):
          valCScenarios[s] = sumCScenarios[s]/nyears        
        oString += printTableRecord("- "+outSpGroup, d, el, nscen, valRScenario, valCScenarios, singleComparation)
    
  oString += "\\hline\n"
  oString += "\\end{tabularx}\n"
  oString += "\\end{footnotesize}\n"
  oString += "\\label{tab:"+filename+"}\n"
  if (singleComparation and nscen > 5):
    oString += "\\begin{tablenotes}\n"
    oString += "\\begin{footnotesize}\n"
    oString += "\\item [a] Significantly different from 0 at $\\alpha=0.01$\n"
    oString += "\\item [b] Significantly different from 0 at $\\alpha=0.001$\n"
    oString += "\\end{footnotesize}\n"
    oString += "\\end{tablenotes}\n"
  oString += "\\end{threeparttable}\n"
  oString += "\\end{center}\n"
  oString += "\\end{table}\n" 
  
  ofilename = g.tableoutdir+'/'+filename+'.tex'
  ofile = open(ofilename,'w')
  ofile.write(oString)
  ofile.close()
  
  omasterfilename = g.tableoutdir+'/'+g.tablesmaster+'.tex'
  omfile = open(omasterfilename,'a')
  omfile.write("\\input{"+g.tableoutdir+'/'+filename+"}\n")
  omfile.close()
  
# =============================================================================
def printAATable(ref_scenarios, comparing_scenarios, regions_h, years_h, title, filename, refYear=0) :
#def printTable(ref_scenario, comparing_scenarios, variables_h, regions_h, years_h, title, filename): 
#printAATable(['cc1','cc1_nospvar','cc2','cc2_nospvar','cc3','cc3_nospvar','cc3','cc3_nospvar'],['bau','bau_nospvar','bau','bau_nospvar','bau','bau_nospvar','bau','bau_nospvar'], selectedregions, ['2100'],'Area allocation [% variation over bau]', 'area_allocation')
  d = " & "
  el = " \\\\"
  
  scenario_labels = []
  nscen = len(ref_scenarios)
  nscen_comp = len(comparing_scenarios)
  if nscen <> nscen_comp:
    print "Error in printAATable: number of comparing vs reference scenarios must be the same !"
    exit(1)
  nyears = len(years_h)
  nregions = len(regions_h)
  ntotcol = nscen+1
  for scenario in comparing_scenarios:
    scenario_labels.append(scenario.replace("_", "\\_"))
    
    
  oString = ""
  oString += "\\begin{table}[htbp]\n"
  oString += "\\begin{center}\n"
  oString += "\\begin{threeparttable}\n"
  oString += "\\centering\n"
  oString += "\\caption{"+title.replace("_", "\\_").replace("%", "\\%")+"}\n"
  oString += "\\begin{footnotesize}\n"
  oString += "\\begin{tabularx}{\\textwidth}{l "
  for i in range(nscen):
    oString += " r"
  oString += "}\n"

  oString += "\\hline\n"
  oString += "Region"
  for scenario in scenario_labels:
    oString += d+scenario
  oString += el+'\n'      
  for spGroup in sorted(g.spAggregates.keys()):
    oString += "\\multicolumn{"+str(ntotcol)+"}{l}{"+spGroup.replace("_", "\\_")+"}"+el+'\n'
    for region in regions_h:
      oString += g.regions[region]
      for s in range(len(comparing_scenarios)):
        sum_value_b = 0.0
        sum_value_c = 0.0
        for year in years_h:
          rYear = str(refYear) if refYear else year # If we overrided the reference year we gonna pick it up here
          key_b = region, 'forArea', ref_scenarios[s], spGroup, rYear
          key_c = region, 'forArea', comparing_scenarios[s], spGroup, year
          sum_value_b += g.idata[key_b]
          sum_value_c += g.idata[key_c]
        reldiff = (100*(sum_value_c-sum_value_b)/sum_value_b) if  sum_value_b != 0 else 0
        oString +=  d+"%+0.3f"%(reldiff)
      oString += el+'\n'
     
     
  oString += "\\hline\n"
  oString += "\\end{tabularx}\n"
  oString += "\\end{footnotesize}\n"
  oString += "\\label{tab:"+filename+"}\n"
  oString += "\\end{threeparttable}\n"
  oString += "\\end{center}\n"
  oString += "\\end{table}\n" 
  
  ofilename = g.tableoutdir+'/'+filename+'.tex'
  ofile = open(ofilename,'w')
  ofile.write(oString)
  ofile.close()
  
  omasterfilename = g.tableoutdir+'/'+g.tablesmaster+'.tex'
  omfile = open(omasterfilename,'a')
  omfile.write("\\input{"+g.tableoutdir+'/'+filename+"}\n")
  omfile.close()

# =============================================================================
def printCarbonTable(ref_scenario, comparing_scenarios, region, year_start, year_end, title, filename, avg=False, singleComparation=True ) :
  #Print carbon balance
  # @params:
  # avg:               true  => output is the yearly average in the period,
  #                    false => output is the difference between year_start and year_end
  # singleComparation: true  => comparing scenarios are seens as repetition of a unique scenario, hence stats on their variance is performed,
  #                    false => each comparing scenarios is presented indipendently 
  d = " & "
  el = " \\\\"
  
  cvariables = [
      ['Pools', "- Total pools", [
         ['STOCK_INV',"- Inventoried forest pool"],
         ['STOCK_EXTRA',"- Extra forest pool (branches and roots)"],
         ['STOCK_PRODUCTS',"- Wood products pool"]
         ]],
      ['Emissions', "- Net substitution",
        [['EM_ENSUB',"- Energy substitution"], 
         ['EM_MATSUB',"- Material substitution"],
         ['EM_FOROP',"- Emissions from forest operations"]
         ]],
  ]       
   
  label_comparing_scenario = "comparing scenarios"
  labels_comparing_scenarios = []
  nscen = len(comparing_scenarios)
  nyears = (int(year_end) - int(year_start) + 1) if avg else 1
  ncol = 4
  label_ref_scenario = ref_scenario.replace("_", "\\_")
  
  for comp_scenario in comparing_scenarios:
    labels_comparing_scenarios.append(comp_scenario.replace("_", "\\_"))
  
  if (singleComparation and nscen == 1):
    label_comparing_scenario = labels_comparing_scenarios[0]
    
  if (singleComparation):
    if nscen > 2:
      ncol = 5
  else:
    ncol = nscen+2 
    
  oString = ""
  oString += "\\begin{table*}[!htbp]\n"
  oString += "\\begin{center}\n"
  oString += "\\begin{threeparttable}\n"
  oString += "\\centering\n"
  oString += "\\begin{footnotesize}\n"
  oString += "\\begin{tabularx}{\\textwidth}{l "
  for nc in range(1,ncol):
    oString += "r "
  oString += "}\n"
  oString += "\\hline\n"
  
  if (singleComparation):
    if nscen > 2:
      oString += d+"\\texttt{"+label_ref_scenario+"}"+d+"\\texttt{"+label_comparing_scenario+"}"+d+"difference"+d+"cv"+el+"\n"  
    else:
      oString += d+"\\texttt{"+label_ref_scenario+"}"+d+"\\texttt{"+label_comparing_scenario+"}"+d+"difference"+el+"\n"
  else:
    oString += d+label_ref_scenario
    for label_comparing_scenarios in labels_comparing_scenarios:
      oString += d+label_comparing_scenarios
    oString += el+'\n' 
 
  if(nyears > 1):
    oString += "\\multicolumn{"+str(ncol)+"}{l}{Carbon balance ($Mt~ \ce{CO2}eq.~y^{-1}$)}"+el+'\n'
  else:
    oString += "\\multicolumn{"+str(ncol)+"}{l}{Carbon balance ($Mt~ \ce{CO2}eq.)$}"+el+'\n'
    
  # Total totals..
  totSumValRScenario = 0
  totSumValCScenarios = [0] * nscen
  for vargroup in cvariables:
    # Group totals..
    grSumValRScenario = 0
    grSumValCScenarios = [0] * nscen
    oString += "\\multicolumn{"+str(ncol)+"}{l}{"+vargroup[0]+"}"+el+'\n'
    # Working on the single variables..
    for cvar in vargroup[2]:
      cvar_name           = cvar[0]
      cvar_label          = cvar[1]
      valRScenario        = (g.idata[region, cvar_name, ref_scenario, "", year_end]-g.idata[region, cvar_name, ref_scenario, "", year_start])/nyears
      grSumValRScenario  += valRScenario
      totSumValRScenario += valRScenario
      valCScenarios       = [0] * nscen
      
      for s in range(nscen):
        valCScenarios[s] = (g.idata[region, cvar_name, comparing_scenarios[s], "", year_end]-g.idata[region, cvar_name, comparing_scenarios[s], "", year_start])/nyears
        grSumValCScenarios[s] += valCScenarios[s]
        totSumValCScenarios[s] += valCScenarios[s]
      oString += printTableRecord(cvar_label, d, el, nscen, valRScenario, valCScenarios,singleComparation)
    oString += printTableRecord(vargroup[1], d, el, nscen, grSumValRScenario, grSumValCScenarios,singleComparation)
  oString += printTableRecord("Total \ce{CO2} balance", d, el, nscen, totSumValRScenario, totSumValCScenarios,singleComparation)
   
  oString += "\\hline\n"
  oString += "\\end{tabularx}\n"
  oString += "\\end{footnotesize}\n"
  oString += "\\label{tab:"+filename+"}\n"
  if (singleComparation and nscen > 2):
    oString += "\\begin{tablenotes}\n"
    oString += "\\begin{footnotesize}\n"
    oString += "\\item [a] Significantly different from 0 at $\\alpha=0.01$\n"
    oString += "\\item [b] Significantly different from 0 at $\\alpha=0.001$\n"
    oString += "\\end{footnotesize}\n"
    oString += "\\end{tablenotes}\n"
  oString += "\\end{threeparttable}\n"
  oString += "\\begin{footnotesize}\n"
  oString += "\\caption{"+title+"}\n"
  oString += "\\end{footnotesize}\n"  
  oString += "\\end{center}\n"
  oString += "\\end{table*}\n" 
  
  ofilename = g.tableoutdir+'/'+filename+'.tex'
  ofile = open(ofilename,'w')
  ofile.write(oString)
  ofile.close()
  
  omasterfilename = g.tableoutdir+'/'+g.tablesmaster+'.tex'
  omfile = open(omasterfilename,'a')
  omfile.write("\\input{"+g.tableoutdir+'/'+filename+"}\n")
  omfile.close()
# =============================================================================
def printTableRecord(cvar_label, d, el, nscen, valRScenario, valCScenarios, singleComparation):
  
  oString = ""
  if singleComparation:
    avgCScenarios = sum(valCScenarios) / float(nscen)
    scenarioDiff = avgCScenarios-valRScenario
    scenarioRelativeDiff = 100 * scenarioDiff/valRScenario if valRScenario else 0.0
    if nscen > 2:
      significance = ""
      qdiffCScenarios = [0] * nscen
      sumqdiffCScenarios = 0
      for s in range(nscen):
        qdiffCScenarios[s] = (valCScenarios[s] - avgCScenarios)**2.0
        sumqdiffCScenarios += qdiffCScenarios[s]
      sd = (sumqdiffCScenarios/(nscen-1))**0.5
      t = abs(scenarioDiff)*nscen**0.5/sd if sd>0.0 else 0.0 
      cv = 100.0 * sd/abs(avgCScenarios) if abs(avgCScenarios)> 0.0 else 0.0
      if t >= g.tvalue001[nscen-1-1]:
        significance = '$^a$'
      if t >= g.tvalue0001[nscen-1-1]:
        significance = '$^b$'
      oString += cvar_label+d+"%0.3f"%(valRScenario)+d+"%0.3f"%(avgCScenarios)+d+"%0.3f"%(scenarioDiff)+significance+' ('+"%0.3f"%(scenarioRelativeDiff)+'\\%)'+d+"%0.2f"%(cv)+' \\%'+el+'\n'
    else:
      oString += cvar_label+d+"%0.3f"%(valRScenario)+d+"%0.3f"%(avgCScenarios)+d+"%0.3f"%(scenarioDiff)+' ('+"%0.2f"%(scenarioRelativeDiff)+'\\%)'+el+'\n'
  else:
    oString += cvar_label+d+"%0.3f"%(valRScenario)
    for valCScenario in valCScenarios:
      scenarioDiff = valCScenario-valRScenario
      scenarioRelativeDiff = 100 * scenarioDiff/valRScenario if valRScenario else 0.0
      oString += d+"%0.2f"%(scenarioRelativeDiff)+'\\%'
    oString += el + '\n'
  return oString


 
# =============================================================================
def title (cat, level, title):
  filename = ""
  if cat == 't':
    filename = g.tableoutdir+'/'+g.tablesmaster+'.tex'
  elif cat == 'c': 
    filename = g.chartoutdir+'/'+g.chartsmaster+'.tex'
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
    filename = g.tableoutdir+'/'+g.tablesmaster+'.tex'
  elif cat == 'c': 
    filename = g.chartoutdir+'/'+g.chartsmaster+'.tex'
  else:
    print "Error in text: not know where to print the title !"
    exit(1)
  file = open(filename,'a')
  file.write(text_h+"\n")
  file.close()
