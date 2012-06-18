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
#include <sstream>
#include <stdexcept>
#include <algorithm> //alghoritm used to reverse an array ( reverse(v.begin(), v.end()); )
#include <iomanip> // for unzip

// Qt headers..
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QList>

// Unzip headers..
#include "unzip.h"

// RegMAS headers..
#include "RegData.h"
//#include "InputDocument.h"
//#include "InputNode.h"
#include "MainWindow.h"
#include "Scheduler.h"



RegData::RegData(ThreadManager* MTHREAD_h){
	MTHREAD = MTHREAD_h;
}

RegData::~RegData(){

}

vector<string>
RegData::getScenarios(){
	vector<string> toReturn;
	LLData table = getTable("scenarios");
	for(int i=0;i<table.nrecords();i++){
		string scenarioName = table.getData(i,"id");
		toReturn.push_back(scenarioName);
	}
	return toReturn;
}

int
RegData::getScenarioIndex(){
	vector<string> scenarios = getScenarios(); // TODO Check that I can call this function all around the model and not only at the beginning
	string currentScenario = MTHREAD->getScenarioName();
	for(int i=0;i<scenarios.size();i++){
		if (currentScenario == scenarios[i]){
			return i;
		}
	}
	msgOut(MSG_CRITICAL_ERROR, "function getScenarioIndex() didn't found the current scenarioName within those returned by getScenarios().");
	return 0;
}


void 
RegData::setDefaultSettings(){

	LLData table = getTable("settings");
	for (int i=0; i< table.nrecords();i++){
		BasicData SETT;
		SETT.name = table.getData(i,"name");
		string type = table.getData(i,"type");
		SETT.type = getType(type);
		SETT.comment = table.getData(i,"comment");
		vector <string> values;
		for (int z=0;z<100;z++){
			string toSearch = "value_"+i2s(z);
			string value = table.getData(i,toSearch);
			if (value != ""){
				values.push_back(value);
			}
		}
		SETT.values = values;
		programSettingsVector.push_back(SETT);
	}	

	// rescaling if we are in the region mode...
	if(MTHREAD->RD->getBoolSetting("subRegionMode")){	
		int XL  = MTHREAD->RD->getIntSetting("subRegionXLeft");
		int XR   = MTHREAD->RD->getIntSetting("subRegionXRight");
		int YT = MTHREAD->RD->getIntSetting("subRegionYTop");
		int YB = MTHREAD->RD->getIntSetting("subRegionYBottom"); 
		double scaleCoeff =
            ( (double)	((XR-XL)*(YB-YT)) )
                      /
            ( MTHREAD->RD->getIntSetting("nCols") * MTHREAD->RD->getIntSetting("nRows")) ; // ratio between npixel in the subRegion and npixels in the original region
		vector <string> value;
		value.push_back(d2s(scaleCoeff));
		MTHREAD->RD->addSetting("scaleCoeff",value, TYPE_DOUBLE, "Scale coefficient used as this simulation is ran on a sub-region trial");
	}
	else {
		double scaleCoeff = 1;
		vector <string> value;
		value.push_back(d2s(scaleCoeff));
		MTHREAD->RD->addSetting("scaleCoeff",value, TYPE_DOUBLE, "Scale coefficient equal to 1 as this simulation is ran on the whole region");
	}


	// post settings hacks & messages...
	if(getBoolSetting("subRegionMode") ){
		msgOut(MSG_INFO, "The model is currently running on a sub-region area (mainly for initial model debugging).\nThe Scale coefficient is "+d2s(getDoubleSetting("scaleCoeff"))+".\nYou can enable full-region mode with the setting \"subRegionMode\"");
	}
	else {
		msgOut(MSG_INFO, "The model is currently running on a the whole area (normal mode).\nYou can enable sub-region mode with the setting \"subRegionMode\"\n(useful for initial model debugging/calibration)");
	}
	// defining if individual agent data has to be show in the model tree Viewer (that is, if we are in the sub-mode region or if we forced to view it)
	if ( (getBoolSetting("subRegionMode") || getBoolSetting("forceAgentsView")) &&   ! getBoolSetting("forceAgentsHide") ){
		addSetting("agentsView", "true", TYPE_BOOL, "Define if agent data need to be view on the tree viewer");
		if (getBoolSetting("forceAgentsView")){
			msgOut(MSG_WARNING, "View of individual agent data on the Model Tree Viewer has been forced on.\nThis will considerably slow down the simulation.");
		}
	}
	else {
		addSetting("agentsView", "false", TYPE_BOOL, "Define if agent data need to be view on the tree viewer");
			msgOut(MSG_INFO, "View of individual agent data on the Model Tree Viewer has been disabled for performance reasons. \nYou can force it on with the setting \"forceAgensView\".");
	}
	msgOut(MSG_INFO,"### USING SCENARIO: "+MTHREAD->getScenarioName()+" ###");
	
	setOutputDirectory(getStringSetting("outputDirname").c_str());
};

