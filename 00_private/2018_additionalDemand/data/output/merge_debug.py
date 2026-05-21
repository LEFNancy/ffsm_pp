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
#scenarios = [
#  'default',
#  'eta2005',
#  'eta2007',
#  'psi2005',
#  'psi2007',
#]

scenarios = [
#  'default',
#  'default_nocfunctions',
#  'default_no05',
#  'default_no05_nocfunctions',
  'default05',
  'default03',
  'default01',
#  'pol_all_withSubsInObj',
#  'pol_mktStr_s_08',
#  'pol_mktStr_s_06',
#  'pol_mktStr_d_08',
#  'pol_mktStr_d_06',

]



forOFilename = 'results/forestData_merged.csv'
prdOFilename = 'results/productData_merged.csv'
carbonOFilename = 'results/carbonBalance_merged.csv'
years = [str(y) for y in range(2005,2013)]     # [2005-2015]

merge(forIFiles,prdIFiles,carbonIFiles,scenarios,forOFilename,prdOFilename,carbonOFilename, years_h=years)



