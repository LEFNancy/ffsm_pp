/***************************************************************************
 *   Copyright (C) 2015 by Laboratoire d'Economie Forestière               *
 *   http://ffsm-project.org                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version, given the compliance with the     *
 *   exceptions listed in the file COPYING that is distribued together     *
 *   with this file.                                                       *
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

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <algorithm> //alghoritm used to reverse an array ( reverse(v.begin(), v.end()); )
#include <iomanip> // for unzip
#include <math.h>
#include <random> // for random temp directory to unzip

// Qt headers..
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QList>

// Unzip headers..
#include "unzip.h"

// RegMAS headers..
#include "ModelData.h"
//#include "InputDocument.h"
//#include "InputNode.h"
#include "MainWindow.h"
#include "Scheduler.h"
#include "ModelRegion.h"
#include "Pixel.h"

typedef map<string, vector <double> > DataMap;
typedef pair<string, vector <double> > DataPair;



ModelData::ModelData(ThreadManager* MTHREAD_h){
  MTHREAD        = MTHREAD_h;
  errorLevel     = MSG_ERROR;
  tempBool       = true;
  valueFoundBool = true;
}

ModelData::~ModelData(){

}

forType*
ModelData::getForType(string &forTypeId_h){
  for(int i=0;i<forTypes.size();i++){
    if(forTypes[i].forTypeId==forTypeId_h) return &forTypes[i];
  }
  msgOut(MSG_CRITICAL_ERROR,"forTypeId "+forTypeId_h+" not found. Aborting.");
}

int
ModelData::getForTypeCounter(string& forTypeId_h, bool all){
  vector <string> fTIds = getForTypeIds(all);
  for(int i=0;i<fTIds.size();i++){
    if(fTIds[i]==forTypeId_h) return i;
  }
  msgOut(MSG_CRITICAL_ERROR,"forTypeId "+forTypeId_h+" not found in "+((string) __func__ )+". Aborting.");
}

string
ModelData::getForTypeParentId(const string &forTypeId_h){
    for(int i=0;i<forTypes.size();i++){
        if(forTypes[i].forTypeId==forTypeId_h) return forTypes[i].ereditatedFrom;
    }
    msgOut(MSG_CRITICAL_ERROR,"forTypeId "+forTypeId_h+" not found. Aborting.");
}

vector<string>
ModelData::getForTypeChilds(const string &forTypeId_h){
  vector<string> childs;
  for(int i=0;i<forTypes.size();i++){
      if(forTypes[i].ereditatedFrom==forTypeId_h) {
          childs.push_back(forTypes[i].forTypeId);
      }
  }
  return childs;
}

vector<int>
ModelData::getForTypeChilds_pos(const string &forTypeId_h, bool all){
  vector <int> childs;
  vector <string> fTIds = getForTypeIds(all);
  for(int i=0;i<fTIds.size();i++){
      forType* ft = getForType(fTIds[i]);
      if(ft->ereditatedFrom==forTypeId_h) {
          childs.push_back(i);
      }
  }
  return childs;
}

vector<string>
ModelData::getForTypeParents(){
  vector<string> parents;
  for(int i=0;i<forTypes.size();i++){
    string parent = forTypes[i].ereditatedFrom;
    if(!inVector(parent,parents) && parent != ""){
      parents.push_back(parent);
    }
  }
  return parents;
}


int
ModelData::getNForTypesChilds(const string& forTypeId_h){
    int nChilds = 0;
    for(int i=0;i<forTypes.size();i++){
        if(forTypes[i].ereditatedFrom==forTypeId_h) {
            nChilds ++;
        }
    }
    return nChilds;
}

vector<string>
ModelData::getScenarios(){
  vector<string> toReturn;
  LLData table = getTable("scenarios");
  for(int i=0;i<table.nrecords();i++){
    string scenarioName = table.getData(i,"id");
    toReturn.push_back(scenarioName);
  }
  return toReturn;
}

int
ModelData::getScenarioIndex(){
  vector<string> scenarios = getScenarios(); /// \todo Check that I can call this function all around the model and not only at the beginning
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
ModelData::setScenarioData(){
  LLData table = getTable("scenarios");
  string choosenScenario = MTHREAD->getScenarioName();
  for(int i=0;i<table.nrecords();i++){
    string recordScenarioName = table.getData(i,"id");
    if (recordScenarioName == choosenScenario){
      scenario.id             = recordScenarioName;
      scenario.shortDesc      = table.getData(i,"shortDesc");
      scenario.longDesc       = table.getData(i,"longDesc");
      scenario.settingTable   = table.getData(i,"settingTable");
      scenario.forDataTable   = table.getData(i,"forDataTable");
      scenario.prodDataTable  = table.getData(i,"prodDataTable");
      scenario.forToProdTable = table.getData(i,"forToProdTable");
      scenario.pathTable      = table.getData(i,"pathTable");
      scenario.settingFile    = table.getData(i,"settingFile");
      return;
    }
  }
  msgOut(MSG_CRITICAL_ERROR, "Scenario not found!");


}

void 
ModelData::setDefaultSettings(){

  LLData table = getTable("settings");
  int nheaders = table.nheaders();
  for (int i=0; i< table.nrecords();i++){
    BasicData SETT;
    SETT.name = table.getData(i,"name");
    string type = table.getData(i,"type");
    SETT.type = getType(type);
    string region_s= table.getData(i,"region");
    SETT.regId = region_s==""?WORLD:s2i(table.getData(i,"region"));
    SETT.comment = table.getData(i,"comment");
    vector <string> values;
    for (int z=0;z<nheaders-4;z++){ // don't consider name, type, comment and region headers
      string toSearch = "value_"+i2s(z);
      string value = table.getData(i,toSearch);
      if (value != ""){
        values.push_back(value);
      }
    }
    SETT.values = values;
    programSettingsVector.push_back(SETT);
  }  

  msgOut(MSG_INFO,"### USING SCENARIO: "+MTHREAD->getScenarioName()+" ###");
  
  setOutputDirectory(getStringSetting("outputDirname").c_str());
}

void
ModelData::setScenarioSettings(){

  if(scenario.settingTable=="" && scenario.settingFile=="") {return;}
  if(scenario.settingTable!="" && scenario.settingFile!="") {
   msgOut(MSG_CRITICAL_ERROR,"Scenario-specific settings has been indicated to be loaded from BOTH from the spreadsheet and a file. Choose one of them.");
  }
  LLData table(MTHREAD,"settings");
  if(scenario.settingTable!=""){
    table = getTable(scenario.settingTable, MSG_CRITICAL_ERROR); //this scenario could not have an associated setting sheet
  } else {
    table = getTableFromFile("settings",scenario.settingFile);
  }



  int nheaders = table.nheaders();
  for(int i=0; i< table.nrecords(); i++){
    string name = table.getData(i,"name");
    string stype = table.getData(i,"type");
    int type = getType(stype);
    string region_s= table.getData(i,"region");
    int    regId = region_s==""?WORLD:s2i(table.getData(i,"region"));
    string comment = table.getData(i,"comment");
    vector <string> values;
    for (int z=0;z<nheaders-4;z++){ // don't consider name, type and comment headers
      string toSearch = "value_"+i2s(z);
      string value = table.getData(i,toSearch);
      if (value != ""){
        values.push_back(value);
      }
    }
    bool found = false;
    for(uint i=0;i<programSettingsVector.size();i++){
      if(programSettingsVector[i].name == name && programSettingsVector[i].regId == regId){
        programSettingsVector[i].values = values;
        programSettingsVector[i].type = type;
        programSettingsVector[i].comment = comment;
        found = true;
        break;
      }
    }
    if(!found){
      BasicData SETT;
      SETT.name = name;
      SETT.type = type;
      SETT.regId = regId;
      SETT.comment = comment;
      SETT.values = values;
      programSettingsVector.push_back(SETT);
    }

  }

  setOutputDirectory(getStringSetting("outputDirname").c_str());
}

void
ModelData::addSetting(string name_h, vector <string> values_h, int type_h, string comment_h){

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
ModelData::addSetting(string name_h, string value_h, int type_h, string comment_h){
  vector <string> values;
  values.push_back(value_h);
  addSetting(name_h, values, type_h, comment_h);
}

void
ModelData::cacheSettings(){
  cached_initialYear = getIntSetting("initialYear");
  diamClasses = getStringVectorSetting("dClasses");
  priProducts = getStringVectorSetting("priProducts");
  secProducts = getStringVectorSetting("secProducts");
  allProducts = priProducts;
  allProducts.insert( allProducts.end(), secProducts.begin(), secProducts.end() );
}

// ###############################################################################

void
ModelData::createRegions(){
  // first create regions and assign basic data...
  LLData table = getTable("regions");
  for (int i=0; i< table.nrecords();i++){
    ModelRegion REGION(MTHREAD,
               s2i(table.getData(i,"regId")),
               table.getData(i,"regSName"),
               table.getData(i,"regLName"),
               s2i(table.getData(i,"regLevel")),
               s2i(table.getData(i,"parRegId")),
               s2b(table.getData(i,"isResidual")));
    regionsVector.push_back(REGION);
  }
  // Now let's assign the parent/children pointers..
  for (int i=0; i< regionsVector.size();i++){
    // let's assign the parent:
    regionsVector[i].setParent(this->getRegion(regionsVector[i].getParRegId()));
    // let's assign the children:
    vector<ModelRegion*> kids;
    for (int y=0; y< regionsVector.size();y++){
      if(regionsVector[y].getParRegId() == regionsVector[i].getRegId() ){
        kids.push_back(&regionsVector[y]);
      }
    }
    regionsVector[i].setChildren(kids);
  }
}

ModelRegion*
ModelData::getRegion(int regId_h){
  for (int i=0; i< regionsVector.size();i++){
    if(regionsVector[i].getRegId()==regId_h){
      return &regionsVector[i];
    }
  }
  msgOut(MSG_CRITICAL_ERROR, "Region id "+i2s(regId_h)+" not found, check your input data. Aborting simulation.");
}

bool
ModelData::regionExist (const int & regId_h) const {
  for (int i=0; i< regionsVector.size();i++){
    if(regionsVector[i].getRegId()==regId_h){
      return true;
    }
  }
  return false;
}

vector <int>
ModelData::getRegionIds(int level_h, bool excludeResidual){
  vector <int> toReturn;
  for(uint i=0;i<regionsVector.size();i++){
    if(regionsVector[i].getRegLevel()==level_h){
      if( (!excludeResidual) || (!regionsVector[i].getIsResidual())){
        toReturn.push_back(regionsVector[i].getRegId());
      }
    }
  }
  return toReturn;
}

vector <ModelRegion*>
ModelData::getAllRegions(bool excludeResidual){
  vector <ModelRegion*> toReturn;
  for(uint i=0;i<regionsVector.size();i++){
    if( (!excludeResidual) || (!regionsVector[i].getIsResidual())){
      toReturn.push_back(&regionsVector[i]);
    }
  }
  return toReturn;
}

vector < vector <int> >
ModelData::getRegionIds( bool excludeResidual){
  vector < vector <int> > toReturn;
  vector <int> l1regIds =  MTHREAD->MD->getRegionIds(1, excludeResidual);
  for(uint i=0;i<l1regIds.size();i++){
    vector<int> l2ChildrenIds;
    ModelRegion* l1Region = MTHREAD->MD->getRegion(l1regIds[i]);
    vector<ModelRegion*> l2Childrens = l1Region->getChildren(excludeResidual);
    for(uint j=0;j<l2Childrens.size();j++){
      l2ChildrenIds.push_back(l2Childrens[j]->getRegId());
    }
    if(l2ChildrenIds.size()){
      toReturn.push_back(l2ChildrenIds);
    }
  }
  return toReturn;
}

string
ModelData::regId2RegSName (const int & regId_h) const {
  ModelRegion* reg = MTHREAD->MD->getRegion(regId_h);
  return reg->getRegSName();
}

int
ModelData::regSName2RegId (const string & regSName_h) const{
  ModelRegion* reg;
  for(uint i=0; i<3; i++){
    vector <int> regIds =  MTHREAD->MD->getRegionIds(i, false);
    for(uint j=0;j<regIds.size();j++){
      reg = MTHREAD->MD->getRegion(regIds[j]);
      if(reg->getRegSName()==regSName_h) {return regIds[j];}
    }
  }
  msgOut(MSG_CRITICAL_ERROR,"Regional short name not found.");
}




vector <string>
ModelData::getForTypeIds(bool all){
  vector <string> toReturn;
  for(uint i=0;i<forTypes.size();i++){
    if(forTypes[i].memType!=1 || all) {
      toReturn.push_back(forTypes[i].forTypeId);
    }
  }
  return toReturn;
}

const bool
ModelData::assessProdPossibility(const string &prod_h, const string &forType_h, const string &dClass_h){
  bool ok=false;
  for(uint i=0;i<forToProdVector.size();i++){
    if(    forToProdVector[i].product == prod_h
      && forToProdVector[i].forType == forType_h
      && forToProdVector[i].dClass  == dClass_h
    ){
      return true;
    }
  }
  return false;
}


const int
ModelData::getMaxYearUsableDeathTimber(){
  int maxMaxYears = 0;
  for(uint i=0;i<forToProdVector.size();i++){
    if(forToProdVector[i].maxYears > maxMaxYears){
      maxMaxYears = forToProdVector[i].maxYears;
    }
  }
  return maxMaxYears;
}


const int
ModelData::getMaxYearUsableDeathTimber(const string &prod_h, const string &forType_h, const string &dClass_h){
  for(uint i=0;i<forToProdVector.size();i++){
    if(    forToProdVector[i].product == prod_h
      && forToProdVector[i].forType == forType_h
      && forToProdVector[i].dClass  == dClass_h
    ){
      return forToProdVector[i].maxYears;
    }
  }
  msgOut(MSG_CRITICAL_ERROR,"In getMaxYearUsableDeathTimber() I has been asked of a combination that I don't know how to handle.");
}

void
ModelData::setDefaultForData(){
  msgOut(MSG_DEBUG,"Loading forest sector data..");
  LLData table = getTable("forData");
  int nheaders = table.nheaders();
  for (int i=0; i< table.nrecords();i++){
    vector <double> values;
    for (int z=0;z<nheaders-4;z++){ // don't consider parName, region, forType and diamClass headers
      string toSearch = "value_"+i2s(z);
      string value = table.getData(i,toSearch);
      if (value != ""){
        values.push_back(s2d(value));
      }
    }
        string keys = makeKeyForData(table.getData(i,"parName"), table.getData(i,"region"),table.getData(i,"forType"),table.getData(i,"freeDim"));
    forDataMap.insert(std::pair<string, vector<double> >(keys, values));
  }
}

void
ModelData::setScenarioForData(){

  if(scenario.forDataTable==""){return;}
  LLData table = getTable(scenario.forDataTable, MSG_CRITICAL_ERROR);

  int nheaders = table.nheaders();
  for(int i=0; i< table.nrecords(); i++){
    bool found = false;
        string key = makeKeyForData(table.getData(i,"parName"),table.getData(i,"region"),table.getData(i,"forType"),table.getData(i,"freeDim"));
    vector <double> values;
    for (int z=0;z<nheaders-4;z++){ // don't consider parName, region, forType and diamClass headers
      string toSearch = "value_"+i2s(z);
      string value = table.getData(i,toSearch);
      if (value != ""){
        values.push_back(s2d(value));
      }
    }
    map <string, vector < double > >::iterator p;
    p=forDataMap.find(key);
    if(p != forDataMap.end()) {
      // updating an existing record
      p->second = values;
    }
    else {
      // new one, adding it
      forDataMap.insert(std::pair<string, vector<double> >(key, values));
    }
  }
}

void
ModelData::setDefaultProdData(){

  msgOut(MSG_DEBUG,"Loading products data..");
  LLData table = getTable("prodData");
  int nheaders = table.nheaders();

  for (int i=0; i< table.nrecords();i++){
//    prodData PDATA;
//    PDATA.parName = table.getData(i,"parName");
//    PDATA.region = s2i(table.getData(i,"region"));
//    PDATA.prod = table.getData(i,"prod");
//    PDATA.freeDim = table.getData(i,"freeDim");
    vector <double> values;
    for (int z=0;z<nheaders-4;z++){ // don't consider parName, region, prod and freeDim headers
      string toSearch = "value_"+i2s(z);
      string value = table.getData(i,toSearch);
      if (value != ""){
        values.push_back(s2d(value));
      }
    }
//    PDATA.values = values;
//    prodDataVector.push_back(PDATA);
    string keys = makeKeyProdData(table.getData(i,"parName"), table.getData(i,"region"),table.getData(i,"prod"),table.getData(i,"freeDim"));
    prodDataMap.insert(std::pair<string, vector<double> >(keys, values));
    //giving a link to it to its own region:
//    getRegion(PDATA.region)->addProdData(&PDATA);
  }
}

void
ModelData::setScenarioProdData(){

  if(scenario.prodDataTable==""){return;}
  LLData table = getTable(scenario.prodDataTable, MSG_CRITICAL_ERROR); //this scenario could not have an associated setting sheet

  int nheaders = table.nheaders();
  int debug = table.nrecords();
  for(int i=0; i< table.nrecords(); i++){
    //prodData PDATA;
    bool found = false;
    string key = makeKeyProdData(table.getData(i,"parName"),table.getData(i,"region"),table.getData(i,"prod"),table.getData(i,"freeDim"));

    //PDATA.parName = table.getData(i,"parName");
    //PDATA.region = s2i(table.getData(i,"region"));
    //PDATA.prod = table.getData(i,"prod");
    //PDATA.freeDim = table.getData(i,"freeDim");
    vector <double> values;
    for (int z=0;z<nheaders-4;z++){// don't consider parName, region, prod and freeDim headers
      string toSearch = "value_"+i2s(z);
      string value = table.getData(i,toSearch);
      if (value != ""){
        values.push_back(s2d(value));
      }
    }
    //PDATA.values = values;
    //for(uint i=0;i<prodDataVector.size();i++){
    //  if(prodDataVector[i].parName == PDATA.parName
    //     && prodDataVector[i].region == PDATA.region
    //     && prodDataVector[i].prod == PDATA.prod
    //     && prodDataVector[i].freeDim == PDATA.freeDim){
    //    // existing prodData..
    //    prodDataVector[i].values = PDATA.values;
    //    found = true;
    //    break;
    //  }
    //}
    //if(!found){
    //  // new one, adding it
    //  prodDataVector.push_back(PDATA);
    //  //giving a link to it to its own region:
    //  getRegion(PDATA.region)->addProdData(&PDATA);
    //}

    map <string, vector < double > >::iterator p;
    p=prodDataMap.find(key);
    if(p != prodDataMap.end()) {
      // updating an existing record
      p->second = values;
    }
    else {
      // new one, adding it
      prodDataMap.insert(std::pair<string, vector<double> >(key, values));
    }
  }
}

void
ModelData::setDefaultProductResourceMatrixLink(){
  msgOut(MSG_DEBUG,"Loading forest resource to primary products io matrix..");
  LLData table = getTable("forToProd");
  for (int i=0; i< table.nrecords();i++){
    forToProd F2PDATA;
    F2PDATA.product = table.getData(i,"product");
    F2PDATA.forType = table.getData(i,"forType");
    F2PDATA.dClass = table.getData(i,"dClass");
    F2PDATA.maxYears = s2i(table.getData(i,"maxYears"));
    forToProdVector.push_back(F2PDATA);
  }
}

void
ModelData::setScenarioProductResourceMatrixLink(){
  if(scenario.forToProdTable==""){return;}
  LLData table = getTable(scenario.forToProdTable, MSG_CRITICAL_ERROR); //this scenario could not have an associated setting sheet

  int nheaders = table.nheaders();
  forToProdVector.clear();
  for (int i=0; i< table.nrecords();i++){
    forToProd F2PDATA;
    F2PDATA.product = table.getData(i,"product");
    F2PDATA.forType = table.getData(i,"forType");
    F2PDATA.dClass = table.getData(i,"dClass");
    F2PDATA.maxYears = s2i(table.getData(i,"maxYears"));
    forToProdVector.push_back(F2PDATA);
  }
}

void
ModelData::setForestTypes(){
  LLData table = getTable("forTypes");
  for (int i=0; i< table.nrecords();i++){
    forType FTYPE;
    FTYPE.forTypeId = table.getData(i,"forTypeId");
    FTYPE.forLabel = table.getData(i,"forLabel");
    FTYPE.memType = s2i(table.getData(i,"memType"));
    FTYPE.forLayer = table.getData(i,"forLayer");
    FTYPE.ereditatedFrom = table.getData(i,"ereditatedFrom");
    if(FTYPE.memType == 3 && !getBoolSetting("useSpExplicitForestTypes")) continue;
    forTypes.push_back(FTYPE);
  }
}

void
ModelData::setReclassificationRules(){

  msgOut(MSG_DEBUG,"Loading (but not yet applying) reclassification rules..");
  LLData table = getTable("reclRules");
  for (int i=0; i< table.nrecords();i++){
    reclRule RL;
    RL.regId = s2i(table.getData(i,"regID"));
    RL.forTypeIn = table.getData(i,"forTypeIn");
    RL.forTypeOut = table.getData(i,"forTypeOut");
    RL.coeff = s2d(table.getData(i,"coeff"));
    reclRules.push_back(RL);
  }
}

void
ModelData::setDefaultPathogenRules(){

  if(!getBoolSetting("usePathogenModule")) return;
  msgOut(MSG_DEBUG,"Loading pathogen rules..");
  LLData table = getTable("pathRules");
  int nheaders = table.nheaders();
  for (int i=0; i< table.nrecords();i++){
    pathRule PR;
    PR.forType = table.getData(i,"forType");
    PR.dClass = table.getData(i,"dClass");
    PR.pathId = table.getData(i,"path_name");
    PR.pres_min = s2d(table.getData(i,"pres_min"));

    vector <double> values;
    for (int z=0;z<nheaders-4;z++){ // don't consider forType, dClass, path_name and pres_min  headers
      string toSearch = "year_"+i2s(z);
      string value = table.getData(i,toSearch);
      if (value != ""){
        values.push_back(s2d(value));
      }
    }
    PR.mortCoefficents = values;

    pathRules.push_back(PR);
  }
}

void
ModelData::setScenarioPathogenRules(){

  if(scenario.pathTable==""){return;}
  LLData table = getTable(scenario.pathTable, MSG_CRITICAL_ERROR); //this scenario could not have an associated setting sheet

  int nheaders = table.nheaders();
  for (int i=0; i< table.nrecords();i++){
    pathRule PR;
    PR.forType = table.getData(i,"forType");
    PR.dClass = table.getData(i,"dClass");
    PR.pathId = table.getData(i,"path_name");
    PR.pres_min = s2d(table.getData(i,"pres_min"));

    vector <double> values;
    for (int z=0;z<nheaders-4;z++){ // don't consider forType, dClass, path_name and pres_min  headers
      string toSearch = "year_"+i2s(z);
      string value = table.getData(i,toSearch);
      if (value != ""){
        values.push_back(s2d(value));
      }
    }
    PR.mortCoefficents = values;

    bool found = false;
    for(uint i=0;i<pathRules.size();i++){
      if(    pathRules[i].forType == PR.forType
          && pathRules[i].dClass == PR.dClass
          && pathRules[i].pathId == PR.pathId
         ){
        pathRules[i].pres_min = PR.pres_min;
        pathRules[i].mortCoefficents = PR.mortCoefficents;
        found = true;
        break;
      }
    }
    if(!found){
      pathRules.push_back(PR);
    }
  } // end for each table record
}

/// Cancel all reg1 level data and trasform them in reg2 level if not already existing
void
ModelData::applyOverrides(){

  if(!getBoolSetting("applyOverriding")) return;
  msgOut(MSG_INFO, "Starting regional overriding analysis..");

  DataMap::iterator p;
  string parName,prod,freeDim,forType,diamClass, key;
  int regId;
  DataMap toBeAdded;
  vector <string> keysToRemove;


  //apply override from level 0 to level 1 for forestry data
  toBeAdded.clear();
  keysToRemove.clear();
  for(p=forDataMap.begin();p!=forDataMap.end();p++){
    unpackKeyForData(p->first,parName,regId,forType,diamClass);
    if(!regionExist(regId)){
      msgOut(MSG_CRITICAL_ERROR,"Forest variable has a unknown region code If you want not to model a region, set residual=true.: "+parName+"\t"+i2s(regId));
      continue;
    }
    if(getRegion(regId)->getRegLevel() == 0){
      vector<ModelRegion*> childs = getRegion(regId)->getChildren(false);
      for(uint j=0;j<childs.size();j++){
        bool found = false;
        key = makeKeyForData(parName,i2s(childs[j]->getRegId()),forType,diamClass);
        if (!dataMapCheckExist(forDataMap,key,true)){
          toBeAdded.insert(DataPair(key,p->second));
        }
      }
      keysToRemove.push_back(p->first);
    }
  }
  forDataMap.insert(toBeAdded.begin(),toBeAdded.end());
  for(uint i=0;i<keysToRemove.size();i++){
    DataMap::iterator rem = forDataMap.find(keysToRemove[i]);
    if(rem != forDataMap.end()){
      forDataMap.erase(rem);
    }
  }




  //apply override from level 1 to level 2 for forestry data
  toBeAdded.clear();
  keysToRemove.clear();
  for(p=forDataMap.begin();p!=forDataMap.end();p++){
    unpackKeyForData(p->first,parName,regId,forType,diamClass);
    if(!regionExist(regId)){
      msgOut(MSG_CRITICAL_ERROR,"Forest variable has a unknown region code If you want not to model a region, set residual=true.: "+parName+"\t"+i2s(regId));
      continue;
    }
    if(getRegion(regId)->getRegLevel() == 1){
      vector<ModelRegion*> childs = getRegion(regId)->getChildren(false);
      for(uint j=0;j<childs.size();j++){
        bool found = false;
        key = makeKeyForData(parName,i2s(childs[j]->getRegId()),forType,diamClass);
        if (!dataMapCheckExist(forDataMap,key,true)){
          toBeAdded.insert(DataPair(key,p->second));
        }
      }
      keysToRemove.push_back(p->first);
    }
  }
  forDataMap.insert(toBeAdded.begin(),toBeAdded.end());
  for(uint i=0;i<keysToRemove.size();i++){
    DataMap::iterator rem = forDataMap.find(keysToRemove[i]);
    if(rem != forDataMap.end()){
      forDataMap.erase(rem);
    }
  }

  //apply override from level 0 to level 1 for production data
  toBeAdded.clear();
  keysToRemove.clear();
  for(p=prodDataMap.begin();p!=prodDataMap.end();p++){
    unpackKeyProdData(p->first,parName,regId,prod,freeDim);
    if(!regionExist(regId)){
      msgOut(MSG_CRITICAL_ERROR,"Product variable has a unknown region code If you want not to model a region, set residual=true.: "+parName+"\t"+i2s(regId));
      continue;
    }
    if(getRegion(regId)->getRegLevel() == 0){
      vector<ModelRegion*> childs = getRegion(regId)->getChildren(false);
      for(uint j=0;j<childs.size();j++){
        bool found = false;
        key = makeKeyProdData(parName,i2s(childs[j]->getRegId()),prod,freeDim);
        if (!dataMapCheckExist(prodDataMap,key,true)){
          toBeAdded.insert(DataPair(key,p->second));
        }
      }
      //prodDataMap.erase(p);
      //p--;
      keysToRemove.push_back(p->first);
    }
  }
  prodDataMap.insert(toBeAdded.begin(),toBeAdded.end());
  for(uint i=0;i<keysToRemove.size();i++){
    DataMap::iterator rem = prodDataMap.find(keysToRemove[i]);
    if(rem != prodDataMap.end()){
      prodDataMap.erase(rem);
    }
  }


  //apply override from level 1 to level 2 for production data
  toBeAdded.clear();
  keysToRemove.clear();
  for(p=prodDataMap.begin();p!=prodDataMap.end();p++){
    string debug = p->first;
    unpackKeyProdData(p->first,parName,regId,prod,freeDim);
    if(!regionExist(regId)){
      msgOut(MSG_CRITICAL_ERROR,"Product variable has a unknown region code If you want not to model a region, set residual=true.: "+parName+"\t"+i2s(regId));
      continue;
    }
    if(getRegion(regId)->getRegLevel() == 1){
      vector<ModelRegion*> childs = getRegion(regId)->getChildren(false);
      for(uint j=0;j<childs.size();j++){
        bool found = false;
        key = makeKeyProdData(parName,i2s(childs[j]->getRegId()),prod,freeDim);
        if (!dataMapCheckExist(prodDataMap,key,true)){
          toBeAdded.insert(DataPair(key,p->second));
        }
      }
      //prodDataMap.erase(p);
      //p--;
      keysToRemove.push_back(p->first);
    }
  }
  prodDataMap.insert(toBeAdded.begin(),toBeAdded.end());
    for(uint i=0;i<keysToRemove.size();i++){
    DataMap::iterator rem = prodDataMap.find(keysToRemove[i]);
    if(rem != prodDataMap.end()){
      prodDataMap.erase(rem);
    }
  }

  //apply override from level 0 to level 1 for reclassification rules
  for(uint i=0;i<reclRules.size();i++){
    if(reclRules[i].regId == 0){
      if(!regionExist(reclRules[i].regId)){
        msgOut(MSG_CRITICAL_ERROR,"Reclassification rule has a unknown region code If you want not to model a region, set residual=true.: "+i2s(reclRules[i].regId));
        continue;
      }
      for(uint j=0;j<getRegion(reclRules[i].regId)->getNChildren(false);j++){
        vector<ModelRegion*> childs = getRegion(reclRules[i].regId)->getChildren(false);
        bool found = 0;
        for(uint z=0;z<reclRules.size();z++){
          if(   reclRules[z].regId == childs[j]->getRegId()
             && reclRules[z].forTypeIn == reclRules[i].forTypeIn
             && reclRules[z].forTypeOut == reclRules[i].forTypeOut
          ){
            found = true; // do nothing, this child has been already manually overritten
            break;
          }
        }
        if(!found){
          reclRule RR;
          RR.regId      = childs[j]->getRegId();
          RR.forTypeIn  = reclRules[i].forTypeIn;
          RR.forTypeOut = reclRules[i].forTypeOut;
          RR.coeff      = reclRules[i].coeff;
          reclRules.push_back(RR);
        }
      }
      reclRules.erase(reclRules.begin()+i);
      i--;
    }
  }

  //apply override from level 1 to level 2 for reclassification rules
  for(uint i=0;i<reclRules.size();i++){
    if(!regionExist(reclRules[i].regId)){
      msgOut(MSG_CRITICAL_ERROR,"Reclassification rule has a unknown region code. If you want not to model a region, set residual=true.: "+i2s(reclRules[i].regId));
      continue;
    }
    if(getRegion(reclRules[i].regId)->getRegLevel() == 1){
      for(uint j=0;j<getRegion(reclRules[i].regId)->getNChildren(false);j++){
        vector<ModelRegion*> childs = getRegion(reclRules[i].regId)->getChildren(false);
        bool found = 0;
        for(uint z=0;z<reclRules.size();z++){
          if(   reclRules[z].regId == childs[j]->getRegId()
             && reclRules[z].forTypeIn == reclRules[i].forTypeIn
             && reclRules[z].forTypeOut == reclRules[i].forTypeOut
          ){
            found = true; // do nothing, this child has been already manually overritten
            break;
          }
        }
        if(!found){
          reclRule RR;
          RR.regId      = childs[j]->getRegId();
          RR.forTypeIn  = reclRules[i].forTypeIn;
          RR.forTypeOut = reclRules[i].forTypeOut;
          RR.coeff      = reclRules[i].coeff;
          reclRules.push_back(RR);
        }
      }
      reclRules.erase(reclRules.begin()+i);
      i--;
    }
  }
}

/**
The applyDebugMode flag all level2 regions not in the "debugRegions" option as "residual" (so they are in the map but not in the model code) and remove the primary and secondary products that are not included in the debugPriProducts and debugSecProducts options.
*/
void
ModelData::applyDebugMode(){
  if(! getBoolSetting("debugFlag")) return;

  vector <int> debugRegions = getIntVectorSetting("debugRegions");
  vector <string> debugPriProducts = getStringVectorSetting("debugPriProducts");
  vector <string> debugSecProducts = getStringVectorSetting("debugSecProducts");

  for(uint i=0;i< regionsVector.size();i++){
    if (regionsVector[i].getRegLevel()==2){
      bool found= false;
      for(uint j=0;j<debugRegions.size();j++){
        if (debugRegions[j] == regionsVector[i].getRegId()){
          found = true;
          break;
        }
      }
      if(!found){ // not in the list to keep
        regionsVector[i].setIsResidual(true);
      }
    }
  }

  for (uint i=0; i<programSettingsVector.size();i++){
    if (programSettingsVector.at(i).name == "priProducts"){
      programSettingsVector.at(i).values = debugPriProducts;
    } else if (programSettingsVector.at(i).name == "secProducts"){
      programSettingsVector.at(i).values = debugSecProducts;
    }
  }

}

