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
#ifndef STDGIS_H
#define STDGIS_H

#include <cstdlib>
#include <list>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <sstream>

// regmas headers..
#include "BaseClass.h"
#include "ModelData.h"
#include "Layers.h"
#include "Pixel.h"
#include "ModelRegion.h"

using namespace std;

struct lUseCats;
struct reclassRules;
class Pixel;
class Agent_space;
class QImage;


/// Class to manage the spatial dimension
/**
Gis class is responsable to provide all methods for spatial analysis.
<br>It is equipped with two important vectors: 
  - pxVector   contains the array of all pixels on the screen 
  - layerVector contains the layer objects 
<br>Along the model, IDs of pixels are assigned from left to right, from top to down:
<br>	--->
<br>	 /
<br>	--->
<br>	 /
<br>	--->
<p>Pixel origin (0,0) on the top left corner is also the system used by the underlying libraries, but put attencion that instead geographical coordinates, if we are on the North emisfere, are increasing along the up-right direction.

@author Antonello Lobianco
*/

class Gis: public BaseClass{

public:
	                    Gis(ThreadManager* MTHREAD_h); ///< Constructor
	                   ~Gis();
	/// Set the initial space environment, including loading data from files
	void                setSpace();
	/// Init the layers
	void                initLayers();
	void                initLayersPixelData();
	void                initLayersModelData(const int& year_h=DATA_NOW);
	/// Apply the forest reclassification with the rules defined in reclRules sheet
	void                applyForestReclassification();
	/// If subregion mode is on, this function place noValues on the selected layer for all out-of-region pixels
	void                filterSubRegion(string layerName_h);
	///< Update the image behind a layer to the GUI;
	void                updateImage(string layerName_h);
	///< Add one layer to the system
	void                addLayer(string name_h, string label_h, bool isInteger_h, bool dynamicContent_h, string fullFileName_h = "", bool display_h=true);
	///< Fill a layer with empty values
	void                resetLayer(string layerName_h);
	///< Check if a layer with a certain name is loaded in the model. Used e.g. to check if the dtm layer (optional) exist.
	bool                layerExist(const string & layerName_h, bool exactMatch = true) const;
	///< Return a pointer to a layer given its name
	Layers*              getLayer(const string& layerName_h);
	///< Add a legend item to an existing layer
	void                addLegendItem (
	                        string       name_h,
	                        int             D_h,
	                        string      label_h,
	                        int        rColor_h,
	                        int        gColor_h,
	                        int        bColor_h,
	                        double   minValue_h,
	                        double   maxValue_h   );
	/// Add a reclassification rule to an existing layer
	void                addReclassificationRule(string name_h, int inCode_h, int outCode_h, double p_h);
	/// Count the pixels within each legend item for the selected layer
	void                countItems(const string& layerName_h, const bool& debug=false);
	/// Return a pointer to a plot with a specific value for the specified layer
	Pixel*              getRandomPlotByValue(string layer_h, int layerValue__h);
	/// Return the vector (shuffled) of all plots with a specific value for a specified layer. It is also possible to specify the level in case of failure
	vector <Pixel*>     getAllPlotsByValue(string layer_h, int layerValue_h, int outputLevel=MSG_WARNING);
	/// Return the vector (shuffled) of all plots with specific values for a specified layer. It is also possible to specify the level in case of failure
	vector <Pixel*>     getAllPlotsByValue(string layer_h, vector<int> layerValues_h, int outputLevel=MSG_WARNING);
	/// Return the vector (shuffled) of all plots. It is also possible to specify the level in case of failure
	vector <Pixel*>     getAllPlots(int outputLevel=MSG_WARNING);
	/// Return the vector of all plots by a specific region (main region or subregion), optionally shuffled;
	vector <Pixel*>     getAllPlotsByRegion(ModelRegion &region_h, bool shuffle=false);
	vector <Pixel*>     getAllPlotsByRegion(int regId_h, bool shuffle=false);
	/// Return a vector of the layer ids (as string)
	vector <string>    getLayerNames();
	/// Return a vector of pointers of existing layers
	vector <Layers*>    getLayerPointers();
	/// Print the specified layer or all layers (if param layerName_h is missing). @see Layers::print()
	void                printLayers(string layerName_h="");
	/// Save an image in standard png format
	void                printBinMaps(string layerName_h="");


