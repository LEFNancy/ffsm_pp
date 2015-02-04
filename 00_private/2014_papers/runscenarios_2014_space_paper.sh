#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a runscenarios_space_paper.sh 


# 2014 space paper scenarios
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'bau' 
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc1'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc2'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc3'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc4'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'bau_nospvar'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc1_nospvar'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc2_nospvar'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc3_nospvar'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc4_nospvar'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc1_hetAgents'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'cc4_hetAgents'

./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null
./ffsm -c -i data/ffsmInput_2014_papers.ods -s randomSpace 1> /dev/null 2> /dev/null