void
RegData::setScenarioSettings(){

	string scenarioName = MTHREAD->getScenarioName();
	LLData table = getTable(scenarioName+"_settings", MSG_WARNING); //this scenario could not have an associated setting sheet
	if(table.getTableName() == "") return;

	for(int i=0; i< table.nrecords(); i++){
		BasicData SETT;
		string name = table.getData(i,"name");
		string stype = table.getData(i,"type");
		int type = getType(stype);
		string comment = table.getData(i,"comment");
		vector <string> values;
		for (int z=0;z<100;z++){
			string toSearch = "value_"+i2s(z);
			string value = table.getData(i,toSearch);
			if (value != ""){
				values.push_back(value);
			}
		}

		for(uint i=0;i<programSettingsVector.size();i++){
			if(programSettingsVector[i].name == name){
				programSettingsVector[i].values = values;
				programSettingsVector[i].type = type;
				programSettingsVector[i].comment = comment;
				break;
			}
		}

	}

	// scaling to subregionmode again as some parameters may have been changed..
	if(MTHREAD->RD->getBoolSetting("subRegionMode")){	
		int XL  = MTHREAD->RD->getIntSetting("subRegionXLeft");
		int XR   = MTHREAD->RD->getIntSetting("subRegionXRight");
		int YT = MTHREAD->RD->getIntSetting("subRegionYTop");
		int YB = MTHREAD->RD->getIntSetting("subRegionYBottom"); 
		double scaleCoeff =
            ( (double)	((XR-XL)*(YB-YT)) )
                      /
            ( MTHREAD->RD->getIntSetting("nCols") * MTHREAD->RD->getIntSetting("nRows")) ; // ratio between npixel in the subRegion and npixels in the original region
		setBasicData("scaleCoeff",scaleCoeff);
	}
	else {
		setBasicData("scaleCoeff",((double)1));
	}

	// post settings hacks & messages...
	if(getBoolSetting("subRegionMode") ){
		msgOut(MSG_INFO, "The model is currently running on a sub-region area (mainly for initial model debugging).\nThe Scale coefficient is "+d2s(getDoubleSetting("scaleCoeff"))+".\nYou can enable full-region mode with the setting \"subRegionMode\"");
	}
	else {
		msgOut(MSG_INFO, "The model is currently running on a the whole area (normal mode).\nYou can enable sub-region mode with the setting \"subRegionMode\"\n(useful for initial model debugging/calibration)");
	}
	// defining if individual agent data has to be show in the model tree Viewer (that is, if we are in the sub-mode region or if we forced to view it)
	if ( (getBoolSetting("subRegionMode") || getBoolSetting("forceAgentsView")) &&   ! getBoolSetting("forceAgentsHide") ){
		setBasicData("agentsView", true);
		if (getBoolSetting("forceAgentsView")){
			msgOut(MSG_WARNING, "View of individual agent data on the Model Tree Viewer has been forced on.\nThis will considerably slow down the simulation.");
		}
	}
	else {
		setBasicData("agentsView", false);
			msgOut(MSG_INFO, "View of individual agent data on the Model Tree Viewer has been disabled for performance reasons. \nYou can force it on with the setting \"forceAgensView\".");
	}
	setOutputDirectory(getStringSetting("outputDirname").c_str());
}

