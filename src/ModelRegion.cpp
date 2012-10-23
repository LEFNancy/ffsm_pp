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

#include "ThreadManager.h"
#include "ModelRegion.h"
#include "ModelData.h"
#include "Pixel.h"
#include "Gis.h"


/**
The constructor of REGION instances want:
@param MTHREAD_h Pointer to the main thread manager
*/
ModelRegion::ModelRegion(ThreadManager* MTHREAD_h, int regId_h, string regSName_h, string regLName_h, int regLevel_h, int parRegId_h, bool isResidual_h){
	MTHREAD=MTHREAD_h;
	regId = regId_h;
	regSName = regSName_h;
	regLName = regLName_h;
	regLevel = regLevel_h;
	parRegId = parRegId_h;
	isResidual =  isResidual_h;
}

ModelRegion::~ModelRegion(){
}

vector<ModelRegion*>
ModelRegion::getChildren(bool excludeResidual){
	if(excludeResidual){
		vector<ModelRegion*> toReturn;
		for(uint i=0;i<chRegions.size();i++){
			if(!chRegions[i]->getIsResidual()){
				toReturn.push_back(chRegions[i]);
			}
		}
		return toReturn;
	}
	return chRegions;
}

int
ModelRegion::getNChildren(bool excludeResidual){
	if(excludeResidual){
		int toReturn;
		for(uint i=0;i<chRegions.size();i++){
			if(!chRegions[i]->getIsResidual()){
				toReturn++;
			}
		}
		return toReturn;
	}
	return chRegions.size();
}



double
ModelRegion::getVolumes(){
	//TODO;
	return 0;
}

vector<double>
ModelRegion::getVolumes(int fType_h){
	//TODO;
	vector<double> toReturn;
	return toReturn;
}

vector < vector <double> >
ModelRegion::getVolumes(int fType_h, string dClass_h){
	//TODO;
	vector < vector <double> > toReturn;
	return toReturn;
}

double
ModelRegion::getValue(string layerName, int op){
	int nPx = myPixels.size();
	double sumvalue=0;
	for(uint i=0;i<nPx; i++){
		sumvalue += myPixels[i]->getDoubleValue(layerName,true);
	}
	if(op==OP_SUM){
		return sumvalue;
	} else if (op == OP_AVG) {
		return sumvalue/nPx;
	} else {
		string thisf = __PRETTY_FUNCTION__;
		msgOut(MSG_CRITICAL_ERROR, "in "+thisf+", operation not supported");
	}
	return 0.;
}

void
ModelRegion::setMyPixels(){
	int xyNPixels = MTHREAD->GIS->getXyNPixels();
	for(uint i=0;i<xyNPixels;i++){
			Pixel* px = MTHREAD->GIS->getPixel(i);
			if(px->getDoubleValue("regLev_1")==regId || px->getDoubleValue("regLev_2")==regId){
				myPixels.push_back(px);
			}
		}
}



