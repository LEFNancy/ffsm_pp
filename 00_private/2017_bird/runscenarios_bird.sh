#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a runscenarios_bird.sh 

# Bird scenarios
  ./run_single_scenario.sh 'data/ffsmInput_bird.ods' 'base'
