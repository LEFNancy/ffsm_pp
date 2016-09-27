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

# Sa on market homogeneity
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Me0001-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Me001-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Me01-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Me05-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Me25-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P0-Me5-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Me0001-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Me001-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Me01-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Me05-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Me25-Ei'
./run_single_scenario.sh 'data/ffsmInput_2015_ash.ods' 'P1-Me5-Ei'

# runs of 12 September 2016: new scenarios, different expectedReturns for aggregated ft, new totalShareInvadedArea output var.
# runs of 6 april 2016 are just to print the sigma variable. Previous run: 24 mars












