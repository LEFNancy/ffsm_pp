#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------

# Safe parallel..
#./ffsm -c -s scenarioName1 > logs/scenarioName1.txt &
#./ffsm -c -s scenarioName2 > logs/scenarioName2.txt &


# Running the same scenario (e.g. for repetitions) in parallel is safe as long as newRandomSeed
# is set to true and outputSingleFile is set to false..
#for i in {1..30}
#do
#   ./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null &
#done

# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs <n of jobs> -a runscenarios.sh 
# and put something like this in the script    tail -f logs/no_recycling.txt

#./run_single_scenario.sh 'default' & tail -f logs/default.txt
./run_single_scenario.sh 'no_recycling' & tail -f logs/no_recycling.txt
#./run_single_scenario.sh 'inputFile1' 'scenarioName3'
#./run_single_scenario.sh 'inputFile2' 'scenarioName4'




