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
#ifndef REGDATA_H
#define REGDATA_H

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
//#include "InputDocument.h"
#include "InputNode.h"

using namespace std;

struct IFiles;
struct BasicData;
class  LLData;
struct VectorSettings;
class  RegActivities;
struct RegResources;
class  ModelObject;
//class InputDocument;
//class InputNode;

/// Regional data, including macros and settings

/**
All regional data are within this class. It may have linked other data-classes.
<br>On some variables RegData has just the definition of the objects, but the values may change at the agent-level. This is why each agent has a "personal copy" of them.
@author Antonello Lobianco
*/

class RegData: public BaseClass{

public:
	                    RegData(ThreadManager* MTHREAD_h);
	                   ~RegData();
	

	/// Unzip the OpenOffice input file (NEW 2008.05.13)
	void                loadInput();
	vector<string>      getScenarios();
	int                 getScenarioIndex();
	bool                delDir(QString dirname); ///< Recursivelly delete a directory 
	void                setDefaultSettings();
	void                setScenarioSettings();
	void                setResources();
	void                setDefaultActivities();
	void                setScenarioActivities();
	void                setObjectsDefinitions();
	void                initializeResourcesObjectsMatch(); ///< Cache the vectors of object names and object values that are associated to each resource
	void                setSpace(); 
	/// Create the vector availableObjectsBySoilType so it is faster to assign spacial objects to farmers basing on their land types
	void                initializeSpacialObjectsMatch();
	void                debug();

	/// Return a vector of objects that together provide the specified resource in the specified quantity
	vector <ModelObject> getBestMatchingInitialObjects (string resourceName_h, double resourceQ_h);
	string              getOutputDirectory() const {return outputDirname;};
	string              getFilenameByType(string type_h);
	LLData              getTable(string tableName_h, int debugLevel=MSG_CRITICAL_ERROR);
	vector <IFiles>     getIFilesVector() const {return iFilesVector;};
	string              getBaseDirectory() const {return baseDirectory;};
	int                 getActivitiesDefinitionVectorSize() const {return activitiesDefinitionVector.size();};

	vector <string>     getResourceNames(int type_h=-1) const; ///< get resources, all (default) or by type;
	int                 getResourceSize(int type_h=-1) const; ///< get number of defined resources, all (default) or by type;
	vector <string>     getObjectNames() const;
	vector <string>     getActivityNames() const;
	int                 getActGisCodeByName(string actName_h) const;

	int                 getIntSetting          (const string &name_h, int position=0);
	double              getDoubleSetting       (const string &name_h, int position=0);
	string              getStringSetting       (const string &name_h, int position=0);
	bool                getBoolSetting         (const string &name_h, int position=0);
	vector <int>        getIntVectorSetting    (const string &name_h);
	vector <double>     getDoubleVectorSetting (const string &name_h);
	vector <string>     getStringVectorSetting (const string &name_h);
	vector <bool>       getBoolVectorSetting   (const string &name_h);

	RegResources        getResourceByName(string name_h);
	ModelObject         getObjectByName(string name_h);
	vector<RegActivities*> getRegActivities();
	RegActivities*      getRegActivityByName(string name_h);
	vector <ModelObject*>  getObjectsDefinitionVector();

	/// Retrieve the list of objects type that can be placed on the specified soil type
	vector <ModelObject> getModelObjectsBySoilType(int soilType_h);

	void                setOutputDirectory(const char* output_dirname_h);
	void                setBaseDiretory(string baseDirectory_h){baseDirectory=baseDirectory_h;};
	void                addSetting(string name_h, vector <string> values_h, int type_h, string comment_h);
	void                addSetting(string name_h, string value_h, int type_h, string comment_h);

	void                setBasicData(const string &name_h, int value, int position=0);
	void                setBasicData(const string &name_h, double value, int position=0);
	void                setBasicData(const string &name_h, string value, int position=0);
	void                setBasicData(const string &name_h, bool value, int position=0);

private:
	string              getBaseData (const string &name_h, int type_h, int position=0);
	vector <string>     getVectorBaseData (const string &name_h, int type_h);
	void                setBasicData(const string &name_h, string value, int type_h, int position);

	string                                   inputFilename; // from Qt fileOpen dialog
	string                                   outputDirname; // from main config files
	string                                   baseDirectory; // from Qt fileOpen dialog

