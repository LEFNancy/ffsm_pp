#!/bin/bash

#---------------------------------------
#  Shell script to run a single ffsm scenario, where the scenario name is the first argument and input file is the second (optional) argument.
#---------------------------------------

if [ $# -eq 2 ]
  then
    echo "./ffsm -c -s $2 -i $1 > logs/${2}.txt"
    echo "Ended running scenario" $2 "on input file" $1
  else
   if [ $# -eq 1 ]
    then
     echo "./ffsm -c -s $1 > logs/${1}.txt"
     echo "Ended running scenario" $1
    else
     echo "ERROR: this script must be called with either 1 argument (scenario name) or 2 arguments (input file, scenario name)"
   fi
fi




