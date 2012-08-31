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
struct forType;

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
	void                setDefaultSettings();
	void                setScenarioSettings();
	void                createRegions();
	void                setDefaultForData();
	void                setScenarioForData();
	void                setDefaultProdData();
	void                setScenarioProdData();
	void                setProductResourceMatrixLink();
	void                setForestTypes();
	void                setReclassificationRules();
	void                applyOverrides(); ///< Cancel all reg1 level data and trasform them in reg2 level if not already existing
	void                applyDebugMode(); ///< Works only a specified subset of regions and products
	void                setSpace();
	/// Create the vector availableObjectsBySoilType so it is faster to assign spacial objects to farmers basing on their land types
	void                debug();

	/// Return a vector of objects that together provide the specified resource in the specified quantity
	string              getOutputDirectory() const {return outputDirname;};
	string              getFilenameByType(string type_h);
	LLData              getTable(string tableName_h, int debugLevel=MSG_CRITICAL_ERROR);
	vector <IFiles>     getIFilesVector() const {return iFilesVector;};
	string              getBaseDirectory() const {return baseDirectory;};
	ModelRegion*        getRegion(int regId_h);
	vector<int>         getRegionIds(int level_h, bool excludeResidual=true);
	vector < vector <int> > getRegionIds( bool excludeResidual=true);
	int                 getNForTypes(){return forTypes.size();};
	int                 getNReclRules(){return reclRules.size();};
	forType*            getForType(int position){return &forTypes[position];};
	forType*            getForType(string forTypeId_h);
	vector <string>     getForTypeIds(bool all=false); ///< By default it doesn't return forTypes used only as input
	reclRule*           getReclRule(int position){return &reclRules[position];};
	/// A simple function to assess if a specified product can be made by a certain forest type and diameter class
	const bool          assessProdPossibility(const string &prod_h, const string &forType_h, const string &dClass_h);

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
	const double        getForData(const string &type_h, const int& regId_h, const string &forType_h, const string &diamClass_h, const int& year=DATA_NOW);


	void                setProdData(const double& value_h, const string &type_h, const int& regId_h, const string &prodId_h, const int& year=DATA_NOW, const bool& allowCreate=false, const string &freeDim_h=""); // Remember default arguments must be at the end
	void                setForData(const double& value_h, const string &type_h, const int& regId_h, const string &forType_h, const string &diamClass_h, const int& year=DATA_NOW, const bool& allowCreate=false); // Remember default arguments must be at the end

string                makeKeyProdData(const string& parName, const string& regId, const string& prod, const string& freeDim="") const {return parName+"#"+regId+"#"+prod+"#"+freeDim+"#";};
void                  unpackKeyProdData(const string& key, string& parName, int &regId, string& prod, string& freeDim) const;

double                calculateAnnualisedEquivalent(double amount_h, int years_h); ///< Calculate the annual equivalent flow
double                calculateAnnualisedEquivalent(double amount_h, double years_h); ///< Transform the double to the highest integer and call calculateAnnualisedEquivalent(double amount_h, int years_h)

	/// Retrieve the list of objects type that can be placed on the specified soil type

	void                setOutputDirectory(const char* output_dirname_h);
	void                setBaseDiretory(string baseDirectory_h){baseDirectory=baseDirectory_h;};
	void                addSetting(string name_h, vector <string> values_h, int type_h, string comment_h);
	void                addSetting(string name_h, string value_h, int type_h, string comment_h);
	void                cacheFrequentData(); ///< Called after input reading, it fix frequently used data;
	int                 getCachedInitialYear(){return cached_initialYear;};

	void                setBasicData(const string &name_h, int value, int position=0);
	void                setBasicData(const string &name_h, double value, int position=0);
	void                setBasicData(const string &name_h, string value, int position=0);
	void                setBasicData(const string &name_h, bool value, int position=0);
	friend void         Output::printForestData();
	friend void         Output::printProductData();

private:
	string              getBaseData (const string &name_h, int type_h, int position=0);
	vector <string>     getVectorBaseData (const string &name_h, int type_h);
	void                setBasicData(const string &name_h, string value, int type_h, int position);

	bool                dataMapCheckExist(const DataMap& map, const string& search_for, const bool& exactMatch=false) const;
	double              dataMapGetValue(const DataMap& map, const string& search_for, const int& year_h, const bool& exactMatch=false);
	void                dataMapSetValue(DataMap& map, const string& search_for, const double& value_h, const int& year_h, const bool& exactMatch=false);

	bool                tempBool; ///< a temporary bool variable used for various functions
	int                 cached_initialYear;

	string                                   inputFilename; // from Qt fileOpen dialog
	string                                   outputDirname; // from main config files
	string                                   baseDirectory; // from Qt fileOpen dialog

	vector <forData>                         forDataVector; ///< Forestry data
	map <string, vector<double> >               forDataMap; ///< Forestry data
	//vector <prodData>                       prodDataVector; ///< Product data
	map <string, vector<double> >              prodDataMap; ///< Product data
	vector <forToProd>                     forToProdVector; ///< Vector of coefficients from forest resources to primary products

	vector <IFiles>                           iFilesVector; ///< List of all input files. Simple (struct)
	vector <BasicData>               programSettingsVector; ///< Setting data. Simple (struct)
	vector <LLData>                           LLDataVector; ///< Vector of Low Level Data
	vector <ModelRegion>                     regionsVector; ///< Vector of modelled regions

	vector <forType>                              forTypes; ///< Vector of forest types
	vector <reclRule>                            reclRules; ///< Vector of reclassification rules
	vector < vector <int> >                            l2r; ///< Region2 ids


	 /// For each agricultural soil type (as defined in the setting "agrLandTypes") this list define the objects that can be placed on that soil type
	InputNode                                 mainDocument;  ///< the main input document, loaded in memory at unzipping stage
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

/// Forestry data (struct)
/**
Struct containing the forestry data. This data is stored here but also each region has a link to its own data.
*/

struct forData {
	string                                         parName;
	int                                             region;
	string                                         forType;
	string                                       diamClass;
	vector <double>                                 values;
};

/// Product data (struct)
/**
Struct containing the product-based data. This data is stored here but also each region has a link to its own data.
*/
/*struct prodData {
	string                                         parName;
	int                                             region;
	string                                            prod;
	string                                         freeDim;
	vector<double>                                  values;
};*/
/// IO production matrix between the forest resources and the primary products (struct)
/**
Struct containing the io matrix between the forest resources and the primary products. Not to be confunded with the IO matrix between primary products and secondary products.
*/
struct forToProd {
	string                                         product;
	string                                         forType;
	string                                          dClass;
};

/// Forest types (struct)
/**
Struct containing the list of the forest types managed in the model.
@par memType Parameter to define if this type is used only in initial data reading, then is reclassed and no more used (1) or if it is generated from the reclass rule and then used in the model (2)
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

/// Low level data. XML input is reversed here after unzipping oocalc file and parsing content.xml
class LLData: public BaseClass{

public:
	                    LLData(ThreadManager* MTHREAD_h, string tableName_h);
	                   ~LLData();
	void                clean(); // clean the data from empty headers
	string              getTableName(){return tableName;};
	int                 nrecords(){return records.size();};
	string              getData(int pos_h, string header_h, int debugLevel=MSG_CRITICAL_ERROR);
	friend void ModelData::loadInput();
	friend void ModelData::loadDataFromCache(string tablename);

private:
	string                             tableName;
	vector<string>                       headers;
	vector < vector <string> >           records;

};


#endif
