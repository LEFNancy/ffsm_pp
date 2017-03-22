#!/bin/bash

# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs <n of jobs> -a runscenarios_foretcc.sh 
# and put something like this in the script

./run_single_scenario.sh 'data/ffsmInput_foretcc.ods' 'RCP00_BAS_EXT'
./run_single_scenario.sh 'data/ffsmInput_foretcc.ods' 'RCP00_BAS_ADAPT_BASE'
./run_single_scenario.sh 'data/ffsmInput_foretcc.ods' 'RCP00_BAS_ADAPT_ACTIVEMGM'
./run_single_scenario.sh 'data/ffsmInput_foretcc.ods' 'RCP00_BAS_ADAPT_SUBSIDES'
./run_single_scenario.sh 'data/ffsmInput_foretcc.ods' 'RCP00_BAS_ADAPT_STRUCTURALPOLICIES'
./run_single_scenario.sh 'data/ffsmInput_foretcc.ods' 'RCP00_BAS_ADAPT_ALLPOLICIES'
./run_single_scenario.sh 'data/ffsmInput_foretcc.ods' 'RCP00_BAS_ADAPT'




