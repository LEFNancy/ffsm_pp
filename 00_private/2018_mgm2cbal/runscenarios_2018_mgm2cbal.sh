#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a runscenarios_2018_mgm2cbl.sh 

# Oracle scenarios
  ./run_single_scenario.sh 'data/ffsmInput_2018_mgm2cbal.ods' 'default'
  ./run_single_scenario.sh 'data/ffsmInput_2018_mgm2cbal.ods' 'raHvarL'
  ./run_single_scenario.sh 'data/ffsmInput_2018_mgm2cbal.ods' 'raLvarL'
  ./run_single_scenario.sh 'data/ffsmInput_2018_mgm2cbal.ods' 'expPrHvarL'
  ./run_single_scenario.sh 'data/ffsmInput_2018_mgm2cbal.ods' 'expPrLvarL'
  ./run_single_scenario.sh 'data/ffsmInput_2018_mgm2cbal.ods' 'expCCHvarL'
  ./run_single_scenario.sh 'data/ffsmInput_2018_mgm2cbal.ods' 'expCCLvarL'
 






   
#last run:
# 20180628, testing debugging scenarios for calibration
