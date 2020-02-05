#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs 10 -a runscenarios_hartman.sh 
# and put something like this in the script
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default_y'

./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default_pixel5'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low_pixel5'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_pixel5'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high_pixel5'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_timber_pixel5'

