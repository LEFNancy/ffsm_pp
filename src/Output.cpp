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
	commonInit();
	initOutputMaps();
	initOutputForestData();
	initOutputProductData();

}


void
Output::commonInit(){
	oLevel       = MTHREAD->MD->getIntSetting("outputLevel");
	d            = getOutputFieldDelimiter();
	inYear       = MTHREAD->MD->getIntSetting("initialYear");
	nYears       = MTHREAD->MD->getIntSetting("simulationYears");
	baseDir      = MTHREAD->MD->getBaseDirectory();
	oDir         = MTHREAD->MD->getOutputDirectory();
	scenarioName = MTHREAD->getScenarioName();
	oFileExt     = MTHREAD->MD->getStringSetting("outputFileExtension");
	oHRedeable   = MTHREAD->MD->getBoolSetting("outputHumanReadable");
	oSingleFile  = MTHREAD->MD->getBoolSetting("outputSingleFile");
	wRegId_l1    = MTHREAD->MD->getIntSetting("worldCodeLev1");
	wRegId_l2    = MTHREAD->MD->getIntSetting("worldCodeLev2");
}

void
Output::initOutputForestData(){
	if(oLevel<OUTVL_AGGREGATED) return;

	if(oSingleFile){
		outFileName = baseDir+oDir+"results/forestData"+oFileExt;
		ifstream in(outFileName.c_str(), ios::in);
		if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
			in.close();
			cleanScenario(outFileName, scenarioName, d);
			return;
		} else {
			in.close();
		}
	} else {
		outFileName = baseDir+oDir+"results/forestData_"+scenarioName+oFileExt;
	}

	ofstream out(outFileName.c_str(), ios::out);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
	out << "scen" << d << "parName" << d << "country" << d << "region" << d << "forType" << d << "diamClass" << d;
	if(oHRedeable){
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
	if(oLevel<OUTVL_AGGREGATED) return;

	if(oSingleFile){
		outFileName = baseDir+oDir+"results/productData"+oFileExt;
		ifstream in(outFileName.c_str(), ios::in);
		if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
			in.close();
			cleanScenario(outFileName, scenarioName, d);
			return;
		} else {
			in.close();
		}
	} else {
		outFileName = baseDir+oDir+"results/productData_"+scenarioName+oFileExt;
	}

	ofstream out(outFileName.c_str(), ios::out);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
	out << "scen" << d << "parName" << d << "country" << d << "region" << d << "prod" << d << "freeDim" << d;
	if(oHRedeable){
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
	if(oLevel<OUTVL_MAPS) return;
	string mapBaseDirectory = baseDir+oDir+"maps/";
	string filenameToSaveScenarioName = mapBaseDirectory+"scenarioName";
	string filenameListIntLayers = mapBaseDirectory+"integerListLayers";
	string filenameListFloatLayers = mapBaseDirectory+"floatListLayers";

	// printing the scenario name in the "scenarioName file"...
	ofstream outSN(filenameToSaveScenarioName.c_str(), ios::out);
	if (!outSN){ msgOut(MSG_ERROR,"Error in opening the file "+filenameToSaveScenarioName+".");}
	outSN << scenarioName << "\n";
	outSN.close();
	// cleaning the "integerListLayers" and "floatListLayers" file...
	ofstream outi(filenameListIntLayers.c_str(), ios::out);
	outi.close();
	ofstream outf(filenameListFloatLayers.c_str(), ios::out);
	outf.close();
}

void
Output::print(){
	printMaps();
	printForestData(false);
	printProductData(false);
}

void
Output::printMaps(){
	if(oLevel<OUTVL_MAPS) return;
	MTHREAD->GIS->printLayers();
	if(oLevel<OUTVL_BINMAPS) return;
	MTHREAD->GIS->printBinMaps();
}

void
Output::printFinalOutput(){
	// we do this only if we choosed the outputHumanReadable settings, as we flush the data all in ones at the end.
	// oterwise we flush data every year
	if(oHRedeable){
		printForestData(true);
		printProductData(true);
	}
}

void
Output::printForestData(bool finalFlush){

	if(oLevel<OUTVL_DETAILED) return;
	if(oHRedeable && !finalFlush) return;

	msgOut(MSG_INFO, "Printing forest data..");
	int currentYear = MTHREAD->SCD->getYear();
	if(oSingleFile){
		outFileName = baseDir+oDir+"results/forestData"+oFileExt;
	} else {
		outFileName = baseDir+oDir+"results/forestData_"+scenarioName+oFileExt;
	}
	ofstream out (outFileName.c_str(), ios::app);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for writing.");}

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
		ModelRegion* REG = MTHREAD->MD->getRegion(regId);
		// we don't want to output data from residual region unless it's the world region we are speaking of
		if(REG->getIsResidual() && !(regId==wRegId_l1 || regId==wRegId_l2)) continue;
		out << scenarioName << d;
		out << parName << d;
		if (REG->getRegLevel()==2){
			ModelRegion* pREG = MTHREAD->MD->getRegion(REG->getParRegId());
			out << pREG->getRegSName() << d;
			out << REG->getRegSName() << d;
		} else if (REG->getRegLevel()==1){
			out << REG->getRegSName() << d;
			out << d;
		} else {
			out << d << d;
		}
		out << forType << d;
		out << diamClass << d;
		if (oHRedeable){
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

	if(oLevel<OUTVL_DETAILED) return;
	if(oHRedeable && !finalFlush) return;

	msgOut(MSG_INFO, "Printing forest data..");
	int currentYear = MTHREAD->SCD->getYear();

	if(oSingleFile){
		outFileName = baseDir+oDir+"results/productData"+oFileExt;
	} else {
		outFileName = baseDir+oDir+"results/productData_"+scenarioName+oFileExt;
	}
	ofstream out (outFileName.c_str(), ios::app);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for writing.");}

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
		ModelRegion* REG = MTHREAD->MD->getRegion(regId);
		// we don't want to output data from residual region unless it's the world region we are speaking of
		if(REG->getIsResidual() && !(regId==wRegId_l1 || regId==wRegId_l2)) continue;
		out << scenarioName << d;
		out << parName << d;
		if (REG->getRegLevel()==2){
			ModelRegion* pREG = MTHREAD->MD->getRegion(REG->getParRegId());
			out << pREG->getRegSName() << d;
			out << REG->getRegSName() << d;
		} else if (REG->getRegLevel()==1){
			out << REG->getRegSName() << d;
			out << d;
		} else {
			out << d << d;
		}
		out << prod << d;
		out << freeDim << d;
		if (oHRedeable){
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

