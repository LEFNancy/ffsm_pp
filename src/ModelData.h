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
#ifndef MODELDATA_H
#define MODELDATA_H

// Core C++ headers
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <sstream>

// Qt headers...
#include <QString>

// RegMAS headers...
#include "BaseClass.h"
#include "InputNode.h"
#include "Output.h"

using namespace std;

typedef map<string, vector <double> > DataMap;
typedef pair<string, vector <double> > DataPair;

struct IFiles;
struct BasicData;
class  LLData;
class  ModelRegion;
class  Layers;
class  Output;
struct forData;
struct prodData;
struct forToProd;
struct reclRule;
struct pathRule;
struct forType;

struct scenarioData {
  string                                            id;
  string                                     shortDesc;
  string                                      longDesc;
  string                                  settingTable;
  string                                  forDataTable;
  string                                 prodDataTable;
  string                                forToProdTable;
  string                                     pathTable;
};

/// Regional data, including macros and settings

/**
All regional data are within this class. It may have linked other data-classes.
<br>On some variables ModelData has just the definition of the objects, but the values may change at the agent-level. This is why each agent has a "personal copy" of them.
@author Antonello Lobianco
*/

class ModelData: public BaseClass{

public:
            ModelData(ThreadManager* MTHREAD_h);
             ~ModelData();

  /// Unzip the OpenOffice input file (NEW 2008.05.13)
  void                loadInput();

  void                loadDataFromCache(string tablename); ///< Load data from a cached CSV instead of the openoffice file
  vector<string>      getScenarios();
  int                 getScenarioIndex();
  bool                delDir(QString dirname); ///< Recursivelly delete a directory 
  void                setScenarioData(); ///< Set the infos about this scenario (long description and overriding tables)
  void                setDefaultSettings();
  void                setScenarioSettings();
  void                createRegions();
  void                setDefaultForData();
  void                setScenarioForData();
  void                setDefaultProdData();
  void                setScenarioProdData();
  void                setDefaultProductResourceMatrixLink();
  void                setScenarioProductResourceMatrixLink();
  void                setForestTypes();
  void                setReclassificationRules();
  void                setDefaultPathogenRules();
  void                setScenarioPathogenRules();
  void                applyOverrides(); ///< Cancel all reg1 level data and trasform them in reg2 level if not already existing
  void                applyDebugMode(); ///< Works only a specified subset of regions and products
  void                setSpace();

  /// Return a vector of objects that together provide the specified resource in the specified quantity
  string              getOutputDirectory() const {return outputDirname;}
  int                 getFilenamesByDir (const string & dir, vector<string> &files, const string &filter = ""); ///< Return a list of files in a directory
  string              getFilenameByType(string type_h);
  LLData              getTable(string tableName_h, int debugLevel=MSG_CRITICAL_ERROR);
  vector <IFiles>     getIFilesVector() const {return iFilesVector;}
  string              getBaseDirectory() const {return baseDirectory;}
  ModelRegion*        getRegion(int regId_h);
  bool                regionExist (const int & regId_h) const ;
  vector <ModelRegion*> getAllRegions(bool excludeResidual=true);
  vector<int>         getRegionIds(int level_h, bool excludeResidual=true);
  vector < vector <int> > getRegionIds( bool excludeResidual=true);
  int                 getNForTypes(){return forTypes.size();}
  int                 getNReclRules(){return reclRules.size();}
  forType*            getForType(int position){return &forTypes[position];}
  forType*            getForType(string& forTypeId_h);
  int                 getForTypeCounter(string& forTypeId_h, bool all=false); ///< By default it doesn't return forTypes used only as input
  vector <string>     getForTypeIds(bool all=false); ///< By default it doesn't return forTypes used only as input
  string              getForTypeParentId(const string& forTypeId_h);
  vector<string>      getForTypeChilds(const string &forTypeId_h);
  int                 getNForTypesChilds(const string& forTypeId_h);
  reclRule*           getReclRule(int position){return &reclRules[position];}
  vector <string>     getDiameterClasses(bool productionOnly=false);
  /// A simple function to assess if a specified product can be made by a certain forest type and diameter class
  const bool          assessProdPossibility(const string &prod_h, const string &forType_h, const string &dClass_h);
  int                 setErrorLevel(int errorLevel_h){errorLevel=errorLevel_h;}
  bool                getTempBool(){return tempBool;}

  double              getTimedData(const vector <double> &dated_vector, const int& year_h) const; ///< Return the value for the specified year in a timelly ordered vector, taking the last value if this is smaller than the required position.
  void                setTimedData(const double& value_h, vector<double> &dated_vector, const int& year_h, const int& MSG_LEVEL=MSG_WARNING);

