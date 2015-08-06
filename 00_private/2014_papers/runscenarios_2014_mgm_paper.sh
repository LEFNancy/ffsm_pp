#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a runscenarios_2014_mgm_paper.sh 

# 2014 management paper scenarios
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'vRegFixed'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'vRegFromHr'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'vRegEnd070'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'withRisk06'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'withRisk08'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'withRisk10'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'reference'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'withRisk08-01'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'withRisk08-03'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'withRisk08-04'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'areaIncrease'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'ccPhImpact'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'lowerTrCosts'
./run_single_scenario.sh 'data/ffsmInput_2014_papers.ods' 'higherTrCosts'

