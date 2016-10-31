#!/usr/bin/env python
# -*- coding: utf-8 -*-

from merge_lib import *

forIFiles = [
    'results/forestData_baseline_temp.csv',
]
prdIFiles = [
    'results/productData_baseline_temp.csv',
]
carbonIFiles = [
    'results/carbonBalance_baseline_temp.csv',
]
scenarios = [
  'baseline_temp',
]

forOFilename = 'results/forestData_merged.csv'
prdOFilename = 'results/productData_merged.csv'
carbonOFilename = 'results/carbonBalance_merged.csv'

merge(forIFiles,prdIFiles,carbonIFiles,scenarios,forOFilename,prdOFilename,carbonOFilename)
