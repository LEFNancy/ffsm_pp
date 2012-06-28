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
#ifndef MODELCORE_H
#define MODELCORE_H

// Core C++ headers
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <sstream>

// Qt headers...

// RegMAS headers...
#include "BaseClass.h"
#include "ThreadManager.h"
#include "ModelData.h"

class ModelCore : public BaseClass{

public:
						ModelCore(ThreadManager* MTHREAD_h);
					   ~ModelCore();
	void                runInitPeriod();
	void                runSimulationYear(int year_h);

private:
	// convenient handles to equivalent ModelData functions..
	double              gpd(const string &type_h, int regId_h, const string &prodId_h, int year=DATA_NOW, const string &freeDim_h=""){return MTHREAD->MD->getProdData(type_h, regId_h, prodId_h, year, freeDim_h);};
	double              gfd(const string &type_h, int regId_h, const string &forType_h, const string &diamClass_h, int year=DATA_NOW){return MTHREAD->MD->getForData(type_h, regId_h, forType_h, diamClass_h, year);};
	void                spd(double value_h, const string &type_h, int regId_h, const string &prodId_h, int year=DATA_NOW, bool allowCreate=false, const string &freeDim_h=""){MTHREAD->MD->setProdData(value_h, type_h, regId_h, prodId_h, year, allowCreate, freeDim_h);};
	void                sfd(double value_h, const string &type_h, int regId_h, const string &forType_h, const string &diamClass_h, int year=DATA_NOW, bool allowCreate=false){MTHREAD->MD->setForData(value_h, type_h, regId_h, forType_h, diamClass_h, year, allowCreate);};
	bool                app(const string &prod_h, const string &forType_h, const string &dClass_h){return MTHREAD->MD->assessProdPossibility(prod_h, forType_h, dClass_h);};



};

#endif // MODELCORE_H
