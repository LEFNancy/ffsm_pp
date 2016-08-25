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
    'results/forestData_constant.csv',
    'results/forestData_constant_nomgm.csv',
    'results/forestData_constant_nora.csv',
    'results/forestData_constant_carea.csv',
    'results/forestData_arpege_a1b.csv',
    'results/forestData_arpege_a1b_carea.csv',
    'results/forestData_arpege_a1b_noexp.csv',
    'results/forestData_arpege_a1b_fullexp.csv',
    'results/forestData_arpege_a1b_nora.csv',
    'results/forestData_arpege_a1b_nomgm.csv',
    'results/forestData_arpege_a1b_nopr.csv',
    'results/forestData_arpege_a1b_noph.csv',
    'results/forestData_arpege_a2.csv',
    'results/forestData_arpege_b1.csv',
    'results/forestData_CNCM33_1.csv',
    'results/forestData_DMIEH5C_1.csv',
    'results/forestData_DMIEH5C_2.csv',
    'results/forestData_DMIEH5C_3.csv',
    'results/forestData_EGMAM2_3.csv',
    'results/forestData_HADGEM2_1.csv',
    'results/forestData_IPCM4_1.csv',
    'results/forestData_IPCM4_2.csv',
    'results/forestData_IPCM4_3.csv',
    'results/forestData_MPEH5C_1.csv',
    'results/forestData_MPEH5C_2.csv',
    'results/forestData_MPEH5C_3.csv',
  ]
  
  """
  g.prodIFiles = [  
    'results/productData_constant.csv',
    'results/productData_constant_nomgm.csv',
    'results/productData_constant_nora.csv',
    'results/productData_constant_carea.csv',
    'results/productData_arpege_a1b.csv',
    'results/productData_arpege_a1b_carea.csv',
    'results/productData_arpege_a1b_noexp.csv',
    'results/productData_arpege_a1b_fullexp.csv',
    'results/productData_arpege_a1b_nora.csv',
    'results/productData_arpege_a1b_nomgm.csv',
    'results/productData_arpege_a1b_nopr.csv',
    'results/productData_arpege_a1b_noph.csv',
    'results/productData_arpege_a2.csv',
    'results/productData_arpege_b1.csv',
    'results/productData_CNCM33_1.csv',
    'results/productData_DMIEH5C_1.csv',
    'results/productData_DMIEH5C_2.csv',
    'results/productData_DMIEH5C_3.csv',
    'results/productData_EGMAM2_3.csv',
    'results/productData_HADGEM2_1.csv',
    'results/productData_IPCM4_1.csv',
    'results/productData_IPCM4_2.csv',
    'results/productData_IPCM4_3.csv',
    'results/productData_MPEH5C_1.csv',
    'results/productData_MPEH5C_2.csv',
    'results/productData_MPEH5C_3.csv',
  ]
  """
  
  g.prodIFiles = []
 
  g.carbonIFiles = [
    'results/carbonBalance_constant.csv',
    'results/carbonBalance_constant_nomgm.csv',
    'results/carbonBalance_constant_nora.csv',
    'results/carbonBalance_constant_carea.csv',
    'results/carbonBalance_arpege_a1b.csv',
    'results/carbonBalance_arpege_a1b_carea.csv',
    'results/carbonBalance_arpege_a1b_noexp.csv',
    'results/carbonBalance_arpege_a1b_fullexp.csv',
    'results/carbonBalance_arpege_a1b_nora.csv',
    'results/carbonBalance_arpege_a1b_nomgm.csv',
    'results/carbonBalance_arpege_a1b_nopr.csv',
    'results/carbonBalance_arpege_a1b_noph.csv',
    'results/carbonBalance_arpege_a2.csv',
    'results/carbonBalance_arpege_b1.csv',
    'results/carbonBalance_CNCM33_1.csv',
    'results/carbonBalance_DMIEH5C_1.csv',
    'results/carbonBalance_DMIEH5C_2.csv',
    'results/carbonBalance_DMIEH5C_3.csv',
    'results/carbonBalance_EGMAM2_3.csv',
    'results/carbonBalance_HADGEM2_1.csv',
    'results/carbonBalance_IPCM4_1.csv',
    'results/carbonBalance_IPCM4_2.csv',
    'results/carbonBalance_IPCM4_3.csv',
    'results/carbonBalance_MPEH5C_1.csv',
    'results/carbonBalance_MPEH5C_2.csv',
    'results/carbonBalance_MPEH5C_3.csv',    
  ]

  g.scenarios = {
    'constant':            '#8c0000', # red-brown
    'constant_nomgm':      '#610e0e',
    'constant_nora':       '#610e0e',
    'constant_carea':      '#610e0e',   
    'arpege_a1b':          '#000080', #blue 
    'arpege_a1b_carea':    '#CF0E0E',
    'arpege_a1b_noexp':    '#D67044',
    'arpege_a1b_fullexp':  '#894A4A',
    'arpege_a1b_nora':     '#815E5E',
    'arpege_a1b_nomgm':    '#9C7831',
    'arpege_a1b_nopr':     '#CB9426',
    'arpege_a1b_noph':     '#F7C35A',
    'arpege_a2':           '#2329C1', # blue
    'arpege_b1':           '#0015db', # green
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
  g.years = [str(y) for y in range(2015,2101)]     # [2007-2100]
  g.printChartsFlag = True
  g.printTablesFlag = True
  g.printAATablesFlag = True
  g.chartoutdir = 'charts'
  g.tableoutdir = 'tables'
  # key: var short name
  # value: turple with long name, unit and optionally variable to act for ponderation and name of aggregated variable
  g.forVars = {'hV': ['Harvested volumes', r"$Mm^3$"],
       'vReg': ['Regeneration volumes', r"$Mm^3$"],
       'vol': ['Forest volumes', r"$Mm^3$"],
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
  print ("Printing charts..")
  
  #text('c',"Automatically generated charts.")  
  #plotLegend(['nonspatial','reference','vRegEnd070','vRegFixed','vRegFromHr','withRisk06','withRisk08','withRisk10','withVariance','withSpVarianceAndRisk'],'Scenarios Legend','legend')
  
  selectedregions=['11000', '11041']
  
  for region in selectedregions: #selectedregions:#g.sortedregions:
    title('c','subsection',g.regions[region])

    title('c', 'subsubsection', "a1b vs constant")
    plotMultivariable(['constant', 'arpege_a1b'], ['expReturns', 'vol', 'hV', 'forArea'], region, 'arpege\_a1b vs constant', 'a1b')

    title('c','subsubsection',"IPCC scenarios (using arpege)")
    plotMultivariable(['constant','arpege_a1b', 'arpege_a2', 'arpege_b1'],['expReturns','vol','hV','forArea'],region,'IPCC Scenarios','ipcc_scenarios')

    title('c', 'subsubsection', "Components of climate change")
    plotMultivariable(['constant', 'arpege_a1b','arpege_a1b_nopr','arpege_a1b_noph'], ['expReturns', 'vol', 'hV', 'forArea'],region, 'Components of cc', 'cc_components')

    title('c', 'subsubsection', "Characteristics of the system that interact with climate change")
    plotMultivariable(['arpege_a1b', 'arpege_a1b_nomgm','arpege_a1b_nora', 'arpege_a1b_noexp', 'arpege_a1b_fullexp', 'arpege_a1b_carea'], ['expReturns', 'vol', 'hV', 'forArea'], region, 'Characteristics of the system', 'components')

    title('c', 'subsubsection', "Just volume and area")
    plotMultivariable(['arpege_a1b'], ['vol', 'forArea'], region, 'arpege\_a1b', 'vol_area1', True, fwidth=10, fheight=8)
    plotMultivariable(['arpege_a1b'], ['vol'], region, 'arpege\_a1b', 'vol1', True, fwidth=10, fheight=6)
    plotMultivariable(['arpege_a1b'], ['vol'], region, 'arpege\_a1b', 'vol1_nolegend', False, fwidth=10, fheight=4)
    plotLegend(['arpege_a1b'], 'vol1_legend')
    plotMultivariable(['constant', 'arpege_a1b'], ['vol', 'forArea'], region, 'constant vs arpege\_a1b', 'vol_area2', True, fwidth=10, fheight=8)
    plotMultivariable(['constant', 'arpege_a1b'], ['vol'], region, 'constant vs arpege\_a1b', 'vol2', True, fwidth=10, fheight=8)
    
    #title('c','subsubsection',"IPCC scenario a1b")
    #plotMultivariable(['constant','arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea'],region,'All a1b scenarios','a1b',False)
    #plotMultivariable(['constant','arpege_a1b'],['expReturns','vReg','vol','hV','forArea'],region,'Arpege a1b','arpege')
    #plotMultivariable(['constant','CNCM33_1'],['expReturns','vReg','vol','hV','forArea'],region,'CNCM33','cncm33')
    #plotMultivariable(['constant','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3'],['expReturns','vReg','vol','hV','forArea'],region,'DMIEH5C','dmieh5c')    
    #plotMultivariable(['constant','EGMAM2_3'],['expReturns','vReg','vol','hV','forArea'],region,'EGMAM2','egmam2') 
    #plotMultivariable(['constant','HADGEM2_1'],['expReturns','vReg','vol','hV','forArea'],region,'HADGEM2','hadgem2') 
    #plotMultivariable(['constant','IPCM4_1','IPCM4_2','IPCM4_3'],['expReturns','vReg','vol','hV','forArea'],region,'IPCM4','ipcm4') 
    #plotMultivariable(['constant','MPEH5C_1','MPEH5C_2','MPEH5C_3'],['expReturns','vReg','vol','hV','forArea'],region,'MPEH5C','mpeh5c') 
  
  title('c','subsection', "Carbon charts")

  plotCarbonChart(['arpege_a1b'],'11000','','carb_arpege_a1b')
  plotCarbonChart(['constant', 'arpege_a1b'],'11000','','carb_constant_and_arpege_a1b')
  plotCarbonChart(['constant', 'arpege_a1b', 'arpege_a2', 'arpege_b1'], '11000', '', 'carb_balance')
  plotCarbonChart(['arpege_a1b','arpege_a1b_nopr','arpege_a1b_noph'], '11000', '', 'carb_cc_components')
  plotCarbonChart(['arpege_a1b', 'arpege_a1b_nomgm','arpege_a1b_nora', 'arpege_a1b_noexp', 'arpege_a1b_fullexp', 'arpege_a1b_carea'], '11000', '', 'carb_system_components')

# =============================================================================  
def printTables():
  print ("Printing tables blaa..")

  oyears = [str(y) for y in range(2015,2101)]     # [2015-2100]
   

  selectedregions=['11000', '11041']
  
  outvars = ['expReturns','harvestedArea','hV','regArea','vReg','forArea','vol']
  
  for region in selectedregions: #selectedregions:#g.sortedregions:
    title('t','subsection',g.regions[region])


    
    title('t','subsubsection',"Variation from today")
    printTable('constant',['arpege_a1b'],outvars,[region],['2015'],'Effects of CC (a1b) already visible today (2015)','cc_effect_2015_'+region,False)
    printTable('arpege_a1b',['constant', 'arpege_a1b'],outvars,[region],['2100'],'Overall evolution of the forest [2015-2100] (2105 is a1b)','vars2_2100_2015_'+region, False, 2015)
    printTable('constant',['constant', 'arpege_a1b'],outvars,[region],['2100'],'Future forest relavie to current forest [2100 vs 2015] (2015 is constant)','vars_2100_2015_'+region, False, 2015)
    printTable('arpege_a1b',['arpege_a1b', 'CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],outvars,[region],['2100'],'Future forest relavie to current forest [2100 vs 2015] (2100 is all a1b scenarios, 2015 is arpege\_a1b)','vars_all_a1b_2100_2015'+region, True, 2015)

    title('t','subsubsection',"Components of the change")
    printTable('constant',['arpege_a1b','arpege_a1b_nopr','arpege_a1b_noph'],outvars,[region],oyears,'CC effects by components [avg. 2015-2100]','cc_components_2015-2100_'+region)
    printTable('constant',['arpege_a1b','arpege_a1b_nopr','arpege_a1b_noph'],outvars,[region],['2100'],'CC effects by components  [2100]','cc_components_2100_'+region)

    title('t','subsubsection',"Characteristics of the system that interact with the change")
    printTable('arpege_a1b',['arpege_a1b_nomgm','arpege_a1b_nora', 'arpege_a1b_noexp', 'arpege_a1b_fullexp', 'arpege_a1b_carea'],outvars,[region],oyears,'Elements that interact with the change [avg. 2015-2100]','sa_2015-2100_'+region,False)
    printTable('arpege_a1b',['arpege_a1b_nomgm','arpege_a1b_nora', 'arpege_a1b_noexp', 'arpege_a1b_fullexp', 'arpege_a1b_carea'],outvars,[region],['2100'],'Elements that interact with the change [2100]','sa_2100_'+region,False)
  
    title('t','subsubsection',"Expected returns today")
    printTable('constant',['arpege_a1b','arpege_a1b_nopr','arpege_a1b_noph','arpege_a1b_noexp', 'arpege_a1b_fullexp'],['expReturns'],[region],['2015'],'Effect of different assumptions on today [2015] forest expected returns','expectedReturns_2015_'+region,False)
    
    title('t','subsubsection',"Uncertainly relative to the IPCC scenario")
    printTable('arpege_a1b',['arpege_a2','arpege_b1'],outvars,[region],oyears,'Uncertainly from IPCC scenario [avg. 2015-2100]','sa_ipcc_scenarios_2015-2100_'+region,False)
    printTable('arpege_a1b',['arpege_a2','arpege_b1'],outvars,[region],['2100'],'Uncertainly from IPCC scenario [2100]','sa_ipcc_scenarios_2100_'+region,False)
    printTable('constant',['arpege_a1b','arpege_a2','arpege_b1'],outvars,[region],oyears,'Uncertainly from IPCC scenario [avg. 2015-2100]','sa_ipcc_scenarios_cv_2015-2100_'+region,True)
    printTable('constant',['arpege_a1b','arpege_a2','arpege_b1'],outvars,[region],['2100'],'Uncertainly from IPCC scenario [2100]','sa_ipcc_scenarios_cv_2100_'+region,True)
    
    title('t','subsubsection',"Uncertainly relative to the climatic model")
    printTable('arpege_a1b',['CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],outvars,[region],oyears,'Uncertainly from climatic model [avg. 2015-2100]','sa_climatic_model_2015-2100_'+region,False)
    printTable('arpege_a1b',['CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],outvars,[region],['2100'],'Uncertainly from climatic model [2100]','sa_climatic_model_2100_'+region,False)
    printTable('constant',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],outvars,[region],oyears,'Uncertainly from climatic model [avg. 2015-2100]','sa_climatic_model_cv_2015-2100_'+region,True)
    printTable('constant',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],outvars,[region],['2100'],'Uncertainly from climatic model [2100]','sa_climatic_model_cv_2100_'+region,True)
    


  text('t',"\clearpage")
  
  title('t','subsection',"Carbon tables")
  #printCarbonTable(ref_scenario, comparing_scenarios, region, year_start, year_end, title, filename, avg=False, singleComparation=True )
  printCarbonTable('constant',['arpege_a1b'],'11000', '2015', '2100', "\\ce{CO2} balance [yearly avg. 2015-2100]",'cbalance_2015-2100_11000', True, False)
  printCarbonTable('constant', ['arpege_a1b','arpege_a1b_nopr','arpege_a1b_noph'], '11000', '2015', '2100', "\\ce{CO2} balance [yearly avg. 2015-2100]",'cbalance_component_changes_2015-2100_11000', True, False)
  printCarbonTable('arpege_a1b', ['arpege_a1b_nomgm','arpege_a1b_nora', 'arpege_a1b_noexp', 'arpege_a1b_fullexp', 'arpege_a1b_carea'], '11000', '2015', '2100', "\\ce{CO2} balance [yearly avg. 2015-2100]",'cbalance_characteristics_system_2015-2100_11000', True, False)
  printCarbonTable('constant',['arpege_a1b','arpege_a2','arpege_b1'],'11000', '2015', '2100', "\\ce{CO2} balance for different IPCC storylines [yearly avg. 2015-2100]",'cbalance_sa_ipcc_2015-2100_11000', True, True)
  printCarbonTable('constant',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],'11000', '2015', '2100', "\\ce{CO2} balance for different climatic models [a1b, yearly avg. 2015-2100]",'cbalance_sa_a1b_models_2015-2100_11000', True, True)
 # printCarbonTable('constant',['arpege_a1b','CNCM33_1','DMIEH5C_1','DMIEH5C_2','DMIEH5C_3','EGMAM2_3','HADGEM2_1','IPCM4_1','IPCM4_2','IPCM4_3','MPEH5C_1','MPEH5C_2','MPEH5C_3'],'11000', '2007', '2100', "\\ce{CO2} balance a1b [yearly avg. 2007-2100]",'cbalance_a1b_2007-2100_11000', True, True)
 # printCarbonTable('constant',['arpege_a1b','arpege_a2','arpege_b1'],'11000', '2015', '2100', "\\ce{CO2} balance arpege \{a1b, a2, b1\} [yearly avg. 2015-2100]",'cbalance_arpege_2015-2100_11000', True, True)
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

  #title('t','section', "Sa on price and physical effects")
  #printTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],g.years,'SA [avg. 2007-2100]','sa_vars_2007-2100_11000',False)
  #printCarbonTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],'11000', '2007', '2100', "Sensitivity analisys \\ce{CO2} balance [avg. 2007-2100]",'sa_cbalance_2007-2100_11000', True, False)
  #printTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],oyears,'SA [avg. 2015-2100]','sa_vars_2015-2100_11000',False)
  #printCarbonTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],'11000', '2015', '2100', "Sensitivity analisys \\ce{CO2} balance [yearly avg. 2015-2100]",'sa_cbalance_2015-2100_11000', True, False)
  #printTable('constant',['arpege_a1b','arpege_a1b_priceEffectOnly','arpege_a1b_phImpactOnly'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],['2100'],'SA [2100]','sa_vars_2100_11000',False)


  text('t', "\clearpage")

  title('t', 'subsection', "Expectations problem")
  oyears_exp = [str(y) for y in range(2015,2101)]
  startyears = [str(y) for y in range(2015,2025)]
  printTable('arpege_a1b', ['arpege_a1b_noexp'], ['expReturns', 'vReg', 'vol', 'hV', 'forArea', 'regArea', 'harvestedArea'], ['11000'], oyears_exp, 'Noexp effect [avg 2015-2100]', 'noexp_2015-2100_11000', True)
  printTable('arpege_a1b',['arpege_a1b_fullexp'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],oyears_exp,'Fullexp effect [avg 2015-2100]','fullexp_2015-2100_11000',True)
  printTable('arpege_a1b', ['arpege_a1b_noexp'],['expReturns', 'vReg', 'vol', 'hV', 'forArea', 'regArea', 'harvestedArea'], ['11000'], startyears, 'Noexp effect [avg 2015-2025]', 'noexp_2015-2025_11000', True)
  printTable('arpege_a1b',['arpege_a1b_fullexp'],['expReturns','vReg','vol','hV','forArea','regArea','harvestedArea'],['11000'],startyears,'Fullexp effect [avg 2015-2025]','fullexp_2015-2025_11000',True)


# =============================================================================
def printAATables():
  print ("Printing area allocation tables..")

 
  #selectedregions=['11000', '11041', '11061', '11062', '11081', '11071', '11082', '11083']
  selectedregions=['11000', '11024', '11061', '11071','11082','11041']
  regions = selectedregions #selectedregions:#g.sortedregions:

  title('t','section',"Area allocation")
  printAATable(['constant', 'constant', 'constant'],['arpege_a1b','arpege_a2','arpege_b1'], regions, ['2100'],'Area allocation [% variation over constant]', 'area_allocation_2100')
  printAATable(['constant', 'constant', 'constant', 'constant'],['constant', 'arpege_a1b','arpege_a2','arpege_b1'], regions, ['2100'],'Area allocation [% variation over constant in 2015]', 'area_allocation_2100_vs_2015',2015)


  printAATable(['constant', 'constant', 'constant',], ['arpege_a1b','arpege_a1b_nopr','arpege_a1b_noph'],regions, ['2100'], 'Area allocation [% variation over constant in 2015]', 'area_allocation_components_cc_2100_vs_2015',2015)
  printAATable(['constant', 'constant', 'constant','constant','constant','constant'], ['arpege_a1b','arpege_a1b_nomgm','arpege_a1b_nora','arpege_a1b_noexp','arpege_a1b_fullexp','arpege_a1b_carea'],regions, ['2100'], 'Area allocation [% variation over constant in 2015]', 'area_allocation_characteristics_system_2100_vs_2015',2015)

# =============================================================================
# EXECUTION ACTUALLY STARTS HERE.....
main()