void
RegData::addSetting(string name_h, vector <string> values_h, int type_h, string comment_h){

	for (uint i=0;i<programSettingsVector.size();i++){
		if (programSettingsVector.at(i).name == name_h){
			msgOut(MSG_ERROR, "I already have setting "+name_h+".. Nothing is added..");
			return;
		}
	}
	BasicData SETT;
	SETT.name = name_h;
	SETT.values = values_h;
	SETT.type= type_h;
	SETT.comment = comment_h;
	programSettingsVector.push_back(SETT);
}

void
RegData::addSetting(string name_h, string value_h, int type_h, string comment_h){
	vector <string> values;
	values.push_back(value_h);
	addSetting(name_h, values, type_h, comment_h);
}


// ###############################################################################
void
RegData::setOutputDirectory(const char* output_dirname_h){

	if (strlen(output_dirname_h)==0){
		outputDirname=baseDirectory+"output/";
	}
	else {
		outputDirname=output_dirname_h;
	}	
	MTHREAD->setOutputDirName(outputDirname); //for the GUI
}	

string
RegData::getBaseData (const string &name_h, int type_h, int position){
	// If the data is called with DATA_NOW we interpret the array of values as a temporal array and we return the value at the current time.
	if(position == DATA_NOW) {
		position = MTHREAD->SCD->getIteration();
	}
	for (uint i=0; i<programSettingsVector.size();i++){
		if (programSettingsVector.at(i).name == name_h){
			int type = programSettingsVector.at(i).type;
			if(type != type_h){msgOut(MSG_CRITICAL_ERROR, "mismatching type in calling getBaseData() for "+name_h);}
			if(programSettingsVector.at(i).values.size() > ((uint)position)) {
				return programSettingsVector.at(i).values.at(position);
			} else if (programSettingsVector.at(i).values.size() > 0 ){
				// returning the last available value...
				return programSettingsVector.at(i).values.at( programSettingsVector.at(i).values.size()-1 );
			}
			else {msgOut(MSG_CRITICAL_ERROR, "Error: "+name_h+" doesn't have any value, even on the first position(year)!"); }
		}
	}
	if(type_h==TYPE_BOOL){
		msgOut(MSG_DEBUG, "Possible error calling getBaseData(TYPE_BOOL) for "+ name_h +". No setting option or macro data found with this name. Returning false.");
		return "0";
	} else {
		msgOut(MSG_CRITICAL_ERROR, "Error calling getBaseData() for "+ name_h +". No setting option or macro data found with this name.");
	}
	return "";
};

vector <string>
RegData::getVectorBaseData (const string &name_h, int type_h){
	for (uint i=0; i<programSettingsVector.size();i++){
		if (programSettingsVector.at(i).name == name_h){
			int type = programSettingsVector.at(i).type;
			if(type != type_h){msgOut(MSG_CRITICAL_ERROR, "mismatching type in calling getVectorBaseData() for "+name_h);}
			return programSettingsVector.at(i).values;
		}
	}
	msgOut(MSG_CRITICAL_ERROR, "Error calling getVectorBaseData() for "+ name_h +". No setting option or macro data found with this name.");
	vector <string> toReturn;
	return toReturn;
};

// ------------- start getSetting() amd getMacro() functions ....... -----------------
int
RegData::getIntSetting(const string &name_h, int position){
	return s2i( MTHREAD->RD->getBaseData(name_h,TYPE_INT,position) );
}
double
RegData::getDoubleSetting(const string &name_h, int position){
	return s2d( MTHREAD->RD->getBaseData(name_h,TYPE_DOUBLE,position) );
}
string
RegData::getStringSetting(const string &name_h, int position){
	return MTHREAD->RD->getBaseData(name_h,TYPE_STRING,position);
}
bool
RegData::getBoolSetting(const string &name_h, int position){
	return s2b( MTHREAD->RD->getBaseData(name_h,TYPE_BOOL,position) );
}
vector<int>
RegData::getIntVectorSetting(const string &name_h){
	return s2i(MTHREAD->RD->getVectorBaseData(name_h,TYPE_INT));
}
vector<double>
RegData::getDoubleVectorSetting(const string &name_h){
	return s2d(MTHREAD->RD->getVectorBaseData(name_h,TYPE_DOUBLE));
}
vector<string>
RegData::getStringVectorSetting(const string &name_h){
	return MTHREAD->RD->getVectorBaseData(name_h,TYPE_STRING);
}
vector<bool>
RegData::getBoolVectorSetting(const string &name_h){
	return s2b(MTHREAD->RD->getVectorBaseData(name_h,TYPE_BOOL));
}

