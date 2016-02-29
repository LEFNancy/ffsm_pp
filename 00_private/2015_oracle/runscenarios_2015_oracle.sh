#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a runscenarios_2015_oracle.sh 

# Oracle scenarios
  ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'constant'
  ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_a1b'
  ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_a2'
  ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_b1'
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
  ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_a1b_priceEffectOnly'
  ./run_single_scenario.sh 'data/ffsmInput_2015_oracle.ods' 'arpege_a1b_phImpactOnly'
 
#last run:
# 20150212, git commit 224aae9b4caee64253b5534f1a9820f4b1fa623a Fri Feb 12 10:56:06 2016 +0100 Added for area change in ffsmInput_2015_oracle.ods and corrected a parameter in ffsmInput_2015_ash.ods
# 20150515, git commit b00ad5fe37ca79effb571805a2fba7ff23f4fb43 Thu Mar 12 15:46:40 2015 +0100 Added option to add forest type from known spatial data of area and vol