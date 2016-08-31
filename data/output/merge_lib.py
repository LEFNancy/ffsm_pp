#!/usr/bin/env python
# -*- coding: utf-8 -*-

# =============================================================================
def merge(forIFiles_h=[],prdIFiles_h=[],carbonIFiles_h=[],scenarios_h=[],forOFilename_h="",prdOFilename_h="",carbonOFilename_h="", variables_h=[], regions_h=[], years_h=[]):
  print("*** Processing..")
  if len(forIFiles_h)>0:
   open(forOFilename_h,'w').close()
  if len(prdIFiles_h)>0:
    open(prdOFilename_h,'w').close()
  if len(carbonIFiles_h)>0:  
    open(carbonOFilename_h,'w').close()
  forCounter=0
  prdCounter=0
  carbonCounter=0
  for forIFile in forIFiles_h:
    merge_single_file(forIFile, forOFilename_h, scenarios_h, False if forCounter else True, variables_h, regions_h, years_h)
    forCounter += 1
  for prdIFile in prdIFiles_h:
    merge_single_file(prdIFile, prdOFilename_h, scenarios_h, False if prdCounter else True, variables_h, regions_h, years_h)
    prdCounter += 1
  for carbonIFile in carbonIFiles_h:
    merge_single_file(carbonIFile, carbonOFilename_h, scenarios_h, False if carbonCounter else True, variables_h, regions_h, years_h)
    carbonCounter += 1
  print ("*** Done!")
  

# =============================================================================
def determinePositions(headerRow):
  fields = headerRow.split(';')
  returnValues = [-1,-1,-1]
  for idx, field in enumerate(fields):
    if(field == 'parName' or field == 'balItem'): returnValues[0] = idx
    if(field == 'region'):                        returnValues[1] = idx
    if(field == 'year'):                          returnValues[2] = idx
  if (returnValues[0] == -1 or returnValues[1] == -1 or returnValues[2] == -1):
    print ("There has been an error reading the headers of a file.")
    exit(1)
  return returnValues
    
# =============================================================================
def merge_single_file(i_filename_h, o_filename_h, scenarios_h, keepHeader=False, variables_h=[], regions_h=[], years_h=[]):
  i_file = open(i_filename_h,'r') 
  o_file = open(o_filename_h,'a') 
  newRow     =  1
  counterRow =  0
  parNamePos = -1
  regionPos  = -1
  yearPos    = -1
  positions  = []
 
  while newRow:
    row = i_file.readline()
    scenarioFilter = False
    variableFilter = False
    regionFilter   = False
    yearFilter     = False
    finalFilter    = False
    
    if row == '':
      break
    if(counterRow == 0):
      positions = determinePositions(row)
      parNamePos = positions[0]
      regionPos  = positions[1]
      yearPos    = positions[2]
      if(keepHeader):
        o_file.write(row)     
    counterRow += 1  
    fields = row.split(';')
    rowScenario = fields[0]
    
    if(rowScenario in scenarios_h):
      scenarioFilter = True
    
    if( (len(variables_h) == 0 ) or (fields[parNamePos] in variables_h) ):
      variableFilter = True

    if( (len(regions_h) == 0) or (fields[regionPos] in regions_h) ):
      regionFilter = True

    if( (len(years_h) == 0) or (fields[yearPos] in years_h) ):
      yearFilter = True
      
    if (scenarioFilter and variableFilter and regionFilter and yearFilter):
      finalFilter = True
         
    if(finalFilter):
      o_file.write(row) 
  i_file.close()
  o_file.close()
 