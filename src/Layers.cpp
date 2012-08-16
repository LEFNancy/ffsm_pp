/***************************************************************************
 *   Copyright (C) 2006-2008 by Antonello Lobianco                         *
 *   antonello@regmas.org                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <QtCore>

#include <math.h>
#include <algorithm>

#include "Layers.h"
#include "Gis.h"
#include "ThreadManager.h"
#include "Scheduler.h"

Layers::Layers(ThreadManager* MTHREAD_h, string name_h, string label_h, bool isInteger_h, bool dynamicContent_h, string fullFilename_h)
{
	MTHREAD=MTHREAD_h;
	name = name_h;
	label = label_h;
	isInteger = isInteger_h;
	dynamicContent = dynamicContent_h;
	fullFileName = fullFilename_h;
}

Layers::~Layers()
{
}

void
Layers::addLegendItem(int ID_h, string label_h, int rColor_h, int gColor_h, int bColor_h, double minValue_h, double maxValue_h){

	for (uint i=0;i<legendItems.size();i++){
		if (legendItems.at(i).ID == ID_h){
			msgOut(MSG_ERROR, "Trying to add a legend item that already exist on this layer (layer: "+label+" - legend label: "+label_h+")");
			//cout << "ID: "<<ID_h<<" Label: "<<label_h<<" minValue: "<<minValue_h << " maxValue: "<<maxValue_h<<endl;
			return;
		}
	}

	LegendItems ITEM;
	ITEM.ID = ID_h;
	ITEM.label = label_h;
	ITEM.rColor = rColor_h;
	ITEM.gColor = gColor_h;
	ITEM.bColor = bColor_h;
	ITEM.minValue = minValue_h;
	ITEM.maxValue = maxValue_h;
	ITEM.cashedCount=0;
	legendItems.push_back(ITEM);	

}

void
Layers::addLegendItems(vector<LegendItems> legendItems_h){
	vector <LegendItems> toAdd;
	for(uint i=0; i<legendItems_h.size();i++){
		bool existing = false;
		for (uint j=0;j<legendItems.size();j++){
			if(legendItems_h[i].ID == legendItems[j].ID){
				existing = true;
				break;
			}
		}
		if(existing){
			msgOut(MSG_WARNING, "Legend item "+i2s(legendItems_h[i].ID)+" non added on layer "+this->name+" as already existing.");
		} else {
			toAdd.push_back(legendItems_h[i]);
		}
	}
	legendItems.insert( legendItems.end(), toAdd.begin(), toAdd.end() );
}



void
Layers::addReclassificationRule(int inCode_h, int outCode_h, double p_h){

	ReclassRules RULE;
	RULE.inCode=inCode_h;
	RULE.outCode = outCode_h;
	RULE.p = p_h;
	reclassRulesVector.push_back(RULE);
}

/**
Used in the init stage, this function take as input the real map code as just read from the map file, and filter it according to the reclassification rules.
@see ReclassRules
*/
double
Layers::filterExogenousDataset(double code_h){
	bool check =false;
	std::vector <double> cumPVector;
	std::vector <double> outCodesVector;
	double cumP = 0;
	double returnCode=0;
	
	for(uint i=0; i<reclassRulesVector.size(); i++){
		if (reclassRulesVector.at(i).inCode == code_h){
			check = true;
			cumP += reclassRulesVector.at(i).p;
			cumPVector.push_back(cumP);
			outCodesVector.push_back(reclassRulesVector.at(i).outCode);
		}
	}
	if (!check) {return code_h;}
	if (cumP <= 0.99999999 || cumP >= 1.00000001){msgOut(MSG_CRITICAL_ERROR,"the sum of land use reclassification rules is not 1 for at least one input code (input code:  "+d2s(code_h)+"; cumP: "+d2s(cumP)+")");}
	double random;
	//srand(time(NULL)); // this would re-initialise the random seed
	random = ((double)rand() / ((double)(RAND_MAX)+(double)(1)) );
	for(uint i=0; i<cumPVector.size(); i++){
		if (random <= cumPVector.at(i)){
			returnCode = outCodesVector.at(i);
			break;	
		}
	}	
	return returnCode;
}

