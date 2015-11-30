/***************************************************************************
 *   Copyright (C) 2015 by Laboratoire d'Economie Forestière               *
 *   http://ffsm-project.org                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version, given the compliance with the     *
 *   exceptions listed in the file COPYING that is distribued together     *
 *   with this file.                                                       *
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
#include <algorithm> //alghoritm used to shuffle (randomize) the array
#include <QtCore>
#include <math.h>

#include "Gis.h"
#include "Pixel.h"

//#include "InputDocument.h"
#include "MainWindow.h"
#include "Scheduler.h"

using namespace std;

/**
The constructor of the GIS (unique) instance want:
@param RD_h      Pointer to the manager of the regional data
@param MTHREAD_h Pointer to the main thread manager
*/
Gis::Gis(ThreadManager* MTHREAD_h){
  MTHREAD=MTHREAD_h;
  subRegionMode = false;
}

Gis::~Gis(){
}

/**
setSpace is called directly from the init system to setting the space environment in the model.
<br>It is responsable to:
 - define map dimensions (from setting files)
 - create the pixels
 - initialize the layer @see initLayers
 - load the layer data from their fdata-files @see loadLayersDataFromFile
 - tell the GUI that our map will have (x,y) dimensions
*/
void
Gis::setSpace(){



  msgOut(MSG_INFO,"Creating the space...");

  // init basic settings....
  geoTopY = MTHREAD->MD->getDoubleSetting("geoNorthEdge");
  geoBottomY = MTHREAD->MD->getDoubleSetting("geoSouthEdge");
  geoLeftX = MTHREAD->MD->getDoubleSetting("geoWestEdge");
  geoRightX = MTHREAD->MD->getDoubleSetting("geoEastEdge");
  xNPixels = MTHREAD->MD->getIntSetting("nCols");
  yNPixels = MTHREAD->MD->getIntSetting("nRows");
  noValue = MTHREAD->MD->getDoubleSetting("noValue");
  xyNPixels = xNPixels * yNPixels;
  xMetersByPixel = (geoRightX - geoLeftX)/xNPixels;
  yMetersByPixel = (geoTopY - geoBottomY)/yNPixels;
  MTHREAD->treeViewerChangeGeneralPropertyValue("total plots", d2s(getXyNPixels()));
  MTHREAD->treeViewerChangeGeneralPropertyValue("total land", d2s(xyNPixels*getHaByPixel()));
  // creating pixels...
  for (int i=0;i<yNPixels;i++){
    for (int j=0;j<xNPixels;j++){
      Pixel myPixel(i*xNPixels+j, MTHREAD);
      myPixel.setCoordinates(j,i);
      pxVector.push_back(myPixel);
    }
  }
  initLayers();
  loadLayersDataFromFile();

  // Cashing the pixels owned by each region..
  vector <ModelRegion*> regions = MTHREAD->MD->getAllRegions();
  int nRegions = regions.size();
  for(uint i=0;i<nRegions;i++){
    regions[i]->setMyPixels();
  }

  applySpatialStochasticValues(); // regional variance -> different tp in each pixel trought tp modifiers
  applyStochasticRiskAdversion(); // risk adversion to each pixel
  cachePixelValues(); // For computational reasons cache some values in the constant layers directly as properties of the pixel object

//  //< Print a layer of pixels id..
//  addLayer("pxIds", "idx of the pixels", true, true, "pxIds.grd", true);
//  resetLayer("pxIds");
//  vector<Pixel*> allPixels = getAllPlotsByRegion(11000);
//  for (int i=0;i<allPixels.size();i++){
//    int pxId= allPixels[i]->getID();
//    allPixels[i]->changeValue ("pxIds", pxId);
//  }
//  printLayers("pxIds");


  MTHREAD->fitInWindow(); // tell the gui to fit the map to the widget
//  countItems("landUse",false); // count the various records assigned to each legendItem. Do not print debug infos
  return;
}


/**
 * Apply all stochastic modifications required by the model at init time.
 * Currently used to change time of passage devending on regional variance
 **/

