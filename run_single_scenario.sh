#!/bin/bash

#---------------------------------------
#  Shell script to run a single ffsm scenario, where the scenario name is the first argument.
#---------------------------------------

./ffsm -c -s $1 > logs/${1}.txt
echo "Ended running scenario " $1






