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
#include "Scheduler.h"
#include "ModelObject.h" // if I include regData I have problems. I have to include ModelObject that at its top include RegData.
#include "SuperAgentManager.h"
#include "ThreadManager.h"
#include "Output.h"

Scheduler::Scheduler(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
	iteration=0;
};

Scheduler::~Scheduler(){
};

void
Scheduler::run(){

	for (int it=0;it<MTHREAD->RD->getIntSetting("simulationYears");it++){
		iteration = it;
		MTHREAD->upgradeMainSBLabel("New year started..");
		msgOut(MSG_INFO, "### "+i2s(getYear())+ " year started.. ####");
		MTHREAD->upgradeYearSBLabel(iteration+ MTHREAD->RD->getIntSetting("initialYear"));
		MTHREAD->treeViewerChangeGeneralPropertyValue("year",i2s(iteration+ MTHREAD->RD->getIntSetting("initialYear")));

		MTHREAD->SAM->prepare();

		vector<double> landStats = MTHREAD->GIS->getAgrLandStats();
		MTHREAD->treeViewerChangeGeneralPropertyValue("total agr land", d2s(landStats.at(0)));
		MTHREAD->treeViewerChangeGeneralPropertyValue("owned agr land", d2s(landStats.at(1)));
		MTHREAD->treeViewerChangeGeneralPropertyValue("rented agr land", d2s(landStats.at(2)));

		MTHREAD->GIS->calculateDistancesToClosestFarm();

		MTHREAD->SAM->act();

		MTHREAD->DO->collectData();
		MTHREAD->DO->print();

		for(int i=0;i<MTHREAD->GIS->getXNPixels();i++){
			MTHREAD->GIS->getPixel(i)->newYear(); //delete objects for the pixels, in the update the agents will do the same for their objects
		}
		MTHREAD->SAM->update();

		MTHREAD->SAM->planNext();
		MTHREAD->GIS->updateImage("farmsteads");      // update for agents that may have left
		MTHREAD->GIS->updateImage("farmOwnedPlots");  // idem
		MTHREAD->GIS->updateImage("farmRentedPlots"); // idem
	}
}

int
Scheduler::getYear(){
	return (iteration+MTHREAD->RD->getIntSetting("initialYear"));
}