void
Gis::applySpatialStochasticValues(){
  // apply regional volume growth st.dev. -> variance to pixel based t.p.
  // - cashing value to the pixels
  // - apply to the tp layers with change values

  if(!MTHREAD->MD->getBoolSetting("usePixelData")) return;

  vector <int> regIds2 = MTHREAD->MD->getRegionIds(2);
  //ModelRegion* reg = MTHREAD->MD->getRegion(regIds2[i]);
  //vector<Pixel*> regPixels = region->getMyPixels();
  //double sumc = 0;
  //double nc = 0;
  for(uint i=0;i<regIds2.size();i++){
    ModelRegion* reg = MTHREAD->MD->getRegion(regIds2[i]);
    vector <Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regIds2[i]);
    vector <string> fTypes = MTHREAD->MD->getForTypeIds();

    // regional variance
    if(MTHREAD->MD->getBoolSetting("useSpatialRegionalVariance")){
      for(uint j=0; j<fTypes.size(); j++){
        double sStDev = MTHREAD->MD->getForData("sStDev",regIds2[i],fTypes[j],""); // spatial standard deviation
        double agr = MTHREAD->MD->getForData("agr",regIds2[i],fTypes[j],""); // average growth
        // BUG solved 20141220 To obtain a population with the same avg and st.dev of the original using moltipliers, I need to use the cv not the st.dev. !
        // tested with excel
        normal_distribution<double> d(1,sStDev/agr); // default any how to double
        for (uint z=0;z<rpx.size();z++){
          double c = d(*MTHREAD->gen);
          double c2 = max(0.4,min(1.6,c)); /// with simmetric boundary on the cv I do not change the average, but of course I slighly reduce the stdev. See file monte_carlo_with_multipliers_sample_proof.ods
          // TO.DO: Convert it to using normSample where instead of a min/max a loop is used to fund smaples that are within the bounds
          //cout << regIds2[i] << ";" <<sStDev <<";"<< c <<endl
          //rpx[z]->correctInputMultiplier("tp_multiplier",fTypes[j],c);
          //cout << sStDev/agr << "    " << c2 << endl;
          rpx[z]->setSpModifier(c2,j);
          //sumc += c;
          //nc ++;
        }
      }
    }

    // expectation types
    double avgExpTypes       = MTHREAD->MD->getDoubleSetting("expType");
    double avgExpTypesPrices = MTHREAD->MD->getDoubleSetting("expTypePrices");
    double expTypes_cv       = MTHREAD->MD->getDoubleSetting("expType_cv");
    double expTypesPrices_cv = MTHREAD->MD->getDoubleSetting("expTypePrices_cv");
    if((avgExpTypes<0 || avgExpTypes>1) && avgExpTypes != -1){
      msgOut(MSG_CRITICAL_ERROR, "expType parameter must be between 1 (expectations) and 0 (adaptative) or -1 (fixed).");
    }
    if(avgExpTypesPrices<0 || avgExpTypesPrices>1){
      msgOut(MSG_CRITICAL_ERROR, "vgExpTypesPrices parameter must be between 1 (expectations) and 0 (adaptative).");
    }
    //cout << avgExpTypes << "     " << expTypes_cv << endl;

    normal_distribution<double> exp_distr(avgExpTypes,expTypes_cv *avgExpTypes); // works only for double, but default any how to double
    normal_distribution<double> expPrices_distr(avgExpTypesPrices,expTypesPrices_cv *avgExpTypesPrices);

    for (uint z=0;z<rpx.size();z++){
      if(avgExpTypes == -1){
        rpx[z]->expType = -1;
      } else {
        //double c = exp_distr(*MTHREAD->gen);
        //double c2 = max(0.0,min(1.0,c)); /// Bounded [0,1]. With simmetric boundary on the cv I do not change the average, but of course I slighly reduce the stdev. See file monte_carlo_with_multipliers_sample_proof.ods
        double c3 = normSample(exp_distr,*MTHREAD->gen,0.0,1.0);
        //cout << "Sampled:\t" << c3 <<  endl;
        rpx[z]->expType = c3;
      }
      double cPrice = normSample(expPrices_distr,*MTHREAD->gen,0.0,1.0);
      rpx[z]->expTypePrices = cPrice;
    }
  }
}

/**
 * Apply to each agent a random risk-adversion coefficient
 *
 *For now, 1 pixel = 1 agent, and avg and st.dev. are the same in the model, but eventually this can change
 **/
void
Gis::applyStochasticRiskAdversion(){
  // apply regional volume growth st.dev. -> variance to pixel based t.p.
  // - cashing value to the pixels
  // - apply to the tp layers with change values

  if(!MTHREAD->MD->getBoolSetting("usePixelData")) return;

  vector <int> regIds2 = MTHREAD->MD->getRegionIds(2);
  bool raEnabled = MTHREAD->MD->getBoolSetting("heterogeneousRiskAversion");
  for(uint i=0;i<regIds2.size();i++){
    ModelRegion* reg = MTHREAD->MD->getRegion(regIds2[i]);
    vector <Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regIds2[i]);
    for (uint z=0;z<rpx.size();z++){
      if(raEnabled){
        double raStDev = MTHREAD->MD->getDoubleSetting("riskAversionAgentSd");
        double avg = MTHREAD->MD->getDoubleSetting("riskAversionAgentAverage");
        normal_distribution<double> d(avg,raStDev); // default any how to double
        double c = d(*MTHREAD->gen);
        rpx[z]->setValue ("ra", c);
      } else {
        rpx[z]->setValue ("ra", 0.0);
      }
    }
  }
}

void
Gis::cachePixelValues(){
  /// Set the avalCoef (availability coefficient) from layer
  if(!MTHREAD->MD->getBoolSetting("usePixelData")) return;

  bool applyAvalCoef = MTHREAD->MD->getBoolSetting("applyAvalCoef");
  vector <int> regIds2 = MTHREAD->MD->getRegionIds(2);

  for(uint i=0;i<regIds2.size();i++){
    ModelRegion* reg = MTHREAD->MD->getRegion(regIds2[i]);
    vector <Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regIds2[i]);
    for (uint p=0;p<rpx.size();p++){
      if(applyAvalCoef){
        rpx[p]->avalCoef = rpx[p]->getDoubleValue("avalCoef", true);
      }
    }
  }
}

