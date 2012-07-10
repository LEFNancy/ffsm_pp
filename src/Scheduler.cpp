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
#include "ThreadManager.h"
#include "Output.h"
#include "ModelData.h"
#include "Gis.h"
#include "ModelCore.h"

Scheduler::Scheduler(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
	iteration=-1;
};

Scheduler::~Scheduler(){
};

void
Scheduler::run(){

	for (int it=0;it<MTHREAD->MD->getIntSetting("simulationYears");it++){
		iteration = it;
		MTHREAD->upgradeMainSBLabel("New year started..");
		msgOut(MSG_INFO, "### "+i2s(getYear())+ " year started.. ####");
		MTHREAD->upgradeYearSBLabel(iteration+ MTHREAD->MD->getIntSetting("initialYear"));
		MTHREAD->treeViewerChangeGeneralPropertyValue("year",i2s(iteration+ MTHREAD->MD->getIntSetting("initialYear")));

		MTHREAD->CORE->runSimulationYear();

		MTHREAD->DO->collectData();
		MTHREAD->DO->print();

		for(int i=0;i<MTHREAD->GIS->getXNPixels();i++){
			MTHREAD->GIS->getPixel(i)->newYear(); //delete objects for the pixels, in the update the agents will do the same for their objects
		}


	}
}

int
Scheduler::getYear(){
	return (iteration+MTHREAD->MD->getIntSetting("initialYear"));
}
