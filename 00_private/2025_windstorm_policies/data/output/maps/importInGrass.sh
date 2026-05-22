#!/bin/bash

#---------------------------------------
#  Shell script to import the outputed RegMAS maps in grass
#  together with their associated category and colour legend
#  files...
#  It first read the scenario name, then create an empty mapset
#  into Grass using such scenario name and finally call the grass
#  to execute the second script ("runWithinGrass.sh") that will
#  do the import work.
#---------------------------------------

echo Importing RegMAS generated maps in Grass..
echo ""

# ATTENCTION TO NOT LEAVE SPACES BEFORE AND AFTER THE "=" SIGN !!!!
REGION_NAME="france"
SCENARIO_NAME="default"
SCENARIO_FILENAME="scenarioName"

# load the scenario name...
if [ -r $SCENARIO_FILENAME ]; then
	for i in $(cat $SCENARIO_FILENAME);
	do
		SCENARIO_NAME=$i
	done
else
	echo "File with the scenario name do not exist or is not redable, aborting"
	exit 1
fi

# remove the $SCENARIO_NAME mapset from grass (if present) and create a new
# empty one starting from a mould zipped file.
rm -rf grass/$REGION_NAME/$SCENARIO_NAME
cd grass/$REGION_NAME
tar -zxf emptyMapSet.tar.gz
if [ $SCENARIO_NAME != 'default' ]; then
    mv default $SCENARIO_NAME
fi
cd ../../

# call Grass to execute the "runWithinGrass.sh" script..
grass -text grass/$REGION_NAME/$SCENARIO_NAME  < runWithinGrass.sh

