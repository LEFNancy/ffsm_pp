#!/usr/bin/env python

import sys, csv, math


''' Scope of this script
- parse the pythia output to produce nice summarized tables
'''

#parameters
forIFiles = [  
  'output_nonspatial/results/forestData.csv',
  'output_reference/results/forestData.csv',
  'output_vRegEnd075/results/forestData.csv',
  'output_vRegFixed/results/forestData.csv',
  'output_vRegFromHr/results/forestData.csv',
  'output_withRisk06/results/forestData.csv',
  'output_withRisk08/results/forestData.csv',
  'output_withRisk10/results/forestData.csv',
  'output_withVariance/results/forestData.csv',
]
scenarios = [
  'nonspatial',
  'reference',
  'vRegEnd075',
  'vRegFixed',
  'vRegFromHr',
  'withRisk06',
  'withRisk08',
  'withRisk10',
  'withVariance', 
]
years = ['2030','2050','2100']
ofilename = 'output.csv'
ofilename_hml = 'output.html'
sep = ';'
spAggregates = {}

countries = {}
countries['11000'] = [
  '11042', '11061', '11072', '11025', '11026', '11052', '11024', '11021',
  '11083', '11043', '11023', '11010', '11081', '11063', '11041', '11062',
  '11030', '11051', '11022', '11053', '11082', '11071',
]
regions = [
  '11042', '11061', '11072', '11025', '11026', '11052', '11024', '11021',
  '11083', '11043', '11023', '11010', '11081', '11063', '11041', '11062',
  '11030', '11051', '11022', '11053', '11082', '11071',
]
variables = ['hV','vReg','vol','expReturns','forArea']
ponderatedVariables = {}
ponderatedVariables['expReturnsXforArea'] = ['expReturns','forArea'] # the original and the variable that acts as ponderation

spGroups = ['broadL_highF','broadL_mixedF','broadL_copp','con_highF','con_mixedF','con_copp']
spAggregates = {}
spAggregates['00_Broadleaved'] = ['broadL_highF','broadL_mixedF','broadL_copp']
spAggregates['01_Coniferous'] = ['con_highF','con_mixedF','con_copp']
spAggregates['02_High Forest'] = ['broadL_highF','con_highF']
spAggregates['03_Mixed Forests'] = ['broadL_mixedF','con_mixedF']
spAggregates['04_Coppers'] = ['broadL_copp','con_copp']

# input data:
# - scen;parName;country;region;forType;diamClass;year;value;



# =============================================================================
def main():  
  odata = {}
  
  print "Creating nicely formatted output.."
  
  # creating empty dictionaries with just the keys  
  for varProd, varsToBeProd in ponderatedVariables.iteritems():
    #print varsToBeProd
    variables.append(varProd) 
  for country, regionsInTheCountry in countries.iteritems():
    #print varsToBeProd
    regions.append(country)  
  for region in regions:
    for variable in variables:
      for scenario in scenarios:
	for spGroup in spGroups:
	  for year in years:
            key = region, variable, scenario, spGroup, year
	    odata[key] = 0.0
  #print variables
  
  # loading data..	    
  for ifile in forIFiles:	    
    idata_raw = csv.DictReader(open(ifile, 'rb'), delimiter=sep)
    for rec in idata_raw:
      key = rec['region'],rec['parName'],rec['scen'],rec['forType'],rec['year']
      if key in odata:
        odata[key] += float (rec['value'])

  # computing the ponderated variables..
  for region in regions:
    for varProd, varsToBeProd in ponderatedVariables.iteritems():
      for scenario in scenarios:
	for spGroup in spGroups:
	  for year in years:
	    destKey = region, varProd, scenario, spGroup, year
	    odata[destKey] = 1.0
	    for varToBeProd in varsToBeProd:
	      varToBeProdKey = region, varToBeProd, scenario, spGroup, year
	      odata[destKey] = odata[destKey] * odata[varToBeProdKey]
	   
  # summing up to the country level..
  for country, regionsInTheCountry in countries.iteritems():
    for variable in variables:
      for scenario in scenarios:
	for spGroup in spGroups:
	  for year in years:
	    destKey = country, variable, scenario, spGroup, year
	    odata[destKey] = 0.0
	    for regionInTheCountry in regionsInTheCountry:
	      varToBeSumKey = regionInTheCountry, variable, scenario, spGroup, year
	      odata[destKey] +=  odata[varToBeSumKey]
        
  # printing results ready..
  print "Printing results to "+ofilename+" .."
  ofile = open(ofilename,'w')
  nyears = len(years)
  header1 = 'region'+sep+'variable'+sep+'scenario'+sep
  #for spGroup in spGroups:
  for spAggregate, species in sorted(spAggregates.iteritems()):
   for year in years:
     header1 += (spAggregate+sep)
  header1 += '\n'
  header2 = sep+sep+sep
  #for spGroup in spGroups:
  for spAggregate, species in spAggregates.iteritems():
   for year in years:
     header2 += (year+sep)
  header2 += '\n'

  ofile.write(header1)
  ofile.write(header2)
  
  for region in regions:
    for variable in variables:
      for scenario in scenarios:
        outputRow = region+sep+variable+sep+scenario+sep
        #for spGroup in spGroups:
	for spAggregate, species in sorted(spAggregates.iteritems()):  
          for year in years:
            value = 0;
            for specie in species:
	      key = region, variable, scenario, specie, year
	      value += odata[key]
            outputRow += str(value)+sep
        outputRow += '\n'
        ofile.write(outputRow)
  ofile.close()
  
    
# =============================================================================
# EXECUTION ACTUALLY STARTS HERE.....
main()