void
ModelData::setOutputDirectory(const char* output_dirname_h){

  if (strlen(output_dirname_h)==0){
    outputDirname=baseDirectory+"output/";
  }
  else {
    outputDirname=output_dirname_h;
  }  
  MTHREAD->setOutputDirName(outputDirname); //for the GUI
}  

/**
Returns the value stored in the settings database.


Regional overriding.

Concerning the param regId_h:
it expect either world (default), a L2 or L1 region.
Settings in the memory are stored also with data in one or more of these levels (e.g. we may have a general WORLD value and then a more specific L2 value.
The function is done such that if WORLD is used to query it, it returns only a world value, if L1 is used instead, it looks for this L1 value and then eventually
(if nothing is found) for WORLD. Finally, if L2 is used, it query, in the order, for L2, L1 or world levels.

*/
string
ModelData::getBaseData (const string &name_h, int type_h, int position, int regId_h){
  // If the data is called with DATA_NOW we interpret the array of values as a temporal array and we return the value at the current time.
  if(position == DATA_NOW) {
    position = MTHREAD->SCD->getIteration();
  }

  vector <int> regIds;

  if(regId_h == WORLD){
    regIds.push_back(WORLD);
  } else if (getRegion(regId_h)->getRegLevel()==1){
    regIds.push_back(regId_h);
    regIds.push_back(WORLD);
  } else if (getRegion(regId_h)->getRegLevel()==2) {
    regIds.push_back(regId_h);
    regIds.push_back(getRegion(regId_h)->getParRegId());
    regIds.push_back(WORLD);
  } else {
    msgOut(MSG_CRITICAL_ERROR, "Error in getBaseData(). Data requested for a unknown region level.");
  }


  for (uint j=0; j<regIds.size();j++){
    int regId = regIds[j];
    for (uint i=0; i<programSettingsVector.size();i++){
      if (programSettingsVector.at(i).name == name_h & programSettingsVector.at(i).regId == regId){
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
  }

  msgOut(MSG_CRITICAL_ERROR, "Error calling getBaseData() for "+ name_h +". No setting option or macro data found with this name.");

//  switch(type_h){
//    case TYPE_BOOL:
//      return "0";
//    case TYPE_INT:
//      return "0";
//    case TYPE_DOUBLE:
//      return "0.0";
//    case TYPE_STRING:
//      return "";
//    default:
//      msgOut(MSG_CRITICAL_ERROR, "Error calling getBaseData() for "+ i2s(type_h) +". I don't know this type.");
//  }
  return "";
}


vector <string>
ModelData::getVectorBaseData (const string &name_h, int type_h, int regId_h){
  vector <int> regIds;
  if(regId_h == WORLD){
    regIds.push_back(WORLD);
  } else if(getRegion(regId_h)->getRegLevel()==1){
    regIds.push_back(regId_h);
    regIds.push_back(WORLD);
  } else if (getRegion(regId_h)->getRegLevel()==2) {
    regIds.push_back(regId_h);
    regIds.push_back(getRegion(regId_h)->getParRegId());
    regIds.push_back(WORLD);
  } else {
    msgOut(MSG_CRITICAL_ERROR, "Error in getBaseData(). Data requested for a unknown region level.");
  }

  for (uint j=0; j<regIds.size();j++){
    int regId = regIds[j];
    for (uint i=0; i<programSettingsVector.size();i++){
      if (programSettingsVector.at(i).name == name_h & programSettingsVector.at(i).regId == regId){
        int type = programSettingsVector.at(i).type;
        if(type != type_h){msgOut(MSG_CRITICAL_ERROR, "mismatching type in calling getVectorBaseData() for "+name_h);}
        return programSettingsVector.at(i).values;
      }
    }
  }
  msgOut(MSG_CRITICAL_ERROR, "Error calling getVectorBaseData() for "+ name_h +". No setting option or macro data found with this name.");
  vector <string> toReturn;
  return toReturn;
}

// ------------- start getSetting() amd getMacro() functions ....... -----------------
int
ModelData::getIntSetting(const string &name_h, int position, int reg) const{
  return s2i( MTHREAD->MD->getBaseData(name_h,TYPE_INT,position, reg) );
}
double
ModelData::getDoubleSetting(const string &name_h, int position, int reg) const{
  return s2d( MTHREAD->MD->getBaseData(name_h,TYPE_DOUBLE,position,reg) );
}
string
ModelData::getStringSetting(const string &name_h, int position, int reg) const{
  return MTHREAD->MD->getBaseData(name_h,TYPE_STRING,position,reg);
}
bool
ModelData::getBoolSetting(const string &name_h, int position, int reg) const{
  return s2b( MTHREAD->MD->getBaseData(name_h,TYPE_BOOL,position,reg) );
}
vector<int>
ModelData::getIntVectorSetting(const string &name_h, int reg) const{
  return s2i(MTHREAD->MD->getVectorBaseData(name_h,TYPE_INT,reg));
}
vector<double>
ModelData::getDoubleVectorSetting(const string &name_h, int reg) const{
  return s2d(MTHREAD->MD->getVectorBaseData(name_h,TYPE_DOUBLE,reg));
}
vector<string>
ModelData::getStringVectorSetting(const string &name_h, int reg) const{
  return MTHREAD->MD->getVectorBaseData(name_h,TYPE_STRING,reg);
}
vector<bool>
ModelData::getBoolVectorSetting(const string &name_h, int reg) const{
  return s2b(MTHREAD->MD->getVectorBaseData(name_h,TYPE_BOOL,reg));
}

// ------ END of getSetting() functions --------------------- 

//// Never used
//void
//ModelData::setBasicData(const string &name_h, int value, int position){
//  setBasicData(name_h, i2s(value), TYPE_INT, position);
//}
//void
//ModelData::setBasicData(const string &name_h, double value, int position){
//  setBasicData(name_h, d2s(value), TYPE_DOUBLE, position);
//}
//void
//ModelData::setBasicData(const string &name_h, string value, int position){
//  setBasicData(name_h, value, TYPE_STRING, position);
//}
//void
//ModelData::setBasicData(const string &name_h, bool value, int position){
//  setBasicData(name_h, b2s(value), TYPE_BOOL, position);
//}

//void
//ModelData::setBasicData(const string &name_h, string value, int type_h, int position){
//  for (uint i=0; i<programSettingsVector.size();i++){
//    if (programSettingsVector.at(i).name == name_h){
//      int type = programSettingsVector.at(i).type;
//      if(type != type_h){msgOut(MSG_CRITICAL_ERROR, "mismatching type in calling setBasicData() for "+name_h);}
//      if(programSettingsVector.at(i).values.size() > ((uint)position)) {
//        programSettingsVector.at(i).values.at(position)=value;
//        return;
//      }
//      else {msgOut(MSG_CRITICAL_ERROR, "out-of-bound error calling setBasicData() for "+name_h); }
//    }
//  }
//  msgOut(MSG_CRITICAL_ERROR, "Error calling setBasicData() for "+ name_h +". No setting option or macro data found with this name.");
//  return;
//}

std::string
ModelData::getFilenameByType(std::string type_h){
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
}

vector <string>
ModelData::getDiameterClasses(bool productionOnly){
  int i;
  if(productionOnly){
    i=1;
  } else {
    i=0;
  }
  vector <string> toReturn;
  for (i;i<diamClasses.size();i++){
    toReturn.push_back(diamClasses[i]);
  }
  return toReturn;
}

/**
Basic function to retrieve products-related data.
It addmits the following "filters":
@type_h Name of the specific parameter requested
@regId_h Look for level1 or level 2 region.
@prodId_h Product. It accept three keywords, for summing up all products, primary products or secondary products, namelly PROD_ALL, PROD_PRI, PROD_SEC.
@year Unless specified, get the value of the current year. If array is smaller (e.g. because it is time-independent), get the last value.
@freeDim_h If specified, look exactly for it, otherwise simply doesn't filter for it.

*/
const double
ModelData::getProdData(const string &type_h, const int& regId_h, const string &prodId_h, const int& year, const string &freeDim_h) {

  double value=0;
  vector <int> regIds;
  string key;
  DataMap::const_iterator p;

  bool found = false;
  vector <string> products;
  bool exactMatch=true;

  if(prodId_h == PROD_PRI){
    products = priProducts;
  } else if (prodId_h == PROD_SEC){
    products = secProducts;
  } else if (prodId_h == PROD_ALL || prodId_h == ""){
    products = allProducts;
    products.push_back("");
  } else  {
    products.push_back(prodId_h);
  }
  if(freeDim_h=="") exactMatch=false;

  // Make sure to set the new value to all l2 regions if requested for a reg1 level
  if(getRegion(regId_h)->getRegLevel()==2){
    regIds.push_back(regId_h);
  } else if (getRegion(regId_h)->getRegLevel()==1) {
    for(uint i=0;i<getRegion(regId_h)->getNChildren();i++){
      regIds.push_back(getRegion(regId_h)->getChildren()[i]->getRegId());
    }
  } else {
    msgOut(MSG_CRITICAL_ERROR, "Error in setProdData(). Setting a value for the whole World is not supported.");
  }
  int regIdsS = regIds.size();


  for(uint r=0;r<regIdsS;r++){
    for(uint i=0;i<products.size();i++){
      key = makeKeyProdData(type_h,i2s(regIds[r]),products[i],freeDim_h);
      if (!exactMatch && key.size () > 0)  key.resize (key.size () - 1); // bug 20140402, removing the last #
      value += dataMapGetValue(prodDataMap,key,year,exactMatch);
      if(tempBool) found = true;
    }
  }

  if(!found){
    valueFoundBool = false;
    msgOut(errorLevel, "Error in getProdData: no combination found for "+type_h+", "+i2s(regId_h)+", "+prodId_h+", "+i2s(year)+", "+freeDim_h+". Returning 0, but double check that this is ok for your model.");
  }
  return value;


}

/**
Basic function to retrieve forest-related data.
It addmits the following "filters":
@type_h    Name of the specific parameter requested
@regId_h   Look for a level1 or level2 region
@forType_h If specified, look exactly for the specified forest type, otherwise accept the keyword FT_ALL for summing all of them
@freeDim_h Normally used for diameter class, but occasionally used for other uses (changed 20140514). It accepts three keywords, for summing up all diameters, production-ready diameters or sub-production ones, namelly DIAM_ALL, DIAM_PROD, DIAM_FIRST.\\
If a diameter-independed variable is required, put it in the data with an empty diameter class and retrieve it here using DIAM_ALL.
@year      Unless specified, get the value of the current year. If array is smaller (e.g. because it is time-independent), get the last value.
*/
const double
ModelData::getForData(const string &type_h, const int& regId_h, const string &forType_h, const string &freeDim_h, const int& year){
  vector<int> regIds;
  vector <string> dClasses;
  vector <string> fTypes;
  string key;
  DataMap::const_iterator p;
  bool found = false;
  double value = 0;

  // creating the arrays to look up if keywords were specified..
  if (forType_h == FT_ALL){ // || forType_h == ""){
    fTypes = getForTypeIds();
    fTypes.push_back("");
  } else {
    fTypes.push_back(forType_h);
  }
  if(freeDim_h == DIAM_ALL){ // || freeDim_h == ""){
    dClasses = diamClasses;
    dClasses.push_back("");
  } else if (freeDim_h == DIAM_PROD){
    dClasses = getDiameterClasses(true);
  } else if (freeDim_h == DIAM_FIRST){
    dClasses.push_back(diamClasses.at(0));
  } else  {
    dClasses.push_back(freeDim_h);
  }
  // Make sure to set the new value to all l2 regions if requested for a reg1 level
  if(getRegion(regId_h)->getRegLevel()==2){
    regIds.push_back(regId_h);
  } else if (getRegion(regId_h)->getRegLevel()==1) {
    for(uint i=0;i<getRegion(regId_h)->getNChildren();i++){
      regIds.push_back(getRegion(regId_h)->getChildren()[i]->getRegId());
    }
  } else {
    msgOut(MSG_CRITICAL_ERROR, "Error in getProdData(). Setting a value for the whole World is not supported.");
  }
  int regIdsS = regIds.size();

  // getting the actual data...
  for(uint r=0;r< regIds.size();r++){
    for(uint i=0;i<dClasses.size();i++){
      for (uint y=0;y<fTypes.size();y++){
        key = makeKeyForData(type_h,i2s(regIds[r]),fTypes[y],dClasses[i]);
        value += dataMapGetValue(forDataMap,key,year,true);
        if(tempBool) found = true;
      }
    }
  }

  if(!found){
    valueFoundBool = false;
    msgOut(errorLevel, "Error in getForData(): no combination found for "+type_h+", "+i2s(regId_h)+", "+forType_h+", "+i2s(year)+", "+freeDim_h+". Returning 0, but double check that this is ok for your model.");
  }
  return value;
}


/**
Basic function to set products-related data.
It can change an existing value or extend in time a serie, but it requires the keys (par. name/regId/prodId/freedim) to be already present in the data.
@value_h New value to change with/add
It addmits the following "filters":
@type_h Name of the specific parameter requested
@regId_h Set a specific level 2 region, or all its childred l2 region if a reg1 level is specified.
@prodId_h Product. It accept three keywords, for changing/inserting the new value to all products, primary products or secondary products, namelly PROD_ALL, PROD_PRI, PROD_SEC.
@year Unless specified, set the value of the current year. If array is smaller (e.g. because it is time-independent) fill all the values till the requested one.
@create If true, allow creation of new data if not found. Default false (rise an error)
@freeDim_h If specified, look exactly for it, otherwise simply doesn't filter for it.

*/
void
ModelData::setProdData(const double& value_h, const string &type_h, const int& regId_h, const string &prodId_h, const int& year, const bool& allowCreate, const string &freeDim_h){

  vector<int> regIds;
  string key;
  DataMap::const_iterator p;
  vector <string> products;

  if(prodId_h == PROD_PRI){
    products = priProducts;
  } else if (prodId_h == PROD_SEC){
    products = secProducts;
  } else if (prodId_h == PROD_ALL){
    products = allProducts;
  } else  {
    products.push_back(prodId_h);
  }

  // Make sure to set the new value to all l2 regions if requested fora reg1 level
  if(getRegion(regId_h)->getRegLevel()==2){
    regIds.push_back(regId_h);
  } else if (getRegion(regId_h)->getRegLevel()==1) {
    for(uint i=0;i<getRegion(regId_h)->getNChildren();i++){
      regIds.push_back(getRegion(regId_h)->getChildren()[i]->getRegId());
    }
  } else {
    msgOut(MSG_CRITICAL_ERROR, "Error in setProdData(). Setting a value for the whole World is not supported.");
  }

  bool found = false;
  bool tempFound = false;

  for(uint r=0;r< regIds.size();r++){
    for(uint i=0;i<products.size();i++){
      key = makeKeyProdData(type_h,i2s(regIds[r]),products[i],freeDim_h);
      tempFound = dataMapSetValue(prodDataMap,key,value_h, year,true);
      if(tempFound) found = true;
    }
  }

  if(!found){
    if(!allowCreate){
      msgOut(MSG_CRITICAL_ERROR, "Error in setProdData: no combination found for "+type_h+", "+i2s(regId_h)+", "+prodId_h+", "+i2s(year)+", "+freeDim_h+". You can allow new variables to be created using the \"allowCreate\" flag.");
    } else {
      for(uint r=0;r< regIds.size();r++){
        for(uint i=0;i<products.size();i++){
          key = makeKeyProdData(type_h,i2s(regIds[r]),products[i],freeDim_h);
          vector <double> values;
          setTimedData(value_h,values,year,MSG_NO_MSG);
          prodDataMap.insert(DataPair(key,values));
        }
      }
    }
  }

}




void
ModelData::setForData(const double& value_h, const string &type_h, const int& regId_h, const string &forType_h, const string &freeDim_h, const int& year, const bool& allowCreate){

  vector<int> regIds;
  vector <string> dClasses;
  vector <string> fTypes;
  string key;
  DataMap::const_iterator p;
  bool found = false;
  bool tempFound = false;

  if (forType_h == FT_ALL){
    fTypes = getForTypeIds();
  } else {
    fTypes.push_back(forType_h);
  }

    if(freeDim_h == DIAM_ALL){
    dClasses = diamClasses;
    } else if (freeDim_h == DIAM_PROD){
    dClasses = getDiameterClasses(true);
    } else if (freeDim_h == DIAM_FIRST){
    dClasses.push_back(diamClasses.at(0));
  } else  {
        dClasses.push_back(freeDim_h);
  }

  // Make sure to set the new value to all l2 regions if requested for a reg1 level
  if(getRegion(regId_h)->getRegLevel()==2){
    regIds.push_back(regId_h);
  } else if (getRegion(regId_h)->getRegLevel()==1) {
    for(uint i=0;i<getRegion(regId_h)->getNChildren();i++){
      regIds.push_back(getRegion(regId_h)->getChildren()[i]->getRegId());
    }
  } else {
    msgOut(MSG_CRITICAL_ERROR, "Error in setProdData(). Setting a value for the whole World is not supported.");
  }
  int regIdsS = regIds.size();

  for(uint r=0;r< regIds.size();r++){
    for(uint i=0;i<dClasses.size();i++){
      for (uint y=0;y<fTypes.size();y++){
        key = makeKeyForData(type_h,i2s(regIds[r]),fTypes[y],dClasses[i]);
        tempFound = dataMapSetValue(forDataMap,key,value_h, year,true);
        if(tempFound) found = true;
      }
    }
  }

  if(!found){
    if(!allowCreate){
            msgOut(MSG_CRITICAL_ERROR, "Error in setForData: no combination found for "+type_h+", "+i2s(regId_h)+", "+forType_h+", "+i2s(year)+", "+freeDim_h+". You can allow new variables to be created using the \"allowCreate\" flag.");
    } else {
      for(uint r=0;r< regIds.size();r++){
        for(uint i=0;i<dClasses.size();i++){
          for (uint y=0;y<fTypes.size();y++){
            key = makeKeyForData(type_h,i2s(regIds[r]),fTypes[y],dClasses[i]);
            vector <double> values;
            setTimedData(value_h,values,year,MSG_NO_MSG);
            forDataMap.insert(DataPair(key,values));
          }
        }
      }
    }
  }
}


double
ModelData::getTimedData(const vector<double> &dated_vector, const int& year_h) const{

  int position;
  if(year_h==DATA_NOW){
    position = MTHREAD->SCD->getYear()-cached_initialYear;
  } else {
    position = year_h-cached_initialYear;
  }

  if(dated_vector.size() > position) {
    return dated_vector[position];
  } else if (dated_vector.size() > 0 ){
    // returning the last available value...
    return dated_vector[dated_vector.size()-1];
  } else {
    msgOut(MSG_CRITICAL_ERROR, "Error in getTimedData: requested value doesn't have any value, even on the first position(year)!");
  }
  return 0;
}

void
ModelData::setTimedData(const double& value_h, vector<double> &dated_vector, const int& year_h, const int& MSG_LEVEL){

  int position;
  if(year_h==DATA_NOW){
    position = MTHREAD->SCD->getYear()-cached_initialYear;
  } else {
    position = year_h-cached_initialYear;
  }
  if(position < 0){ // would otherwise loop forever (unsigned comparisons below)
    msgOut(MSG_CRITICAL_ERROR, "setTimedData: year "+i2s(position+cached_initialYear)+" is before the initial year.");
    return;
  }

  int originalVectorSize = dated_vector.size();
  if(dated_vector.size() > position) {
    dated_vector[position]=value_h;
  } else {
    // extending the vector and filling it with the incoming value, but issuing a warning if done for more than one year

    for(uint i=0;i<position-originalVectorSize+1;i++){
      dated_vector.push_back(value_h);
    }
    if(position-originalVectorSize > 0 ){
      msgOut(MSG_LEVEL, "setTimedData: a dated vector has been filled several years ("+i2s(1+position-originalVectorSize)+") with incoming values to reach desidered position in time.");
    }
  }
}


void
ModelData::loadInput(){
  msgOut(MSG_INFO, "Loading input files (this can take a few minutes)...");
  //QString iFile("data/ffsmInput.ods");
  QString iFile(MTHREAD->getInputFileName().c_str());
  //cout << "PIPPO !!!!! " << MTHREAD->getInputFileName().c_str() << endl;

  //std::random_device rd;
  //std::mt19937 localgen(rd());
  std::mt19937 localgen(time(0));
  std::uniform_int_distribution<> dis(10, 1000000);
  int randomNumber = dis(localgen);

  QString oDir((MTHREAD->getBaseDirectory()+"tempInput-"+MTHREAD->getScenarioName()+i2s(randomNumber)).c_str());
  
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
  if (ec != UnZip::Ok)  {
        //cout << "Failed to open archive: " << uz.formatError(ec).toAscii().data() << endl << endl; // Qt4
        cout << "Failed to open archive: " << uz.formatError(ec).toLatin1().data() << endl << endl;  // Qt5
    //return false;
  }
  ec = uz.extractAll(oDir);
  if (ec != UnZip::Ok){
    //cout << "Extraction failed: " << uz.formatError(ec).toAscii().data() << endl << endl; // Qt4
    cout << "Extraction failed: " << uz.formatError(ec).toLatin1().data() << endl << endl;  // Qt5
    uz.closeArchive();
    //return false;
  }
  
  // loading input file into memory...
  string inputXMLFileName = MTHREAD->getBaseDirectory()+"tempInput-"+MTHREAD->getScenarioName()+i2s(randomNumber)+"/content.xml";
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
    string cachedFileName = "cachedInput/"+tableName+".csv"; // relative to the base directory, as getTableFromFile() expects
    if( (tableName == "forData" || tableName == "prodData") && QFile::exists((MTHREAD->getBaseDirectory()+cachedFileName).c_str()) )  {
      msgOut(MSG_INFO,"Attenction, using cached data (csv) for "+tableName);
      LLDataVector.push_back(MTHREAD->MD->getTableFromFile(tableName, cachedFileName));
      continue;
    }
    LLData data(MTHREAD,tables[i].getStringAttributeByName("name"));
    vector <InputNode> rows = tables[i].getNodesByName("table-row",MSG_NO_MSG,true);
    if(rows.size()<2) continue; //empty table or only with headers
    // building headers..
    vector <InputNode> cells = rows[0].getNodesByName("table-cell",MSG_NO_MSG,true);
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
      vector <InputNode> cells = rows[j].getNodesByName("table-cell",MSG_NO_MSG,true);
      //vector <InputNode> cells = rows[j].getChildNodes();
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
            // changed 20120625 as for float values the content of p is the visualised value, not the full memorised one.
            // this is strange because  tought I already tested it.. but maybe is changed the format??
            if(cells[y].getStringAttributeByName("value-type")=="float"){
              record.push_back(cells[y].getStringAttributeByName("value"));
            } else {
              record.push_back(cells[y].getNodeByName("p",MSG_NO_MSG,true).getStringContent());
            }
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
    //  cout << "["<<j<<"] " << LLDataVector[i].headers[j] << endl;
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
      //  cout << "["<<y<<"] " << LLDataVector[i].records[j][y] << endl;
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


/*
void
ModelData::loadDataFromCache(string tablename){
  msgOut(MSG_INFO,"Attenction, using cached data (csv) for "+tablename);
  string fileName = MTHREAD->getBaseDirectory()+"cachedInput/"+tablename+".csv";
  QFile file(fileName.c_str());
  if (!file.open(QFile::ReadOnly)) {
      msgOut(MSG_ERROR, "Cannot open file "+fileName+" for reading.");
  }
  QTextStream in(&file);
  LLData data(MTHREAD, tablename);
  int countRow = 0;
  while (!in.atEnd()) {
    QString line = in.readLine();
    QStringList fields = line.split(';');
    if (countRow==0){ // building headers
      for(uint i =0;i<fields.size();i++){
        data.headers.push_back(fields.at(i).toStdString());
      }
    } else {
      vector<string> record ; //= fields.toVector().toStdVector();
      unsigned int z = fields[0].toStdString().find("#");
      if( z!=string::npos && z == 0) continue; // found "#" on fist position, it's a comment!
      for(uint i =0;i<fields.size();i++){
        string field = fields.at(i).toStdString();
        replace(field.begin(), field.end(), ',', '.');
        record.push_back(field);
      }
      data.records.push_back(record);
    }
    countRow++;
  }
  data.clean();
  LLDataVector.push_back(data);

}
*/

LLData
ModelData::getTableFromFile(string tablename, string filename_h){
  string fileName = MTHREAD->getBaseDirectory()+filename_h;
  QFile file(fileName.c_str());
  if (!file.open(QFile::ReadOnly)) {
      msgOut(MSG_ERROR, "Cannot open file "+fileName+" for reading.");
  }
  QTextStream in(&file);
  LLData data(MTHREAD, tablename);
  int countRow = 0;
  while (!in.atEnd()) {
    QString line = in.readLine();
    if(line=="") continue; //skipping completelly empty lines, even without separators
    QStringList fields = line.split(';');
    bool emptyLine = true;
    if (countRow==0){ // building headers
      for(uint i =0;i<fields.size();i++){
        data.headers.push_back(fields.at(i).toStdString());
      }
    } else {
      vector<string> record ; //= fields.toVector().toStdVector();
      unsigned int z = fields[0].toStdString().find("#");
      if( z!=string::npos && z == 0) continue; // found "#" on fist position, it's a comment!
      for(uint i =0;i<fields.size();i++){
        string field = fields.at(i).toStdString();
        replace(field.begin(), field.end(), ',', '.');
        record.push_back(field);
        if(field!="") emptyLine = false;
      }
      if(!emptyLine){data.records.push_back(record);};
    }
    countRow++;
  }
  data.clean();
  return data;
}


bool
ModelData::delDir(QString dirname) {
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
ModelData::getTable(string tableName_h, int debugLevel){
  LLData toReturn(MTHREAD,"");
  for(uint i=0;i<LLDataVector.size();i++){
    if (LLDataVector[i].getTableName() == tableName_h)return LLDataVector[i];
  }
  msgOut(debugLevel,"No table found with name "+tableName_h);
  return toReturn;
}


bool
ModelData::dataMapCheckExist(const DataMap& map, const string& search_for, const bool& exactMatch) const {
  /*int dummyYear=MTHREAD->SCD->getYear();
  if(dataMapGetValue(map, search_for, dummyYear, exactMatch)==DATA_ERROR) {
    return false;
  } else {
    return true;
  }
  return false;
}*/
  bool found = false;
  DataMap::const_iterator i;
  if(!exactMatch){
    i = map.lower_bound(search_for);
    for(;i != map.end();i++){
      const string& key = i->first;
      if (key.compare(0, search_for.size(), search_for) == 0) {// Really a prefix?
          return true;
      } else {
        return false;
      }
    }
  } else {
    i = map.find(search_for);
    if (i!=map.end()){
      return true;
    }
  }
  return false;
}


double
ModelData::dataMapGetValue(const DataMap& map, const string& search_for, const int& year_h, const bool& exactMatch) {
  double toReturn = 0;
  tempBool = false;
  DataMap::const_iterator i;
  if(!exactMatch){
    i = map.lower_bound(search_for);
    for(;i != map.end();i++){
      const string& key = i->first;
      if (key.compare(0, search_for.size(), search_for) == 0) {// Really a prefix?
          tempBool = true;
          toReturn += getTimedData( i->second, year_h );
      } else {
        break;
      }
    }
  } else {
    i = map.find(search_for);
    if (i!=map.end()){
      tempBool = true;
      return  getTimedData( i->second, year_h );
    }
  }
  return toReturn;
}



int
ModelData::dataMapSetValue( DataMap& map, const string& search_for, const double& value_h, const int& year_h, const bool& exactMatch){
  bool found = false;
  DataMap::iterator i;
  if(!exactMatch){
    i = map.lower_bound(search_for);
    for(;i != map.end();i++){
      const string& key = i->first;
      if (key.compare(0, search_for.size(), search_for) == 0) {// Really a prefix?
          found = true;
          setTimedData(value_h, i->second, year_h);
      } else {
        break;
      }
    }
  } else {
    i = map.find(search_for);
    if (i!=map.end()){
      found = true;
      setTimedData(value_h, i->second, year_h, errorLevel);
    }
  }
  // removed 20120903 as the insertion of new values must be explicitly done, not in all cases we want a new insertion
  /*if(!found){
    vector < double> newValues;
    setTimedData(value_h, newValues, year_h, MSG_NO_MSG); // don't warning if we are making a multi-year value vector, as it is a new one
    map.insert(DataPair (search_for,newValues));
  }*/
  return found;
}

void
ModelData::unpackKeyProdData(const string& key, string& parName, int& regId, string& prod, string& freeDim) const{

  int parNameDelimiter = key.find("#",0);
  int regIdDelimiter = key.find("#",parNameDelimiter+1);
  int prodDelimiter = key.find("#",regIdDelimiter+1);
  int freeDimDelimiter = key.find("#",prodDelimiter+1);
  if (freeDimDelimiter == string::npos){
    msgOut(MSG_CRITICAL_ERROR, "Error in unpacking a key in the map of production data.");
  }
  parName.assign(key,0,parNameDelimiter);
  string regIdString="";
  regIdString.assign(key,parNameDelimiter+1,regIdDelimiter-parNameDelimiter-1);
  regId = s2i(regIdString);
  prod.assign(key,regIdDelimiter+1,prodDelimiter-regIdDelimiter-1);
  freeDim.assign(key,prodDelimiter+1,freeDimDelimiter-prodDelimiter-1);

}

void
ModelData::unpackKeyForData(const string& key, string& parName, int &regId, string& forType, string& diamClass) const{
  int parNameDelimiter = key.find("#",0);
  int regIdDelimiter = key.find("#",parNameDelimiter+1);
  int forTypeDelimiter = key.find("#",regIdDelimiter+1);
  int diamClassDelimiter = key.find("#",forTypeDelimiter+1);
  if (diamClassDelimiter == string::npos){
    msgOut(MSG_CRITICAL_ERROR, "Error in unpacking a key in the map of production data.");
  }
  parName.assign(key,0,parNameDelimiter);
  string regIdString="";
  regIdString.assign(key,parNameDelimiter+1,regIdDelimiter-parNameDelimiter-1);
  regId = s2i(regIdString);
  forType.assign(key,regIdDelimiter+1,forTypeDelimiter-regIdDelimiter-1);
  diamClass.assign(key,forTypeDelimiter+1,diamClassDelimiter-forTypeDelimiter-1);

}


/**
calculating the discount factor

Revenues at years n will be transforemed as average year rate as

av.y.rev = rev(n)/ ( (1+ir)^(n-1)+(1+ir)^(n-2)+(1+ir)^(n-3)+...+(1+ir)^(n-n) )

Objective is to have the present value of the final harvest (A) equal to the sum pf the present values of yearly activities (B):

\image html diagram_calculateAnnualisedEquivalent.png "Comparing present values" width=10cm

\f[ PV(A) = SUM(PV(B) \f]
\f[ A/(1+r)^n = B/(1+r)^1 + B/(1+r)^2 + … + B/(1+r)^n \f]
\f[ A/(1+r)^n = B * ( 1/(1+r)^1 + 1/(1+r)^2 + … + 1/(1+r)^n ) \f]
\f[ A/(1+r)^n = B * ( (1+r)^(n-1) + (1+r)^(n-2) + … + (1+r)^(n-n) ) \f]
\f[ B = A / ( (1+r)^(n-1) + (1+r)^(n-2) + … + (1+r)^(n-n) ) \f]

20131204. Changed for the equivalent but simpler eai = rev(t)*i / ((1+i)^t-1)

*/
double
ModelData::calculateAnnualisedEquivalent(const double& amount_h, const int& years_h, const double& ir) const {
  // modified and tested 20120912. Before it was running this formula instead:
  // av.y.rev = rev(n)/ ( (1+ir)^1+(1+ir)^2+(1+ir)^3+...+(1+ir)^n )
  // the difference is that in this way the annual equivalent that is calulated doesn't need to be further discounted for yearly activites (e.g. agric)

  //loop(fy$(ord(fy)=1),
  //  df(fy)= (1+ir)**(ord(fy));
  //);
  //loop(fy$(ord(fy)>1),
  //  df(fy)=df(fy-1)+(1+ir)**(ord(fy));
  //);
  if(years_h<0) return 0.;
  if(years_h==0) return amount_h;
  //double ir = getDoubleSetting("ir",DATA_NOW);
  double eai = amount_h * ir / (pow(1.0+ir,years_h)-1.0);

  return eai;

    /*
  vector <double> df_by;
  for(int y=0;y<years_h;y++){
    double df;
    if(y==0){
      df = pow((1+ir),y);
    } else {
      df = df_by.at(y-1)+pow((1+ir),y);
    }
    if (y==years_h-1) {
         cout << eai << "   " << amount_h/df << endl;
     return amount_h/df; // big bug 20120904
    }
    df_by.push_back(df);
  }
  exit(1);
  return 0; // never reached, just to avoid compilation warnings
    */
}

double
ModelData::calculateAnnualisedEquivalent(const double& amount_h, const double& years_h, const double& ir) const{
  //ceil(x) DNLP returns the smallest integer number greater than or equal to x
  //loop( (u,i,lambda,essence),
  //  cumTp(u,i,lambda,essence) =   ceil(cumTp(u,i,lambda,essence));
  //);
  int ceiledYear = ceil(years_h);
  return calculateAnnualisedEquivalent(amount_h, ceiledYear, ir);
}

/** Get a list of files in a directory */
int
ModelData::getFilenamesByDir (const string & dir, vector<string> &files, const string & filter){
  DIR *dp;
  struct dirent *dirp;
  if((dp  = opendir(dir.c_str())) == NULL) {
    msgOut(MSG_ERROR, "Error " + i2s(errno) + " opening the " + dir + " directory.");
    //cout << "Error(" << errno << ") opening " << dir << endl;
    return errno;
  }
  while ((dirp = readdir(dp)) != NULL) {
    string filename = dirp->d_name;
    size_t dot = filename.find_last_of(".");
    string ext = (dot == string::npos) ? "" : filename.substr(dot); // ".ext", empty for a file without extension (substr(npos) used to throw)
    if(
      (filter != "" && ext == filter) // there is a filter and the last bit of the filename match the filter
      || (filter == "" && (dot == string::npos || dot+1 < filename.size())) // there isn't any filter but we don't want stuff like ".." or "."
    ) {
      files.push_back(string(dirp->d_name));
    }
  }
  closedir(dp);
  return 0;
}


vector<pathRule*>
ModelData::getPathMortalityRule(const string& forType, const string& dC){
  vector<pathRule*> toReturn;
  for(uint i=0;i<pathRules.size();i++){
    if(pathRules[i].forType == forType && pathRules[i].dClass == dC){
      toReturn.push_back(&pathRules[i]);
    }
  }
  return toReturn;
}

/**
 * @brief ModelData::createCombinationsVector
 * Return a vector containing any possible combination of nItems items (including all subsets).
 *
 * For example with nItems = 3:
 * 0: []; 1: [0]; 2: [1]; 3: [0,1]; 4: [2]; 5: [0,2]; 6: [1,2]; 7: [0,1,2]

 * @param nItems number of items to create p
 * @return A vector with in each slot the items present in that specific combination subset.
 */
vector < vector <int> >
ModelData::createCombinationsVector(const int& nItems) {
  // Not confuse combination with permutation where order matter. Here it doesn't matter, as much as the algorithm is the same and returns
  // to as each position always the same subset
  vector < vector <int> > toReturn;
  int nCombs = pow(2,nItems);
  //int nCombs = nItems;
  for (uint i=0; i<nCombs; i++){
    vector<int> thisCombItems; //concernedPriProducts;
    for(uint j=0;j<nItems;j++){
      uint j2 = pow(2,j);
      if(i & j2){ // bit a bit operator, p217 C++ book
        thisCombItems.push_back(j);
      }
    }
    toReturn.push_back(thisCombItems);
  }
  return toReturn;
}


double
ModelData::getAvailableDeathTimber(const vector<string> &primProd_h, int regId_h, int year_h){
  if (!getBoolSetting("useDeathTimber",DATA_NOW)) return 0;
  double toReturn = 0.0;
  vector <string> forTypesIds = getForTypeIds();
  for (uint i=0;i<forTypesIds.size();i++){
    string ft = forTypesIds[i];
    for(uint u=0;u<diamClasses.size();u++){
      string dc = diamClasses[u];
      bool possible = false;
      int maxYears = 0;
      for (int p=0; p<primProd_h.size();p++){
          string primProd = primProd_h[p];
          if(assessProdPossibility(primProd,ft, dc)){
              possible = true;
              maxYears=max(maxYears,getMaxYearUsableDeathTimber(primProd, ft, dc));
          }
      }
      if(possible){
        for(int y=year_h;y>year_h-maxYears;y--){
          iisskey key(y,regId_h,ft,dc);
          toReturn += findMap(deathTimberInventory,key,MSG_NO_MSG,0.0);
        }
      }
    }
  }
  return toReturn;
}

vector <int>
ModelData::getAllocableProductIdsFromDeathTimber(const int &regId_h, const string &ft, const string &dc, const int &harvesting_year, int request_year){
  vector<int> allocableProductIds;
  if (!getBoolSetting("useDeathTimber",DATA_NOW)) return allocableProductIds;
  if (request_year == DATA_NOW) request_year = MTHREAD->SCD->getYear();
  for(uint p=0;p<priProducts.size();p++){
    string primProd = priProducts[p];
    if(assessProdPossibility(primProd,ft, dc)){
      int maxYears = getMaxYearUsableDeathTimber(primProd, ft, dc);
      if (request_year-harvesting_year < maxYears){
        allocableProductIds.push_back(p);
      }
    }
  }
  return allocableProductIds;
}



double
ModelData::getAvailableAliveTimber(const vector<string> &primProd_h, int regId_h){
  double toReturn = 0.0;
  ModelRegion* REG = MTHREAD->MD->getRegion(regId_h);
  vector <Pixel*> regPx = REG->getMyPixels();
  vector <string> forTypesIds = getForTypeIds();
  for (uint i=0;i<forTypesIds.size();i++){
    string ft = forTypesIds[i];
    for(uint u=0;u<diamClasses.size();u++){
      string dc = diamClasses[u];
      bool possible = false;
      for (int p=0; p<primProd_h.size();p++){
          string primProd = primProd_h[p];
          if(assessProdPossibility(primProd,ft, dc)){
              possible = true;
          }
      }
      if(possible){
        for (uint p=0;p<regPx.size();p++){
          Pixel* px = regPx[p];
          toReturn += px->vol_l.at(i).at(u)*px->avalCoef.at(i);
        }
      }
    }
  }
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
      vector <string> record = records[i];
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
LLData::getData(const int &pos_h, const string &header_h, const int &debugLevel) const{

  if (records.size()<= pos_h){
    msgOut(debugLevel, "Requested position "+i2s(pos_h)+" too high! Not enought records !!");
    return "";
  }
  int hsize = headers.size();
  for (uint i=0;i<hsize;i++){
    if(headers[i] == header_h) return records[pos_h][i];
  }
  msgOut(debugLevel, "Header string "+header_h+" not found!");
  return "";
}
