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

  g.forIFiles = [  
    'results/forestData_merged.csv',
  ]
  
  g.carbonIFile = [
    'results/carbonBalance_merged.csv',
  ]

  g.scenarios = {
    'baseline_temp':       '#000000',  # Black

  }
  g.years = [str(y) for y in range(2013,2051)]     # [2013-2050]
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
  
  
  for region in g.sortedregions: #selectedregions:#g.sortedregions:
    title('c','subsection',g.regions[region])
    plotMultivariable(['baseline_temp'], ['expReturns', 'vol', 'hV', 'forArea'], region, 'baseline\_temp', 'baseline_temp')

  title('c','subsection', "Carbon charts")
  plotCarbonChart(['baseline_temp'],'FRA','','cbalance')
  
  
# =============================================================================  
def printTables():
  print ("Printing tables..")

  for region in g.sortedregions: #selectedregions:#g.sortedregions:
    title('c','subsection',g.regions[region])
    #printTable(ref_scenario, comparing_scenarios, variables_h, regions_h, years_h, title, filename, singleComparation=False, refYear=0):
    printTable('baseline_temp',['baseline_temp'],['expReturns', 'vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2050'],'[2050 vs 2013]','baseline_temp_2013-2050_11000',True,2013)


  
# =============================================================================  
def printAATables():
  print ("Printing area allocation tables..")

# =============================================================================
# EXECUTION ACTUALLY STARTS HERE.....
main()