	///< Print debug information (for each pixel in the requested interval, their values on the specified layer)
	void                printDebugValues (string layerName_h, int min_h=0, int max_h=0);
	double              getDistance(const Pixel* px1, const Pixel* px2);

	int                 getXNPixels() const {return xNPixels;};      ///< Return the number of pixels on X
	int                 getYNPixels() const {return yNPixels;};      ///< Return the number of pixels on Y
	double              getXyNPixels()const {return xyNPixels;};     ///< Return the total number of pixels
	double              getHaByPixel() const {return ((xMetersByPixel*yMetersByPixel)/10000) ;};
	double              getNoValue() const {return noValue;};
	Pixel*              getPixel(int x_h, int y_h){return &pxVector.at(x_h+y_h*xNPixels);};  ///< Return a pixel pointer from its coordinates
	Pixel*              getPixel(int ID_h){return &pxVector.at(ID_h);};                   ///< Return a pixel pointer from its ID
	double              getGeoTopY() const {return geoTopY;};
	double              getGeoBottomY() const {return geoBottomY;};
	double              getGeoLeftX() const {return geoLeftX;};
	double              getGeoRightX() const {return geoRightX;};
	double              getXMetersByPixel() const {return xMetersByPixel;};
	double              getYMetersByPixel() const {return yMetersByPixel;};
	int                 getSubXL() const {return subXL;};
	int                 getSubXR() const {return subXR;};
	int                 getSubYT() const {return subYT;};
	int                 getSubYB() const {return subYB;};
	/// Transform the ID of a pixel in subregion coordinates to the real (and model used) coordinates
	int                 sub2realID(int id_h);
	string  pack(const string& parName, const string& forName, const string& dClass, const int& year) const {return parName+"#"+forName+"#"+dClass+"#"+i2s(year)+"#";};
	void unpack(const string& key, string& parName, string& forName, string& dClass, int& year) const;

	void                swap(const int &swap_what);

private:
	void                loadLayersDataFromFile(); ///< Load the data of a layer its datafile
	void          applySpatialStochasticValues(); ///< Apply stochastic simulation, e.g. regional volume growth s.d. -> tp multipliers
	void          applyStochasticRiskAdversion(); ///< Give to each agend a stochastic risk adversion. For now Pixel = Agent
	vector <Pixel>                      pxVector; ///< array of Pixel objects
	vector <Layers>                  layerVector; ///< array of Layer objects	
	vector <double>                   lUseTotals; ///< totals, in ha, of area in the region for each type (cached values)
	int                                 xNPixels; ///< number of pixels along the X dimension
	int                                 yNPixels; ///< number of pixels along the Y dimension
	double                             xyNPixels; ///< total number of pixels
	double                        xMetersByPixel; ///< pixel dimension (meters), X
	double                        yMetersByPixel; ///< pixel dimension (meters), Y
	double                              geoLeftX; ///< geo-coordinates of the map left border
	double                               geoTopY; ///< geo-coordinates of the map upper border
	double                             geoRightX; ///< geo-coordinates of the map right border
	double                            geoBottomY; ///< geo-coordinates of the map bottom border
	double                               noValue; ///< value internally use as novalue (individual layer maps can have other values)
	int                                    subXL; ///< sub region left X
	int                                    subXR; ///< sub region right X
	int                                    subYT; ///< sub region top Y
	int                                    subYB; ///< sub region bottom Y
	bool                           subRegionMode;


};

#endif
