#!/bin/bash

#---------------------------------------
#  Shell script to clean the RegMAS output
#---------------------------------------

echo Cleaning the RegMAS output...
echo ""

# maps...
rm -rf output/maps/asciiGrids/*
rm -rf output/maps/bitmaps/*
rm -rf output/maps/cats/*
rm -rf output/maps/colr/*
rm -rf output/maps/grass/france/default
rm -rf output/maps/floatListLayers
rm -rf output/maps/integerListLayers
rm -rf output/maps/scenarioName
# results...
rm -rf output/results/*.csv

echo Done cleaning Pythia output!
echo ""