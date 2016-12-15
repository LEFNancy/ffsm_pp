#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# It can be run in parallel using GNU parallel:
# parallel --jobs <n of jobs> -a this_file.sh 

./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Mh-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Mh-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Mi-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Mi-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Mi-E0'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Mi-E0'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Mi-E1'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Mi-E1'

# Sa on market homogeneity
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me0001-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me001-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me01-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me05-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me25-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me5-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me0001-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me001-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me01-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me05-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me25-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me5-Ei'
# 31/10/2016:
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me10-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me100-Ei'
#./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me200-Ei'
#./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me500-Ei'
#./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me1000-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me10-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me100-Ei'
#./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me200-Ei'
#./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me500-Ei'
#./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me1000-Ei'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Mi-Ei_do'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Mi-Ei_do'
# 7/12/2016:
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Mi-Ei_fxRepl'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Mi-Ei_fxRepl'
# 13/12/2016:
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Mh-Ei_fxRepl'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Mh-Ei_fxRepl'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Me0001-Ei_fxRepl'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Me0001-Ei_fxRepl'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Mi-E1_fxRepl'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Mi-E1_fxRepl'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P0-Mi-E0_fxRepl'
./run_single_scenario.sh 'data/ffsmInput_ash.ods' 'P1-Mi-E0_fxRepl'


# run of 13 Dec 2016: added various "*_fxRepl" scenarios
# run of 12 Dec 2016: just same of 7 Dec 2016 but for all the scenarios
# run of 7 Dec 2016: run the two scenarios on fixed replanting to scorporate physical mortality effect from human adatation
# run of 6 Nov 2016: forgot to run the two scenarios P0-Me500-Ei and P1-Me500-Ei, so I run them today. No variations from 3 Nov. They don't solve, so trying with Me200.
# run of 3 Nov 2016: corrected a problem in the previous fix for expectedReturns, where ash ft expectedReturns where not outputed.
# run of 31 October 2016: new scenarios with sigma 10, 100 and 1000, and with detailed output by dc. Also now manual fix to not consider ash in the expected returns of the bralL aggregated ft expected return
# runs of 12 September 2016: new scenarios, different expectedReturns for aggregated ft, new totalShareInvadedArea output var.
# runs of 6 april 2016 are just to print the sigma variable. Previous run: 24 mars