/**
Called from setSpace(), initLayers() is responsable of:
 - load each layer propriety (name, label, datafile..)
 - add the layer to the system @see addLayer
<p>If the layer is to be read at start-up:
 - adding to the layer each legend item (ID, label, min-max values..) @see addLegendItem
 - eventually adding to the layer each reclassification rules @see addReclassificationRule
**/
void
Gis::initLayers(){
  // setting layers...
  //string filename_complete= MTHREAD->MD->getFilenameByType("gis");
  string filename_complete = MTHREAD->getBaseDirectory()+MTHREAD->MD->getStringSetting("gisFilename");

  InputNode gisDocument;
  bool test=gisDocument.setWorkingFile(filename_complete);
  if (!test){msgOut(MSG_CRITICAL_ERROR, "Error opening the gis file "+filename_complete+".");}
  vector<InputNode> layerNodes = gisDocument.getNodesByName("layer");
  for (uint i=0; i<layerNodes.size();i++){
    
    string name = layerNodes.at(i).getNodeByName("name").getStringContent();
    string label = layerNodes.at(i).getNodeByName("label").getStringContent();
    bool isInteger = layerNodes.at(i).getNodeByName("isInteger").getBoolContent();
    bool dynamicContent = layerNodes.at(i).getNodeByName("dynamicContent").getBoolContent();
    string readAtStart = layerNodes.at(i).getNodeByName("readAtStart").getStringContent();
    if (readAtStart != "true") continue;
    string dirName = layerNodes.at(i).getNodeByName("dirName").getStringContent();
    string fileName = layerNodes.at(i).getNodeByName("fileName").getStringContent();
    //cout <<"getBaseDirectory(): " << MTHREAD->MD->getBaseDirectory() << endl;
    
    string fullFileName = MTHREAD->MD->getBaseDirectory()+dirName+fileName;
    //cout <<"fullFileName: " << fullFileName << endl;
    addLayer(name,label,isInteger,dynamicContent,fullFileName);
    //legend..
    vector<InputNode> legendItemsNodes = layerNodes.at(i).getNodesByName("legendItem");
    for (uint j=0; j<legendItemsNodes.size();j++){
      int lID = legendItemsNodes.at(j).getIntContent();
      string llabel = legendItemsNodes.at(j).getStringAttributeByName("label");
      int rColor = legendItemsNodes.at(j).getIntAttributeByName("rColor");
      int gColor = legendItemsNodes.at(j).getIntAttributeByName("gColor");
      int bColor = legendItemsNodes.at(j).getIntAttributeByName("bColor");
      double minValue, maxValue;
      if (isInteger){
        minValue = ((double)lID);
        maxValue = ((double)lID);
      }
      else {
        minValue = legendItemsNodes.at(j).getDoubleAttributeByName("minValue");
        maxValue = legendItemsNodes.at(j).getDoubleAttributeByName("maxValue");
      }
      addLegendItem(name, lID, llabel, rColor, gColor, bColor, minValue, maxValue);
    }
    //reclassification rule..
    vector<InputNode> reclassificationRulesNodes = layerNodes.at(i).getNodesByName("reclassRule");
    for (uint z=0; z<reclassificationRulesNodes.size();z++){
      int inCode = reclassificationRulesNodes.at(z).getIntContent();
      int outCode = reclassificationRulesNodes.at(z).getIntAttributeByName("outCode");
      double p = reclassificationRulesNodes.at(z).getDoubleAttributeByName("p");
      addReclassificationRule(name, inCode, outCode, p);
    }
  }
  initLayersPixelData();
  //initLayersModelData(DATA_INIT); // only the layers relative to the initial years are inserted now. All the simulation year layers will be added each year before mainSimulationyear()
}

/** Init the layers of exogenous data at pixel level (e.g. time of passage, multipliers, volumes of sp. espl. ft, spread models)
    These layers will then be read from datafile
*/
void
Gis::initLayersPixelData(){
  if (!MTHREAD->MD->getBoolSetting("usePixelData")){return;}
  string dir = MTHREAD->MD->getBaseDirectory()+MTHREAD->MD->getStringSetting("spatialDataSubfolder");
  string fileExt = MTHREAD->MD->getStringSetting("spatialDataFileExtension");
  vector<string> files = vector<string>();
  string fullFilename, filename, fullPath;
  //string parName, forName, dClass, yearString;
  //int year;

  MTHREAD->MD->getFilenamesByDir (dir,files, fileExt); // Ugly format. Files is the output (reference)

  for (unsigned int i = 0;i < files.size();i++) {
    fullFilename = files[i];
    fullPath = dir+"/"+fullFilename;
    filename = fullFilename.substr(0,fullFilename.find_last_of("."));
    addLayer(filename,filename,false,false,fullPath,false);
  }

  // Loading volumes of forest types that are spatially known..
  if(MTHREAD->MD->getBoolSetting("useSpExplicitForestTypes")){
    string dir2 = MTHREAD->MD->getBaseDirectory()+MTHREAD->MD->getStringSetting("spExplicitForTypesInputDir");
    string fileExt2 = MTHREAD->MD->getStringSetting("spExplicitForTypesFileExtension");
    vector<string> files2 = vector<string>();
    string fullFilename2, filename2, fullPath2;
    MTHREAD->MD->getFilenamesByDir (dir2,files2, fileExt2); // Ugly format. Files is the output (reference)
    for (unsigned int i = 0;i < files2.size();i++) {
      fullFilename2 = files2[i];
      fullPath2 = dir2+"/"+fullFilename2;
      filename2 = fullFilename2.substr(0,fullFilename2.find_last_of("."));
      addLayer(filename2,filename2,false,false,fullPath2,false);
    }
  }

  // Loading pathogens exogenous spread models...
  if(MTHREAD->MD->getBoolSetting("usePathogenModule")){
    string dir2 = MTHREAD->MD->getBaseDirectory()+MTHREAD->MD->getStringSetting("pathogenExogenousSpreadModelFolder");
    string fileExt2 = MTHREAD->MD->getStringSetting("pathogenExogenousSpreadModelFileExtension");
    vector<string> files2 = vector<string>();
    string fullFilename2, filename2, fullPath2;
    MTHREAD->MD->getFilenamesByDir (dir2,files2, fileExt2); // Ugly format. Files is the output (reference)
    for (unsigned int i = 0;i < files2.size();i++) {
      fullFilename2 = files2[i];
      fullPath2 = dir2+"/"+fullFilename2;
      filename2 = fullFilename2.substr(0,fullFilename2.find_last_of("."));
      addLayer(filename2,filename2,false,false,fullPath2,false);
    }
  }

}

