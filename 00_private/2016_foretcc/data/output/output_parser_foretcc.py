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
  
  print "Done!"

# =============================================================================
def override_globals():

  g.forIFiles = [  
    'output/results/forestData_baseline.csv',
    'output/results/forestData_constant.csv',
    'output/results/forestData_Ph_L.csv',
    'output/results/forestData_Ph_U.csv',
    'output/results/forestData_Pr_C.csv',
    'output/results/forestData_Pr_U.csv',
    'output/results/forestData_Exp_0.csv',
    'output/results/forestData_Exp_1.csv',
    'output/results/forestData_EOL_en_U.csv', 
  ]
  
  g.carbonIFile = [
    'output/results/carbonBalance_baseline.csv',
    'output/results/carbonBalance_constant.csv',
    'output/results/carbonBalance_Ph_L.csv',
    'output/results/carbonBalance_Ph_U.csv',
    'output/results/carbonBalance_Pr_C.csv',
    'output/results/carbonBalance_Pr_U.csv',
    'output/results/carbonBalance_Exp_0.csv',
    'output/results/carbonBalance_Exp_1.csv',
    'output/results/carbonBalance_EOL_en_U.csv',
  ]

  g.scenarios = {
    'baseline':             '#000000',  # Black
    'constant':             '#cccccc',  # Grey
    'Ph_L':                 '#b5ff95',  # Light green
    'Ph_U':                 '#f40303',  # Red
    'Pr_C':                 '#b5ff95',  # Light green
    'Pr_U':                 '#f40303',  # Red
    'Exp_0':                '#b5ff95',  # Light green 
    'Exp_1':                '#f40303',  # Red
    'EOL_en_U':             '#011bb7',  # Ink blue

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

# =============================================================================   
def printCharts():
  print "Printing charts.."

  title('c','subsection', "Carbon charts")
  plotCarbonChart(['constant','baseline'],'11000','','cbalance_overall')
  plotCarbonChart(['baseline','Exp_0','Exp_1'],'11000','','cbalance_expectations')
  plotCarbonChart(['baseline','Pr_C','Pr_U'],'11000','','cbalance_prices')
  plotCarbonChart(['baseline','Ph_L','Ph_U'],'11000','','cbalance_ph_impact')
  
# =============================================================================  
def printTables():
  print "Printing tables.."

  y2014_2060 =  [str(y) for y in range(2014,2061)] # [2014-2060]
  
  title('t','section', "Overall effect")
  printTable('constant',['baseline'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],g.years,'\\texttt{Baseline} vs \\texttt{constant} [avg. 2007-2100]','cceffect_overall_vars_2007-2100_11000')
  printTable('constant',['baseline'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],['2100'],'\\texttt{Baseline} vs \\texttt{constant} [2100]','cceffect_overall_vars_2100_11000')
  printCarbonTable('constant',['baseline'],'11000', '2007', '2100', "\\ce{CO2} balance of \\texttt{baseline} scenario vs. \\texttt{constant} [yearly avg 2007-2100]",'cceffect_cbalance_2007-2100_11000', True, True)
  printCarbonTable('constant',['baseline'],'11000', '2013', '2020', "\\ce{CO2} balance of \\texttt{baseline} scenario vs. \\texttt{constant} [yearly avg 2013-2020]",'cceffect_cbalance_2013-2020_11000', True, True)

  title('t','section', "Sa on price, physical and expectation effects")
  printTable('baseline',['Pr_C','Pr_U','Ph_L','Ph_U','Exp_0','Exp_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],g.years,'SA [avg. 2007-2100]','sa_vars_2007-2100_11000',False)
  printCarbonTable('baseline',['Pr_C','Pr_U','Ph_L','Ph_U','Exp_0','Exp_1'],'11000', '2007', '2100', "Sensitivity analisys \\ce{CO2} balance [avg. 2007-2100]",'sa_cbalance_2007-2100_11000', True, False)
  printTable('baseline',['Pr_C','Pr_U','Ph_L','Ph_U','Exp_0','Exp_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],y2014_2060,'SA [avg. 2014-2060]','sa_vars_2014-2060_11000',False)
  printCarbonTable('baseline',['Pr_C','Pr_U','Ph_L','Ph_U','Exp_0','Exp_1'],'11000', '2014', '2060', "Sensitivity analisys \\ce{CO2} balance [yearly avg. 2014-2060]",'sa_cbalance_2014-2060_11000', True, False)
  printTable('baseline',['Pr_C','Pr_U','Ph_L','Ph_U','Exp_0','Exp_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],['2100'],'SA [2100]','sa_vars_2100_11000',False)

  printCarbonTable('baseline',['EOL_en_U'],'11000', '2007', '2100', "\\ce{CO2} balance of \\texttt{EOL\\_en\\_U} scenario vs. \\texttt{baseline} [yearly avg 2007-2100]",'EOL_en_U_cbalance_2007-2100_11000', True, True)

  
# =============================================================================  
def printAATables():
  print "Printing area allocation tables.."

# =============================================================================
# EXECUTION ACTUALLY STARTS HERE.....
main()
