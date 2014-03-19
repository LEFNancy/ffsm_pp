#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------

# OLD: Scenarios that write to different output can be executed in parallel, while scenario which write to the same output file (like the one repeated n times) should be run serialised

#Safe parallel..
./ffsm -c -s scenarioName1 > logs/scenarioName1.txt &
./ffsm -c -s scenarioName2 > logs/scenarioName2.txt &


#Now also running the same scenario in parallel is safe as long as newRandomSeed is set to true and outputSingleFile is set to false..
for i in {1..30}
do
   ./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null &
done

# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue
# In that case run this script as:
# parallel --jobs <n of jobs> -a runscenarios.sh 
# and put something like this in the script

./run_single_scenario.sh 'scenarioName1'
./run_single_scenario.sh 'scenarioName2'





