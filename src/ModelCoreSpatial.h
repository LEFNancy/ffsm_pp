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
  void                  initializePixelValues();  ///< distribuite regional exogenous volumes to pixel volumes using corine land cover area as weight
  void                  initializePixelArea();    ///< compute px->area for each ft and dc
  void                  resetPixelValues();       ///< swap volumes->lagged_volumes and reset the other pixel vectors
  void                  cachePixelExogenousData();///< computes pixel level tp, meta and mort
  void                  computeInventary();       ///< in=f(vol_t-1)
  void                  computeCumulativeData();  ///< computes cumTp_exp, vHa_exp, vHa
  void                  updateMapAreas();         ///< computes forArea_{ft}
  double                computeExpectedPrice(const double & curPrice, const double & worldCurPrice, const double & worldFutPrice, const double & expCoef); ///< Compute local (or weighted local-world) expected price for a given product.

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
};

#endif // MODELCORESPATIAL_H
