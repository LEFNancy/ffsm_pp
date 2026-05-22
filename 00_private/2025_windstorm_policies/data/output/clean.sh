#!/bin/bash

#---------------------------------------
#  Shell script to clean the FFSM++ output
#---------------------------------------

echo Cleaning the FFSM++ output...
echo ""

# maps...
rm -rf maps/asciiGrids/*
rm -rf maps/bitmaps/*
rm -rf maps/cats/*
rm -rf maps/colr/*
rm -rf maps/grass/france/default
rm -rf maps/floatListLayers/*
rm -rf maps/integerListLayers/*
rm -rf maps/scenarioNames/*
# results...
rm -rf results/*.csv
# charts..
rm -rf charts/*.pdf
rm -rf charts/png/*.png
# tables..
rm -rf tables/*
# optimisation logs
rm -rf optimisationLogs/*
# debugs..
rm -rf debugs/*

# copy back the do-not-remove warning file..
cp 00_doNotRemove.txt maps/asciiGrids/
cp 00_doNotRemove.txt maps/bitmaps/
cp 00_doNotRemove.txt maps/cats/
cp 00_doNotRemove.txt maps/colr/
cp 00_doNotRemove.txt maps/floatListLayers/
cp 00_doNotRemove.txt maps/integerListLayers/
cp 00_doNotRemove.txt maps/scenarioNames/
cp 00_doNotRemove.txt charts/
cp 00_doNotRemove.txt charts/png/
cp 00_doNotRemove.txt tables/
cp 00_doNotRemove.txt optimisationLogs/
cp 00_doNotRemove.txt debugs/

# cp the results ods template
cp results_template.ods results/results.ods

echo Done cleaning FFSM++ output!
echo ""