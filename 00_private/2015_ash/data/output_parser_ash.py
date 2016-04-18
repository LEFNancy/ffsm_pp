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
    #'output/results/forestData_P0-Mh-Ei.csv',
    'output/results/forestData_P1-Mh-Ei.csv',
    'output/results/forestData_P0-Mi-Ei.csv',
    'output/results/forestData_P1-Mi-Ei.csv',
    #'output/results/forestData_P0-Me-Ei.csv',
    'output/results/forestData_P1-Me-Ei.csv',
    #'output/results/forestData_P0-Mi-E0.csv',
    'output/results/forestData_P1-Mi-E0.csv',
    #'output/results/forestData_P0-Mi-E1.csv', 
    'output/results/forestData_P1-Mi-E1.csv',
  ]
  
  
  g.prodIFiles = [
    #'output/results/productData_P0-Mh-Ei.csv',
    #'output/results/productData_P1-Mh-Ei.csv',
    #'output/results/productData_P0-Mi-Ei.csv',
    #'output/results/productData_P1-Mi-Ei.csv',
    #'output/results/productData_P0-Me-Ei.csv',
    #'output/results/productData_P1-Me-Ei.csv',
    #'output/results/productData_P0-Mi-E0.csv',
    #'output/results/productData_P1-Mi-E0.csv',
    #'output/results/productData_P0-Mi-E1.csv',
    #'output/results/productData_P1-Mi-E1.csv',
    
  ]
  g.carbonIFiles = [
    #'output/results/carbonBalance_P0-Mh-Ei.csv',
    'output/results/carbonBalance_P1-Mh-Ei.csv',
    'output/results/carbonBalance_P0-Mi-Ei.csv',
    'output/results/carbonBalance_P1-Mi-Ei.csv',
    #'output/results/carbonBalance_P0-Me-Ei.csv',
    'output/results/carbonBalance_P1-Me-Ei.csv',
    #'output/results/carbonBalance_P0-Mi-E0.csv',
    'output/results/carbonBalance_P1-Mi-E0.csv',
    #'output/results/carbonBalance_P0-Mi-E1.csv',
    'output/results/carbonBalance_P1-Mi-E1.csv',
  ]

  g.scenarios = {
    #'P0-Mh-Ei':             '#000000',  # Black
    'P1-Mh-Ei':             '#cccccc',  # Grey
    'P0-Mi-Ei':             '#b5ff95',  # Light green
    'P1-Mi-Ei':             '#f40303',  # Red
    #'P0-Me-Ei':             '#b5ff95',  # Light green
    'P1-Me-Ei':             '#f40303',  # Red
    #'P0-Mi-E0':             '#b5ff95',  # Light green 
    'P1-Mi-E0':             '#f40303',  # Red
    #'P0-Mi-E1':             '#011bb7',  # Ink blue
    'P1-Mi-E1':             '#011bb7',  # Ink blue
  }
  
  
  
  g.years = [str(y) for y in range(2007,2101)]     # [2007-2100]
  g.printChartsFlag = True
  g.printTablesFlag = True
  g.printAATablesFlag = False
  g.chartoutdir = 'output/charts'
  g.tableoutdir = 'output/tables'
  # key: var short name
  # value: turple with long name, unit and optionally variable to act for ponderation and name of aggregated variable
  g.variables = {'hV': ['Harvested Volumes', r"$Mm^3$"],
       'vReg': ['Regeneration Volumes', r"$Mm^3$"],
       'vol': ['Forest Volumes', r"$Mm^3$"],
       'expReturns': ['Expected returns','€/ha','forArea', 'totalExpReturns'],
       'forArea': ['Forest area','ha'],
       'harvestedArea': ['Harvested area','ha'],
       'regArea': ['Regeneration area','ha'],
       'STOCK_INV': ['Carbon pool in inventoried forest resources', r"$Mt CO_2$"],
       'STOCK_EXTRA': ['Carbon pool in non-inventoried forest resources (branches, roots)', r"$Mt CO_2$"],
       'STOCK_PRODUCTS': ['Carbon pool in forest products', r"$Mt CO_2$"],
       'EM_ENSUB': ['Cumulative emissions from energy substitution', r"$Mt CO_2$"],
       'EM_MATSUB': ['Cumulative emissions from material substitution', r"$Mt CO_2$"],
       'EM_FOROP': ['Cumulative emissions from forest operations', r"$Mt CO_2$"],
       
  }
  g.spGroups = ['ash','broadL_highF','broadL_mixedF','broadL_copp','con_highF']

  #key: human name
  #value[0]: list of sp groups
  #value[1]: chart line type
  #value[2]: chart line width
  g.spAggregates = {'00_Total': [['ash','broadL_highF','broadL_mixedF','broadL_copp','con_highF'],'-',4],
                  '01_Ash': [['ash'],':',3],
                  '02_Broadleaved': [['broadL_highF','broadL_mixedF','broadL_copp'],'--',3],
                  '03_Coniferous': [['con_highF'],'-.',3]}

