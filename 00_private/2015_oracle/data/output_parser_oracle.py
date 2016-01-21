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
    'output/results/forestData_constant.csv',
    'output/results/forestData_arpege_a1b.csv',
    'output/results/forestData_arpege_a2.csv',
    'output/results/forestData_arpege_b1.csv',
    'output/results/forestData_CNCM33_1.csv',
    'output/results/forestData_DMIEH5C_1.csv',
    'output/results/forestData_DMIEH5C_2.csv',
    'output/results/forestData_DMIEH5C_3.csv',
    'output/results/forestData_EGMAM2_3.csv',
    'output/results/forestData_HADGEM2_1.csv',
    'output/results/forestData_IPCM4_1.csv',
    'output/results/forestData_IPCM4_2.csv',
    'output/results/forestData_IPCM4_3.csv',
    'output/results/forestData_MPEH5C_1.csv',
    'output/results/forestData_MPEH5C_2.csv',
    'output/results/forestData_MPEH5C_3.csv',
    'output/results/forestData_arpege_a1b_priceEffectOnly.csv',
    'output/results/forestData_arpege_a1b_phImpactOnly.csv', 
  ]
  
  g.carbonIFile = [
    'output/results/carbonBalance_constant.csv',
    'output/results/carbonBalance_arpege_a1b.csv',
    'output/results/carbonBalance_arpege_a2.csv',
    'output/results/carbonBalance_arpege_b1.csv',
    'output/results/carbonBalance_CNCM33_1.csv',
    'output/results/carbonBalance_DMIEH5C_1.csv',
    'output/results/carbonBalance_DMIEH5C_2.csv',
    'output/results/carbonBalance_DMIEH5C_3.csv',
    'output/results/carbonBalance_EGMAM2_3.csv',
    'output/results/carbonBalance_HADGEM2_1.csv',
    'output/results/carbonBalance_IPCM4_1.csv',
    'output/results/carbonBalance_IPCM4_2.csv',
    'output/results/carbonBalance_IPCM4_3.csv',
    'output/results/carbonBalance_MPEH5C_1.csv',
    'output/results/carbonBalance_MPEH5C_2.csv',
    'output/results/carbonBalance_MPEH5C_3.csv',    
    'output/results/carbonBalance_arpege_a1b_priceEffectOnly.csv',
    'output/results/carbonBalance_arpege_a1b_phImpactOnly.csv',
  ]

  g.scenarios = {
    'constant':            '#610e0e',
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
    'arpege_a1b_priceEffectOnly': '#0015db', 
    'arpege_a1b_phImpactOnly':    '#6dcaf1',
  }
  g.years = [str(y) for y in range(2007,2101)]     # [2007-2100]
  g.printChartsFlag = True
  g.printTablesFlag = True
  g.printAATablesFlag = True
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
  
  #text('c',"Automatically generated charts.")  
  #plotLegend(['nonspatial','reference','vRegEnd070','vRegFixed','vRegFromHr','withRisk06','withRisk08','withRisk10','withVariance','withSpVarianceAndRisk'],'Scenarios Legend','legend')
  
  selectedregions=['11000', '11041']
  
  for region in g.sortedregions: #selectedregions:#g.sortedregions:
    title('c','subsection',g.regions[region])

    title('c','subsubsection',"IPCC scenarios (using arpege)")
    plotMultivariable(['constant','arpege_a1b', 'arpege_a2', 'arpege_b1'],['expReturns','vReg','vol','hV','forArea'],region,'IPCC Scenarios','ipcc_scenarios')
    
    title('c','subsubsection',"IPCC scenario a1b")
    plotMultivariable(['constant','arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea'],region,'All a1b scenarios','a1b',False)
    plotMultivariable(['constant','arpege_a1b'],['expReturns','vReg','vol','hV','forArea'],region,'Arpege a1b','arpege')
    plotMultivariable(['constant','CNCM33_1'],['expReturns','vReg','vol','hV','forArea'],region,'CNCM33','cncm33')
    plotMultivariable(['constant','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3'],['expReturns','vReg','vol','hV','forArea'],region,'DMIEH5C','dmieh5c')    
    plotMultivariable(['constant','EGMAM2_3'],['expReturns','vReg','vol','hV','forArea'],region,'EGMAM2','egmam2') 
    plotMultivariable(['constant','HADGEM2_1'],['expReturns','vReg','vol','hV','forArea'],region,'HADGEM2','hadgem2') 
    plotMultivariable(['constant','IPCM4_1','IPCM4_2','IPCM4_3'],['expReturns','vReg','vol','hV','forArea'],region,'IPCM4','ipcm4') 
    plotMultivariable(['constant','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea'],region,'MPEH5C','mpeh5c') 
  
  title('c','subsection', "Carbon charts")
  plotCarbonChart(['constant', 'arpege_a1b','arpege_a2','arpege_b1'],'11000','','cbalance')
  
