#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os, sys
import csv, math
import matplotlib.pyplot as plt
import output_parser_globals as g
from output_parser_lib import *


# =============================================================================
def main():  
  
  override_globals()
  prepare_data()
  reset_output()
  
  # H - Printing charts
  if g.printChartsFlag:
    printCharts()
  
  # I - Print tables
  if g.printTablesFlag:
    printTables()
  
  # L - Print area allocation confrontation
  if g.printAATablesFlag:
    printAATables()
  
  print ("Done!")

# =============================================================================
def override_globals():

  g.scenarios = {
    'RCP00_BAS_EXT':                           '#8c0000', # red-brown
    'RCP00_BAS_ADAPT_BASE':                    '#000000',
    'RCP00_BAS_ADAPT_REFORESTATION_ACTIVEMGM': '#000000',
    'RCP00_BAS_ADAPT_SUBSIDES':                '#000000',
    'RCP00_BAS_ADAPT_OPENMARKETS':             '#000000',
    'RCP00_BAS_ADAPT_STUCTURALPOLICIES':       '#000000',
    'RCP00_BAS_ADAPT':                         '#2329C1', # blue  
  }
  
  g.forIFiles   = [ 'results/forestData_'+i+'.csv' for i in  g.scenarios]
  g.carbonIFiles = [ 'results/carbonBalance_'+i+'.csv' for i in  g.scenarios]

  
  
  g.years = [str(y) for y in range(2012,2051)]     # [2012-2050]
  g.printChartsFlag = True
  g.printTablesFlag = True
  g.printAATablesFlag = False
  g.chartoutdir = 'charts'
  g.tableoutdir = 'tables'
  # key: var short name
  # value: turple with long name, unit and optionally variable to act for ponderation and name of aggregated variable
  g.variables = {'hV':   ['Harvested Volumes', r"$Mm^3$"],
       'vReg':           ['Regeneration Volumes', r"$Mm^3$"],
       'vol':            ['Forest Volumes', r"$Mm^3$"],
       'expReturns':     ['Expected returns','€/ha','forArea', 'totalExpReturns'],
       'forArea':        ['Forest area','ha'],
       'harvestedArea':  ['Harvested area','ha'],
       'regArea':        ['Regeneration area','ha'],
       'STOCK_INV':      ['Carbon pool in inventoried forest resources', r"$Mt CO_2$"],
       'STOCK_EXTRA':    ['Carbon pool in non-inventoried forest resources (branches, roots)', r"$Mt CO_2$"],
       'STOCK_PRODUCTS': ['Carbon pool in forest products', r"$Mt CO_2$"],
       'EM_ENSUB':       ['Cumulative emissions from energy substitution', r"$Mt CO_2$"],
       'EM_MATSUB':      ['Cumulative emissions from material substitution', r"$Mt CO_2$"],
       'EM_FOROP':       ['Cumulative emissions from forest operations', r"$Mt CO_2$"],
  }
  
  
  g.spGroups = ['Fut_Feu', 'Fut_Mix', 'Fut_Con', 'Mel_Feu', 'Mel_Mix', 'Tail_Feu',]
  
  g.countries = {'FRA': [['AlsChaLor','AquPoi','AuvLim','BasHau','BouFra','BrePay','Cen','Cor','IleNorPic','LanPro','Mid','Rho'],'France']}

  g.regions = {'AlsChaLor': 'Alsace ; Champagne Ardenne ; Lorraine',
               'AquPoi':    'Aquitaine ; Poitou Charentes',
               'AuvLim':    'Auvergne ; Limousin',
               'BasHau':    'Basse Normandie ; Haute Normandie',
               'BouFra':    'Bourgogne : Franche Comté',
               'BrePay':    'Bretagne ; pays de la Loire',
               'Cen':       'Centre Val de Loire',
               'Cor':       'Corse',
               'IleNorPic': 'Ile de France ; Nord Pas de Calais ; Picardie',
               'LanPro':    'Languedoc Roussillon ; Provence Alpes Côtes d\'Azur',
               'Mid':       'Midi Pyrénées',
               'Rho':       'Rhône Alpes',} 

  
  
  
  g.pProd    = ['hardWRoundW','softWRoundW','pulpWFuelW']
  g.tProd    = ['fuelW','hardWSawnW','softWSawnW','plyW','pulpW','pannels']

  #key: human name
  #value[0]: list of sp groups
  #value[1]: chart line type
  #value[2]: chart line width
  #value[3]: (optional) alias in the data. If present, the input data will be converted to the name at input time 
  g.spAggregates = {'00_Total': [['Fut_Feu', 'Fut_Mix', 'Fut_Con', 'Mel_Feu', 'Mel_Mix', 'Tail_Feu',],'-',4,''],
                    '01_Broadleaved': [['Fut_Feu', 'Mel_Feu','Tail_Feu'],'--',3,'broadL'],
                    '02_Coniferous': [['Fut_Con'],':',3,'con'],
                    '03_Mixed': [['Fut_Mix','Mel_Mix'],'-.',3,'mixedBC']
                    }
  