/**
This function take as input the value stored in the pixel for the specific layer, loops over the legend item and find the one that match it, returning its color.
<br>If the layer is of type integer, the match is agains legendItem IDs, otherwise we compare the legendItem ranges.
@see LegendItems
*/
QColor
Layers::getColor(double ID_h){
	QColor nocolor(255,255,255);
	if (ID_h == MTHREAD->MD->getDoubleSetting("noValue")){
		return nocolor;
	}
	if (isInteger){
		for(uint i=0; i<legendItems.size(); i++){
			if (legendItems.at(i).ID == ((int)ID_h)){
				QColor color(legendItems.at(i).rColor, legendItems.at(i).gColor, legendItems.at(i).bColor);
				return color;
			}
		}
		return nocolor;
	}
	else {
		for(uint i=0; i<legendItems.size(); i++){
			if (ID_h < legendItems.at(i).maxValue &&  ID_h >= legendItems.at(i).minValue){
				QColor color(legendItems.at(i).rColor, legendItems.at(i).gColor, legendItems.at(i).bColor);
				return color;
			}
		}
		return nocolor;	
	}
}
/**
This function take as input the value stored in the pixel for the specific layer, loops over the legend item and find the one that match it, returning its label.
<br>If the layer is of type integer, the match is agains legendItem IDs, otherwise we compare the legendItem ranges.
@see LegendItems
*/
string
Layers::getCategory(double ID_h){
	if (ID_h == MTHREAD->MD->getDoubleSetting("noValue")){
		return "";
	}
	if (isInteger){
		for(uint i=0; i<legendItems.size(); i++){
			if (legendItems.at(i).ID == ((int)ID_h)){
				return legendItems.at(i).label;
			}
		}
		return "";
	}
	else {
		for(uint i=0; i<legendItems.size(); i++){
			if (ID_h < legendItems.at(i).maxValue &&  ID_h >= legendItems.at(i).minValue){
				return legendItems.at(i).label;
			}
		}
		return "";	
	}
}