/** Init the layers of exogenous data at pixel level (e.g. time of passage)
    These layers will NOT be read by datafile, but volume for each pixel will be calculated from regional data and area map
*/
/*
void
Gis::initLayersModelData(const int& year_h){

  if (!MTHREAD->MD->getBoolSetting("usePixelData")) return;

  vector <int> years;
  if(year_h==DATA_NOW){
    years.push_back(MTHREAD->SCD->getYear());
  } else if (year_h==DATA_INIT){
    int initialYear = MTHREAD->MD->getIntSetting("initialYear");
    int initialOptYear = MTHREAD->MD->getIntSetting("initialOptYear");
    for(int y=initialYear;y<initialOptYear;y++){
      years.push_back(y);
    }
  } else {
    years.push_back(year_h);
  }

  vector <string> dClasses = MTHREAD->MD->getStringVectorSetting("dClasses");
  vector <string> fTypes = MTHREAD->MD->getForTypeIds();
  //int initialYear = MTHREAD->MD->getIntSetting("initialYear");
  //int initialOptYear = MTHREAD->MD->getIntSetting("initialOptYear");
  //int simYears = MTHREAD->MD->getIntSetting("simulationYears");
  string layerName_vol,layerName_cumTp,layerName_regArea,layerName_area;
  for(uint i=0;i< fTypes.size();i++){
    for(int y=0;y<years.size();y++){
      layerName_regArea = pack("regArea",fTypes[i],"",years[y]);
      addLayer(layerName_regArea,layerName_regArea,false,true,"",false);
      for (uint j=0;j<dClasses.size();j++){
        layerName_vol = pack("vol",fTypes[i],dClasses[j],years[y]);
        layerName_cumTp = pack("cumTp",fTypes[i],dClasses[j],years[y]);
        layerName_area = pack("area",fTypes[i],dClasses[j],years[y]);
        addLayer(layerName_vol,layerName_vol,false,true,"",false);
        addLayer(layerName_cumTp,layerName_cumTp,false,true,"",false);
        addLayer(layerName_area,layerName_area,false,true,"",false);
      }

    }

  }
  string debug = "done";

}
*/

Layers*
Gis::getLayer(const string& layerName_h){
  for(uint i=0;i<layerVector.size();i++){
    if(layerVector[i].getName() == layerName_h){
      return &layerVector[i];
    }
  }
  msgOut(MSG_CRITICAL_ERROR, "Layer "+layerName_h+" not found. Aborting.");
}

void
Gis:: applyForestReclassification(){
/*per ogni forest type:
 - crea i layers delle forest type nuovi
 - riempi con zero
 - passa le info dal layerr ereditato al nuovo
 per ogni pixel
 */

  // caching
  int nReclassRules = MTHREAD->MD->getNReclRules();
  vector <reclRule*> RRs;
  for(uint z=0;z<nReclassRules;z++){
    RRs.push_back(MTHREAD->MD->getReclRule(z));
  }



  for (uint i=0;i< MTHREAD->MD->getNForTypes();i++){
    forType* FT = MTHREAD->MD->getForType(i);
    if(!layerExist(FT->forLayer)){
      addLayer(FT->forLayer, "Are layer for forest type "+FT->forTypeId, false, true);
      resetLayer(FT->forLayer);
      Layers* newLayer = getLayer(FT->forLayer);
      Layers* ereditatedLayer = getLayer(MTHREAD->MD->getForType(FT->ereditatedFrom)->forLayer);
      newLayer->addLegendItems(ereditatedLayer->getLegendItems());
    }
  }


  for (uint i=0;i< MTHREAD->MD->getNForTypes();i++){
    forType* FT = MTHREAD->MD->getForType(i);
    for(uint j=0;j<xyNPixels;j++){
      Pixel* PX = getPixel(j);
      //int regL1 =  PX->getDoubleValue ("regLev_1");
      int regL2 =  PX->getDoubleValue ("regLev_2");
      double value =  PX->getDoubleValue (FT->forLayer, true);
      for(uint z=0;z<nReclassRules;z++){
        reclRule* RR = RRs[z];
        //if( (RR->regId == regL2 || RR->regId == regL1) && RR->forTypeOut == FT->forTypeId ){ // we found a reclassification rule for the region where is located this pixel and that output on the for type we are using
        if( RR->regId == regL2  && RR->forTypeOut == FT->forTypeId ){ // we found a reclassification rule for the region where is located this pixel and that output on the for type we are using
          string debugForTypeIn = RR->forTypeIn;
          double inputValue = PX->getDoubleValue(MTHREAD->MD->getForType(RR->forTypeIn)->forLayer, true);
          double reclassCoeff = RR->coeff;
          value += inputValue * reclassCoeff ;
          // not breaking because we may have more than one input for the same output
        }
      }
      PX->changeValue(FT->forLayer, value, true);
    }
    updateImage(FT->forLayer);
  }
  //countItems("forType_B_HF", true);
  refreshGUI();
  /*Pixel* DP = getPixel(8386);
  msgOut(MSG_DEBUG,"Debug info on plot 8386");
  for (uint i=0;i< MTHREAD->MD->getNForTypes();i++){
    forType* FT = MTHREAD->MD->getForType(i);
    msgOut(MSG_DEBUG,FT->forTypeId+" - "+d2s(DP->getDoubleValue (FT->forLayer)));
  }
  */
}