  int                 getIntSetting          (const string &name_h, int position=0) const;
  double              getDoubleSetting       (const string &name_h, int position=0) const;
  string              getStringSetting       (const string &name_h, int position=0) const;
  bool                getBoolSetting         (const string &name_h, int position=0) const;
  vector <int>        getIntVectorSetting    (const string &name_h) const;
  vector <double>     getDoubleVectorSetting (const string &name_h) const;
  vector <string>     getStringVectorSetting (const string &name_h) const;
  vector <bool>       getBoolVectorSetting   (const string &name_h) const;


  const double        getProdData(const string &type_h, const int& regId_h, const string &prodId_h, const int &year=DATA_NOW, const string &freeDim_h="");
  const double        getForData(const string &type_h, const int& regId_h, const string &forType_h, const string &freeDim_h, const int& year=DATA_NOW);


  void                setProdData(const double& value_h, const string &type_h, const int& regId_h, const string &prodId_h, const int& year=DATA_NOW, const bool& allowCreate=false, const string &freeDim_h=""); // Remember default arguments must be at the end
  void                setForData(const double& value_h, const string &type_h, const int& regId_h, const string &forType_h, const string &freeDim_h, const int& year=DATA_NOW, const bool& allowCreate=false); // Remember default arguments must be at the end

  string              makeKeyProdData(const string& parName, const string& regId, const string& prod, const string& freeDim="") const {return parName+"#"+regId+"#"+prod+"#"+freeDim+"#";}
  string              makeKeyForData(const string& parName, const string& regId, const string& forType, const string& diamClass) const {return parName+"#"+regId+"#"+forType+"#"+diamClass+"#";}
  void                unpackKeyProdData(const string& key, string& parName, int &regId, string& prod, string& freeDim) const;
  void                unpackKeyForData(const string& key, string& parName, int &regId, string& forType, string& diamClass) const;

  vector<pathRule*>   getPathMortalityRule(const string& forType, const string& dC); ///< Return the pathogen mortality rule(s) associated with a given ft and dc (plural as more than a single pathogen could be found)

  double              calculateAnnualisedEquivalent(double amount_h, int years_h); ///< Calculate the annual equivalent flow
  double              calculateAnnualisedEquivalent(double amount_h, double years_h); ///< Transform the double to the highest integer and call calculateAnnualisedEquivalent(double amount_h, int years_h)

  void                setOutputDirectory(const char* output_dirname_h);
  void                setBaseDiretory(string baseDirectory_h){baseDirectory=baseDirectory_h;}
  void                addSetting(string name_h, vector <string> values_h, int type_h, string comment_h);
  void                addSetting(string name_h, string value_h, int type_h, string comment_h);
  void                cacheSettings(); ///< Called after input reading, it fix frequently used data;
  int                 getCachedInitialYear(){return cached_initialYear;}

  void                setBasicData(const string &name_h, int value, int position=0);
  void                setBasicData(const string &name_h, double value, int position=0);
  void                setBasicData(const string &name_h, string value, int position=0);
  void                setBasicData(const string &name_h, bool value, int position=0);
  friend void         Output::printForestData(bool finalFlush=false);
  friend void         Output::printProductData(bool finalFlush=false);
  scenarioData        scenario;

private:
  string              getBaseData (const string &name_h, int type_h, int position=0);
  vector <string>     getVectorBaseData (const string &name_h, int type_h);
  void                setBasicData(const string &name_h, string value, int type_h, int position);

  bool                dataMapCheckExist(const DataMap& map, const string& search_for, const bool& exactMatch=true) const;
  double              dataMapGetValue(const DataMap& map, const string& search_for, const int& year_h, const bool& exactMatch=true);
  int                 dataMapSetValue(DataMap& map, const string& search_for, const double& value_h, const int& year_h, const bool& exactMatch=true);

  string                                   inputFilename; // from Qt fileOpen dialog
  string                                   outputDirname; // from main config files
  string                                   baseDirectory; // from Qt fileOpen dialog

  map <string, vector<double> >               forDataMap; ///< Forestry data
  map <string, vector<double> >              prodDataMap; ///< Product data
  vector <forToProd>                     forToProdVector; ///< Vector of coefficients from forest resources to primary products

  vector <IFiles>                           iFilesVector; ///< List of all input files. Simple (struct)
  vector <BasicData>               programSettingsVector; ///< Setting data. Simple (struct)
  vector <LLData>                           LLDataVector; ///< Vector of Low Level Data
  vector <ModelRegion>                     regionsVector; ///< Vector of modelled regions