# =============================================================================   
def printCharts():
  print ("Printing charts..")

  for region in g.sortedregions:
    title('c','subsection',g.regions[region])
    plotMultivariable(['P0-Mi-Ei', 'P1-Mi-Ei'],['expReturns','vReg','vol','hV','forArea'],region,'Effect of tyhe pathogen','pathogen_effect')
  
  # If we want to get carbon balance charts.. I don't think it's significant here..
  #title('c','subsection', "Carbon charts")
  #plotCarbonChart(['constant','baseline'],'11000','','cbalance_overall')
  #plotCarbonChart(['baseline','Exp_0','Exp_1'],'11000','','cbalance_expectations')

  
# =============================================================================  
def printTables():
  print ("Printing tables..")

  #y2015_2100 =  [str(y) for y in range(2015,2101)] # if we want to focus on a subperiod..
 
  
  
  
  for region in g.sortedregions:
    title('t','subsection',g.regions[region])
    title('t','subsubsection', "Overall effect")
    printTable('P0-Mi-Ei',['P1-Mi-Ei'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],g.years,'\\texttt{P0-Mi-Ei} vs \\texttt{P1-Mi-Ei} [avg. 2007-2100]','path_overall_effect_for_2007-2100_'+region)
    printTable('P0-Mi-Ei',['P1-Mi-Ei'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2100'],'\\texttt{P0-Mi-Ei} vs \\texttt{P1-Mi-Ei} [2100]','path_overall_effect_for_2100_'+region)
    printTable('P1-Mi-Ei',['P1-Mi-Ei'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2100'],'\\texttt{P1-Mi-Ei} evolution [2100 vs 2015]','path_time_dynamic_for_2100_vs_2015'+region,False,'2015')
  
  #printTable('P0-Mi-Ei',['P1-Mi-Ei'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],g.years,'\\texttt{P0-Mi-Ei} vs \\texttt{P1-Mi-Ei} [avg. 2007-2100]','path_overall_effect_for_2007-2100_11000')
  #printTable('P0-Mi-Ei',['P1-Mi-Ei'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],['2100'],'\\texttt{P0-Mi-Ei} vs \\texttt{P1-Mi-Ei} [2100]','path_overall_effect_for_2100_11000')
  #printTable('P1-Mi-Ei',['P1-Mi-Ei'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],['2100'],'\\texttt{P1-Mi-Ei} evolution [2100 vs 2015]','path_time_dynamic_for_2100_vs_2015_11000',True,'2015')
  

    title('t','subsubsection', "Sa on expectations and degree of substitutability")
    printTable('P1-Mi-Ei',['P1-Mi-E0','P1-Mi-E1','P1-Mh-Ei','P1-Me-Ei'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],g.years,'SA [avg. 2007-2100]','sa_for_2007-2100_11000',False)
    printTable('P1-Mi-Ei',['P1-Mi-E0','P1-Mi-E1','P1-Mh-Ei','P1-Me-Ei'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],['2100'],'SA [2100]','sa_for_2100_11000',False)

  
# =============================================================================  
def printAATables():
  print ("Printing area allocation tables..")

# =============================================================================
# EXECUTION ACTUALLY STARTS HERE.....
main()
