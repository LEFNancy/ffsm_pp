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
#ifndef MODELCORE_H
#define MODELCORE_H

// Core C++ headers
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <sstream>

// External libraries headers
#include "IpIpoptApplication.hpp"

// Qt headers...

// RegMAS headers...
#include "BaseClass.h"
#include "ThreadManager.h"
#include "ModelData.h"

class ModelCore : public BaseClass{

public:
												ModelCore(ThreadManager* MTHREAD_h);
											 ~ModelCore();

	void                  runInitPeriod();
	void                  runSimulationYear();

	void                  initMarketModule();       ///< computes st and pw for second year and several needed-only-at-t0-vars for the market module
	void                  runMarketModule();        ///< computes st (supply total) and pw (weighted price). Optimisation inside.
	void                  runBiologicalModule();    ///< computes hV, hArea and new vol at end of year
	void                  runManagementModule();    ///< computes regArea and expectedReturns

	void                  cacheSettings();          ///< just cache exogenous settings from ModelData
	void                  cachePixelExogenousData();///< computes pixel level tp, meta and mort
	void                  computeInventary();       ///< in=f(vol_t-1)
	void                  computeCumulativeData();  ///< computes cumTp, vHa, cumTp_exp, vHa_exp,
	void                  updateMapAreas();         ///< computes forArea_{ft}


private:
	// convenient handles to equivalent ModelData functions..
	double              gpd(const string &type_h, const int& regId_h, const string &prodId_h, const int& year=DATA_NOW, const string &freeDim_h="") const {return MTHREAD->MD->getProdData(type_h, regId_h, prodId_h, year, freeDim_h);};
    double              gfd(const string &type_h, const int& regId_h, const string &forType_h, const string &freeDim_h, const int& year=DATA_NOW) const {return MTHREAD->MD->getForData(type_h, regId_h, forType_h, freeDim_h, year);};
	void                spd(const double& value_h, const string &type_h, const int& regId_h, const string &prodId_h, const int& year=DATA_NOW, const bool& allowCreate=false, const string &freeDim_h="") const {MTHREAD->MD->setProdData(value_h, type_h, regId_h, prodId_h, year, allowCreate, freeDim_h);};
    void                sfd(const double& value_h, const string &type_h, const int& regId_h, const string &forType_h, const string &freeDim_h, const int& year=DATA_NOW, const bool& allowCreate=false) const {MTHREAD->MD->setForData(value_h, type_h, regId_h, forType_h, freeDim_h, year, allowCreate);};
	bool                app(const string &prod_h, const string &forType_h, const string &dClass_h) const {return MTHREAD->MD->assessProdPossibility(prod_h, forType_h, dClass_h);};

	//vector <vector <vector <double> cumTp; /// cumulative time to reach a certain diameter class;
	//vector <vector <vector <double> vHa;   /// volumes at hectar [m^3/ha];

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
	vector < vector < vector < vector <double> > > > hV_byPrd; // by regId, ft, dc, pp
	//Ipopt::SmartPtr<Ipopt::IpoptApplication> application;
    bool rescaleFrequencies;


};

#endif // MODELCORE_H
