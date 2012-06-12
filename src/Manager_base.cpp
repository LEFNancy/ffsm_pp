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
#include <math.h>
#include <algorithm>
#include "Manager_base.h"
#include "ThreadManager.h"
#include "SuperAgentManager.h"

//constructor
Manager_base::Manager_base(ThreadManager* MTHREAD_h, string name_h){
	MTHREAD=MTHREAD_h;
	mouldLocalIDCounter=0;
	agentLocalIDCounter=0;
	name = name_h;
	managerID = MTHREAD->SAM->getNewUniqueManagerID();
	MTHREAD_h->treeViewerAddManager(name_h);
	MTHREAD->treeViewerAddManagerProperty(name_h, "managed agents");
}

//destructor
Manager_base::~Manager_base(){
	MTHREAD->treeViewerRemoveManager(getName());

	for(uint i=0;i<agentsMoulds.size();i++){
		delete agentsMoulds[i];
	}
	for(uint i=0;i<managedAgents.size();i++){
		delete managedAgents[i];
	}
	for(uint i=0;i<removedAgents.size();i++){
		delete removedAgents[i];
	}

}

/////////////////////////////////////////////////////////////////////////////
//functions:

Agent_base*
Manager_base::getAgentByLocalID(double localID_h){
	for (uint i=0;i< managedAgents.size();i++){
		if (managedAgents.at(i)->getAgentLocalID() == localID_h){
			return managedAgents.at(i);
		}
	}

	Agent_base * noAgent;
	noAgent = 0;
	msgOut(MSG_CRITICAL_ERROR, "Search for an unknow agent localID!");
	return noAgent;
}

/**
This function assign objects without spacial attributes to agents according to their resources.
*/
void
Manager_base::assignObjectsToAgents(){
	msgOut(MSG_DEBUG,"Manager_base::assignObjectsToAgents()");
	vector <ModelObject> assignedInitialObjects;
	for (uint i=0;i<managedAgents.size();i++){
		map<string,double> agentInitialResources = managedAgents.at(i)->getInitialResources();
		map<string,double>::iterator p;
		for(p=agentInitialResources.begin();p!=agentInitialResources.end();p++){ // loop over a map
			assignedInitialObjects = MTHREAD->RD->getBestMatchingInitialObjects (p->first, p->second);
			for (uint j=0;j<assignedInitialObjects.size();j++){
				managedAgents.at(i)->acquireObject(assignedInitialObjects.at(j),STAGE_INIT);
			}
		}
	}	
};

void
Manager_base::rescaleMoulds(double factor_h){
	for (uint i=0; i<agentsMoulds.size(); i++){
		double oldCoeff = agentsMoulds.at(i)->getMouldCoeff();
		int newCoeff = ((int) round(oldCoeff*factor_h) );
		agentsMoulds.at(i)->setMouldCoeff(newCoeff);
	}
}

vector <Agent_base*>
Manager_base::getRandomAgents(int size_h){
	vector <Agent_base*>  mixedAgents = managedAgents;
	if(((uint)size_h) > managedAgents.size()){
		msgOut(MSG_CRITICAL_ERROR, "You call the getRandomAgents(size) with size too big ("+i2s(size_h)+")! Not enought agents!"); // 2007.10.18 Antonello, corrected bossible bug
	}
	random_shuffle(mixedAgents.begin(), mixedAgents.end()); // randomize the elements of the array.
	mixedAgents.resize(size_h);
	return mixedAgents;
}


vector<int>
Manager_base::getMouldIDs(){
	vector<int> toReturn;
	for (int i=0; i<mouldLocalIDCounter; i++){ toReturn.push_back(((int)i)); }
	return toReturn;
}
