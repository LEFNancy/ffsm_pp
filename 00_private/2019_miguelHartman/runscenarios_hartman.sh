#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------


# A better approach to run scenarios in parallel is using GNU parallel: you can set the maximum
# number of processes and then the jobs are put in a queue.
# In that case run this script as:
# parallel --jobs 3 -a runscenarios_hartman.sh 
# and put something like this in the script

./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default_test'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_low_test'
./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_low_sh_test'

#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_low2'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_low_sh2'

#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_54'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_100'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_150'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_200'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_300'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_400'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_500'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_600'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_700'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_800'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_900'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_1000'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_1100'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_1200'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_1300'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'Constant_1400'




#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'default_gamma'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_low_sh_gamma'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_med_sh_gamma'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_high_sh_gamma'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_coop_sh_gamma'


#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_low'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_med'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_high'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_coop'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_low_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_med_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_high_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'quinet_coop_sh'

#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_54'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_100'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_150'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_200'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_300'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_400'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_500'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_600'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_700'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_800'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_900'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1000'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1100'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1200'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1300'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1400'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1500'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_54_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_100_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_150_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_200_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_300_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_400_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_500_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_600_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_700_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_800_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_900_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1000_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1100_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1200_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1300_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1400_sh'
#./run_single_scenario.sh 'data/ffsmInput_carbon_miguel_2013.ods' 'end_1500_sh'