void
Layers::countMyPixels(bool debug){

	for (uint i=0; i<legendItems.size(); i++){
		legendItems.at(i).cashedCount=0; //initialized with 0 values...
	}
	double totPixels = MTHREAD->GIS->getXyNPixels();
	double pixelValue;
	for (uint j=0;j<totPixels;j++){
		pixelValue = MTHREAD->GIS->getPixel(j)->getDoubleValue(name);
		if (isInteger){
			for(uint i=0; i<legendItems.size(); i++){
				if (legendItems.at(i).ID == ((int)pixelValue)){
					legendItems.at(i).cashedCount++;
					break;
				}
			}
		}
		else {
			for(uint i=0; i<legendItems.size(); i++){
				if (pixelValue < legendItems.at(i).maxValue &&  pixelValue >= legendItems.at(i).minValue){
					legendItems.at(i).cashedCount++;
					break;
				}
			}
		}
	}
	if (debug){
		msgOut(MSG_INFO, "Layer statistics - Count by Legend items");
		msgOut(MSG_INFO, "Layer name: "+label);
		msgOut(MSG_INFO, "Total plots: "+ d2s(totPixels));
		for(uint i=0;i<legendItems.size();i++){
			msgOut(MSG_INFO, legendItems.at(i).label+": "+i2s(legendItems.at(i).cashedCount));
		}
	}
}
void
Layers::randomShuffle(){


	vector <double> origValues;
	int maskValue = -MTHREAD->GIS->getNoValue();
	double totPixels = MTHREAD->GIS->getXyNPixels();
	for (uint i=0;i<totPixels;i++){
		double pxValue= MTHREAD->GIS->getPixel(i)->getDoubleValue(name);
		if(pxValue != MTHREAD->GIS->getNoValue()){
			origValues.push_back(pxValue);
			MTHREAD->GIS->getPixel(i)->changeValue(name,maskValue);
		}
	}
	random_shuffle(origValues.begin(), origValues.end()); // randomize the elements of the array.

	for (uint i=0;i<totPixels;i++){
		double pxValue= MTHREAD->GIS->getPixel(i)->getDoubleValue(name);
		if(pxValue != MTHREAD->GIS->getNoValue()){
			double toChangeValue = origValues.at(origValues.size()-1);
			//cout << toChangeValue << endl;
			origValues.pop_back();
			MTHREAD->GIS->getPixel(i)->changeValue(name,toChangeValue);
		}
	}

}
void
Layers::print(){

	if(MTHREAD->MD->getIntSetting("outputLevel")<OUTVL_MAPS) return;
	string mapBaseDirectory = MTHREAD->MD->getBaseDirectory()+MTHREAD->MD->getOutputDirectory()+"maps/";
	string mapGridOutputDirectory = mapBaseDirectory+"asciiGrids/";
	string catsOutputDirectory = mapBaseDirectory+"cats/";
	string coloursOutputDirectory = mapBaseDirectory+"colr/";

	string mapFilename      = mapGridOutputDirectory +name+ "_" +i2s(MTHREAD->SCD->getYear()) +"_" +MTHREAD->getScenarioName();
	string catsFilename     = catsOutputDirectory    +name+ "_" +i2s(MTHREAD->SCD->getYear()) +"_" +MTHREAD->getScenarioName();
	string coloursFilename  = coloursOutputDirectory +name+ "_" +i2s(MTHREAD->SCD->getYear()) +"_" +MTHREAD->getScenarioName();
	string filenameListIntLayers = mapBaseDirectory+"integerListLayers";
	string filenameListFloatLayers = mapBaseDirectory+"floatListLayers";

	// printing the map...
	string header;
	if(MTHREAD->MD->getIntSetting("mapOutputFormat") == 1){ // GRASS ASCII Grid
		header =      "north: " + d2s(MTHREAD->GIS->getGeoTopY()) + "\n"
					+ "south: " + d2s(MTHREAD->GIS->getGeoBottomY()) + "\n"
					+ "east: " + d2s(MTHREAD->GIS->getGeoRightX()) + "\n"
					+ "west: " + d2s(MTHREAD->GIS->getGeoLeftX()) + "\n"
					+ "rows: " + i2s(MTHREAD->GIS->getYNPixels()) + "\n"
					+ "cols: " + i2s(MTHREAD->GIS->getXNPixels()) + "\n"
					+ "null: " + d2s(MTHREAD->GIS->getNoValue()) + "\n";

	} else if(MTHREAD->MD->getIntSetting("mapOutputFormat") == 2){
		header =      "ncols: " + i2s(MTHREAD->GIS->getXNPixels()) + "\n"
					+ "lrows: " + i2s(MTHREAD->GIS->getYNPixels()) + "\n"
					+ "xllcornel: " + d2s(MTHREAD->GIS->getGeoLeftX()) + "\n"
					+ "yllcorner: " + d2s(MTHREAD->GIS->getGeoBottomY()) + "\n"
					+ "cellsize: "  + d2s(MTHREAD->GIS->getXMetersByPixel()) + "\n"
					+ "nodata_value: " + d2s(MTHREAD->GIS->getNoValue()) + "\n";
		if(MTHREAD->GIS->getXMetersByPixel() != MTHREAD->GIS->getYMetersByPixel()){
			msgOut(MSG_ERROR, "The X resolution is different to the Y resolution. I am exporting the map in ArcInfo ASCII Grid format using the X resolution, but be aware that it is incorrect, as this format doesn't support different X-Y resolutions.");
		}
	
	} else {
		msgOut(MSG_ERROR,"Map not print for unknow output type.");
	}

	ofstream outm; //out map
	outm.open(mapFilename.c_str(), ios::out); //ios::app to append..
	if (!outm){ msgOut(MSG_ERROR,"Error in opening the file "+mapFilename+".");}
	outm << header << "\n";

	for (int i=0;i<MTHREAD->GIS->getYNPixels();i++){
		for (int j=0;j<MTHREAD->GIS->getXNPixels();j++){
			outm << MTHREAD->GIS->getPixel(j, i)->getDoubleValue(name) << " ";
		}
		outm << "\n";
	}
	outm.close();

	//printing the cat file
	ofstream outc; //out category file
	outc.open(catsFilename.c_str(), ios::out); //ios::app to append..
	if (!outc){ msgOut(MSG_ERROR,"Error in opening the file "+catsFilename+".");}
	outc << "# " << name  << "_-_" << i2s(MTHREAD->SCD->getYear()) << "\n\n\n";
	outc << "0.00 0.00 0.00 0.00"<<"\n";

	if (isInteger){
		for(uint i=0;i<legendItems.size();i++){
			outc << legendItems[i].ID << ":"<< legendItems[i].label << "\n";
		}
	}
	else {
		for(uint i=0;i<legendItems.size();i++){
			outc << legendItems[i].minValue << ":"<< legendItems[i].maxValue << ":"<< legendItems[i].label << "\n";
		}
	}

	//printing the colour legend file
	ofstream outcl; //out colour file
	outcl.open(coloursFilename.c_str(), ios::out); //ios::app to append..
	if (!outcl){ msgOut(MSG_ERROR,"Error in opening the file "+coloursFilename+".");}
	outcl << "% " << name  << "_-_" << i2s(MTHREAD->SCD->getYear()) << "\n\n\n";
	
	if (isInteger){
		for(uint i=0;i<legendItems.size();i++){
			outcl << legendItems[i].ID << ":"<< legendItems[i].rColor << ":" << legendItems[i].gColor << ":" << legendItems[i].bColor << "\n";
		}
	}
	else {
		for(uint i=0;i<legendItems.size();i++){
			outcl << legendItems[i].minValue << ":"<< legendItems[i].rColor << ":" << legendItems[i].gColor << ":" << legendItems[i].bColor << " "<< legendItems[i].maxValue << ":"<< legendItems[i].rColor << ":" << legendItems[i].gColor << ":" << legendItems[i].bColor << "\n";
		}
	}

	// adding the layer to the list of saved layers..
	ofstream outList;
	if (isInteger){
		outList.open(filenameListIntLayers.c_str(), ios::app); // append !!!
		outList << name << "_" << MTHREAD->SCD->getYear() << "_" << MTHREAD->getScenarioName() << "\n";
	}
	else {
		outList.open(filenameListFloatLayers.c_str(), ios::app); // append !!!
		outList << name << "_" << MTHREAD->SCD->getYear() << "_" << MTHREAD->getScenarioName() << "\n";
	}
	outList.close();
}

