#!/usr/bin/env python
# -*- coding: utf-8 -*-

from merge_lib import *

forIFiles = [
    'results/forestData_default.csv',
]
prdIFiles = [
    'results/productData_default.csv',
]
carbonIFiles = [
    'results/carbonBalance_default.csv',
]
scenarios = [
  'default',
]

forOFilename = 'results/forestData_merged.csv'
prdOFilename = 'results/productData_merged.csv'
carbonOFilename = 'results/carbonBalance_merged.csv'

merge(forIFiles,prdIFiles,carbonIFiles,scenarios,forOFilename,prdOFilename,carbonOFilename)
