#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------

./ffsm -c -s reference > logs/reference.txt &
./ffsm -c -s nonspatial > logs/nonspatial.txt &
./ffsm -c -s vRegFixed > logs/vRegFixed.txt &
./ffsm -c -s vRegFromHr > logs/vRegFromHr.txt &
./ffsm -c -s vRegEnd075 > logs/vRegEnd075.txt &
./ffsm -c -s withVariance > logs/withVariance.txt &
./ffsm -c -s withRisk06 > logs/withRisk06.txt
./ffsm -c -s withRisk08 > logs/withRisk08.txt
./ffsm -c -s withRisk10 > logs/withRisk10.txt

./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
./ffsm -c -s random > logs/random.txt
