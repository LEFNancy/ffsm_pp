#!/usr/bin/env python
# -*- coding: utf-8 -*-

from merge_lib import *

forIFiles = [
    'results/forestData.csv',
]
prdIFiles = [
    'results/productData.csv',
]
carbonIFiles = [
    'results/carbonBalance.csv',
]
scenarios = [
  'P0-Me-Ei',
  'P1-Me-Ei',
]

forOFilename = 'results/forestData_merged.csv'
prdOFilename = 'results/productData_merged.csv'
carbonOFilename = 'results/carbonBalance_merged.csv'

merge(forIFiles,prdIFiles,carbonIFiles,scenarios,forOFilename,prdOFilename,carbonOFilename)
