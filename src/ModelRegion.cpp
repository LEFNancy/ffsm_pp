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

  // Create an empty vector of inventory bounds for each possible primary products combination
  int nInBounds = pow(2,MTHREAD->MD->getStringVectorSetting("priProducts").size());
  //int nInBounds = MTHREAD->MD->getStringVectorSetting("priProducts").size(); // TODO todo !Important
  vector <double> inBounds(nInBounds,0.); // should have ceated a vector of size priProducts.size(), all filled with zeros
  inResByAnyCombination             = inBounds;
  //inResByAnyCombination_deathTimber = inBounds;
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
  /*if(excludeResidual){
    int toReturn = 0; // Bug found 20180927
    for(uint i=0;i<chRegions.size();i++){
      if(!chRegions[i]->getIsResidual()){
        toReturn++;
      }
    }
    return toReturn;
  }
  return chRegions.size();
  */
  return getChildren(excludeResidual).size();
}

vector<ModelRegion*>
ModelRegion::getSiblings(bool excludeResidual){
  vector<ModelRegion*> toReturn;
  vector<ModelRegion*> allRegions = MTHREAD->MD->getAllRegions(excludeResidual);
  for (uint i=0;i<allRegions.size();i++){
    if (allRegions[i]->parRegId == parRegId && allRegions[i]->getRegId() != regId){
      toReturn.push_back(allRegions[i]);
    }
  }
  return toReturn;
}








double
ModelRegion::getVolumes(){
  /// \todo Implement me (but really needed?)
  return 0;
}

vector<double>
ModelRegion::getVolumes(int fType_h){
  /// \todo Implement me (but really needed?)
  vector<double> toReturn;
  return toReturn;
}

vector < vector <double> >
ModelRegion::getVolumes(int fType_h, string dClass_h){
  /// \todo Implement me (but really needed?)
  vector < vector <double> > toReturn;
  return toReturn;
}


double
ModelRegion::getArea(const string &fType_h, const string &dClass_h){
  vector <string> dClasses = MTHREAD->MD->getStringVectorSetting("dClasses");
  vector <string> fTypes= MTHREAD->MD->getForTypeIds();
  int ft_pos = -1000;
  int dc_pos = -1000;
  for(uint j=0;j<fTypes.size();j++){
    if (fTypes[j] == fType_h){
      ft_pos = j;
      break;
    }
  }
  for(uint u=0;u<dClasses.size();u++){
    if (dClasses[u] == dClass_h){
      dc_pos = u;
      break;
    }
  }
  if(ft_pos<0) msgOut(MSG_CRITICAL_ERROR,"Forest type "+fType_h+" not found in getArea() function.");
  if(dc_pos<0) msgOut(MSG_CRITICAL_ERROR,"Diameter class"+dClass_h+" not found in getArea() function.");

  return getArea(ft_pos, dc_pos);
}

double
ModelRegion::getArea(const string &fType_h){
  vector <string> fTypes= MTHREAD->MD->getForTypeIds();
  int ft_pos = -1000;
  for(uint j=0;j<fTypes.size();j++){
    if (fTypes[j] == fType_h){
      ft_pos = j;
      break;
    }
  }
  if(ft_pos<0) msgOut(MSG_CRITICAL_ERROR,"Forest type "+fType_h+" not found in getArea() function.");
  return getArea(ft_pos);
}

double
ModelRegion::getArea(const int& ft_pos, const int& dc_pos){
  double totalarea = 0.0;
  for(uint i=0;i<myPixels.size(); i++){
     totalarea += myPixels[i]->area.at(ft_pos).at(dc_pos);
  }
  return totalarea;
}

double
ModelRegion::getArea(const int& ft_pos){
  double totalarea = 0.0;
  for(uint i=0;i<myPixels.size(); i++){
     totalarea += vSum(myPixels[i]->area.at(ft_pos));
  }
  return totalarea;
}

double
ModelRegion::getArea(){
  vector<Pixel*> regPx = this->getMyPixels();
  double totalarea = 0.0;
  for(uint i=0;i<myPixels.size(); i++){
     totalarea += vSum(myPixels[i]->area);
  }
  return totalarea;
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

/***
 * It establishs a link between pixels and regions.
 *
 * Pixels are stored in myPixels vectors and, only if this is a level2 region, a pointer to this region is passed to the pixel
 *
 * */
void
ModelRegion::setMyPixels(){
  int xyNPixels = MTHREAD->GIS->getXyNPixels();
  for(uint i=0;i<xyNPixels;i++){
    Pixel* px = MTHREAD->GIS->getPixel(i);
    if(px->getDoubleValue("regLev_1")==regId || px->getDoubleValue("regLev_2")==regId){
      myPixels.push_back(px);
      if(regLevel == 2){
        px->setMyRegion(this);
      }
    }
  }
}

void
ModelRegion::swap(const int& swap_what){

  for(uint i=0;i<myPixels.size();i++) {
    myPixels[i]->swap(swap_what);
  }

}



