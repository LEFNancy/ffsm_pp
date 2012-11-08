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
#include "Pixel.h"
#include "ThreadManager.h"
#include "Scheduler.h"
#include "Init.h"

Pixel::Pixel(double ID_h, ThreadManager* MTHREAD_h): ID(ID_h)
{
	MTHREAD=MTHREAD_h;

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

/*void //moved as inline function
Pixel::setValue(const string & layerName_h, const double & value_h){

	//tempValuePair.first = layerName_h;     // type of first is string
	//tempValuePair.second = value_h;            // type of second is double
	//tempValuePair = make_pair (layerName_h,value_h);
	values.insert(pair<string, double>(layerName_h, value_h));
	//values.insert(tempValuePair);


}*/

inline void
Pixel::setValue (const string& parName, const string& forName, const string& dClass, const int& year, const double& value_h){
	values.insert(pair<string, double>(MTHREAD->GIS->pack(parName, forName, dClass, year), value_h));
}


void
Pixel::changeValue(const string &layerName_h, const double &value_h, const bool &setNoValueForZero){
	map<string, double>::iterator p;
	p=values.find(layerName_h);
	if(p != values.end()){
		if(setNoValueForZero && value_h == 0){
			p->second = MTHREAD->GIS->getNoValue();
		} else {
			p->second = value_h;
		}
	} else {
		msgOut(MSG_ERROR, "Coud not change pixel value for layer "+layerName_h+". Layer don't found.");
	}
	return;
}

void
Pixel::changeValue (const double &value_h, const string& parName, const string& forName, const string &dClass, const int &year, const bool &setNoValueForZero){
	changeValue(MTHREAD->GIS->pack(parName, forName, dClass, year), value_h, setNoValueForZero);
}

double
Pixel::getDoubleValue(const string &layerName_h, const bool &returnZeroForNoValue) const{
	vIter=values.find(layerName_h);
	if(vIter != values.end()) {
		if(returnZeroForNoValue && vIter->second==MTHREAD->GIS->getNoValue()){
			return 0.0;
		} else {
			return vIter->second;
		}
	} else {
		msgOut(MSG_WARNING, "No layer \""+layerName_h+"\" found on pixel ("+i2s(getX())+","+i2s(getY())+"). Sure you didn't mispelled it?");
		if(returnZeroForNoValue){
			return 0.0;
		} else {
			return MTHREAD->GIS->getNoValue();
		}
	}
}

/**
getMultiplier() returns the value of the multiplier as memorized in the spatialDataSubfolder layers.
It will look for exact match or for lower years if available.
If no layers are available or the usePixelData option is not used, it will return 1.
*/
double
Pixel::getMultiplier (const string& multiplierName, const string& forName, int year){

	// return 1 if not using pixel mode
	if(!MTHREAD->MD->getBoolSetting("usePixelData")) return 1.0;

	if(year==DATA_NOW){year = MTHREAD->SCD->getYear();}

	string search_for = multiplierName+"#"+forName+"##"+i2s(year);
	map <string,double>::const_iterator i = values.upper_bound(search_for); //return the position always upper to the found one, even if it's an equal match.
	if(i!= values.begin())	i--; // this rewind the position to the one just before or equal
	const string& key = i->first;
	string search_base = search_for.substr(0,search_for.size()-4);
	if (key.compare(0, search_base.size(), search_base) == 0){
		//cout << "MATCH: " << search_for <<", "<< i->first << ", " << i->second << endl;
		return i->second;
	} else {
		//cout << "NOTM:  " << search_for <<", "<< i->first << endl;
		return 1.0;
	}
}

double
Pixel::getDoubleValue (const string& parName, const string& forName, const string& dClass, const int& year, const  bool& returnZeroForNoValue){
	return getDoubleValue(MTHREAD->GIS->pack(parName, forName, dClass, year), returnZeroForNoValue);
}

void
Pixel::newYear(){

}

double
Pixel::getPastRegArea(const int& ft_idx, const int& year){
	map <int,vector<double> >::const_iterator i=regArea.find(year);
	if(i != regArea.end()) {
		return i->second.at(ft_idx);
	} else {
		msgOut(MSG_ERROR, "Asking for a pastRegArea of a not-registered year. I don't have year "+i2s(year)+"!");
	}
}

void
Pixel::setPastRegArea(const double& value, const int& ft_idx, const int& year){
	msgOut(MSG_CRITICAL_ERROR,"TODO");
	/*map <int,vector<double> >::const_iterator i=regArea.find(year);
	if(i != regArea.end()) {
		// we already have this year, let's see if the vector is bif enough
		int currsize = i->second.size();
		for(j=0;j<ft_idx-currside;j++){

		}
		return i->second.at(ft_idx);
	} else {
		// new year
	}


	pair<int,vector<double> newRegArea;
	*/


}

void
Pixel::swap(const int& swap_what){
	switch (swap_what){
		case VAR_VOL:
			vol_l = vol;
			break;
		default:
			msgOut(MSG_CRITICAL_ERROR,"Don't know how to swap "+swap_what);
	}
}

