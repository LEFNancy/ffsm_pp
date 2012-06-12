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
#include "Agent_base.h"
#include "ThreadManager.h"
#include "Scheduler.h"
#include "Output.h"
#include "Agent_space.h"
#include "Pixel.h"
#include "Manager_base.h"

/** The constructor get the following pointers:
  @param MTHREAD_h pointer to the main thread
*/

Agent_base::Agent_base(ThreadManager* MTHREAD_h, Manager_base* manager_h):OPT(MTHREAD_h, this){
	MTHREAD=MTHREAD_h;
	liquidity = 0;
	netProductionProfit = 0;
	manager = manager_h;
}

Agent_base::Agent_base(ThreadManager* MTHREAD_h, int uniqueID_h, Manager_base* manager_h):OPT(MTHREAD_h, this){
	MTHREAD=MTHREAD_h;
	liquidity = 0;
	netProductionProfit = 0;
	sunkCostsPaid = 0;
	agentUniqueID=uniqueID_h;
	if(MTHREAD->RD->getBoolSetting("agentsView")){
		MTHREAD->treeViewerAddAgent(agentUniqueID);
		MTHREAD->treeViewerAddAgentProperty(agentUniqueID, "liquidity");
		MTHREAD->treeViewerAddAgentProperty(agentUniqueID, "capital");
	}
	manager = manager_h;
}

Agent_base::~Agent_base(){
	if (MTHREAD->RD){ 
		if(MTHREAD->RD->getBoolSetting("agentsView")){
			MTHREAD->treeViewerRemoveAgent(agentUniqueID);
		}
	}
}

// **************************************************************************

void
Agent_base::acquireObject(ModelObject acquiredObject_h, int case_h){
	if(case_h==STAGE_INIT){
		acquiredObject_h.setRandomAge();
	}
	else {
		acquiredObject_h.setStartYear(MTHREAD->SCD->getYear());
	}
	acquiredObject_h.setOwner(this);
	myObjects.push_back(acquiredObject_h);
}

double
Agent_base::getResource(string resourceName_h){
	RegResources resource = MTHREAD->RD->getResourceByName(resourceName_h);
	switch (resource.source){
	case RESSOURCE_ZERO:
		return 0;
		break;
	case RESSOURCE_FIXED:
		return getInitialResourceByName(resource.name);
		break;
	case RESSOURCE_CALCULATED:
		return getCalculatedResourceByName(resource.name);
		break;
	case RESSOURCE_OBJECTS:
		return getObjectResourceByName(resource.name);
		break;
	default:
		msgOut(MSG_ERROR,"Resource "+resource.name+" has an unexpected source ("+i2s(resource.source)+"), I don't know how to handle that! I left 0, please check that it has a sence!");
		return 0;
	}
	return 0;
}

double
Agent_base::getInitialResourceByName(string resourceName_h){
	map<string, double>::iterator p;
	p=initialResources.find(resourceName_h);
	if(p != initialResources.end())
		return p->second;
	else 
		msgOut(MSG_ERROR, "Coud not find initial resource "+resourceName_h+". I returned 0.");
	return 0;
}

double
Agent_base::getCalculatedResourceByName(string resourceName_h){

	if (resourceName_h == "liquidity"){
		return liquidity;
	}
	else if (resourceName_h == "debsLevel"){
		// owned capital / (owned capital+debs level) = minEqCShare
		// ==> debsLevel = owned capital * (1-minEqCShare) / minEqCShare;
		double minEqCShare = MTHREAD->RD->getDoubleSetting("minEC");
		return (this->getCapital() * (1-minEqCShare)/minEqCShare);
	}
	else {
		msgOut(MSG_ERROR,"I don't know how to calculate resource "+resourceName_h+". I return 0, please check that it has a sence!");
	}
	return 0;
}

double
Agent_base::getObjectResourceByName(string resourceName_h){
	double resourceValue=0;
	vector <ModelObject*> objects = getAvailableObjects();
	for (uint i=0; i<objects.size();i++){
		resourceValue += -objects.at(i)->getResourceValue(resourceName_h);
	}
	return resourceValue;
}

double
Agent_base::getCapital(bool endOfYear){
	double capital;
	capital = liquidity;
	vector <ModelObject*> objects = getOwnedObjects();
	for (uint i=0; i<objects.size();i++){
		capital += objects.at(i)->getCurrentValue(endOfYear);
	}
	capital += getLandCapital();
	return capital;
}

