#!/bin/bash

#---------------------------------------
#  Shell script to run ffsm scenarios
#---------------------------------------

# Scenarios that write to different output can be executed in parallel, while scenario which write to the same output file (like the one repeated n times) should be run serialised

#Safe parallel..
#./ffsm -c -s reference > logs/reference.txt &
#./ffsm -c -s nonspatial > logs/nonspatial.txt &
#./ffsm -c -s vRegFixed > logs/vRegFixed.txt &
#./ffsm -c -s vRegFromHr > logs/vRegFromHr.txt &
#./ffsm -c -s vRegEnd070 > logs/vRegEnd070.txt &
#./ffsm -c -s withVariance > logs/withVariance.txt &
#./ffsm -c -s withRisk06 > logs/withRisk06.txt &
#./ffsm -c -s withRisk08 > logs/withRisk08.txt &
#./ffsm -c -s withRisk10 > logs/withRisk10.txt &


# Must be run serialised..
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

./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt
./ffsm -c -s randomSpaceAndRisk > logs/random2.txt