void
Layers::printBinMap(){

	int xNPixels            = MTHREAD->GIS->getXNPixels();
	int subXR               = MTHREAD->GIS->getSubXR();
	int subXL               = MTHREAD->GIS->getSubXL();
	int subYT               = MTHREAD->GIS->getSubYT();
	int subYB               = MTHREAD->GIS->getSubYB();

	string mapBaseDirectory = MTHREAD->MD->getBaseDirectory()+MTHREAD->MD->getOutputDirectory()+"maps/bitmaps/";
	string mapFilename      = mapBaseDirectory +name+ "_" +i2s(MTHREAD->SCD->getYear()) +"_" +MTHREAD->getScenarioName()+".png";

	QImage image = QImage(subXR-subXL+1, subYB-subYT+1, QImage::Format_RGB32);
	image.fill(qRgb(255, 255, 255));
	for (int countRow=subYT;countRow<subYB;countRow++){
		for (int countColumn=subXL;countColumn<subXR;countColumn++){
			double value = MTHREAD->GIS->getPixel(countRow*xNPixels+countColumn)->getDoubleValue(name);
			QColor color = this->getColor(value);
			image.setPixel(countColumn-subXL,countRow-subYT,color.rgb());
		}
	}
	image.save(mapFilename.c_str());
}
