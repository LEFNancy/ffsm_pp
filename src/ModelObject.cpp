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
#include "ModelObject.h"
#include "ThreadManager.h"
#include "Scheduler.h"

ModelObject::ModelObject(ThreadManager* MTHREAD_h){
	MTHREAD = MTHREAD_h;
	owner=NULL;
	user=NULL;
	startYear=-1;
	requiredFor = "";
}

ModelObject::~ModelObject(){
}

double
ModelObject::getResourceValue(string resourceName_h){
	map<string, double>::iterator p;
	p=matrixObjRes.find(resourceName_h);
	if(p != matrixObjRes.end())
		return p->second;
	return 0;
}

/**
 Set random age is called at init time, so currentAge is set anywhere from 1 to lifeTerm-1

*/
void
ModelObject::setRandomAge(){
	int currentAge = int (1+( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )* (lifeTerm-1)); // [1,lifeterm-1]
	startYear = MTHREAD->SCD->getYear()-currentAge;
}

vector <double>
ModelObject::getMatrixCoefficients(){
	vector <string> resourceNames= MTHREAD->RD->getResourceNames();
	vector <double> toReturn;
	for (uint i=0;i<resourceNames.size();i++){
		map <string,double>::iterator p;
		p=matrixObjRes.find(resourceNames.at(i));
		if(p != matrixObjRes.end())
			toReturn.push_back(p->second);
		else 
			toReturn.push_back(0);
	}
	return toReturn;
}

double
ModelObject::getCurrentValue(bool endOfYear){
	if(!endOfYear)
		return initialCost * (1-( ( (double) getAge() ) / ( (double)lifeTerm) )); // DONE: check!! ..and thanks I checked !!!!!
	else
		return initialCost * (1-( ( (double) getAge() +1 ) / ( (double)lifeTerm) ));
	return 0;
}

int
ModelObject::getAge(){
	return MTHREAD->SCD->getYear()-startYear;
}
