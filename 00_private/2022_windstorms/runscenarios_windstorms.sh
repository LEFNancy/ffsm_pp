#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs <n of jobs> -a runscenarios_windstorms.sh
# and put something like this in the script

./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'default'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp1'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp2'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp3'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp4'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp5'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp6'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp7'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp8'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp9'
./run_single_scenario.sh 'data/ffsmInput_windstorms.ods' 'windstorms_sp10'
