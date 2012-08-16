/***************************************************************************
 *   Copyright (C) 2006-2008 by Antonello Lobianco                         *
 *   http://regmas.org                                                     *
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
Output::collectData(){

}

void
Output::initOutput(){
	//initOutputAggregatedData();
	initOutputMaps();
	//initOutputDetailedData();
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

		for(int i=0;i<nYears;i++){
			out << i+inYear << d;
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
		for(int i=0;i<nYears;i++){
			out << i+inYear << d;
		}
		out << "\n";
		out.close();
}

void
Output::initOutputAggregatedData(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_AGGREGATED) return;


	char d = getOutputFieldDelimiter();

	string outFileName;


	if(MTHREAD->MD->getBoolSetting("aggregatedOutputHumanReadable")) {
		outFileName = MTHREAD->MD->getBaseDirectory()
					  + MTHREAD->MD->getOutputDirectory()
		              + "results/aggregatedOutput_"
		              + MTHREAD->getScenarioName()
					  + MTHREAD->MD->getStringSetting("outputFileExtension");
		ofstream out(outFileName.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
		out << "YEAR"                << d << "\n";
		out.close();	
	} else { // output is for database processing...
		outFileName = MTHREAD->MD->getBaseDirectory()
					  + MTHREAD->MD->getOutputDirectory()
		              + "results/aggregatedOutput"
					  + MTHREAD->MD->getStringSetting("outputFileExtension");
		ifstream in(outFileName.c_str(), ios::in);
		if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
			in.close();
			cleanScenario(outFileName, MTHREAD->getScenarioName(), d);
			return;
		} else {
			in.close();
		}
		ofstream out(outFileName.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
		out << "SCENARIO" << d ;
		out << "YEAR"                << d ;
		out << "\n";
		out.close();
	}





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
Output::initOutputDetailedData(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_DETAILED) return;

	char d = getOutputFieldDelimiter();
	vector<int> landTypes;


	if(MTHREAD->MD->getBoolSetting("detailedOutputHumanReadable")){
		string outFileName =     MTHREAD->MD->getBaseDirectory()
							   + MTHREAD->MD->getOutputDirectory()
							   + "results/ag_"
							   + "detailID"
							   + "_"
							   + MTHREAD->getScenarioName()
							   + MTHREAD->MD->getStringSetting("outputFileExtension");
		ofstream out(outFileName.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+".");}
		out << "RECORDID"             << d << "\n"
			<< "YEAR"                 << d << "\n";
		    out.close();
	} else { // output is on one big file....
		string outFileName =     MTHREAD->MD->getBaseDirectory()
								   + MTHREAD->MD->getOutputDirectory()
			                       + "results/agentsDetails"
								   + MTHREAD->MD->getStringSetting("outputFileExtension");
		ifstream in(outFileName.c_str(), ios::in);
		if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
			in.close();
			cleanScenario(outFileName, MTHREAD->getScenarioName(), d);
			return;
		} else {
			in.close();
		}
		ofstream out(outFileName.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+".");}
		out << "SCENARIO"             << d ;
		out << "RECORDID"             << d ;
		out << "YEAR"                 << d ;
		out.close();
	}
}




void
Output::print(){
	//printAggregatedData();
	printMaps();
	//printDetailedData();
	//printDebugMatrices();
}

void
Output::printMaps(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_MAPS) return;
	MTHREAD->GIS->printLayers();
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_BINMAPS) return;
	MTHREAD->GIS->printBinMaps();

}

void
Output::printDetailedData(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_DETAILED) return;

	char           d         = getOutputFieldDelimiter();

	string out;

	vector <string> sData; //surveyData
	//sData.push_back(surveyID);
	//sData.push_back(i2s(surveys[i].year));


	if(MTHREAD->MD->getBoolSetting("detailedOutputHumanReadable")){
		string inOutFileName =     MTHREAD->MD->getBaseDirectory()
								 + MTHREAD->MD->getOutputDirectory()
								 + "results/ag_"
								 + "detailID"
								 + "_"
								 + MTHREAD->getScenarioName()
								 + MTHREAD->MD->getStringSetting("outputFileExtension");

		ifstream inFile (inOutFileName.c_str(), ios::in);
		if (!inFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for reading.");}
		for(uint i=0;i<sData.size();i++) {
			string tempString;
			getline (inFile,tempString);
			out.append(tempString);
			out.append(sData[i]+d);
			out.append("\n");
		}
		inFile.close();

		ofstream outFile (inOutFileName.c_str(), ios::out);
		if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}
			outFile << out ;
			outFile.close();
	} else {  // one big file...
		string outFileName =     MTHREAD->MD->getBaseDirectory()
								 + MTHREAD->MD->getOutputDirectory()
								 + "results/agentsDetails"
								 //+ MTHREAD->getScenarioName()
								 + MTHREAD->MD->getStringSetting("outputFileExtension");
		ofstream outFile (outFileName.c_str(), ios::app);
		if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+".");}
		outFile << MTHREAD->getScenarioName() << d ;
		for (uint i=0;i<sData.size();i++){
			outFile << sData[i] << d ;
		}
		outFile << "\n" ;
		outFile.close();
	}
}

void
Output::printFinalOutput(){
	printForestData();
	printProductData();
}

void
Output::printForestData(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_AGGREGATED) return;
	msgOut(MSG_INFO, "Printing forest data..");
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

	for(uint i=0;i<MTHREAD->MD->forDataVector.size();i++){
		out << MTHREAD->getScenarioName() << d;
		out << MTHREAD->MD->forDataVector[i].parName << d;
		out << MTHREAD->MD->forDataVector[i].region << d;
		out << MTHREAD->MD->forDataVector[i].forType << d;
		out << MTHREAD->MD->forDataVector[i].diamClass << d;
		for(int y=0;y<nYears;y++){
			out << MTHREAD->MD->getTimedData(MTHREAD->MD->forDataVector[i].values,y+inYear) << d;
		}
		out << "\n";
	}
	out.close();
}

void
Output::printProductData(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_AGGREGATED) return;
	msgOut(MSG_INFO, "Printing product data..");
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
		for(int y=0;y<nYears;y++){
			out << MTHREAD->MD->getTimedData(values,y+inYear) << d;
		}
		out << "\n";
	}
	out.close();
}

void
Output::printAggregatedData(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_AGGREGATED) return;

	string inOutFileName;

	char                 d               = getOutputFieldDelimiter();


	string out;
	vector<string> data; // vector of the newly added data
	//double          SUMcapital=0;

	data.push_back(i2s(MTHREAD->SCD->getYear()));


	if (MTHREAD->MD->getBoolSetting("aggregatedOutputHumanReadable")){
		inOutFileName = MTHREAD->MD->getBaseDirectory()
						+ MTHREAD->MD->getOutputDirectory()
		                + "results/aggregatedOutput_"
		                + MTHREAD->getScenarioName()
						+ MTHREAD->MD->getStringSetting("outputFileExtension");
		ifstream inFile (inOutFileName.c_str(), ios::in);
		if (!inFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for reading.");}
		for(uint i=0;i<data.size();i++) {
			string tempString;
			getline (inFile,tempString);
			out.append(tempString);
			out.append(data[i]+d);
			out.append("\n");
		}
		inFile.close();

		ofstream outFile (inOutFileName.c_str(), ios::out);
		if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}
		outFile << out ;
		outFile.close();
	} else { // outpout for DB...

		inOutFileName = MTHREAD->MD->getBaseDirectory()
						+ MTHREAD->MD->getOutputDirectory()
		                + "results/aggregatedOutput"
		                //+ MTHREAD->getScenarioName()
						+ MTHREAD->MD->getStringSetting("outputFileExtension");

		ofstream outFile (inOutFileName.c_str(), ios::app);
		if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}
		outFile << MTHREAD->getScenarioName() << d ;
		for (uint i=0;i<data.size();i++){
				outFile << data[i] << d ;
		}
		outFile << "\n" ;
		outFile.close();
	}
}



void
Output::printDebugMatrices(){
	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_ALL) return;
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
	
	/*QFile infile(fileName.c_str());
	if (!infile.open(QFile::ReadOnly)) {
		//cerr << "Cannot open file for reading: "
		//	<< qPrintable(file.errorString()) << endl;
		//msgOut(MSG_ERROR, "Cannot open map file "+fileName+" for reading.");
		return;
	}
	QTextStream in(&file);
	int countRow = 0;
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList fields = line.split(' ');

	*/

	/*		for(uint i=0;i<sData.size();i++) {
				string tempString;
				getline (inFile,tempString);
				out.append(tempString);
				out.append(sData[i]+d);
				out.append("\n");
			}
			inFile.close();

			ofstream outFile (inOutFileName.c_str(), ios::out);
			if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}
			outFile << out ;
			outFile.close(); 
	*/
}

