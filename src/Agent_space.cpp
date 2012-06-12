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
#include "Agent_space.h"
#include "Pixel.h"
#include "ThreadManager.h"
#include "Scheduler.h"
#include "Manager_base.h"

Agent_space::Agent_space(ThreadManager* MTHREAD_h, Manager_base* manager_h): Agent_base(MTHREAD_h, manager_h){
	homePlot = NULL;
}

Agent_space::Agent_space(ThreadManager* MTHREAD_h, int uniqueID_h, Manager_base* manager_h): Agent_base(MTHREAD_h, uniqueID_h, manager_h){
	if(MTHREAD->RD->getBoolSetting("agentsView")){
		MTHREAD->treeViewerAddAgentProperty(agentUniqueID, "owned land");
		MTHREAD->treeViewerAddAgentProperty(agentUniqueID, "rented land");
	}
	homePlot = NULL;
}

Agent_space::~Agent_space(){

}

int
Agent_space::uncompletePlotDotation (int landCode_h, int ptype){
	map<int, double>::iterator p;
	int freePlots=0;
	int alreadyOwnedPlots =0;
	int alreadyRentedPlots =0;

	// getting the pixels already owned and calculating the free ones..
	if (ptype==PLOTS_ALL || ptype ==PLOTS_OWNED){
		for (uint i=0;i<ownedPlots.size();i++){
			if (ownedPlots.at(i)->getDoubleValue("landUse") == landCode_h) alreadyOwnedPlots++;
		}
		p = initialOwnedPlots.find(landCode_h);
		if(p != initialOwnedPlots.end()){
			int initialOwned = (int) p->second;
			freePlots += initialOwned - alreadyOwnedPlots;
		}
	}
	// getting the pixels already rented and calculating the free ones..
	if (ptype==PLOTS_ALL || ptype == PLOTS_RENTED){
		for (uint i=0;i<rentedPlots.size();i++){
			if (rentedPlots.at(i)->getDoubleValue("landUse") == landCode_h) alreadyRentedPlots++;
		}
		p = initialRentedPlots.find(landCode_h);
		if(p != initialRentedPlots.end()){
			int initialRented = (int) p->second;
			freePlots += initialRented - alreadyRentedPlots;
		}	
	}
	return freePlots;
}

/** 
@param ptype Type of returned plots: PLOTS_ALL(d), PLOTS_OWNED, PLOTS_RENTED
@param landCode_h Land use code of returned plots. If 0 all landTypes are returned
*/
vector <Pixel *>
Agent_space::getPlots(int ptype, int landCode_h){
	vector <Pixel *> toReturn;
	switch (ptype) {
		case PLOTS_ALL:
			merge(ownedPlots.begin(), ownedPlots.end(), rentedPlots.begin(), rentedPlots.end(), back_inserter(toReturn));
			break;
		case PLOTS_OWNED:
			toReturn = ownedPlots;
			break;
		case PLOTS_RENTED:
			toReturn = rentedPlots;
			break;
		default:
			msgOut(MSG_ERROR, "You have called Agent_space::getPlots(int ptype) with an invalid parameter. All plots have been returned.");
			merge(ownedPlots.begin(), ownedPlots.end(), rentedPlots.begin(), rentedPlots.end(), back_inserter(toReturn));
	}
	if(landCode_h == 0){return toReturn;}
	else{
		vector <Pixel *> toReturn2;
		for(uint i=0;i<toReturn.size();i++){
			if(toReturn.at(i)->getDoubleValue("landUse") == landCode_h){
				toReturn2.push_back(toReturn.at(i));
			}
		}
		return toReturn2;
	}
	
}

/** 
@param ptype Type of returned plots: PLOTS_ALL(d), PLOTS_OWNED, PLOTS_RENTED
@param landCode_h Land use code of returned plots. If 0 all landTypes are returned
*/
int
Agent_space::getNPlots(int ptype, int landCode_h){

	if(landCode_h == 0){ // we don't need the array as we have just to count them...
		switch (ptype) {
			case PLOTS_ALL:
				return ownedPlots.size()+rentedPlots.size();
			case PLOTS_OWNED:
				return ownedPlots.size();
			case PLOTS_RENTED:
				return rentedPlots.size();
			default:
				msgOut(MSG_ERROR, "You have called Agent_space::getNPlots(int ptype) with an invalid parameter. Count of all plots has been returned.");
				return ownedPlots.size()+rentedPlots.size();
		}
	}
	else{ // we need the array of Pixel* as we need to perform a check on them...
		vector <Pixel* > plots = getPlots(ptype, landCode_h);
		return plots.size();
	}
}

int
Agent_space::countMyObjects(){
	int counter = myObjects.size();
	for (uint i=0; i<ownedPlots.size();i++){
		counter += ownedPlots.at(i)->countMyObjects();
	}
	return counter;
}

double
Agent_space::getDistance(const Pixel* px1){
	return MTHREAD->GIS->getDistance(px1, homePlot);
}

void
Agent_space::payInitialYearCosts(){
	sunkCostsPaid = 0;
	// paying rental cost of still rented plots...
	for (uint i=0;i<rentedPlots.size();i++){
		sunkCostsPaid += rentedPlots[i]->getRentalCosts();
	}
	pay (sunkCostsPaid,false); // remove liquidity. Don't updateResourceValues() as done a full initMIP just next function in prepare()	
	// no investment mainainance costs for now...
}

/**
This function free the land rented and owned by the leaving agents.
Plots are marked again as "available".
While the rentedPlots array is cleared, the ownedPlots one is kept untouched, so we can eventually still refear to the original plots owned by the agent.
*/
void
Agent_space::freeLand(){

	for(uint i=0;i<rentedPlots.size();i++){
		rentedPlots[i]->destroyRentalContract();
	}
	rentedPlots.clear(); // remove all the elements of the vector
	for(uint i=0;i<ownedPlots.size();i++){
		ownedPlots[i]->setOwner(0);
	}

}
