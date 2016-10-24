#!/usr/bin/env python
# -*- coding: utf-8 -*-

from merge_lib import *

forIFiles = [
    'results/forestData_changed.csv',
    'results/forestData_changed2.csv',
]
prdIFiles = [
    'results/productData_changed.csv',
    'results/productData_changed2.csv',
]
carbonIFiles = [
    'results/carbonBalance_changed.csv',
    'results/carbonBalance_changed2.csv',
]
scenarios = [
  'changed',
  'changed2',
]

forOFilename = 'results/forestData_merged.csv'
prdOFilename = 'results/productData_merged.csv'
carbonOFilename = 'results/carbonBalance_merged.csv'

merge(forIFiles,prdIFiles,carbonIFiles,scenarios,forOFilename,prdOFilename,carbonOFilename)
