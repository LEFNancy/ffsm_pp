#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a this_file.sh 

# 2015 workshop du lef paper scenarios
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'withAshPath'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'withoutAshPath'













