#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs <n of jobs> -a runscenarios_tiga.sh
# and put something like this in the script

./run_single_scenario.sh 'data/ffsmInput_streisand.ods' 'defaultS'

