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
#include <algorithm>

#include "SuperAgentManager.h"
#include "agents/Manager_farmers.h"
#include "ThreadManager.h"

/**
 * Constructor of the Super Agent Manager object.
 * It will cover all the aspects of dealing with agents
 */
SuperAgentManager::SuperAgentManager(ThreadManager* MTHREAD_h){
	//I have to write it specifically as it is a field of the base class..
	MTHREAD=MTHREAD_h;
	uniqueAgentIDCounter=0;
	uniqueManagerIDCounter=0;

}

SuperAgentManager::~SuperAgentManager(){
	for (uint i=0; i< managers.size();i++){
		delete managers[i];
	}
}

// **************************************************************************
vector<string>
SuperAgentManager::getAgentTypeNames(){
	vector<string> toReturn;
	for (uint i=0; i<managers.size(); i++){
		toReturn.push_back(managers.at(i)->getName());
	}
	return toReturn;
}

vector<int>
SuperAgentManager::getAgentCountByType(){
	vector<int> toReturn;
	for (uint i=0; i<managers.size(); i++){
		toReturn.push_back(managers.at(i)->countMyAgents());
	}
	return toReturn;
}

void
SuperAgentManager::setAgentMoulds(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->setAgentMoulds();
	}
}

void
SuperAgentManager::rescaleMoulds(double factor_h){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->rescaleMoulds(factor_h);
	}
}

void
SuperAgentManager::assignObjectsToAgents(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->assignObjectsToAgents();
	}
}

void
SuperAgentManager::assignSpatialObjectsToAgents(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->assignSpatialObjectsToAgents();
	}
}

void
SuperAgentManager::populateAgents(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->riseMyAgents();
	}
}

void
SuperAgentManager::locate(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->locateMyAgents();
	}
}

void
SuperAgentManager::createBehaviours(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->createBehaviours();
	}
}

void
SuperAgentManager::endInit(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->endInit();
	}
}

void
SuperAgentManager::prepare(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->prepare();
	}
}

void
SuperAgentManager::act(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->act();
	}
}

void
SuperAgentManager::update(){

	// perform agent independent update..
	for(int i=0;i<MTHREAD->GIS->getXNPixels();i++){
		MTHREAD->GIS->getPixel(i)->newYear();
	}

	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->update();
	}
}

void
SuperAgentManager::planNext(){
	for (uint i=0; i<managers.size(); i++){
		managers.at(i)->planNext();
	}
}

vector <Agent_base*>
SuperAgentManager::getAgentsByType(bool onlySpatial, string category_h){
	vector <Agent_base*> toReturn;
	for (uint i=0; i<managers.size(); i++){
		if(managers.at(i)->getName() == category_h || category_h==""){
			if( dynamic_cast<Manager_space*> (managers.at(i)) || !onlySpatial){
				vector <Agent_base*> agents= managers.at(i)->getMyAgents();
				copy (agents.begin(), agents.end(), back_inserter(toReturn));
			}
		}
	}
	return toReturn;
}
