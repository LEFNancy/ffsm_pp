#!/usr/bin/env python
# -*- coding: utf-8 -*-

from merge_lib import *

forIFiles = [
    #'results/forestData_constant.csv',
    #'results/forestData_arpege_a1b.csv',
    #'results/forestData_arpege_a2.csv',
    #'results/forestData_arpege_b1.csv',
    'results/forestData_constant_carea.csv',
    'results/forestData_arpege_a1b_carea.csv',
    'results/forestData_arpege_a2_carea.csv',
    'results/forestData_arpege_b1_carea.csv',    
    #'results/forestData_CNCM33_1.csv',
    #'results/forestData_DMIEH5C_1.csv',
    #'results/forestData_DMIEH5C_2.csv',
    #'results/forestData_DMIEH5C_3.csv',
    #'results/forestData_EGMAM2_3.csv',
    #'results/forestData_HADGEM2_1.csv',
    #'results/forestData_IPCM4_1.csv',
    #'results/forestData_IPCM4_2.csv',
    #'results/forestData_IPCM4_3.csv',
    #'results/forestData_MPEH5C_1.csv',
    #'results/forestData_MPEH5C_2.csv',
    #'results/forestData_MPEH5C_3.csv',
    #'results/forestData_arpege_a1b_priceEffectOnly.csv',
    #'results/forestData_arpege_a1b_phImpactOnly.csv', 
]
prdIFiles = [
    #'results/productData_constant.csv',
    #'results/productData_arpege_a1b.csv',
    #'results/productData_arpege_a2.csv',
    #'results/productData_arpege_b1.csv',
    #'results/productData_CNCM33_1.csv',
    #'results/productData_DMIEH5C_1.csv',
    #'results/productData_DMIEH5C_2.csv',
    #'results/productData_DMIEH5C_3.csv',
    #'results/productData_EGMAM2_3.csv',
    #'results/productData_HADGEM2_1.csv',
    #'results/productData_IPCM4_1.csv',
    #'results/productData_IPCM4_2.csv',
    #'results/productData_IPCM4_3.csv',
    #'results/productData_MPEH5C_1.csv',
    #'results/productData_MPEH5C_2.csv',
    #'results/productData_MPEH5C_3.csv',
    #'results/productData_arpege_a1b_priceEffectOnly.csv',
    #'results/productData_arpege_a1b_phImpactOnly.csv',
]
carbonIFiles = [
    #'results/carbonBalance_constant.csv',
    #'results/carbonBalance_arpege_a1b.csv',
    #'results/carbonBalance_arpege_a2.csv',
    #'results/carbonBalance_arpege_b1.csv',
    #'results/carbonBalance_CNCM33_1.csv',
    #'results/carbonBalance_DMIEH5C_1.csv',
    #'results/carbonBalance_DMIEH5C_2.csv',
    #'results/carbonBalance_DMIEH5C_3.csv',
    #'results/carbonBalance_EGMAM2_3.csv',
    #'results/carbonBalance_HADGEM2_1.csv',
    #'results/carbonBalance_IPCM4_1.csv',
    #'results/carbonBalance_IPCM4_2.csv',
    #'results/carbonBalance_IPCM4_3.csv',
    #'results/carbonBalance_MPEH5C_1.csv',
    #'results/carbonBalance_MPEH5C_2.csv',
    #'results/carbonBalance_MPEH5C_3.csv',
    #'results/carbonBalance_arpege_a1b_priceEffectOnly.csv',
    #'results/carbonBalance_arpege_a1b_phImpactOnly.csv',
]

scenarios = [
  #'constant',
  #'arpege_a1b',
  #'arpege_a2',
  #'arpege_b1',
  'constant_carea',
  'arpege_a1b_carea',
  'arpege_a2_carea',
  'arpege_b1_carea',
  #'CNCM33_1',
  #'DMIEH5C_1',
  #'DMIEH5C_2',
  #'DMIEH5C_3',
  #'EGMAM2_3',
  #'HADGEM2_1',
  #'IPCM4_1',
  #'IPCM4_2',
  #'IPCM4_3',
  #'MPEH5C_1',
  #'MPEH5C_2',
  #'MPEH5C_3',
  #'arpege_a1b_priceEffectOnly',
  #'arpege_a1b_phImpactOnly',
]

forOFilename = 'results/forestExpectedReturns_carea.csv'
#prdOFilename = 'results/productData_merged.csv'
#carbonOFilename = 'results/carbonBalance_merged.csv'

#merge(forIFiles,prdIFiles,carbonIFiles,scenarios,forOFilename,prdOFilename,carbonOFilename)
merge(forIFiles_h=forIFiles,prdIFiles_h=[],carbonIFiles_h=[],scenarios_h=scenarios,forOFilename_h=forOFilename,prdOFilename_h="",carbonOFilename_h="", variables_h=['expReturns','forArea'], regions_h=[], years_h=[],)
