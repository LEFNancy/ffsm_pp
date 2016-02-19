#!/usr/bin/env python
# -*- coding: utf-8 -*-

from merge_lib import *

forIFiles = [
    #'results/forestData_debug_mi.csv',
    #'results/forestData_debug_me.csv',
    'results/forestData_P0-Mh-Ei.csv',
    'results/forestData_P1-Mh-Ei.csv',
    'results/forestData_P0-Mi-Ei.csv',
    'results/forestData_P1-Mi-Ei.csv',
    'results/forestData_P0-Me-Ei.csv',
    'results/forestData_P1-Me-Ei.csv',
    'results/forestData_P0-Mi-E0.csv',
    'results/forestData_P1-Mi-E0.csv',
    'results/forestData_P0-Mi-E1.csv',
    'results/forestData_P1-Mi-E1.csv',   
]
prdIFiles = [
    #'results/productData_debug_mi.csv',
    #'results/productData_debug_me.csv',
    'results/productData_P0-Mh-Ei.csv',
    'results/productData_P1-Mh-Ei.csv',
    'results/productData_P0-Mi-Ei.csv',
    'results/productData_P1-Mi-Ei.csv',
    'results/productData_P0-Me-Ei.csv',
    'results/productData_P1-Me-Ei.csv',
    'results/productData_P0-Mi-E0.csv',
    'results/productData_P1-Mi-E0.csv',
    'results/productData_P0-Mi-E1.csv',
    'results/productData_P1-Mi-E1.csv',
]
carbonIFiles = [
    #'results/carbonBalance_debug_mi.csv',
    #'results/carbonBalance_debug_me.csv',
    'results/carbonBalance_P0-Mh-Ei.csv',
    'results/carbonBalance_P1-Mh-Ei.csv',
    'results/carbonBalance_P0-Mi-Ei.csv',
    'results/carbonBalance_P1-Mi-Ei.csv',
    'results/carbonBalance_P0-Me-Ei.csv',
    'results/carbonBalance_P1-Me-Ei.csv',
    'results/carbonBalance_P0-Mi-E0.csv',
    'results/carbonBalance_P1-Mi-E0.csv',
    'results/carbonBalance_P0-Mi-E1.csv',
    'results/carbonBalance_P1-Mi-E1.csv',
]
scenarios = [
  #'debug_mi',
  #'debug_me',
  #'P0-Mh-Ei',
  'P1-Mh-Ei',
  #'P0-Mi-Ei',
  #'P1-Mi-Ei',
  #'P0-Me-Ei',
  'P1-Me-Ei',
  #'P0-Mi-E0',
  #'P1-Mi-E0',
  #'P0-Mi-E1',
  #'P1-Mi-E1',
]

forOFilename = 'results/forestData_merged.csv'
prdOFilename = 'results/productData_merged.csv'
carbonOFilename = 'results/carbonBalance_merged.csv'

merge(forIFiles,prdIFiles,carbonIFiles,scenarios,forOFilename,prdOFilename,carbonOFilename)
