#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
<<<<<<< HEAD
# parallel --jobs 10 -a runscenarios_hartman.sh 
# and put something like this in the script
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default_y'

./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default_pixel5'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low_pixel5'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_pixel5'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high_pixel5'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_timber_pixel5'
=======
# parallel --jobs 7 -a runscenarios_hartman.sh 
# and put something like this in the script

#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default_y'
>>>>>>> 93d69c8... Modified the Hartman framework. There was a problem which led owners to chose dc=0cm sometimes.... Has been fixed. Some more cleaning up to do before merging again.

#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low_new'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low_new_y'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_new'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_new_y'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high_new'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high_new_y'

#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_new_pixel1'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_new_pixel2'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_new_pixel3'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_new_pixel4'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'med_new_pixel5'

#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low_new_pixel1'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low_new_pixel2'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low_new_pixel3'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low_new_pixel4'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'low_new_pixel5'

./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high_new_pixel1'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high_new_pixel2'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high_new_pixel3'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high_new_pixel4'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'high_new_pixel5'