vector <ModelObject*>
Agent_base::getOwnedObjects(){
	vector <ModelObject*> toReturn;
	for (uint i=0; i<myObjects.size();i++){
		toReturn.push_back(&myObjects.at(i));
	}
	Agent_space* spatialAgent = dynamic_cast<Agent_space*> (this);
	if(spatialAgent){
		vector<Pixel*> ownedPlots  = spatialAgent->getPlots(PLOTS_OWNED);
		vector<Pixel*> rentedPlots = spatialAgent->getPlots(PLOTS_RENTED);
		for(uint i=0;i<ownedPlots.size();i++){
			vector <ModelObject*> plotObjects = ownedPlots[i]->getIncludedObjects();
			for (uint j=0;j<plotObjects.size();j++){
				Agent_base* objOwner = plotObjects[j]->getOwner();
				if (objOwner && objOwner== this){
					toReturn.push_back(plotObjects[j]);
				}
			}
		}
		for(uint i=0;i<rentedPlots.size();i++){
			vector <ModelObject*> plotObjects = rentedPlots[i]->getIncludedObjects();
			for (uint j=0;j<plotObjects.size();j++){
				Agent_base* objOwner = plotObjects[j]->getOwner();
				if (objOwner && objOwner == this){
					toReturn.push_back(plotObjects[j]);
				}
			}
		}
	}
	return toReturn;
	//FIXME: if we allow farm2farm rental this function would need to be upgraded, as it would not catch objects owned but rented to an other farm
}

vector <ModelObject*>
Agent_base::getAvailableObjects(){
	vector <ModelObject*> toReturn;
	for (uint i=0; i<myObjects.size();i++){
		toReturn.push_back(&myObjects.at(i));
	}
	Agent_space* spatialAgent = dynamic_cast<Agent_space*> (this);
	if(spatialAgent){
		vector<Pixel*> ownedPlots  = spatialAgent->getPlots(PLOTS_OWNED);
		vector<Pixel*> rentedPlots = spatialAgent->getPlots(PLOTS_RENTED);
		for(uint i=0;i<ownedPlots.size();i++){
			if(!ownedPlots[i]->getTenant() || ownedPlots[i]->getTenant() == this){     //if the owned plot is not rented to some other agent
				vector <ModelObject*> plotObjects = ownedPlots[i]->getIncludedObjects();
				for (uint j=0;j<plotObjects.size();j++){
					toReturn.push_back(plotObjects[j]);
				}
			}
		}
		for(uint i=0;i<rentedPlots.size();i++){
			vector <ModelObject*> plotObjects = rentedPlots[i]->getIncludedObjects();
			for (uint j=0;j<plotObjects.size();j++){
				toReturn.push_back(plotObjects[j]);
			}
		}
	}
	return toReturn;
}

void
Agent_base::withdraw(){
	double fixedMoneyToWithdraw = MTHREAD->RD->getDoubleSetting("perCapMinWithdrawal")* getResource("labour") / MTHREAD->RD->getDoubleSetting("AWU_h");
	double variableMoneyToWithDraw = max(((double)0),getOverallProfit())*MTHREAD->RD->getDoubleSetting("withdrawalProfitShare");
	liquidity -= (fixedMoneyToWithdraw+variableMoneyToWithDraw);
}

