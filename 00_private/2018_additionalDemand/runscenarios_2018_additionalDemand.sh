#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a this_file.sh 


./run_single_scenario.sh 'data/ffsmInput_additionalDemand.ods' 'default'
./run_single_scenario.sh 'data/ffsmInput_additionalDemand.ods' 'additionalDemand'














