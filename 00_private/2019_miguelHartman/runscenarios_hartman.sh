#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs 2 -a runscenarios_hartman.sh 
# and put something like this in the script

#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'default'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'Carbon_low'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'Carbon_med'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'Carbon_high'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'Carbon_const31'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'Carbon_const15'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'hartman_BAU_low'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'hartman_BAU_low_pickling'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'hartman_BAU_med'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'hartman_BAU_const31'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'hartman_BAU_const15'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'hartman_BAU_high'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'hartman_t0'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel.ods' 'hartman_noRef'



