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
#include "Pixel.h"
#include "ThreadManager.h"
#include "Scheduler.h"
#include "Init.h"

Pixel::Pixel(double ID_h, ThreadManager* MTHREAD_h): ID(ID_h)
{
	pxOwner=NULL;
	pxTenant=NULL;
	cachedAgent=NULL;
	MTHREAD=MTHREAD_h;
	rentingTerm=0;
	startRentingYear=0;
	rentalCosts=0;
	shRentalPriceCoeff=0;
}

Pixel::~Pixel()
{
}

/**
The function return a vector of pointers to Pixels at the gived distance from the caller pixel.\\
The list start with those on the Top, then add those on the right, those on the bottom and those on the left. Finally it had the corner pixels (that are more far).\\
It takes into consideration borders correctly.

Fully tested on internal points as well semi-border cases, border cases and corner cases. ALL OK.

@param distLevel_h Distance in number of adiacent pixels. It has to be at least 1 (the function return an error if it is 0).
*/
vector <Pixel *>
Pixel::getPixelsAtDistLevel(int distLevel_h) const{

	if (distLevel_h<1) {
		msgOut(MSG_CRITICAL_ERROR, "getPixelsAtDistLevel() is defined for distances of at least 1 !");
	}

	vector <Pixel *> toReturn;
	int xNPixels = MTHREAD->GIS->getXNPixels();
	int yNPixels = MTHREAD->GIS->getYNPixels();
	int thisX = this->getX();
	int thisY = this->getY();
	int minX = max(0        , (thisX - distLevel_h)+1);
	int maxX = min(xNPixels , thisX + distLevel_h);
	int minY = max(0        , (thisY - distLevel_h)+1);
	int maxY = min(yNPixels , thisY + distLevel_h);

	// getting the top pixels (corner exluded)...
	if (thisY-distLevel_h >=0){
		for(int i=minX;i<maxX;i++){
			toReturn.push_back(MTHREAD->GIS->getPixel(i,thisY-distLevel_h));
		}
	}
	// getting the right pixels (corner exluded)...
	if (thisX+distLevel_h < xNPixels){
		for(int i=minY;i<maxY;i++){
			toReturn.push_back(MTHREAD->GIS->getPixel(thisX+distLevel_h,i));
		}
	}
	// getting the bottom pixels (corner exluded)...
	if (thisY+distLevel_h < yNPixels){
		for(int i=minX;i<maxX;i++){
			toReturn.push_back(MTHREAD->GIS->getPixel(i,thisY+distLevel_h));
		}
	}
	// getting the left pixels (corner exluded)...
	if (thisX-distLevel_h >= 0){
		for(int i=minY;i<maxY;i++){
			toReturn.push_back(MTHREAD->GIS->getPixel(thisX-distLevel_h,i));
		}
	}

	// getting the corners (correctly at the end, after already retrieved the other pixels..)...
	// top-left..
	if (thisX-distLevel_h >= 0 && thisY-distLevel_h >=0){
		toReturn.push_back(MTHREAD->GIS->getPixel(thisX-distLevel_h,thisY-distLevel_h));
	}
	// top-right..
	if (thisX+distLevel_h < xNPixels && thisY-distLevel_h >=0){
		toReturn.push_back(MTHREAD->GIS->getPixel(thisX+distLevel_h,thisY-distLevel_h));
	}
	// bottom-right..
	if (thisX+distLevel_h < xNPixels && thisY+distLevel_h <yNPixels){ // bug discovered 20070719
		toReturn.push_back(MTHREAD->GIS->getPixel(thisX+distLevel_h,thisY+distLevel_h));
	}
	// bottom-left..
	if (thisX-distLevel_h >= 0 && thisY+distLevel_h <yNPixels){
		toReturn.push_back(MTHREAD->GIS->getPixel(thisX-distLevel_h,thisY+distLevel_h));
	}
	return toReturn;
} 

void
Pixel::setValue(string layerName_h, double value_h){
	values.insert(pair<string, double>(layerName_h, value_h));
}