// ------ END of getSetting() functions --------------------- 

void
RegData::setBasicData(const string &name_h, int value, int position){
	setBasicData(name_h, i2s(value), TYPE_INT, position);
}
void
RegData::setBasicData(const string &name_h, double value, int position){
	setBasicData(name_h, d2s(value), TYPE_DOUBLE, position);
}
void 
RegData::setBasicData(const string &name_h, string value, int position){
	setBasicData(name_h, value, TYPE_STRING, position);
}
void
RegData::setBasicData(const string &name_h, bool value, int position){
	setBasicData(name_h, b2s(value), TYPE_BOOL, position);
}

void
RegData::setBasicData(const string &name_h, string value, int type_h, int position){
	for (uint i=0; i<programSettingsVector.size();i++){
		if (programSettingsVector.at(i).name == name_h){
			int type = programSettingsVector.at(i).type;
			if(type != type_h){msgOut(MSG_CRITICAL_ERROR, "mismatching type in calling setBasicData() for "+name_h);}
			if(programSettingsVector.at(i).values.size() > ((uint)position)) {
				programSettingsVector.at(i).values.at(position)=value;
				return;
			}
			else {msgOut(MSG_CRITICAL_ERROR, "out-of-bound error calling setBasicData() for "+name_h); }
		}
	}
	msgOut(MSG_CRITICAL_ERROR, "Error calling setBasicData() for "+ name_h +". No setting option or macro data found with this name.");
	return;
}

std::string
RegData::getFilenameByType(std::string type_h){
	std::string directory;
	std::string filename;
	std::string filename_complete;
	for (uint i=0; i<iFilesVector.size(); i++){
		if (iFilesVector.at(i).type == type_h){
			directory=iFilesVector.at(i).directory;
			filename=iFilesVector.at(i).name;
			break;
		}
	}
	filename_complete = baseDirectory+directory+filename;
	return filename_complete;
};



