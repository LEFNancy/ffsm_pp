#!/bin/bash

#---------------------------------------
#  This shell script is used by importInGams.sh
#  It is executed within the grass prompt, as it has few grass
#  specific commands.
#  It first import the RegMAS produced layers (tacking into account of the integer/
#  float difference) and then copy their associated categories and colour files.
#---------------------------------------


# ATTENCTION TO NOT LEAVE SPACES BEFORE AND AFTER THE "=" SIGN !!!!
REGION_NAME="colliEsini"
SCENARIO_NAME="default"
SCENARIO_FILENAME="scenarioName"
I_LAYERS_FILENAME="integerListLayers"
F_LAYERS_FILENAME="floatListLayers"

# loading the scenario name..
if [ -r $SCENARIO_FILENAME ]; then
	for i in $(cat $SCENARIO_FILENAME);
	do
		SCENARIO_NAME=$i
	done
else
	echo "File with the scenario name do not exist or is not redable, aborting"
	exit 1
fi

# import the layers to grass. It gets the names from the two files "integerListLayers"
# and "floatListLayers"...
if [ -r $I_LAYERS_FILENAME ]; then
	for i in $(cat $I_LAYERS_FILENAME); # integer layers..
	do
		r.in.ascii -i input=asciiGrids/$i output=$i # grass command
	done
else
	echo "List of integer layers file not exist or not redable"
fi
if [ -r $F_LAYERS_FILENAME ]; then
	for i in $(cat $F_LAYERS_FILENAME); # float layers..
	do
		r.in.ascii -f input=asciiGrids/$i output=$i # grass command
	done
else
	echo "List of float layers file not exist or not redable"
fi

# copy the category description files...
cp cats/* grass/$REGION_NAME/$SCENARIO_NAME/cats/

# copy the colour definition files...
cp colr/* grass/$REGION_NAME/$SCENARIO_NAME/colr/

# reclass the dtm layer, making it an integer
r.reclass input=dtm_2001_$SCENARIO_NAME@$SCENARIO_NAME output=dtm_classes rules=dtmReclassRules.txt title='dtm reclassed'

