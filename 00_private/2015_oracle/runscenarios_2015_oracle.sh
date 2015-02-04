#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a runscenarios_oracle.sh 

# Oracle scenarios
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'oracle_bau'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_a1b'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_a2'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_b1'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_a1b_noExpectations'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_a1b_fullExpectations'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'CNCM33_1'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'DMIEH5C_1'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'DMIEH5C_2'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'DMIEH5C_3'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'EGMAM2_3'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'HADGEM2_1'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'IPCM4_1'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'IPCM4_2'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'IPCM4_3'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'MPEH5C_1'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'MPEH5C_2'
 ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'MPEH5C_3'
 
