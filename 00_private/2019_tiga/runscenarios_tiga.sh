#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs <n of jobs> -a runscenarios_tiga.sh
# and put something like this in the script

./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'defaultTiga'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill1_noSpatial'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill1_diffused'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill1_concentrated'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill2_noSpatial'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill2_diffused'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill2_concentrated'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill3_noSpatial'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill3_diffused'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill3_concentrated'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill4_noSpatial'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill4_diffused'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill4_concentrated'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill5_noSpatial'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill5_diffused'
./run_single_scenario.sh 'data/ffsmInput_tiga.ods' 'withSawMill5_concentrated'
