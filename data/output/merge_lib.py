#!/usr/bin/env python
# -*- coding: utf-8 -*-

# =============================================================================
def merge(forIFiles,prdIFiles,carbonIFiles,scenarios,forOFilename,prdOFilename,carbonOFilename):
  print("*** Processing..")
  open(forOFilename,'w').close()
  open(prdOFilename,'w').close()
  open(carbonOFilename,'w').close()
  forCounter=0
  prdCounter=0
  carbonCounter=0
  for forIFile in forIFiles:
    merge_single_file(forIFile, forOFilename, scenarios, False if forCounter else True )
    forCounter += 1
  for prdIFile in prdIFiles:
    merge_single_file(prdIFile, prdOFilename, scenarios, False if prdCounter else True )
    prdCounter += 1
  for carbonIFile in carbonIFiles:
    merge_single_file(carbonIFile, carbonOFilename, scenarios, False if carbonCounter else True )
    carbonCounter += 1
  print ("*** Done!")
  


# =============================================================================
def merge_single_file(i_filename, o_filename, scenarios_h, keepHeader=False):
  i_file = open(i_filename,'r') 
  o_file = open(o_filename,'a') 
  newRow=1;
  counterRow = 0
  while newRow:
    row = i_file.readline()
    if row == '':
      break
    if(counterRow==0 and keepHeader):
      o_file.write(row)     
    counterRow += 1  
    fields = row.split(';')
    rowScenario = fields[0]
    if(rowScenario in scenarios_h):
      o_file.write(row) 
  i_file.close()
  o_file.close()
 