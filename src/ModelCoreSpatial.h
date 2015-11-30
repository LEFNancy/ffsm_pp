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
#ifndef MODELCORESPATIAL_H
#define MODELCORESPATIAL_H

// Core C++ headers
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <sstream>

// Qt headers...

// FFSM headers...
#include "BaseClass.h"
#include "ThreadManager.h"
#include "ModelData.h"
#include "Pixel.h"

/**
* \brief The core of the model (spatial version).
*
* Once the environment is initialised (mainly data load, space created), the model is run through the two functions runInitPeriod()
* and runSimulationYear().
*
*
*  Some importan notes:
*    V (volumes)    -> at the end of the year
*    In (inventary) -> at the beginning of the year
*    Area           -> at the end of the year
*    Harvesting     -> at the beginning of the year
*    Volumes are in Mm^3, Areas in the model in Ha (10000 m^2), in the layers in m^2, vHa in m^3/ha
*
*    BALANCE:
*    PROD_forLocal (sl) + PROD_forExp (sa) + IMP (da) = CONS_fromLocal (dl) + CONS_fromImp (da) + EXP (sa)
*
* Where are volumes information ?
* - ip px->vol                                                         - by px, ft and dc
* - in forDataMap (through gft())                                      - by reg, ft and dc
* Where is area information ?
* - in px->area                                                        - by px, ft and dc
* - in forDataMap (through gft())                                      - by reg, ft and dc
* - in  px->values map (forArea_* layer, through px->getDoubleValue()) - by px and ft
*/
class ModelCoreSpatial : public BaseClass {

public:
                        ModelCoreSpatial(ThreadManager* MTHREAD_h);
                        ~ModelCoreSpatial();

  void                  runInitPeriod();
  void                  runSimulationYear();

  void                  initMarketModule();       ///< computes st and pw for second year and several needed-only-at-t0-vars for the market module
  void                  runMarketModule();        ///< computes st (supply total) and pw (weighted price). Optimisation inside.
  void                  runBiologicalModule();    ///< computes hV, hArea and new vol at end of year
  void                  runManagementModule();    ///< computes regArea and expectedReturns
  void                  sumRegionalForData();     ///< computes vol, hV, harvestedArea, regArea and expReturns at reg level from the pixel level
  void                  initialiseCarbonModule(); ///< call initialiseDeathBiomassStocks(), initialiseProductsStocks() and initialiseEmissionCounters()

  void                  registerCarbonEvents();   ///< call registerHarvesting(), registerDeathBiomass(), registerProducts() and registerTransports()
  void                  cacheSettings();          ///< just cache exogenous settings from ModelData
  void                  initializePixelVolumes();  ///< distribuite regional exogenous volumes to pixel volumes using corine land cover area as weight
  void                  assignSpMultiplierPropToVols(); // assign the spatial multiplier (used in the time of return) based no more on a Normal distribution but on the volumes present in the pixel: more volume, more the pixel is fit for the ft
  void                  initializePixelArea();    ///< compute px->area for each ft and dc
  void                  resetPixelValues();       ///< swap volumes->lagged_volumes and reset the other pixel vectors
  void                  cachePixelExogenousData();///< computes pixel level tp, meta and mort
  void                  computeInventary();       ///< in=f(vol_t-1)
  void                  computeCumulativeData();  ///< computes cumTp_exp, vHa_exp, vHa
  void                  updateMapAreas();         ///< computes forArea_{ft}
  double                computeExpectedPrice(const double & curLocPrice, const double & worldCurPrice, const double & worldFutPrice, const double & sl, const double & sa, const double & expCoef); ///< Compute weighted expected price for a given product.


  void                  loadExogenousForestLayers(const string & what); ///< Set pixel volumes (what="vol") OR areas (what="area") by specific forest types as defined in gis layers for volumes and proportionally to volumes for areas.

  // convenient handles to equivalent ModelData functions..
  double              gpd(const string &type_h, const int& regId_h, const string &prodId_h, const int& year=DATA_NOW, const string &freeDim_h="") const {return MTHREAD->MD->getProdData(type_h, regId_h, prodId_h, year, freeDim_h);};
  double              gfd(const string &type_h, const int& regId_h, const string &forType_h, const string &freeDim_h, const int& year=DATA_NOW) const {return MTHREAD->MD->getForData(type_h, regId_h, forType_h, freeDim_h, year);};
  void                spd(const double& value_h, const string &type_h, const int& regId_h, const string &prodId_h, const int& year=DATA_NOW, const bool& allowCreate=false, const string &freeDim_h="") const {MTHREAD->MD->setProdData(value_h, type_h, regId_h, prodId_h, year, allowCreate, freeDim_h);};
  void                sfd(const double& value_h, const string &type_h, const int& regId_h, const string &forType_h, const string &freeDim_h, const int& year=DATA_NOW, const bool& allowCreate=false) const {MTHREAD->MD->setForData(value_h, type_h, regId_h, forType_h, freeDim_h, year, allowCreate);};
  bool                app(const string &prod_h, const string &forType_h, const string &dClass_h) const {return MTHREAD->MD->assessProdPossibility(prod_h, forType_h, dClass_h);};

private:
  ModelData* MD;
  int firstYear;
  int secondYear;
  int thirdYear;
  int WL2;
  vector <int> regIds2;
  vector <string> priProducts;
  vector <string> secProducts;
  vector <string> allProducts;
  vector <string> dClasses;
  vector <string> pDClasses;
  vector <string> fTypes;
  vector <vector <int> > l2r;
  string regType;
  string natRegAllocation;
  double mr;
  vector <Pixel*> regPx; // pixels behaving to the current region
  bool rescaleFrequencies;
  bool oldVol2AreaMethod;
  string forestAreaChangeMethod;
  double ir; // interest rate
};

#endif // MODELCORESPATIAL_H
