#!/bin/bash

# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs <n of jobs> -a runscenarios_foretcc.sh 
# and put something like this in the script

./run_single_scenario.sh 'data/ffsmInput_2016_foretcc.ods' 'passive_management_nocc'
./run_single_scenario.sh 'data/ffsmInput_2016_foretcc.ods' 'active_management_nocc'