/**
Called at init time from initLayers, or during model run-time, this function will add a layer to the system.
@param name_h ID of the layer (no spaces!)
@param label_h layer label
@param type_h type of the layer, integer or contiguous
@param dynamicContent_h if it change during the time (so it needs to be printed each year) or not
@param fullFilename_h if the layer has to be read at the beginning, the name of the associated datafile (default="")
<p>It:
 - had the layer to the layerVector
 - set all pixels with nodata for that specific layer
 - let the GUI know we have a new layer
*/
void
Gis::addLayer(string name_h, string label_h, bool isInteger_h, bool dynamicContent_h, string fullFileName_h, bool display_h){
  if(name_h == "forArea_ash"){
    bool debug = true;
  }
  for(uint i=0; i<layerVector.size(); i++){
    if (layerVector.at(i).getName() == name_h){
      msgOut(MSG_ERROR, "Layer already exist with that name");
      return;
    }
  }
  Layers LAYER (MTHREAD, name_h, label_h, isInteger_h, dynamicContent_h, fullFileName_h, display_h);
  layerVector.push_back(LAYER);

  for (uint i=0;i<xyNPixels; i++){
    pxVector[i].setValue(name_h,noValue);
  }
  if(display_h){
    MTHREAD->addLayer(name_h,label_h);
  }
  
}

void
Gis::resetLayer(string layerName_h){

  for(uint i=0; i<layerVector.size(); i++){
    if (layerVector.at(i).getName() == layerName_h){
      for (uint i=0;i<xyNPixels; i++){
        pxVector.at(i).changeValue(layerName_h,noValue); // bug solved 20071022, Antonello
      }
      return;
    }
  }
  msgOut(MSG_ERROR, "I could not reset layer "+layerName_h+" as it doesn't exist!");  
}

bool
Gis::layerExist(const string& layerName_h, bool exactMatch) const{

  if(exactMatch){
    for(uint i=0; i<layerVector.size(); i++){
      if (layerVector.at(i).getName() == layerName_h){
        return true;
      }
    }
  } else { // partial matching (stored layer name begin with search parameter)
    for(uint i=0; i<layerVector.size(); i++){
      if (layerVector.at(i).getName().compare(0, layerName_h.size(),layerName_h )){
        return true;
      }
    }
  }

  return false;
}

/**
Search within the layerVector and call addLegendItem(...) to the appropriate one.
<p>Called at init time from initLayers, or during model run-time.
@param name_h Name of the layer
@param ID_h ID of the specific lagend item
@see Layers::addLegendItem
*/
void
Gis::addLegendItem(string name_h, int ID_h, string label_h, int rColor_h, int gColor_h, int bColor_h, double minValue_h, double maxValue_h){
  
  for(uint i=0; i<layerVector.size(); i++){
    if (layerVector.at(i).getName() == name_h){
      layerVector.at(i).addLegendItem(ID_h, label_h, rColor_h, gColor_h, bColor_h, minValue_h, maxValue_h);
      return;
    }
  }
  msgOut(MSG_ERROR, "Trying to add a legend item to a layer that doesn't exist.");
  return;
}

/**
Search within the layerVector and call countMyPixels(...) to the appropriate one.
<p>Called at init time from initLayers, or during model run-time.
@param layerName_h Name of the layer
@param debug Print the values on the GUI
@see Layers::countMyPixels
*/
void
Gis::countItems(const string &layerName_h, const bool &debug){

  for(uint i=0; i<layerVector.size(); i++){
    if (layerVector.at(i).getName() == layerName_h){
      layerVector.at(i).countMyPixels(debug);
      return;
    }
  }
  msgOut(MSG_ERROR, "Trying to get statistics (count pixels) of a layer that doesn't exist.");
  return;
}

/**
Search within the layerVector and call addReclassificationRule(...) to the appropriate one.

Called at init time from initLayers, or during model run-time.
@see Layers::addReclassificationRule
*/
void
Gis::addReclassificationRule(string name_h, int inCode_h, int outCode_h, double p_h){
  for(uint i=0; i<layerVector.size(); i++){
    if (layerVector.at(i).getName() == name_h){
      layerVector.at(i).addReclassificationRule(inCode_h, outCode_h, p_h);
      return;
    }
  }
  msgOut(MSG_ERROR, "Trying to add a reclassification rule to a layer that doesn't exist.");
  return; 
}