# =============================================================================   
def printCharts():
  print ("Printing charts..")
  
  
  title('c','subsection', "Carbon charts")
  # def plotCarbonChart(scenarios_h,region,title, filename):
  plotCarbonChart(['RCP00_BAS_EXT','RCP00_BAS_ADAPT'],'FRA','','cbalance')
  
  
# =============================================================================  
def printTables():
  print ("Printing tables..")

  # def printTable(ref_scenario, comparing_scenarios, variables_h, regions_h, years_h, title, filename, singleComparation=False, refYear=0):
  # Print a LaTeX Table for variables variable_h comparing ref_scenario scenario vs coparing_scenarios.
  # @param singleComparation: if True multiple comparing scenarios are treated as multiple replications of the same scenario and
  # some basic stats are computed; if False they are all represented as diff from the ref_scenario.
  # @param refYear: if 0 reference vs comparing scenarios are compared on the same year (or average of years if years_h has length > 1.).
  # Otherwise the comparing scneario at year(s) years_h is compared with reference scenario at year refYear (useful to see the dynamic
  # effects within a single scenario)  
  
  outvars = ['expReturns','harvestedArea','hV','regArea','vReg','forArea','vol']
  outvars_state = ['forArea','vol']
  outvars_flow = ['expReturns','harvestedArea','hV','regArea','vReg']
  printTable('RCP00_BAS_EXT',['RCP00_BAS_EXT'],outvars,['FRA'],['2050'],'[2050 vs 2012]','RCP00_BAS_EXT_2012-2050',False,2012)
  printTable('RCP00_BAS_ADAPT',['RCP00_BAS_ADAPT'],outvars,['FRA'],['2050'],'[2050 vs 2012]','RCP00_BAS_ADAPT_2012-2050',False,2012)
  
  printTable('RCP00_BAS_EXT',['RCP00_BAS_ADAPT'],outvars_state,['FRA'],['2050'],'EXT\_vs\_ADAPT [2050]','EXT_vs_ADAPT_2050',False)
  printTable('RCP00_BAS_EXT',['RCP00_BAS_ADAPT'],outvars_flow,['FRA'],g.years,'EXT\_vs\_ADAPT [avg 2012-2050]','EXT_vs_ADAPT_2012-2050',False)    
  
  printTable('RCP00_BAS_ADAPT_BASE',[ "RCP00_BAS_ADAPT_REFORESTATION_ACTIVEMGM", "RCP00_BAS_ADAPT_SUBSIDES", "RCP00_BAS_ADAPT_OPENMARKETS", "RCP00_BAS_ADAPT_STUCTURALPOLICIES", "RCP00_BAS_ADAPT"],outvars_state,['FRA'],['2050'],'ADAPT\_* [2050]','ADAPT_2050',False)
  printTable('RCP00_BAS_ADAPT_BASE',[ "RCP00_BAS_ADAPT_REFORESTATION_ACTIVEMGM", "RCP00_BAS_ADAPT_SUBSIDES", "RCP00_BAS_ADAPT_OPENMARKETS", "RCP00_BAS_ADAPT_STUCTURALPOLICIES", "RCP00_BAS_ADAPT"],outvars_flow,['FRA'],g.years,'ADAPT\_* [avg 2012-2050]','ADAPT_2012-2050',False)    
  
  
  title('t','subsection',"Carbon tables")
  #printCarbonTable(ref_scenario, comparing_scenarios, region, year_start, year_end, title, filename, avg=False, singleComparation=True )
  printCarbonTable('RCP00_BAS_EXT',['RCP00_BAS_ADAPT'],'FRA', '2012', '2050', "\\ce{CO2} balance [yearly avg. 2012-2050]",'cbalance_11000', True, False)
  printCarbonTable('RCP00_BAS_ADAPT',['RCP00_BAS_ADAPT'],'FRA', '2012', '2050', "\\ce{CO2} balance [yearly avg. 2012-2050]",'cbalance_adapt_11000', True, False)
# =============================================================================  
def printAATables():
  print ("Printing area allocation tables..")

# =============================================================================
# EXECUTION ACTUALLY STARTS HERE.....
main()