  vector <forType>                              forTypes; ///< Vector of forest types
  vector <reclRule>                            reclRules; ///< Vector of reclassification rules
  vector <pathRule>                            pathRules; ///< Vector of pathogen rules
  vector < vector <int> >                            l2r; ///< Region2 ids

  // cahced setting data..
  vector <string>                            diamClasses; ///< Diameter classes
  int                                 cached_initialYear;
  vector <string>                            priProducts;
  vector <string>                            secProducts;
  vector <string>                            allProducts;

  bool                                         tempBool; ///< a temporary bool variable used for various functions

   /// For each agricultural soil type (as defined in the setting "agrLandTypes") this list define the objects that can be placed on that soil type
  InputNode                                 mainDocument;  ///< the main input document, loaded in memory at unzipping stage
  int                                         errorLevel;
};

/// Input files (struct)
/**
Very short struct containing the input files used (one istance==one file).
<br>A copy of each Istances is saved on vector iFilesVector in class ModelData.
<br>iFiles are defined in the main config file and parsed subsequently.
@author Antonello Lobianco
*/
//Changed from a class to a structure on 2006.10.17.
struct IFiles {
  string                                       directory;
  string                                            type;
  string                                            name;
  string                                         comment;
};

/// Basic data units (struct)
/**
Struct containing the basic data objects. At the moment, data are used to store programm settings or macro data.
@author Antonello Lobianco
*/
struct BasicData {
  string                                            name;
  /// Values are stored as "string" because we don't yet know at this point if they are string, double or integers!
  vector <string>                                 values;
  int                                               type; //< enum TYPE_*
  string                                         comment;
};

/// IO production matrix between the forest resources and the primary products (struct)
/**
Struct containing the io matrix between the forest resources and the primary products. Not to be confunded with the IO matrix between primary products and secondary products.
*/
struct forToProd {
  string                                         product;
  string                                         forType;
  string                                          dClass;
 /// The maximum year for a tree collapse that this product can be harvested from. E.g. a 0 value means it can be obtained only from live trees, a 5 years value mean it can be obtained from trees death no longer than 5 years ago.
  string                                        maxYears;
};

/// Forest types (struct)
/**
Struct containing the list of the forest types managed in the model.
@par memType Parameter to define if this type is used only in initial data reading, then is reclassed and no more used (1) or if it is generated from the reclass rule and then used in the model (2). New 20150311: (3) means a layer with spatial data of vol and area added respectiely in layers and proportionally to volumes
*/
struct forType {
  string                                        forTypeId;
  string                                         forLabel;
  int                                             memType;
  string                                         forLayer;
  string                                   ereditatedFrom;
  Layers*                                           layer;
};

/// IO production matrix between the forest resources and the primary products (struct)
/**
Struct containing the io matrix between the forest resources and the primary products. Not to be confunded with the IO matrix between primary products and secondary products.
*/
struct reclRule {
  int                                              regId;
  string                                       forTypeIn;
  string                                      forTypeOut;
  double                                           coeff;
};

/// Pathogen rule (how pathogen presense influence mortality) for a given forest type and diameter class (struct)
/**
Struct containing the rule that affect the mortality of a given ft and dc by a given pathogen: depending on the number of year of presence
of the pathogen over a given tollerance level the mortality increase more and more.
*/
struct pathRule {
  string                                       forType;
  string                                        dClass;
  string                                        pathId;  ///< Pathogen id (name)
  double                                      pres_min;  ///< Minimum level of presence of the pathogen to be counted as present (tolerance threshold)
  vector<double>                       mortCoefficents;  ///< Mortality coefficients ordered by number of presence of the pathogen, e.g. first value is the mortality increase in the first year of pathogen appareance.
};



/// Low level data. XML input is reversed here after unzipping oocalc file and parsing content.xml
class LLData: public BaseClass{

public:
                      LLData(ThreadManager* MTHREAD_h, string tableName_h);
                     ~LLData();
  void                clean(); // clean the data from empty headers
  string              getTableName(){return tableName;}
  int                 nrecords(){return records.size();}
  int                 nheaders(){return headers.size();}
  string              getData(const int& pos_h, const string& header_h, const int& debugLevel=MSG_CRITICAL_ERROR) const;
  friend void ModelData::loadInput();
  friend void ModelData::loadDataFromCache(string tablename);

private:
  string                             tableName;
  vector<string>                       headers;
  vector < vector <string> >           records;

};


#endif