/**
Called at init time from initLayers, this function load the associated datafile to the existing layers (that if exists at this stage are all of type to be loaded at start-up). 
<br>This function loop over layerVector and works with GRASS/ASCII (tested) or ARC/ASCII (untested) datasets, assigning to each pixel the readed value to the corresponding layer. 
<br>The function also "compose" the initial map with the colors read by the layer (for each specific values) and send the map to the GUI.

NOTE: It uses some Qt functions!!!

@see Pixel::changeValue
@see Layers::filterExogenousDataset
@see Layers::getColor
*/
void
Gis::loadLayersDataFromFile(){
  double localNoValue = noValue;
  double inputValue;
  double outputValue;
  QColor color;

  for(uint i=0;i<layerVector.size();i++){
    string layerName =layerVector.at(i).getName();
    string fileName=layerVector.at(i).getFilename();
    if(fileName == "") continue; // BUGGED !!! 20121017, Antonello. It was "return", so it wasn't reading any layers following a layer with no filename
    QFile file(fileName.c_str());
    if (!file.open(QFile::ReadOnly)) {
      cerr << "Cannot open file for reading: "
        << qPrintable(file.errorString()) << endl;
      msgOut(MSG_ERROR, "Cannot open map file "+fileName+" for reading.");
      continue;
    }
    QTextStream in(&file);
    int countRow = 0;
    QImage image = QImage(xNPixels, yNPixels, QImage::Format_RGB32);
    image.fill(qRgb(255, 255, 255));
    while (!in.atEnd()) {
      QString line = in.readLine();
      QStringList fields = line.split(' ');
      if ( 
        (fields.at(0)== "north:" && fields.at(1).toDouble() != geoTopY)
        || ((fields.at(0)== "south:" || fields.at(0) == "yllcorner" ) && fields.at(1).toDouble() != geoBottomY)
        || (fields.at(0)== "east:" && fields.at(1).toDouble() != geoRightX)
        || ((fields.at(0)== "west:" || fields.at(0) == "xllcorner" ) && fields.at(1).toDouble() != geoLeftX)
        || ((fields.at(0)== "rows:" || fields.at(0) == "nrows" ) && fields.at(1).toInt() != yNPixels)
        || ((fields.at(0)== "cols:" || fields.at(0) == "ncols" ) && fields.at(1).toInt() != xNPixels)
      )
      {
        msgOut(MSG_ERROR, "Layer "+layerName+" has different coordinates. Aborting reading.");
        break;
      } else if (fields.at(0)== "null:" || fields.at(0) == "NODATA_value" || fields.at(0) == "nodata_value" ) {
        localNoValue = fields.at(1).toDouble();
      } else if (fields.size() > 5) {
        for (int countColumn=0;countColumn<xNPixels;countColumn++){
          inputValue = fields.at(countColumn).toDouble();
          if (inputValue == localNoValue){
            outputValue = noValue;
            pxVector.at((countRow*xNPixels+countColumn)).changeValue(layerName,outputValue);
            QColor nocolor(255,255,255);
            color = nocolor;
          }
          else {
            outputValue=layerVector.at(i).filterExogenousDataset(fields.at(countColumn).toDouble());
            pxVector.at((countRow*xNPixels+countColumn)).changeValue(layerName,outputValue);
            color = layerVector.at(i).getColor(outputValue);
          }
          image.setPixel(countColumn,countRow,color.rgb());
        }
        countRow++;
      }
    }
    if (MTHREAD->MD->getBoolSetting("initialRandomShuffle") ){
      layerVector.at(i).randomShuffle();
    }
    this->filterSubRegion(layerName);
    if(layerVector.at(i).getDisplay()){
      MTHREAD->updateImage(layerName,image);
      //send the image to the gui...
      refreshGUI();
    }


  }
}

/**
Update an ALREADY EXISTING image and send the updated image to the GUI.
<br>It is used instead of updating the individual pixels that is much more time consuming than change the individual pixels value and then upgrade the image as a whole. 
@param layername_h Layer from where get the image data
*/
void
Gis::updateImage(string layerName_h){
  msgOut (1, "Update image "+layerName_h+"...");

  // sub{X,Y}{R,L,T,B} refer to the subregion coordinates, but when this is not active they coincide with the whole region
  QImage image = QImage(subXR-subXL+1, subYB-subYT+1, QImage::Format_RGB32);

  image.fill(qRgb(255, 255, 255));
  int layerIndex=-1;
  for (uint i=0;i<layerVector.size();i++){
    if (layerVector.at(i).getName() == layerName_h){
      layerIndex=i;
      break;
    }
  }
  if (layerIndex <0) msgOut(MSG_CRITICAL_ERROR, "Layer not found in Gis::updateImage()");

  for (int countRow=subYT;countRow<subYB;countRow++){
    for (int countColumn=subXL;countColumn<subXR;countColumn++){      
      double value = pxVector.at((countRow*xNPixels+countColumn)).getDoubleValue(layerName_h);
      QColor color = layerVector.at(layerIndex).getColor(value);
      image.setPixel(countColumn-subXL,countRow-subYT,color.rgb());
    }
  }
  MTHREAD->updateImage(layerName_h,image);
  refreshGUI();
}

