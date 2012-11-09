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

// Pythia headers...
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

	void                  initMarketModule();
	void                  runMarketModule();
	void                  runBiologicalModule(const bool& init);
	void                  runManagementModule();
	void                  sumRegionalForData();

	void                  cacheSettings();          ///< just cache exogenous settings
	void                  initializePixelValues();  ///< distribuite regional exogenous volumes to pixel volumes using corine land cover area as weight
	void                  resetPixelValues();       ///< swap volumes->lagged_volumes and reset the other pixel vectors
	void                  cachePixelExogenousData();
	void                  computeInventary();       ///< in=f(vol_t-1)
	void                  computeCumulativeData();










	void                  updateMapAreas();

	// convenient handles to equivalent ModelData functions..
	double              gpd(const string &type_h, const int& regId_h, const string &prodId_h, const int& year=DATA_NOW, const string &freeDim_h="") const {return MTHREAD->MD->getProdData(type_h, regId_h, prodId_h, year, freeDim_h);};
	double              gfd(const string &type_h, const int& regId_h, const string &forType_h, const string &diamClass_h, const int& year=DATA_NOW) const {return MTHREAD->MD->getForData(type_h, regId_h, forType_h, diamClass_h, year);};
	void                spd(const double& value_h, const string &type_h, const int& regId_h, const string &prodId_h, const int& year=DATA_NOW, const bool& allowCreate=false, const string &freeDim_h="") const {MTHREAD->MD->setProdData(value_h, type_h, regId_h, prodId_h, year, allowCreate, freeDim_h);};
	void                sfd(const double& value_h, const string &type_h, const int& regId_h, const string &forType_h, const string &diamClass_h, const int& year=DATA_NOW, const bool& allowCreate=false) const {MTHREAD->MD->setForData(value_h, type_h, regId_h, forType_h, diamClass_h, year, allowCreate);};
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
	double expType;
	double mr;
	vector <Pixel*> regPx; // pixels behaving to the current region
};

#endif // MODELCORESPATIAL_H
