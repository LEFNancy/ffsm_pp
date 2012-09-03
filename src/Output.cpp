/***************************************************************************
 *   Copyright (C) 2012 by Laboratoire d'Economie Forestière               *
 *   http://www.nancy.inra.fr/lef_eng                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <fstream>

#include "Output.h"
#include "ThreadManager.h"
#include "Scheduler.h"
#include "ModelData.h"
#include "Gis.h"

typedef map<string, vector <double> > DataMap;
typedef pair<string, vector <double> > DataPair;


Output::Output(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
}

Output::~Output(){
}

// ---- functions ... ----------------------


void
Output::initOutput(){
	initOutputMaps();
	initOutputForestData();
	initOutputProductData();

}


void
Output::initOutputForestData(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_AGGREGATED) return;
	char d = getOutputFieldDelimiter();
	int inYear = MTHREAD->MD->getIntSetting("initialYear");
	int nYears =  MTHREAD->MD->getIntSetting("simulationYears");
	string outFileName;
	outFileName =   MTHREAD->MD->getBaseDirectory()
								+ MTHREAD->MD->getOutputDirectory()
								+ "results/forestData_"
								+ MTHREAD->getScenarioName()
								+ MTHREAD->MD->getStringSetting("outputFileExtension");
		ofstream out(outFileName.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
		out << "scen" << d << "parName" << d << "region" << d << "forType" << d << "diamClass" << d;
		if(MTHREAD->MD->getBoolSetting("outputHumanReadable")){
			for(int i=0;i<nYears;i++){
				out << i+inYear << d;
			}
		} else {
			out << "year" << d << "value" << d;
		}
		out << "\n";
		out.close();
}

void
Output::initOutputProductData(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_AGGREGATED) return;
	char d = getOutputFieldDelimiter();
	int inYear = MTHREAD->MD->getIntSetting("initialYear");
	int nYears =  MTHREAD->MD->getIntSetting("simulationYears");
	string outFileName;
	outFileName =   MTHREAD->MD->getBaseDirectory()
								+ MTHREAD->MD->getOutputDirectory()
								+ "results/productData_"
								+ MTHREAD->getScenarioName()
								+ MTHREAD->MD->getStringSetting("outputFileExtension");
		ofstream out(outFileName.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
		out << "scen" << d << "parName" << d << "region" << d << "prod" << d << "freeDim" << d;
		if(MTHREAD->MD->getBoolSetting("outputHumanReadable")){
			for(int i=0;i<nYears;i++){
				out << i+inYear << d;
			}
		} else {
			out << "year" << d << "value" << d;
		}
		out << "\n";
		out.close();
}

/**
Resetting the list of printed layers and the scenario name..
<br>Printing scenario name for post-processing scripts
*/
void
Output::initOutputMaps(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_MAPS) return;
	string mapBaseDirectory = MTHREAD->MD->getBaseDirectory()+MTHREAD->MD->getOutputDirectory()+"maps/";
	string filenameToSaveScenarioName = mapBaseDirectory+"scenarioName";
	string filenameListIntLayers = mapBaseDirectory+"integerListLayers";
	string filenameListFloatLayers = mapBaseDirectory+"floatListLayers";

	// printing the scenario name in the "scenarioName file"...
	ofstream outSN(filenameToSaveScenarioName.c_str(), ios::out);
	if (!outSN){ msgOut(MSG_ERROR,"Error in opening the file "+filenameToSaveScenarioName+".");}
	outSN << MTHREAD->getScenarioName() << "\n";
	outSN.close();
	// cleaning the "integerListLayers" and "floatListLayers" file...
	ofstream outi(filenameListIntLayers.c_str(), ios::out);
	outi.close();
	ofstream outf(filenameListFloatLayers.c_str(), ios::out);
	outf.close();

	// init a new layers: agrUsage, overall agricultural usage, to keep track of abandoned and idle plots
	//MTHREAD->GIS->addLayer("agrUsage","Overall agricultural usage",true, true);
	//MTHREAD->GIS->addLegendItem("agrUsage", 0, "Abandoned plots", 255, 0, 0, ((double)0), ((double)0));
	//MTHREAD->GIS->addLegendItem("agrUsage", 1, "Idle plots", 120, 120, 0, ((double)0), ((double)0));
	//MTHREAD->GIS->addLegendItem("agrUsage", 2, "Normal agr. usage", 0, 255, 0, ((double)0), ((double)0));
	
}



void
Output::print(){
	printMaps();
	printForestData(false);
	printProductData(false);
}

void
Output::printMaps(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_MAPS) return;
	MTHREAD->GIS->printLayers();
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_BINMAPS) return;
	MTHREAD->GIS->printBinMaps();

}

void
Output::printFinalOutput(){
	// we do this only if we choosed the outputHumanReadable settings, as we flush the data all in ones at the end.
	// oterwise we flush data every year
	if(MTHREAD->MD->getBoolSetting("outputHumanReadable")){
		printForestData(true);
		printProductData(true);
	}

}