Pixel*
Gis::getRandomPlotByValue(string layer_h, int layerValue_h){
  
  vector <Pixel* > candidates;
  vector <uint> counts;
  for(uint i=0;i<pxVector.size();i++) counts.push_back(i);
  random_shuffle(counts.begin(), counts.end()); // randomize the elements of the array.

  for (uint i=0;i<counts.size();i++){
    if(pxVector.at(counts.at(i)).getDoubleValue(layer_h) == layerValue_h ) {
        return &pxVector.at(counts.at(i));
    }
  }

  msgOut(MSG_CRITICAL_ERROR,"We can't find any plot with "+d2s(layerValue_h)+" value on layer "+layer_h+".");
  Pixel* toReturn;
  toReturn =0;
  return toReturn;
}
/**

@param layer_h      Name of the layer
@param layerValue_h Value we want the plots for
@param onlyFreePlots Flag to get only plots marked as free (d=false)
@param outputLevel Level of output in case of failure (no plots available). Default is warning, but if set as MSG_CRITICAL_ERROR it make stop the model.


*/
vector <Pixel*>
Gis::getAllPlotsByValue(string layer_h, int layerValue_h, int outputLevel){
  // this would be easier to mantain and cleaned code, but slighly slower:
  //vector<int> layerValues;
  //layerValues.push_back(layerValue_h);
  //return getAllPlotsByValue(layer_h, layerValues, onlyFreePlots, outputLevel);

  vector <Pixel* > candidates;
  for (uint i=0;i<pxVector.size();i++){
    if(pxVector.at(i).getDoubleValue(layer_h) == layerValue_h){
      candidates.push_back(&pxVector.at(i));
    }
  }

  if (candidates.size()>0){
    random_shuffle(candidates.begin(), candidates.end()); // randomize ther elements of the array... cool !!! ;-)))
  }
  else {
    msgOut(outputLevel,"We can't find any free plot with "+d2s(layerValue_h)+" value on layer "+layer_h+".");
  }
  return candidates;
}

/**

@param layer_h      Name of the layer
@param layerValues_h Values we want the plots for
@param onlyFreePlots Flag to get only plots marked as free (d=false)
@param outputLevel Level of output in case of failure (no plots available). Default is warning, but if set as MSG_CRITICAL_ERROR it make stop the model.


*/
vector <Pixel*>
Gis::getAllPlotsByValue(string layer_h, vector<int> layerValues_h, int outputLevel){
  vector <Pixel* > candidates;
  string valuesToMatch;
  unsigned int z;

  //string of the required land values to match;
  for (uint j=0;j<layerValues_h.size();j++){
    valuesToMatch = valuesToMatch + " " + i2s(layerValues_h.at(j));
  }

  for (uint i=0;i<pxVector.size();i++){
    z = valuesToMatch.find(d2s(pxVector.at(i).getDoubleValue(layer_h))); // search if in the string of required values is included also the value of the current plot
    if(z!=string::npos){ //z is not at the end of the string, means found!
      candidates.push_back(&pxVector.at(i));
    }
  }

  if (candidates.size()>0){
    random_shuffle(candidates.begin(), candidates.end()); // randomize ther elements of the array... cool !!! ;-)))
  }
  else {
    msgOut(outputLevel,"We can't find any free plot with the specified values ("+valuesToMatch+") on layer "+layer_h+".");
  }
  return candidates;
}

/**

@param onlyFreePlots Flag to get only plots marked as free (d=false)
@param outputLevel Level of output in case of failure (no plots available). Default is warning, but if set as MSG_CRITICAL_ERROR it make stop the model.

*/
vector <Pixel*>
Gis::getAllPlots(int outputLevel){
  vector <Pixel* > candidates;
  for (uint i=0;i<pxVector.size();i++){
    candidates.push_back(&pxVector.at(i));
  }
  if (candidates.size()>0){
    random_shuffle(candidates.begin(), candidates.end()); // randomize ther elements of the array... cool !!! ;-)))
  }
  else {
    msgOut(outputLevel,"We can't find any free plot.");
  }
  return candidates;
}

/// Return the vector of all plots by a specific region (main region or subregion), optionally shuffled;
vector <Pixel*>
Gis::getAllPlotsByRegion(ModelRegion &region_h, bool shuffle){
  vector <Pixel*> regionalPixels = region_h.getMyPixels();
  if(shuffle){
    random_shuffle(regionalPixels.begin(), regionalPixels.end()); // randomize the elements of the array.
  }
  return regionalPixels;
}

vector <Pixel*>
Gis::getAllPlotsByRegion(int regId_h, bool shuffle){
    ModelRegion* reg = MTHREAD->MD->getRegion(regId_h);
    return getAllPlotsByRegion(*reg,shuffle);
}



vector <string>
Gis::getLayerNames(){
  vector <string> toReturn;
  for (uint i=0;i<layerVector.size();i++){
    toReturn.push_back(layerVector[i].getName());
  }
  return toReturn;
}

vector <Layers*>
Gis::getLayerPointers(){
  vector <Layers*> toReturn;
  for (uint i=0;i<layerVector.size();i++){
    toReturn.push_back(&layerVector[i]);
  }
  return toReturn;
}

void
Gis::printDebugValues (string layerName_h, int min_h, int max_h){
  int min=min_h;
  int max;
  int ID, X, Y;
  string out;
  double value;
  //double noValue = MTHREAD->MD->getDoubleSetting("noValue");
  if (max_h==0){
    max= pxVector.size();
  }
  else {
    max = max_h;
  }
  msgOut(MSG_DEBUG,"Printing debug information for layer "+layerName_h+".");
  for (int i=min;i<max;i++){
    value = pxVector.at(i).getDoubleValue(layerName_h);
    if (value != noValue){ 
      ID    = i;
      X     = pxVector.at(i).getX();
      Y     = pxVector.at(i).getY();
      out = "Px. "+i2s(ID)+" ("+i2s(X)+","+i2s(Y)+"): "+d2s(value);
      msgOut(MSG_DEBUG,out);
    }
  }
}

