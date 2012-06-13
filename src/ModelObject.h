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
#ifndef MODELOBJECT_H
#define MODELOBJECT_H

#include <map>

// regams headers...
#include "BaseClass.h"
#include "RegData.h"

class Pixel;
class Agent_base;

/// Generic class for storing object informations
/**
This class keeps instances of all objects of the model.
<br>Objects can be spatially explicit or implicit.
<br>In the former case, the change on deploy them will depend on specific plots with a compatible land use type and with the absence of uncompatible objects (eg. for perennial crops, of other perennial crops).
@author Antonello Lobianco
*/
class ModelObject : public BaseClass
{

public:
	                    ModelObject(ThreadManager* MTHREAD_h);
	                   ~ModelObject();

	double              getResourceValue(string resourceName_h); ///< Return the effects of this object on a specific resource
	void                setRandomAge();
	//void                newYear(); ///< objects no nonger got a counter of their age. The counter is dynamically generated confronting the init year with the current year. No risks of forgetting or doubling calling of this function!

	string              getName() const {return name;};
	string              getObjectTypology() const {return type;};
	string              getObjectSubTypology() const {return subType;};
	string              getComment() const {return comment;};
	double              getMatrixGrossMargin() const {return matrixGrossMargin;};
	double              getInitialCost() const {return initialCost;};;
	map<string, double> getMatrixObjRes() const {return matrixObjRes;};
	vector <int>        getRequiredLandUseCodes() const {return requiredLandUseCodes;};
	string              getRequiredFor() const {return requiredFor;}; ///< Name of the activity it is required for (only sp. obj)
	vector <double>     getMatrixCoefficients(); ///< return the ordered list of coefficients for the matrix
	bool                getSpatiallyExplicit() const {return spatiallyExplicit;};
	Agent_base*         getOwner(){return owner;};

	/// get current value. For now it is derived from a simple linear depretiation, later on we can change this function according to the object type  @param endOfYear Optional parameter to specify that the value requested is those that the object has at the end of the year
	double              getCurrentValue(bool endOfYear=false);
	/// last year before this object will go out of temporal scope
	int                 getLastYear(){return startYear+lifeTerm-1;};
	int                 getStartYear(){return startYear;};
	int                 getLifeTerm() {return lifeTerm;};
	int                 getAge();
	int                 getRemainingAge() {return getLifeTerm()-getAge();};

	void                setName(string name_h){name=name_h;};
	void                setType(string type_h){type=type_h;};
	void                setSubType(string subType_h){subType=subType_h;};
	void                setComment(string comment_h){comment=comment_h;};
	void                setLifeTerm(int lifeTerm_h){lifeTerm=lifeTerm_h;};
	void                setStartYear(int year_h){startYear = year_h;};
	void                setMatrixGrossMargin(double matrixGrossMargin_h){matrixGrossMargin=matrixGrossMargin_h;};
	void                setInitialCost(double initialCost_h){initialCost=initialCost_h;};
	void                setMatrixObjRes(map <string, double> matrixObjRes_h){matrixObjRes=matrixObjRes_h;};
	void                setRequiredFor(string activityName_h){requiredFor = activityName_h;};
	void                setOwner(Agent_base * owner_h){owner=owner_h;};
	friend void RegData::setObjectsDefinitions();

protected:
	string                                  name;
	string                                  type;
	string                               subType;
	string                               comment;
	/// how many years the object can live. lifeTerm= lastYear-startYear  The object start with age=0 and is destroied at the end of year when it reach lifeTerm-1
	int                                  lifeTerm;
	/// year when the object was created
	int                                startYear;
	double                     matrixGrossMargin;
	double                           initialCost;
	map<string, double>             matrixObjRes; ///< MIP matrix
	/// a vector of Corine land use codes where this objects can be performed.
	vector<int>             requiredLandUseCodes;
	/// true if the object is spacially explicit (is permanently linked with one plot)
	bool                       spatiallyExplicit;
	Agent_base*                            owner;
	Agent_base*                             user; //not used !!!
	string                           requiredFor; ///< Activity for which this (spatial) object is required for

};

#endif
