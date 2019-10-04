#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs 2 -a runscenarios_hartman.sh 
# and put something like this in the script
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default_y'
'



#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low15_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high15_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low2_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high2_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'above2_sh'



#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low15'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high15'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low2'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high2'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'above2'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low15_y'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high15_y'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_y'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low2_y'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high2_y'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'above2_y'