/**
This function filter the region, placing noValue on the selected informative layer on pixels that are outside the subregion.
<br>It was thinked for speedup the development without have to run the whole model for testing each new implementation, but it can used to see what happen in the model when only a subset of the region is analysed.

When the subRegionMode is actived also agents mouldCoefficients are rescaled to match the new subregion, however this is just approximated, as the upscaling process was performed on the whole region.
*/
void
Gis::filterSubRegion(string layerName_h){
  if (MTHREAD->MD->getBoolSetting("subRegionMode")){
    subRegionMode = true;
    subXL  = MTHREAD->MD->getIntSetting("subRegionXLeft");
    subXR   = MTHREAD->MD->getIntSetting("subRegionXRight");
    subYT = MTHREAD->MD->getIntSetting("subRegionYTop");
    subYB = MTHREAD->MD->getIntSetting("subRegionYBottom");
    for (int y=0;y<yNPixels;y++){
      for (int x=0;x<xNPixels;x++){
        if (x>=subXL && x<= subXR && y>=subYT && y<=subYB){
        }
        else {
          this->getPixel(x,y)->changeValue(layerName_h, noValue);
        }
      }
    }
    updateImage(layerName_h);
  }
  else {
    subXL = 0;
    subYT = 0;
    subXR = xNPixels-1;
    subYB = yNPixels-1;  
  }
}

double
Gis::getDistance(const Pixel* px1, const Pixel* px2){
  return sqrt (
        pow ( (((double)px1->getX()) - ((double)px2->getX()))*xMetersByPixel,2)
        +
        pow ( (((double)px1->getY()) - ((double)px2->getY()))*yMetersByPixel,2)
      );
}



void
Gis::printLayers(string layerName_h){
  msgOut(MSG_DEBUG,"Printing the layers");
  int iteration = MTHREAD->SCD->getIteration(); // are we on the first year of the simulation ??
  if(layerName_h == ""){
    for (uint i=0;i<layerVector.size();i++){
      // not printing if we are in a not-0 iteration and the content of the map doesn't change
      if (!iteration || layerVector[i].getDynamicContent()) layerVector[i].print();
    }
  } else {
    for (uint i=0;i<layerVector.size();i++){
      if(layerVector[i].getName() == layerName_h){
        if (!iteration || layerVector[i].getDynamicContent()) layerVector[i].print();
        return;
      }
    }
    msgOut(MSG_ERROR, "Layer "+layerName_h+" unknow. No layer printed.");
  }
}

void
Gis::printBinMaps(string layerName_h){
  msgOut(MSG_DEBUG,"Printing the maps as images");
  int iteration = MTHREAD->SCD->getIteration(); // are we on the first year of the simulation ??
  if(layerName_h == ""){
    for (uint i=0;i<layerVector.size();i++){
      if (!iteration || layerVector[i].getDynamicContent()) {layerVector[i].printBinMap();}
    }
  } else {
    for (uint i=0;i<layerVector.size();i++){
      if(layerVector[i].getName() == layerName_h){
        if (!iteration || layerVector[i].getDynamicContent()) {layerVector[i].printBinMap();}
        return;
      }
    }
    msgOut(MSG_ERROR, "Layer "+layerName_h+" unknow. No layer printed.");
  }
}

int
Gis::sub2realID(int id_h){
  // IMPORTANT: this function is called at refreshGUI() times, so if there are output messages, call them with the option to NOT refresh the gui, otherwise we go to an infinite loop...
  if (!MTHREAD->MD->getBoolSetting("subRegionMode")) {return id_h;}
  int subX, subY, realX, realY;
  subX = id_h%((subXR-subXL)+1);
  subY = id_h/((subXR-subXL)+1);

  realX = subX+subXL;
  realY = subY+subYT;

  if ( (realX<subXL) || (realX>subXR) || (realY < subYT) || (realY>subYB)  ){
    msgOut(MSG_ERROR, "Calling sub2realID() function with an ID that is out of subregion range!!", false);
    return -1;
  }
  return realX+realY*xNPixels;

}

void
Gis::unpack(const string& key, string& parName, string& forName, string& dClass, int& year) const{
  int parNameDelimiter = key.find("#",0);
  int forNameDelimiter = key.find("#",parNameDelimiter+1);
  int dClassDelimiter = key.find("#",forNameDelimiter+1);
  int yearDelimiter = key.find("#",dClassDelimiter+1);
  if (yearDelimiter == string::npos){
    msgOut(MSG_CRITICAL_ERROR, "Error in unpacking the key for the layer.");
  }
  parName.assign(key,0,parNameDelimiter);
  forName.assign(key,parNameDelimiter+1,forNameDelimiter-parNameDelimiter-1);
  dClass.assign(key,forNameDelimiter+1,dClassDelimiter-forNameDelimiter-1);
  string yearString="";
  yearString.assign(key,dClassDelimiter+1,yearDelimiter-dClassDelimiter-1);
  year = s2i(yearString);
}

void
Gis::swap(const int& swap_what){

  for(uint i=0;i<pxVector.size();i++) {
    pxVector[i].swap(swap_what);
  }

}