void
Pixel::changeValue(string layerName_h, double value_h){
	map<string, double>::iterator p;
	p=values.find(layerName_h);
	if(p != values.end())
		p->second = value_h;
	else 
		msgOut(MSG_ERROR, "Coud not change pixel value for layer "+layerName_h+". Layer don't found.");
	return;
}

double
Pixel::getDoubleValue(string layerName_h) const{
	map<string, double>::const_iterator p;
	p=values.find(layerName_h);
	if(p != values.end())
		return p->second;
	else 
		msgOut(MSG_WARNING, "No layer \""+layerName_h+"\" found on pixel ("+i2s(getX())+","+i2s(getY())+"). Sure you didn't mispelled it?");
		return MTHREAD->GIS->getNoValue();
}

/**
Assign the object to the plot.
@param case_h Stage where the assignment is done (STAGE_INIT or STAGE_NORMAL). In the first case we random the object age.
*/
void
Pixel::acquireObject(ModelObject acquiredObject_h, int case_h){
	if(case_h==STAGE_INIT){
		acquiredObject_h.setRandomAge();
	}
	else {
		acquiredObject_h.setStartYear(MTHREAD->SCD->getYear());
	}

	if (acquiredObject_h.getLastYear() > this->getLastRentingYear()){ // adjust the renting term to cover all the life period of the spatial object
		setRentingTerm(acquiredObject_h.getLastYear()-MTHREAD->SCD->getYear()+1);
	}

	if(pxTenant){
		acquiredObject_h.setOwner(((Agent_base*)this->pxTenant)); // if it is a rented plot, spatial objects are ALWAYS assigned to the tenant
	}
	else if (pxOwner){
		acquiredObject_h.setOwner(((Agent_base*)this->pxOwner)); // if it is a owned plot, spatial objects are assigned to the pixel owner
	}
	else {
		if(case_h != STAGE_INIT){ // is not an error in the intit stage, as we have objects that are not owned by anyone!
			msgOut(MSG_CRITICAL_ERROR, "who-ho.. who paid for a spatial object to a plot without both owners and tenants???");
		}
	}
	
	myObjects.push_back(acquiredObject_h);
}

