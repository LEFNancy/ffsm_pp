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
#ifndef MODELREGION_H
#define MODELREGION_H

// Core C++ headers
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <sstream>

// Qt headers...
#include <QString>

// regmas headers..
#include "BaseClass.h"

using namespace std;

struct forData;
struct prodData;

class ModelRegion : public BaseClass{

public:
						ModelRegion(ThreadManager* MTHREAD_h, int regId_h, string regSName_h, string regLName_h, int regLevel_h, int parRegId_h, bool isResidual_h); ///< Constructor
					   ~ModelRegion();

	// "set" methods..
	void                setRegId(int regId_h){regId = regId_h;};
	void                setRegSName(string regSName_h){regSName = regSName_h;};
	void                setRegLName(string regLName_h){regLName = regLName_h;};
	void                setRegLevel(int regLevel_h){regLevel = regLevel_h;};
	void                setParRegId(int parRegId_h){parRegId = parRegId_h;};
	void                setIsResidual(bool isResidual_h){isResidual = isResidual_h;};
	void                setParent(ModelRegion* parRegion_h){parRegion = parRegion_h;};
	void                setChildren(vector<ModelRegion*> children_h) {chRegions = children_h;}; ///< Childrens are all the lvel-1 region that are parts of this region.
	void                addForData(forData* data_h){forDataVector.push_back(data_h);};
	void                addProdData(prodData* data_h){prodDataVector.push_back(data_h);};

	// "get" methods..
	int                 getRegId() {return regId;};
	string              getRegSName() {return regSName;};
	string              getRegLName() {return regLName;};
	int                 getRegLevel() {return regLevel;};
	int                 getParRegId() {return parRegId;};
	bool                getIsResidual() {return isResidual;};
	ModelRegion*        getParent(){return parRegion;}; ///< Returns a pointer to the parent regions
	vector<ModelRegion*>getChildren(bool excludeResidual = true); ///< Return a vector of pointers to the direct child regions
	double              getVolumes();
	vector<double>      getVolumes(int fType_h);
	vector < vector <double> > getVolumes(int fType_h, string dClass_h);
	int                 getNChildren(bool excludeResidual = true);

private:
	int                                regId; ///< Regional unique ID
	string                          regSName; ///< A short name of the region
	string                          regLName; ///< Region long name;
	int                             regLevel; ///< The level of the region. 1: country, 2: regions
	int                             parRegId; ///< Id of the parent region;
	bool                          isResidual; ///< A flag if this region should be explicitelly modelled or it is just a residual
	ModelRegion*                   parRegion; ///< Pointer to the parent region
	vector<ModelRegion*>           chRegions; ///< Vector of level-1 children regions
	vector<forData*>           forDataVector; ///< Vector of pointers of forestry data (owned by ModelData)
	vector<prodData*>         prodDataVector; ///< Vector of pointers of product data (owned by ModelData)


};

#endif // REGION_H
