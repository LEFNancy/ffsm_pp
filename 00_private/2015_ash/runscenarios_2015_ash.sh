#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a this_file.sh 

./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Mh-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Mh-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Mi-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Mi-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Me-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Me-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Mi-E0'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Mi-E0'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Mi-E1'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Mi-E1'

# runs of 6 april 2016 are just to print the sigma variable. Previous run: 24 mars