void
Output::printForestData(bool finalFlush){

	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_DETAILED) return;
	bool outputHumanReadable = MTHREAD->MD->getBoolSetting("outputHumanReadable");
	if(outputHumanReadable && !finalFlush) return;

	msgOut(MSG_INFO, "Printing forest data..");
	int currentYear = MTHREAD->SCD->getYear();
	string inOutFileName;
	char d = getOutputFieldDelimiter();
	int inYear = MTHREAD->MD->getIntSetting("initialYear");
	int nYears =  MTHREAD->MD->getIntSetting("simulationYears");
	inOutFileName =   MTHREAD->MD->getBaseDirectory()
									+ MTHREAD->MD->getOutputDirectory()
									+ "results/forestData_"
									+ MTHREAD->getScenarioName()
									+ MTHREAD->MD->getStringSetting("outputFileExtension");
	ofstream out (inOutFileName.c_str(), ios::app);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}

	DataMap::const_iterator i;
	string key;
	vector <double> values;
	string parName;
	int regId;
	string forType;
	string diamClass;
	for(i=MTHREAD->MD->forDataMap.begin();i!=MTHREAD->MD->forDataMap.end();i++){
		key = i->first;
		values = i->second;
		MTHREAD->MD->unpackKeyForData(key, parName, regId, forType, diamClass);
		out << MTHREAD->getScenarioName() << d;
		out << parName << d;
		out << regId << d;
		out << forType << d;
		out << diamClass << d;
		if (outputHumanReadable){
			for(int y=0;y<nYears;y++){
				out << MTHREAD->MD->getTimedData(values,y+inYear) << d;
			}
			out << "\n";
		} else {
			out << currentYear << d;
			out << MTHREAD->MD->getTimedData(values,currentYear) << d;
			out << "\n";
		}
	}
	out.close();
}

void
Output::printProductData(bool finalFlush){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_DETAILED) return;
	bool outputHumanReadable = MTHREAD->MD->getBoolSetting("outputHumanReadable");
	if(outputHumanReadable && !finalFlush) return;

	msgOut(MSG_INFO, "Printing product data..");
	int currentYear = MTHREAD->SCD->getYear();
	string inOutFileName;
	char d = getOutputFieldDelimiter();
	int inYear = MTHREAD->MD->getIntSetting("initialYear");
	int nYears =  MTHREAD->MD->getIntSetting("simulationYears");
	inOutFileName =   MTHREAD->MD->getBaseDirectory()
									+ MTHREAD->MD->getOutputDirectory()
									+ "results/productData_"
									+ MTHREAD->getScenarioName()
									+ MTHREAD->MD->getStringSetting("outputFileExtension");
	ofstream out (inOutFileName.c_str(), ios::app);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}

	DataMap::const_iterator i;
	string key;
	vector <double> values;
	string parName;
	int regId;
	string prod;
	string freeDim;
	for(i=MTHREAD->MD->prodDataMap.begin();i!=MTHREAD->MD->prodDataMap.end();i++){
		key = i->first;
		values = i->second;
		MTHREAD->MD->unpackKeyProdData(key, parName, regId, prod, freeDim);
		out << MTHREAD->getScenarioName() << d;
		out << parName << d;
		out << regId << d;
		out << prod << d;
		out << freeDim << d;
		if (outputHumanReadable){
			for(int y=0;y<nYears;y++){
				out << MTHREAD->MD->getTimedData(values,y+inYear) << d;
			}
			out << "\n";
		} else {
			out << currentYear << d;
			out << MTHREAD->MD->getTimedData(values,currentYear) << d;
			out << "\n";
		}
	}
	out.close();
}




char
Output::getOutputFieldDelimiter(){
	int delimiterID = MTHREAD->MD->getIntSetting("outputFieldDelimiter");
	switch (delimiterID) {
		case 1:
			return ',';
			break;
		case 2:
			return ';';
			break;
		case 3:
			return ':';
			break;
		case 4:
			return '\t';
			break;
		case 5:
			return ' ';
			break;
		default:
			msgOut(MSG_ERROR, "You have specified an unknow output file field delimiter. Using \";\".");
			return ',';
	}
}

/**
This routine clean the output scenario from previous outputs of the defined scenario.
Other scenarios are untouched. The scenarioName must be in the first row.
@param filename Filename of the output file to clean
@param scenarioName Name of the scenario we are replacing
@param d Field delimiter. It must not be changed in the meantime (between the various scenarios)
*/
void
Output::cleanScenario(string fileName, string scenarioName, char d){
	string dStr(&d,1);
	vector <string> rows;
	string tempRow;
	ifstream inFile (fileName.c_str(), ios::in);
	if (!inFile){
		msgOut(MSG_ERROR,"Error in opening the file "+fileName+" for reading.");
		return;
	}
	while( getline (inFile,tempRow) ){
		vector<string> tokens;
		tokenize(tempRow,tokens,dStr);
		if(tokens[0] != scenarioName)
			rows.push_back( tempRow );
	}
	inFile.close();
	ofstream out(fileName.c_str(), ios::out);
	for(uint i=0;i<rows.size();i++){
		out << rows[i];
		out << "\n";
	}
}