/**
Assign to the pixel one spacial object between those that are placeable on it (according to its soil type).
<br>The idea is that spacial objects can have many soil types where they can be placed, and one soil type can have many different objects type that can suit it. However in the init stage only one object can go on each plot, so we select between its candidates.
*/
void
Pixel::initObjects(){
	int landUse = ((int)this->getDoubleValue("landUse"));
	vector <ModelObject> candidates = MTHREAD->RD->getModelObjectsBySoilType(landUse);
	if (candidates.size()>0){
		int randomObject = int (( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*candidates.size());
		this->acquireObject(candidates.at(randomObject),STAGE_INIT);
	}
}

vector <ModelObject*>
Pixel::getIncludedObjects(){
	vector <ModelObject*> toReturn;
	for (uint i=0;i<myObjects.size();i++){
		toReturn.push_back(&myObjects.at(i));
	}
	return toReturn;
}

vector <string>
Pixel::getIncludedObjectNames() const {
	vector <string> toReturn;
	for (uint i=0;i<myObjects.size();i++){
		toReturn.push_back(myObjects.at(i).getName());
	}
	return toReturn;
}

void
Pixel::newYear(){
	
	if (getLastRentingYear() <= MTHREAD->SCD->getYear() ){
		destroyRentalContract();
	}

	vector<ModelObject>::iterator p;
	for ( p = myObjects.begin() ; p != myObjects.end();){
		if(p->getLastYear() <= MTHREAD->SCD->getYear()){
			myObjects.erase(p);
		}
		else {
			++p;
		}
	}

}

/**
Used when the rental contract go out of time-scope or when the farms leave freeing the land
*/
void
Pixel::destroyRentalContract(){
		//TOnotDO:pxTenant->removePixel(this); no.. this will be done by the agent, the plot doesn't know anything about the owner
		// This is the "distructor" of the renting contract...
		pxTenant = 0;
		rentingTerm = 0;
		startRentingYear=0;
		rentalCosts=0;
}


/**
This function set a new renting contract for the plot.
There could be more type of contract, but now only the "agrContract" type is implemented.
Normally contracts are randomly choosed betweer a minimum and a maximum. However, if objects are present in such plots and we are not in the init stage, contracts can not be longer than the object remaining age.
*/
void
Pixel::setRandomRenting(string contractType){
	if(contractType=="agrContract"){
		int minAgrContract = MTHREAD->RD->getIntSetting("minAgrContractLength");
		int maxAgrContract = MTHREAD->RD->getIntSetting("maxAgrContractLength");
		int maxObjectAge = 0;
		int remainingRentingYears =0;
		if(MTHREAD->INIT->getInitState() < 5) {
			for (uint i=0;i<myObjects.size();i++){
				if (myObjects[i].getRemainingAge()>maxObjectAge){
					maxObjectAge=myObjects[i].getRemainingAge();
				}
			}
			if (maxObjectAge > maxAgrContract){
				remainingRentingYears = minAgrContract +( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(maxAgrContract-minAgrContract+1); // [minAgrContract, maxAgrContract]
			}
			else if (maxObjectAge > minAgrContract){
				remainingRentingYears = minAgrContract +( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(maxObjectAge-minAgrContract+1); // [minAgrContract, maxObjectAge]
			}
			else if (maxObjectAge > 0){ // we have objects but with shorter years than minContract
				remainingRentingYears = maxObjectAge;
			}
			else { // no objects on plot
				remainingRentingYears = minAgrContract +( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(maxAgrContract-minAgrContract+1); // [minAgrContract, maxAgrContract]
			// if the farm invest on permanent crop the remainingRentingYears will be updated to the duration of the object
			}
			setRemainingRentingTerm(remainingRentingYears);
		}
		else { //we are in the init stage, remaining age can be less than minimum contract as the renting conctract may have happened in the past. Furthermore no spatial objects are yet assigned to plots, as this happen later on in the init process
			remainingRentingYears = 1 +( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(maxAgrContract-minAgrContract+1); // [1, maxAgrContract]
			setRemainingRentingTerm(remainingRentingYears);
		}
	}
	else {
		msgOut(MSG_ERROR, "Unknow renting contract. Skipped");
	}
}

bool
Pixel::isAgricultural() const{
	
	vector <int> agrLandTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	string valuesToMatch;
	unsigned int z;

	//string of the required land values to match;
	for (uint j=0;j<agrLandTypes.size();j++){
		valuesToMatch = valuesToMatch + " " + i2s(agrLandTypes.at(j));
	}

	z = valuesToMatch.find(d2s(this->getDoubleValue("landUse"))); // search if in the string of required values is included also the value of the current plot
	if( z!=string::npos ){
		return true;
	}
	else {
		return false;
	}
}

void
Pixel::setRemainingRentingTerm(int years_h){
	if(!startRentingYear) { // this is a new renting contract
		startRentingYear = MTHREAD->SCD->getYear(); // bug found 2007.10.16, Antonello
	}
	rentingTerm = MTHREAD->SCD->getYear()+years_h-startRentingYear;
}

int
Pixel::getRentingAge() const{
	return MTHREAD->SCD->getYear()-startRentingYear;
}

double
Pixel::getStartingRentalCosts() const {
	int landType = ((int)this->getDoubleValue("landUse"));
	vector <int> agrLandTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	vector <double> agrLandRents = MTHREAD->RD->getDoubleVectorSetting("agrInitialRentalPrices");
	for (uint i=0;i<agrLandTypes.size();i++){
		if(agrLandTypes[i]==landType) return agrLandRents.at(i);
	}
	msgOut(MSG_CRITICAL_ERROR, "Plot of unknow land type: "+i2s(landType));
	return 0;
}