void
Agent_base::answerStats(AgentSurvey &SURVEY){
	Agent_space*  spatialAgent=NULL;
	if( dynamic_cast<Agent_space*> (this) ){
		spatialAgent = ( (Agent_space*) this );
	}

	// PLOTS..
	vector <Pixel*> ownedPlots;
	vector <Pixel*> rentedPlots;
	vector <int> landTypes = MTHREAD->RD->getIntVectorSetting("landTypes");
	vector <int> ownedPlotsByAllTypes(landTypes.size(), 0);  // initialized with all 0 values
	vector <int> rentedPlotsByAllTypes(landTypes.size(), 0); // initialized with all 0 values
	if( spatialAgent ){
		ownedPlots = spatialAgent->getPlots(PLOTS_OWNED);
		rentedPlots = spatialAgent->getPlots(PLOTS_RENTED);
	}
	for(uint i=0;i<ownedPlots.size();i++){
		for(uint j=0;j<landTypes.size();j++){
			if(ownedPlots[i]->getDoubleValue("landUse") == landTypes[j]){
				ownedPlotsByAllTypes.at(j)++;
				break;
			}
		}
	}
	for(uint i=0;i<rentedPlots.size();i++){
		for(uint j=0;j<landTypes.size();j++){
			if(rentedPlots[i]->getDoubleValue("landUse") == landTypes[j]){
				rentedPlotsByAllTypes.at(j)++;
				break;
			}
		}
	}

	// OBJECTS..
	vector <ModelObject*> objTypes                     = MTHREAD->RD->getObjectsDefinitionVector();
	vector<int>           ownedObjectsByType           (objTypes.size(), 0);
	vector<int>           availableObjectsByType       (objTypes.size(), 0);
	vector<int>           newlyAvailableObjectsByType  (objTypes.size(), 0);
	vector <ModelObject*> ownedObjects                 = getOwnedObjects();
	vector <ModelObject*> availableObjects             = getAvailableObjects();

	for(uint i=0;i<ownedObjects.size();i++){
		for(uint j=0;j<objTypes.size();j++){
			if(ownedObjects[i]->getName() == objTypes[j]->getName()){
				ownedObjectsByType.at(j)++;
				break;
			}
		}
	}
	for(uint i=0;i<availableObjects.size();i++){
		for(uint j=0;j<objTypes.size();j++){
			if(availableObjects[i]->getName() == objTypes[j]->getName()){
				availableObjectsByType.at(j)++;
				if(availableObjects[i]->getAge() == 0){
					 newlyAvailableObjectsByType.at(j)++;
				}
				break;
			}
		}
	}

	// ACTIVITIES..
	vector<RegActivities*> actTypes = MTHREAD->RD->getRegActivities();
	vector<double>         activitiesByType (actTypes.size(), 0);
	vector<matrixActivities*> myActivities = OPT.getActs();

	// changed 24.nov.2007. It was missing to report the quantities of activities associated with the spatial new objects (that are coupled with their activities)
	for(uint i=0;i<myActivities.size();i++){
		string myActName = "";
		if (myActivities[i]->actMould) {myActName = myActivities[i]->actMould->getName();} // activity derive from a normal activity
		else if(myActivities[i]->objMould){myActName = myActivities[i]->objMould->getRequiredFor();} // activity derive from an object
		else {
			continue; // it is a normal, unspatial object
			//msgOut(MSG_CRITICAL_ERROR, "BOOM! We found an activity without any object or activity mould!");
		 }

		for(uint j=0;j<actTypes.size();j++){
			if (actTypes[j]->getName() == myActName) {
				activitiesByType.at(j) += myActivities[i]->q;
				break;
			}
		}
	}

	// FILLING THE SURVEY....
	SURVEY.year                 = MTHREAD->SCD->getYear();
	SURVEY.agentType            = this->getCategory();
	if( spatialAgent){
		SURVEY.spatial          = true;
	}
	else {
		SURVEY.spatial          = false;
	}
	SURVEY.agentID              = this->getAgentUniqueID();
	SURVEY.agentMouldID         = this->getMouldLocalID();
	SURVEY.managerID            = this->manager->getManagerID();
	SURVEY.agent                = this;
	if(spatialAgent){
		SURVEY.hostPlotID       = spatialAgent->getHomePlot()->getID();
		SURVEY.hostPlotX        = spatialAgent->getHomePlot()->getX();
		SURVEY.hostPlotY        = spatialAgent->getHomePlot()->getY();
	}
	SURVEY.capital              = this->getCapital();
	SURVEY.landCapital          = spatialAgent->getLandCapital();
	SURVEY.labour               = this->getResource("labour");
	double objCapital=0;
	for (uint i=0; i<ownedObjects.size();i++){
		objCapital += ownedObjects[i]->getCurrentValue();
	}
	SURVEY.objectsCapital       = objCapital;
	SURVEY.liquidity            = liquidity;

	SURVEY.netProductionProfit  = netProductionProfit;

	if(getGrossCoupledPremiums()>0){
		SURVEY.coupledPremiums      = getGrossCoupledPremiums()  * getNetTotalPremium()/(getGrossCoupledPremiums() + getGrossDecoupledPayment());
	}
	else {
		SURVEY.coupledPremiums = 0;
	}
	if (getGrossDecoupledPayment()>0){
		SURVEY.decoupledPayment     = getGrossDecoupledPayment() * getNetTotalPremium()/(getGrossCoupledPremiums() + getGrossDecoupledPayment());
	}
	else {
		SURVEY.decoupledPayment     = 0;
	}

	SURVEY.lastSunkCostsPaid    = sunkCostsPaid;

	SURVEY.ownedPlotsByAllTypes        = ownedPlotsByAllTypes;
	SURVEY.rentedPlotsByAllTypes       = rentedPlotsByAllTypes;
	SURVEY.ownedObjectsByType          = ownedObjectsByType;
	SURVEY.availableObjectsByType      = availableObjectsByType;
	SURVEY.newlyAvailableObjectsByType = newlyAvailableObjectsByType;
	SURVEY.activitiesByType            = activitiesByType;
}

void
Agent_base::pay(double amount_h, bool updateResourceValues){
	liquidity -= amount_h;
	if (updateResourceValues) OPT.updateResourceValues();
}
