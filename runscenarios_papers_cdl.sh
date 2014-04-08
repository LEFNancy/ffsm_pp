#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------

# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a runscenarios_papers_cdl.sh 


# Papers scenarios
./run_single_scenario.sh 'bau'
./run_single_scenario.sh 'bau_nospvar'
./run_single_scenario.sh 'reference'
./run_single_scenario.sh 'vRegFixed'
./run_single_scenario.sh 'vRegFromHr'
./run_single_scenario.sh 'vRegEnd070'
./run_single_scenario.sh 'withRisk06'
./run_single_scenario.sh 'withRisk08'
./run_single_scenario.sh 'withRisk10'
./run_single_scenario.sh 'nonspatial'
./run_single_scenario.sh 'withSpVariance'
./run_single_scenario.sh 'cc1'
./run_single_scenario.sh 'cc2'
./run_single_scenario.sh 'cc3'
./run_single_scenario.sh 'cc4'
./run_single_scenario.sh 'cc5'
./run_single_scenario.sh 'cc1_nospvar'
./run_single_scenario.sh 'cc2_nospvar'
./run_single_scenario.sh 'cc3_nospvar'
./run_single_scenario.sh 'cc4_nospvar'
./run_single_scenario.sh 'cc5_nospvar'

./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -s randomSpace 1> /dev/null 2> /dev/null




