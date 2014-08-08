#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a runscenarios_oracle.sh 

# Oracle scenarios
 ./run_single_scenario.sh 'oracle_bau'
 ./run_single_scenario.sh 'arpege_a1b'
 ./run_single_scenario.sh 'arpege_a2'
 ./run_single_scenario.sh 'arpege_b1'
 ./run_single_scenario.sh 'arpege_a1b_noExpectations'
 ./run_single_scenario.sh 'arpege_a1b_fullExpectations'
 ./run_single_scenario.sh 'CNCM33_1'
 ./run_single_scenario.sh 'DMIEH5C_1'
 ./run_single_scenario.sh 'DMIEH5C_2'
 ./run_single_scenario.sh 'DMIEH5C_3'
 ./run_single_scenario.sh 'EGMAM2_3'
 ./run_single_scenario.sh 'HADGEM2_1'
 ./run_single_scenario.sh 'IPCM4_1'
 ./run_single_scenario.sh 'IPCM4_2'
 ./run_single_scenario.sh 'IPCM4_3'
 ./run_single_scenario.sh 'MPEH5C_1'
 ./run_single_scenario.sh 'MPEH5C_2'
 ./run_single_scenario.sh 'MPEH5C_3'