	// In the following vectors, when it is stated "just the topology", it means here we have just the definition of the objects, but the values may change at the agent-level. Thi is why each agent hads a "personal copy" of them
	vector <IFiles>                           iFilesVector;  ///< list of all input files. Simple (struct)
	vector <BasicData>               programSettingsVector;  ///<  setting data. Simple (struct)
	vector <RegResources>        resourcesDefinitionVector;  ///<  definition of available resources. Just the topology. (struct)
	vector <RegActivities>      activitiesDefinitionVector;  ///<  available activities. Just the topology.
	vector <ModelObject>           objectsDefinitionVector;  ///<  definition of available objects. Just the topology. (class)
	vector <LLData>                           LLDataVector;  ///< vector of Low Level Data

	vector <vector <string> >       resourcesJoinedObjects;  ///< a vector for each resource with the list of the joined objects
	vector <vector <double> >  resourcesProvidedQuantities;  ///< the corresponding vectors of provided quantities
	 /// For each agricultural soil type (as defined in the setting "agrLandTypes") this list define the objects that can be placed on that soil type
	vector <vector <ModelObject> > availableObjectsBySoilType;
	InputNode                             mainDocument;  ///< the main input document, loaded in memory at unzipping stage
	//InputNode                                     mainNode;  ///< the main input node
};

/// Input files (struct)
/**
Very short struct containing the input files used (one istance==one file).
<br>A copy of each Istances is saved on vector iFilesVector in class RegData.
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
	int                                               type; // enum TYPE_*
	string                                         comment;
};


/// Regional level resources
/**
Struct containing data about the existing regional resources.
<br>Resources are places on rows in the decision matrix.
<br>The final matrix will be composed of this resources plus a further row for each plot in the availability of the farmer.
@author Antonello Lobianco
*/
struct RegResources {
	string                                            name;
	string                                            unit;
	string                                         comment;
	int                                             source;
	int                                               type;
	string                                         subType;
};

/// Regional level activities
/**
Activities can be spatially explicit or implicit.
<br>In the former case, they will depend on specific plots with a compatible land use type and eventually with the presence of certain objects (investments).
Now the requirement can be only over ONE investment object, but it can be changed (see the feature wishes list on the doc)
@author Antonello Lobianco
*/

class RegActivities: public BaseClass{
public:
	                    RegActivities(ThreadManager* MTHREAD_h);
	                   ~RegActivities();

	string              getName() const {return activityName;};
	string              getComment() const {return activityComment;};
	double              getMatrixGrossMargin() const; //todo:get the current one!
	map <string, double> getMatrixActRes() const; ///< Return the standard matrix
	vector <double>     getMatrixCoefficients(); ///< return the ordered list of coefficients for the matrix
	double              getMatrixCoefficientByName(string cname_h); ///< Return a coefficient by its resource name

	bool                getSpatiallyExplicit(){return spatiallyExplicit;};
	vector <int>        getRequiredLandUseCodes(){return requiredLandUseCodes;};
	string              getRequiredObjectOnPlot(){return requiredObjectOnPlot;};
	int                 getMapCode() const {return mapCode;};
	int                 getMapRColor(){return mapRColor;};
	int                 getMapGColor(){return mapGColor;};
	int                 getMapBColor(){return mapBColor;};
	double              getDecouplingOption(int premiumIndex);
	void                setName(string activityName_h){activityName=activityName_h;};
	void                setComment(string activityComment_h){activityComment=activityComment_h;};
	friend void RegData::setDefaultActivities();
	friend void RegData::setScenarioActivities();

private:
	string                          activityName;
	string                       activityComment;
	vector<double>       matrixGrossMarginByYear;
	vector< map<string,double> > matrixActResByYear;
	vector<int>             requiredLandUseCodes; ///< A vector of Corine land use codes where this activity can be made.
	string                 requiredObjectOnPlot; ///< ID of object that need to be present on the plot to allow this activity
	bool                       spatiallyExplicit;
	int                                  mapCode;    ///< Code associated to each activity for map output
	int                                mapRColor;  ///< Red color map output
	int                                mapGColor;  ///< Green color map output
	int                                mapBColor;  ///< Blue color map output
	vector< vector<double> >             decouplingOptions;  ///< Share of decoupling rights associated to the activity

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
	friend void RegData::loadInput();

private:
	string                             tableName;
	vector<string>                       headers;
	vector < vector <string> >           records;

};


#endif
