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
# and put something like this in the script    

#./run_single_scenario.sh 'default'

# simulations on es_d
#./run_single_scenario.sh 'es_d_change'
#./run_single_scenario.sh 'es_d_change_high'
#./run_single_scenario.sh 'es_d_0'
#./run_single_scenario.sh 'es_d_03'
#./run_single_scenario.sh 'es_d_m03'
#./run_single_scenario.sh 'es_d_01'
#./run_single_scenario.sh 'es_d_1'
#./run_single_scenario.sh 'es_d_m01'
#./run_single_scenario.sh 'es_d_m1'

# simulations on m (rpW) with independance
#./run_single_scenario.sh 'm_rpW_10'
#./run_single_scenario.sh 'm_rpW_30'
#./run_single_scenario.sh 'm_rpW_50'
#./run_single_scenario.sh 'm_rpW_70'
#./run_single_scenario.sh 'm_rpW_shock'

# simulations on m (rpW) with complementarity
./run_single_scenario.sh 'm2_rpW_10'
./run_single_scenario.sh 'm2_rpW_30'
./run_single_scenario.sh 'm2_rpW_50'
./run_single_scenario.sh 'm2_rpW_70'
./run_single_scenario.sh 'm2_rpW_shock'

# simulations on m (rpW) with complementarity
./run_single_scenario.sh 'm3_rpW_10'
./run_single_scenario.sh 'm3_rpW_30'
./run_single_scenario.sh 'm3_rpW_50'
./run_single_scenario.sh 'm3_rpW_70'
./run_single_scenario.sh 'm3_rpW_shock'

# simulations for forest impacts
#./run_single_scenario.sh 'm_change'
#./run_single_scenario.sh 'm_es_d_change'
#./run_single_scenario.sh 'es_d_change_high'
#./run_single_scenario.sh 'es_d_m03'

#./run_single_scenario.sh 'no_recycling' 
#./run_single_scenario.sh 'debug' 
#./run_single_scenario.sh 'inputFile2' 'scenarioName4'




