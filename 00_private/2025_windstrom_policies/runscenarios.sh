#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------



# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs <n of jobs> -a runscenarios.sh 
# and put something like this in the script

./run_single_scenario.sh 'data/ffsmInput.ods' 'default'
./run_single_scenario.sh 'data/ffsmInput.ods' 'Windstorm_Ref'
./run_single_scenario.sh 'data/ffsmInput.ods' 'Sto_L'
./run_single_scenario.sh 'data/ffsmInput.ods' 'Sto_M'
./run_single_scenario.sh 'data/ffsmInput.ods' 'Sto_H'
./run_single_scenario.sh 'data/ffsmInput.ods' 'tcSub_L'
./run_single_scenario.sh 'data/ffsmInput.ods' 'tcSub_M'
./run_single_scenario.sh 'data/ffsmInput.ods' 'tcSub_H'