# =============================================================================  
def printTables():
  print "Printing tables.."

  #oyears = ['2030','2031','2032','2033','2034','2035','2036','2037','2038','2039','2040','2041','2042','2043','2044','2045','2046','2047','2048','2049','2050','2051','2052','2053','2054','2055','2056','2057','2058','2059','2060','2061','2062','2063','2064','2065','2066','2067','2068','2069','2070','2071','2072','2073','2074','2075','2076','2077','2078','2079','2080','2081','2082','2083','2084','2085','2086','2087','2088','2089','2090','2091','2092','2093','2094','2095','2096','2097','2098','2099','2100']
  oyears = [str(y) for y in range(2015,2101)]     # [2015-2100]
  oyears_exp = [str(y) for y in range(2015,2086)] # [2015-2085]
  
  #text('t',"Automatically generated result tables.")
  
  selectedregions=['11000', '11041']
  
  for region in g.sortedregions: #selectedregions:#g.sortedregions:
    title('t','subsection',g.regions[region])

    title('t','subsubsection',"IPCC scenario a1b")
    printTable('constant',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'IPCC scenario a1b (average of all models) [avg. 2015-2100]','a1b_all_2015-2100_'+region)
    printTable('constant',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2100'],'IPCC scenario a1b (average of all models) [2100]','a1b_all_2100_'+region)
    printTable('constant',['arpege_a1b'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'arpege\_a1b [avg. 2015-2100]','arpege_a1b_2015-2100_'+region)
    printTable('constant',['arpege_a1b'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2100'],'arpege\_a1b [2100]','arpege_a1b_2100_'+region)
    
    #printTable('constant',['CNCM33_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'CNCM33 [avg. 2030-2085]','cncm33_2030-2085_'+region)
    #printTable('constant',['CNCM33_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'CNCM33 [2085]','cncm33_2085_'+region)
    #printTable('constant',['DMIEH5C_1','DMIEH5C_2','DMIEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'DMIEH5C [avg. 2030-2085]','dmieh5c_2030-2085_'+region)
    #printTable('constant',['DMIEH5C_1','DMIEH5C_2','DMIEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'DMIEH5C [2085]','dmieh5c_2085_'+region)
    #printTable('constant',['EGMAM2_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'EGMAM2 [avg. 2030-2085]','egmam2_2030-2085_'+region)
    #printTable('constant',['EGMAM2_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'EGMAM2 [2085]','egmam2_2085_'+region)
    #printTable('constant',['HADGEM2_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'HADGEM2 [avg. 2030-2085]','hadgem2_2030-2085_'+region)
    #printTable('constant',['HADGEM2_1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'HADGEM2 [2085]','hadgem2_2085_'+region)
    #printTable('constant',['IPCM4_1','IPCM4_2','IPCM4_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'IPCM4 [avg. 2030-2085]','ipcm4_2030-2085_'+region)
    #printTable('constant',['IPCM4_1','IPCM4_2','IPCM4_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'IPCM4 [2085]','ipcm4_2085_'+region)
    #printTable('constant',['MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'MPEH5C [avg. 2030-2085]','mpeh5c_2030-2085_'+region)
    #printTable('constant',['MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'MPEH5C [2085]','mpeh5c_2085_'+region)
    
    text('t',"\clearpage")
    
    title('t','subsubsection',"IPCC scenario a2")
    #printTable('constant',['arpege_a2'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'arpege\_a2 [avg. 2015-2100]','arpege_a2_2015-2100_'+region)
    #printTable('constant',['arpege_a2'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2100'],'arpege\_a2 [2100]','arpege_a2_2100_'+region)
    printTable('arpege_a1b',['arpege_a2'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'arpege a1b vs a2 [avg. 2015-2100]','arpege_a1b_a2_2015-2100_'+region)
    printTable('arpege_a1b',['arpege_a2'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2100'],'arpege a1b vs a2 [2100]','arpege_a1b_a2_2100_'+region)
    title('t','subsubsection',"IPCC scenario b1")    
    #printTable('constant',['arpege_b1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'arpege\_b1 [avg. 2015-2100]','arpege_b1_2015-2100_'+region)
    #printTable('constant',['arpege_b1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2100'],'arpege\_b1 [2100]','arpege_b1_2100_'+region)
    printTable('arpege_a1b',['arpege_b1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears,'arpege a1b vs b1 [avg. 2015-2100]','arpege_a1b_b1_2015-2100_'+region)
    printTable('arpege_a1b',['arpege_b1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2100'],'arpege a1b vs b1 [2100]','arpege_a1b_b1_2100_'+region)   
    #title('t','subsubsection',"Expectations")    
    #printTable('arpege_a1b',['arpege_a1b_noExpectations'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears_exp,'noExp [avg. 2015-2085]','noExp_2015-2085_'+region)
    #printTable('arpege_a1b',['arpege_a1b_noExpectations'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'noExp [2100]','noExp_2085_'+region)
    #printTable('arpege_a1b',['arpege_a1b_fullExpectations'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears_exp,'fullExp [avg. 2015-2085]','fullExp_2015-2085_'+region)
    #printTable('arpege_a1b',['arpege_a1b_fullExpectations'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'fullExp [2085]','fullExp_2085_'+region)   
    #printTable('arpege_a1b_noExpectations',['arpege_a1b_fullExpectations'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],oyears_exp,'noExp vs fullExp [avg. 2015-2085]','noVsfullExp_2015-2085_'+region)
    #printTable('arpege_a1b_noExpectations',['arpege_a1b_fullExpectations'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],[region],['2085'],'noExp vs fullExp [2085]','noVsfullExp_2085_'+region)   
    

    printTable('constant',['arpege_a1b','arpege_a2','arpege_b1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],['2100'],'Variations from constant [2100]','vars_2100_11000',False,)
    printTable('constant',['constant','arpege_a1b','arpege_a2','arpege_b1'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],['2100'],'Variations from actual values [2100 vs 2007]','vars_2100-2007_11000',False, 2007)
  
  text('t',"\clearpage")
  
  title('t','subsection',"Carbon table")  
  #printCarbonTable(ref_scenario, comparing_scenarios, region, year_start, year_end, title, filename, avg=False, singleComparation=True )
  printCarbonTable('constant',['arpege_a1b','arpege_a2','arpege_b1'],'11000', '2015', '2100', "\\ce{CO2} balance [yearly avg. 2015-2100]",'cbalance_2015-2100_11000', True, False)
  printCarbonTable('constant',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],'11000', '2015', '2100', "\\ce{CO2} balance a1b [yearly avg. 2015-2100]",'cbalance_a1b_2015-2100_11000', True, True)
  printCarbonTable('constant',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],'11000', '2007', '2100', "\\ce{CO2} balance a1b [yearly avg. 2007-2100]",'cbalance_a1b_2007-2100_11000', True, True)
  printCarbonTable('constant',['arpege_a1b','arpege_a2','arpege_b1'],'11000', '2015', '2100', "\\ce{CO2} balance arpege \{a1b, a2, b1\} [yearly avg. 2015-2100]",'cbalance_arpege_2015-2100_11000', True, True)
  #def printTable(ref_scenario, comparing_scenarios, variables_h, regions_h, years_h, title, filename, singleComparation=True, refYear=0):
  #title('t','subsection',"Active management effect")
  #printTable('arpege_a1b', ['arpege_a1b_natevol','arpege_a1b_active'], ['expReturns','regArea','harvestedArea','vReg'], ['11000'], oyears, 'Active mgm effect, flow vars [avg. 2015-2100]', 'mgm_effect_2015-2100_11000', False)
  #printTable('arpege_a1b', ['arpege_a1b_natevol','arpege_a1b_active'], ['vol','forArea'], ['11000'], ['2100'], 'Active mgm effect, stock vars [2100]', 'mgm_effect_2100_11000', False)
  #printCarbonTable('arpege_a1b',['arpege_a1b_natevol','arpege_a1b_active'],'11000', '2015', '2100', "Active management effect, \\ce{CO2} balance [yearly avg. 2015-2100]",'mgm_effect_cbalance_2015-2100_11000', True, False)
  
  #title('t','subsection',"Expectations effect") 
  #printTable('arpege_a1b', ['arpege_a1b_noExpectations','arpege_a1b_fullExpectations'], ['expReturns','regArea','harvestedArea','vReg'], ['11000'], oyears, 'Expectations effect, flow vars [avg. 2015-2100]', 'expectations_2015-2100_11000', False)
  #printTable('arpege_a1b', ['arpege_a1b_noExpectations','arpege_a1b_fullExpectations'], ['vol','forArea'], ['11000'], ['2100'], 'Expectations effect, stock vars [2100]', 'expectations_2100_11000', False)
  #printCarbonTable('arpege_a1b',['arpege_a1b_noExpectations','arpege_a1b_fullExpectations'],'11000', '2015', '2100', "Expectations effects, \\ce{CO2} balance [yearly avg. 2015-2100]",'expectations_cbalance_2015-2100_11000', True, False)
  #title('t','subsection',"Risk effect") 
  #printTable('arpege_a1b', ['arpege_a1b_rneutral','arpege_a1b_strongra'], ['expReturns','regArea','harvestedArea','vReg'], ['11000'], oyears, 'Risk effect, flow vars [avg. 2015-2100]', 'risk_2015-2100_11000', False)
  #printTable('arpege_a1b', ['arpege_a1b_rneutral','arpege_a1b_strongra'], ['vol','forArea'], ['11000'], ['2100'], 'Risk effect, stock vars [2100]', 'risk_2100_11000', False)
  #printCarbonTable('arpege_a1b',['arpege_a1b_rneutral','arpege_a1b_strongra'],'11000', '2015', '2100', "Risk effects, \\ce{CO2} balance [yearly avg. 2015-2100]",'risk_cbalance_2015-2100_11000', True, False)

  title('t','section', "Sa on price and physical effects")
  printTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],g.years,'SA [avg. 2007-2100]','sa_vars_2007-2100_11000',False)
  printCarbonTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],'11000', '2007', '2100', "Sensitivity analisys \\ce{CO2} balance [avg. 2007-2100]",'sa_cbalance_2007-2100_11000', True, False)
  printTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],oyears,'SA [avg. 2015-2100]','sa_vars_2015-2100_11000',False)
  printCarbonTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],'11000', '2015', '2100', "Sensitivity analisys \\ce{CO2} balance [yearly avg. 2015-2100]",'sa_cbalance_2015-2100_11000', True, False)
  printTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],['2100'],'SA [2100]','sa_vars_2100_11000',False)

  
# =============================================================================  
def printAATables():
  print "Printing area allocation tables.."

 
  #selectedregions=['11000', '11041', '11061', '11062', '11081', '11071', '11082', '11083']
  selectedregions=['11000', '11061', '11071','11082','11041']

  title('t','section',"Area allocation")
  printAATable(['constant', 'constant', 'constant'],['arpege_a1b','arpege_a2','arpege_b1'], selectedregions, ['2100'],'Area allocation [% variation over bau]', 'area_allocation_2100')
  printAATable(['constant', 'constant', 'constant', 'constant'],['constant', 'arpege_a1b','arpege_a2','arpege_b1'], selectedregions, ['2100'],'Area allocation [% variation over bau in 2007]', 'area_allocation_2100_vs_2007',2007)



# =============================================================================
# EXECUTION ACTUALLY STARTS HERE.....
main()