void
RegData::debug(){
	// ********** START DEBUG CODE....... ************
	double ddebuga=0; //20080209
	uint idebuga=0;
	double ddebugb=0; //20080209
	uint idebugb=0;
	double ddebugc=0; //20080209
	uint idebugc=0;
	double debugmin = 0;
	double debugmax = 1000;
	for (uint q=0;q<10000;q++){
		ddebuga += debugmin + ( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(debugmax-debugmin+1);
		ddebugb += debugmin + ( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(debugmax-debugmin+1);
		ddebugc += debugmin + ( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(debugmax-debugmin+1);
	}
	idebuga = ddebuga;
	idebugb = ddebugb;
	idebugc = ddebugc;
	cout << "idebuga: "<<idebuga<<endl;
	cout << "idebugb: "<<idebugb<<endl;
	cout << "idebugc: "<<idebugc<<endl;
	throw 2;
	// ******** .....END DEBUG CODE *******************
}

void
RegData::loadInput(){
	//QString iFile("data/pythiaInput.ods");
	QString iFile(MTHREAD->getInputFileName().c_str());
	//cout << "PIPPO !!!!! " << MTHREAD->getInputFileName().c_str() << endl;
	QString oDir((MTHREAD->getBaseDirectory()+"tempInput").c_str());
	
	// removing output directory if exist..
	QDir oQtDir(oDir);

	if(oQtDir.exists()){
		bool deleted;
		deleted = delDir(oDir);
		if(deleted){msgOut(MSG_DEBUG,"Correctly deleted old temporary data");}
		else {msgOut(MSG_WARNING, "I could not delete old temporary data dir (hopefully we'll overrite the input files)");}
	}

	if (!QFile::exists(iFile))
	{
		cout << "File does not exist." << endl << endl;
		//return false;
	}
	UnZip::ErrorCode ec;
	UnZip uz;
	ec = uz.openArchive(iFile);
	if (ec != UnZip::Ok)	{
		cout << "Failed to open archive: " << uz.formatError(ec).toAscii().data() << endl << endl;
		//return false;
	}
	ec = uz.extractAll(oDir);
	if (ec != UnZip::Ok)
	{
		cout << "Extraction failed: " << uz.formatError(ec).toAscii().data() << endl << endl;
		uz.closeArchive();
		//return false;
	}
	
	// loading input file into memory...
	string inputXMLFileName = MTHREAD->getBaseDirectory()+"tempInput/content.xml";
	//string inputXMLFileName = MTHREAD->getBaseDirectory()+"test/content.xml";
	//cout << "inputXMLFileName: " << inputXMLFileName << endl;
	//mainDocument = new InputDocument();
	mainDocument.setWorkingFile(inputXMLFileName);
	//InputNode documentContent = mainDocument.getNodeByName("office:document-content");
	//InputNode documentBody = mainDocument.getNodeByName("office:body");
	//InputNode mainNode = mainDocument.getNodeByName("spreadsheet");
	//InputNode pippo = mainDocument.getNodeByName("pippo-pippo");
	//InputNode table = mainDocument.getNodeByName("table");
	//cout << "Test result: " << table.getStringContent() << endl;


	vector <InputNode> tables = mainDocument.getNodesByName("table");
	for(uint i=0;i<tables.size();i++){
		string tableName = tables[i].getStringAttributeByName("name");
		//cout <<tableName<<endl;
		LLData data(MTHREAD,tables[i].getStringAttributeByName("name"));
		vector <InputNode> rows = tables[i].getNodesByName("table-row");
		if(rows.size()<2) continue; //empty table or only with headers
		// building headers..
		vector <InputNode> cells = rows[0].getNodesByName("table-cell");
		for (uint y=0; y<cells.size(); y++){
			int repeated = 1;
			if( cells[y].hasAttributeByName("number-columns-repeated")){
				repeated = cells[y].getIntAttributeByName("number-columns-repeated");
			}
			for (int q=0;q<repeated;q++){
				if( !cells[y].hasChildNode("p") ){
					data.headers.push_back(""); // empty header
				} else {
					data.headers.push_back(cells[y].getNodeByName("p",MSG_NO_MSG,true).getStringContent());
				}
			}
		}
		// loading data...
		for (uint j=1; j<rows.size();j++){
			//cout << j << endl;
			vector <InputNode> cells = rows[j].getNodesByName("table-cell");
			if (cells.size()<1) continue;
			vector<string> record;
			// checking the first cell is not a comment nor is empty..
			int childCount = cells[0].getChildNodesCount();
			if (childCount == 0 || !cells[0].hasChildNode("p")) continue; // empty line, first column empty!
			string fistCol = cells[0].getNodeByName("p",MSG_NO_MSG,true).getStringContent();
			unsigned int z;
			z = fistCol.find("#");
			if( z!=string::npos && z == 0) continue; // found "#" on fist position, it's a comment!
			for (uint y=0; y<cells.size(); y++){
				int repeated = 1;
				if( cells[y].hasAttributeByName("number-columns-repeated")){
					repeated = cells[y].getIntAttributeByName("number-columns-repeated");
				}
				for (int q=0;q<repeated;q++){
					if( !cells[y].hasChildNode("p") ){
						record.push_back(""); // empty header
					} else {
						record.push_back(cells[y].getNodeByName("p",MSG_NO_MSG,true).getStringContent());
					}
				}
			}
			data.records.push_back(record);
		}
		data.clean();
		LLDataVector.push_back(data);
	}

	//debug !!!
	/*for (uint i=0; i<LLDataVector.size();i++){
		cout << "***************** NEW TABLE: " << LLDataVector[i].tableName << endl;
		//cout << "***** Headers: "<< endl;
		int headerSize = LLDataVector[i].headers.size();
		bool ok = true;
		cout << "Header size: " << headerSize << endl;
		//for (uint j=0; j<LLDataVector[i].headers.size();j++){
		//	cout << "["<<j<<"] " << LLDataVector[i].headers[j] << endl;
		//}
		//cout << "***** Records: " << endl;
		for (uint j=0; j<LLDataVector[i].records.size();j++){
			//cout << "** Record "<<j<<":"<<endl; 
			if(LLDataVector[i].records[j].size() != headerSize){
				cout << "There is a problem on record " << j <<"!"<< endl;
				cout << "His size is: "<< LLDataVector[i].records[j].size() << endl;
				ok = false;
			}
			//for (uint y=0; y<LLDataVector[i].records[j].size();y++){
			//	cout << "["<<y<<"] " << LLDataVector[i].records[j][y] << endl;
			//}
		}
		if(!ok) {cout <<"Problems with this table :-( !"<<endl;}
	}*/



	// deleting output directory if exist...
	if(oQtDir.exists()){
		bool deleted;
		deleted = delDir(oDir);
		if(deleted){msgOut(MSG_DEBUG,"Correctly deleted old temporary data");}
		else {msgOut(MSG_WARNING, "I could not delete old temporary data dir (hopefully we'll overrite the input files)");}
	}
}

bool
RegData::delDir(QString dirname) {
        bool deleted = false;
        QDir dir(dirname);
	//msgOut(MSG_DEBUG, dir.absolutePath().toStdString());
	dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
	QFileInfoList list = dir.entryInfoList();
	deleted = dir.rmdir(dir.absolutePath());
	if (deleted) return true;

	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		if (fileInfo.isFile()){
			//msgOut(MSG_DEBUG, "A file, gonna remove it: "+fileInfo.absoluteFilePath().toStdString());
			QFile targetFile(fileInfo.absoluteFilePath());
			bool fileDeleted = targetFile.remove();
			if (!fileDeleted){
				msgOut(MSG_CRITICAL_ERROR, "We have a problem: can't delete file "+fileInfo.absoluteFilePath().toStdString());
			}
		}
		else if (fileInfo.isDir()){
			//msgOut(MSG_DEBUG, "A directory, gonna go inside it: "+fileInfo.absoluteFilePath().toStdString());
			delDir(fileInfo.absoluteFilePath());
			dir.rmdir(fileInfo.absoluteFilePath());
		}	
	}

	deleted = dir.rmdir(dir.absolutePath());
	if (deleted) return true;
	return false;
}

LLData
RegData::getTable(string tableName_h, int debugLevel){
	LLData toReturn(MTHREAD,"");
	for(uint i=0;i<LLDataVector.size();i++){
		if (LLDataVector[i].getTableName() == tableName_h)return LLDataVector[i];
	}
	msgOut(debugLevel,"No table found with name "+tableName_h);
	return toReturn;
}



// ========================== LLData =======================================

LLData::LLData(ThreadManager* MTHREAD_h, string tableName_h){
	MTHREAD = MTHREAD_h;
	tableName = tableName_h;
}

LLData::~LLData(){

}

void
LLData::clean(){
	
	//checking the size is correct...
	int hsize = headers.size();
	for (uint i=0;i<records.size();i++){
		if(records[i].size() != hsize){
			msgOut(MSG_CRITICAL_ERROR,"Error in the input reading table "+tableName+". Record "+i2s(i)+" has "+i2s(records[i].size())+" fields instead of "+i2s(hsize)+".");
		}
	}
	//cleaning empty-header columns...
	for (int i=headers.size()-1;i>=0;i--){
		if(headers[i] == ""){
			headers.erase(headers.begin()+i);
			for (uint j=0;j<records.size();j++){
				records[j].erase(records[j].begin()+i);
			}
		}
	}

}

string
LLData::getData(int pos_h, string header_h, int debugLevel){
	if (records.size()<= pos_h){
		msgOut(debugLevel, "Requested position "+i2s(pos_h)+" too high! Not enought records !!");
		return "";
	}
	for (uint i=0;i<headers.size();i++){
		if(headers[i] == header_h) return records[pos_h][i];
	}
	msgOut(debugLevel, "Header string "+header_h+" not found!");
	return "";
}
