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
#include <cmath>

#include "Init.h"
#include "Agent_farmer.h"
#include "Pixel.h"
#include "ThreadManager.h"
#include "Scheduler.h"
#include "Manager_base.h"
#include "Manager_farmers.h"

Agent_farmer::Agent_farmer(ThreadManager* MTHREAD_h, Manager_base* manager_h): Agent_space(MTHREAD_h, manager_h){
	// constructor for the agent moulds
}

Agent_farmer::Agent_farmer(ThreadManager* MTHREAD_h, int uniqueID_h, Manager_base* manager_h): Agent_space(MTHREAD_h, uniqueID_h, manager_h){
	
	// constructor used for arising the real agents in the model

	//init the decoupling right counters...
	vector<RegActivities*> actMoulds = MTHREAD->RD->getRegActivities();
	int nPolicyPremiums = MTHREAD->RD->getResourceSize(RESTYPE_POLICYPREMIUM);
	vector <double> tempVector1(nPolicyPremiums, 0);
	vector <int> tempVector2(nPolicyPremiums, 0);
	for(uint i=0;i<actMoulds.size();i++){
		decRights.push_back(tempVector1);
		decYears.push_back(tempVector2);
		decHa.push_back(tempVector1);
	}
	grossCoupledPremiums     = 0; // pre-modulation coupled premiums
	grossDecoupledPayment  = 0; // pre-modulation decoupled premium
	netTotalPremium         = 0; // post-modulation coupled and decoupled premiums

}

Agent_farmer::~Agent_farmer(){

}

void
Agent_farmer::initMIP(){
	OPT.update();
	OPT.solve();
}

void
Agent_farmer::test(){
	OPT.solve();
}

double
Agent_farmer::getAgrDistCost(const Pixel* px1){
	return getDistance(px1)* MTHREAD->RD->getDoubleSetting("agrDistCost");
}

bool
Agent_farmer::filterActivity(matrixActivities* ACT){
	if (ACT->plotHost){
		if (MTHREAD->GIS->layerExist("dtm")){
			// subtracting x% from the gross margin every 100 meters (conservative approach..)
			double altitude = ACT->plotHost->getDoubleValue("dtm");
			double toSubtract = fabs(ACT->objValue) * (MTHREAD->RD->getDoubleSetting("altitudeGMReduction")* altitude/100);
			ACT->objValue -= toSubtract;
		}
		// subtracting agricultural distance costs...
		ACT->objValue -= getAgrDistCost(ACT->plotHost);
	}
	return true;
	// no max (0,returnvalue) as some spatial activities are already negative (e.g. gross silage)
}

/**
The function produce the shadow prire of renting an addictional plot.
Do do so it firstly memorize the current value of the object function, then recalculate the OPT and solve it with the added plot and calculate the differential in the target function.
<br>Before returning it, it puts back the OPT object to its initial value.

@param initSolve This optional flag tell if the function needs to perform an initial solving of the OPT object. Attenction! If it is set to false (default) the function will assume that the OPT is already solved on its optimal value.
*/
double
Agent_farmer::getShadowPrice(const Pixel* px_h, bool initSolve){

	//QMutex mutexLocal;

	double z1=0, z2=0;
	//mutexLocal.lock();
	if(initSolve) OPT.solve();
	//mutexLocal.unlock();
	z1 = OPT.getZ();
	OPT.saveToCache();
	OPT.addPixel(px_h);
	OPT.solve();
	z2 = OPT.getZ();
	OPT.removeLastAddedPixel();
	OPT.restoreFromCache();
	return max(((double) 0), z2-z1);
}

/**
Make an offer to rent a specific plot.
<br>It calls getShadowPrice and subtract overhall costs from it.

@param initSolve This optional flag tell if the function needs to perform an initial solving of the OPT object. Attenction! If it is set to false (default) the function will assume that the OPT is already solved on its optimal value.
*/
double
Agent_farmer::offerRentalPrice(const Pixel* px_h, bool initSolve){

	/*
	double debug1=0;
	double debug2=0;
	vector<RegActivities*> actMoulds = MTHREAD->RD->getRegActivities();
	vector<ModelObject*> objects     = MTHREAD->RD->getObjectsDefinitionVector();
	for (int i=0;i<1000;i++){
		debug1 = MTHREAD->RD->getDoubleSetting("fixedRentalCosts")+debug1;
		debug2 = MTHREAD->RD->getDoubleSetting("variableRentalCosts")+debug2;
	}

	return (debug1+debug2)/1000;
	*/


	double shPrice, toOffer;
	double fixedRentalCosts, variableRentalCosts;
	//mutex.lock();
	fixedRentalCosts = MTHREAD->RD->getDoubleSetting("fixedRentalCosts");
	variableRentalCosts = MTHREAD->RD->getDoubleSetting("variableRentalCosts");
	//mutex.unlock();
	shPrice = getShadowPrice(px_h, initSolve);
	toOffer = (shPrice - fixedRentalCosts)*(1-variableRentalCosts);

	if (toOffer >= 0 ) {
		return toOffer;
	} else {
		return ((double)0);
	}

}

double
Agent_farmer::getUAA(int ptype){
	int nPlots=0;
	if (ptype == PLOTS_OWNED || ptype== PLOTS_ALL){
		nPlots += ownedPlots.size();
	}
	if (ptype==PLOTS_RENTED || ptype== PLOTS_ALL){
		nPlots += rentedPlots.size();
	}
	return nPlots*MTHREAD->GIS->getHaByPixel();
}


void
Agent_farmer::collectProductionEffects(){

	//OPT.debug();

	debugLandUse = 0;
	vector <string> policyPremiumNames = MTHREAD->RD->getResourceNames(RESTYPE_POLICYPREMIUM);
	int nPolicyPremiums = policyPremiumNames.size();
	double plotHostID;

	// initializing the recording of decoupling premium rights...
	vector<RegActivities*> actMoulds = MTHREAD->RD->getRegActivities();
	vector< vector<double> > currentCoupledPremiums; // by activities/premiumTypes
	vector< vector<double> > currentCoupledHa; // by activities/premiumTypes
	for(uint i=0;i<actMoulds.size();i++){
			vector<double> currentCoupledPremiumsTemp(nPolicyPremiums,0);
			vector<double> currentCoupledHaTemp(nPolicyPremiums,0);
			currentCoupledPremiums.push_back(currentCoupledPremiumsTemp);
			currentCoupledHa.push_back(currentCoupledHaTemp);
	}

	// add newly invested objects to the farmer or to the plot and editing the farmActivities layer..
	vector <matrixActivities*> activities = OPT.getActs();
	for(uint i=0; i<activities.size(); i++){
		if(activities[i]->isInvest){
			for(uint q=0; q< ((uint)activities[i]->q); q++){
				ModelObject newObject = MTHREAD->RD->getObjectByName(activities[i]->name);
				liquidity -= newObject.getInitialCost();
				if(activities[i]->isSpatial()){ // we assign the object to the plot...
					// activities[i]->plotHost->acquireObject(newObject); // now it's const !
					plotHostID = activities[i]->plotHost->getID();
					MTHREAD->GIS->getPixel(plotHostID)->acquireObject(newObject);
				}
				else { // we assign the object to the farmer...
					this->acquireObject(newObject);
				}
			}
		}

		// changed 24.nov.2007. It was missing to report the quantities of activities associated with the spatial new objects (that are coupled with their activities)
		// If it is a spatial activity, both as "normal" activity or as a spatial new investment (rather than a "traditionale" new investment activity), and has been selected by the farmer
		if(activities[i]->isSpatial() && activities[i]->q >0 ){
			RegActivities* actMould=0;
			plotHostID = activities[i]->plotHost->getID();
			if (activities[i]->actMould) { actMould = activities[i]->actMould;}
			else if ( activities[i]->objMould) {
				actMould=MTHREAD->RD->getRegActivityByName(activities[i]->objMould->getRequiredFor());
			}
			else {msgOut(MSG_CRITICAL_ERROR, "BOOM! All spatial activities should have an object or activity mould!");}
			/**NOTE: as farmers, at current version of the model, can grow several products on a single plot, we sometimes do not know witch one to output. We oustput a production activity on that plot only if it is predominant (e.g. higher than 60%), otherwise we label such plot as "mixed".
			In the output results values are correct, it is only in the map output that we need to make a choice.
			An other option would be to fix the spatial activities to be integer. Eventually this could become an option.*/
			if( activities[i]->q > 0.6){
				MTHREAD->GIS->getPixel(plotHostID)->changeValue("farmActivities",actMould->getMapCode());
			}
			else {
				if(activities[i]->plotHost->getDoubleValue("farmActivities")==MTHREAD->GIS->getNoValue()){
					MTHREAD->GIS->getPixel(plotHostID)->changeValue("farmActivities",900);
				}
			}
			debugLandUse += activities[i]->q;
		}

		// getting the premium right associated to each each activityMould (from activities and spatial objects that embed the activity)
		if(activities[i]->q >0 ){			
			for(uint j=0;j<actMoulds.size();j++){
				bool pass = false;
				if (activities[i]->actMould && (activities[i]->name == actMoulds[j]->getName())) {
					pass = true;
				} else if (activities[i]->objMould && (activities[i]->objMould->getRequiredFor() == actMoulds[j]->getName())) {
					pass = true;
				}

				if(pass ){
					for(uint y=0;y<nPolicyPremiums;y++){
						currentCoupledPremiums[j][y] += -activities[i]->q * actMoulds[j]->getMatrixCoefficientByName(policyPremiumNames[y]);
						currentCoupledHa[j][y] += activities[i]->q;
					}
					break;
				}
			}
		}
	}

	// recording decoupling premium rights...
	for(uint i=0;i<actMoulds.size();i++){
		for(uint y=0;y<nPolicyPremiums;y++){
			if (actMoulds[i]->getDecouplingOption(y) == -1) { // we are in a "registration mode"
				double oldDecRights = decRights[i][y];
				double oldDecHa = decHa[i][y];
				int  oldCountedYears = decYears[i][y];
				// new average = old average * years + new value, all divided by years of the old average plus one (this year)
				// checked
				decRights[i][y]= (oldDecRights*((double)oldCountedYears) + currentCoupledPremiums[i][y]) / ((double) oldCountedYears +1);
				decHa[i][y]= (oldDecHa*((double)oldCountedYears) + currentCoupledHa[i][y]) / ((double) oldCountedYears +1);
				decYears[i][y]++;
			}
		}
	}

	// calculating premiums and storing results...
	// TODO: (maybe) make the modulation enter the MIP, even if now coupled premium are of much less importance..
	grossCoupledPremiums=0;   // pre-modulation coupled premiums
	for(uint i=0;i<actMoulds.size();i++){
		for(uint y=0;y<nPolicyPremiums;y++){
			if(currentCoupledPremiums[i][y]>0){ // without this check also the "cash the premium" activity enter the counter, making it 0 !! 
				grossCoupledPremiums += currentCoupledPremiums[i][y];
			}
		}
	}
	netProductionProfit  = OPT.getZ()-grossCoupledPremiums;
	grossDecoupledPayment = getDecoupledPayments();

	netTotalPremium = applyModulation(grossCoupledPremiums + grossDecoupledPayment);

	//int debugID = agentUniqueID;
	//int debugIteration = MTHREAD->SCD->getIteration();
	//double debugOPT = OPT.getZ();
	//double debugGrossCoupledPremiums = grossCoupledPremiums;
	//double debugGrossDecoupledPayment = grossDecoupledPayment;

	// add profit to liquidity...
	liquidity += (netProductionProfit+netTotalPremium);

}

double
Agent_farmer::applyModulation(double grossPremium){

	double threshold1         = MTHREAD->RD->getDoubleSetting("threshold1",DATA_NOW);
	double threshold2         = MTHREAD->RD->getDoubleSetting("threshold2",DATA_NOW);
	double threshold3         = MTHREAD->RD->getDoubleSetting("threshold3",DATA_NOW);
	double threshold4         = MTHREAD->RD->getDoubleSetting("threshold4",DATA_NOW);
	double threshold5         = MTHREAD->RD->getDoubleSetting("threshold5",DATA_NOW);

	double modulationShare1   = MTHREAD->RD->getDoubleSetting("modulationShare1",DATA_NOW);
	double modulationShare2   = MTHREAD->RD->getDoubleSetting("modulationShare2",DATA_NOW);
	double modulationShare3   = MTHREAD->RD->getDoubleSetting("modulationShare3",DATA_NOW);
	double modulationShare4   = MTHREAD->RD->getDoubleSetting("modulationShare4",DATA_NOW);
	double modulationShare5   = MTHREAD->RD->getDoubleSetting("modulationShare5",DATA_NOW);
	double modulationShare6   = MTHREAD->RD->getDoubleSetting("modulationShare6",DATA_NOW);


	bool firstSpecial  = MTHREAD->RD->getBoolSetting("firstModLevelIsATreasholdOnly",DATA_NOW);

	double byeByeMoney=0; // ;-)

	// If firstModLevelIsATreasholdOnly has been actived the first level of modulation is "spacial" and it applyes only to payments below threshold1..
	if (firstSpecial && grossPremium >= threshold1) modulationShare1 = 0;

	if(grossPremium > threshold5){
		byeByeMoney =    modulationShare6 * (grossPremium-threshold5)
		               + modulationShare5 * (threshold5 -threshold4)
		               + modulationShare4 * (threshold4 -threshold3)
		               + modulationShare3 * (threshold3 -threshold2)
		               + modulationShare2 * (threshold2 -threshold1)
		               + modulationShare1 * threshold1;

	} else if (grossPremium > threshold4){
		byeByeMoney =    modulationShare5 * (grossPremium -threshold4)
		               + modulationShare4 * (threshold4 -threshold3)
		               + modulationShare3 * (threshold3 -threshold2)
		               + modulationShare2 * (threshold2 -threshold1)
		               + modulationShare1 * threshold1;

	} else if (grossPremium > threshold3){
		byeByeMoney =    modulationShare4 * (grossPremium  -threshold3)
		               + modulationShare3 * (threshold3 -threshold2)
		               + modulationShare2 * (threshold2 -threshold1)
		               + modulationShare1 * threshold1;

	} else if (grossPremium > threshold2){
		byeByeMoney =    modulationShare3 * (grossPremium -threshold2)
		               + modulationShare2 * (threshold2 -threshold1)
		               + modulationShare1 * threshold1;

	} else if (grossPremium > threshold1){
		byeByeMoney =    modulationShare2 * (grossPremium -threshold1)
		               + modulationShare1 * threshold1;

	} else {
		byeByeMoney =    modulationShare1 * grossPremium;
	}

	return grossPremium-byeByeMoney;

}

void
Agent_farmer::update(){
	// removing dead investment objects
	vector<ModelObject>::iterator p;
	for ( p = myObjects.begin() ; p != myObjects.end();){
		if(p->getLastYear() <= MTHREAD->SCD->getYear()){
			myObjects.erase(p);
		}
		else {
			++p;
		}
	}

	// removing pixel pointers from the list of rented plots. Before doing that I call nextYear() on plots so I am sure that out-of-scope objects are deleted before run OPT->upgrade()
	vector<Pixel*>:: iterator p2;
	for ( p2 = rentedPlots.begin(); p2 != rentedPlots.end();){
		if( (*p2)->getLastRentingYear() <= MTHREAD->SCD->getYear()){
			(*p2)->newYear();
			rentedPlots.erase(p2);
		}
		else {
			++p2;
		}
	}
	OPT.update(); // update as the farmer may have lost pixels and objects
}

int
Agent_farmer::leaveActivity(){
	//TOexDO: changeValue on farmsteads layer on NULL. Check the owned and rented plots as well. It is the manager that will do it
	double minEqC = MTHREAD->RD->getDoubleSetting("minEC");
	bool forceFarmingForCashPremium = MTHREAD->RD->getBoolSetting("forceFarmingForCashPremium", DATA_NOW);
	
	if(getCapital(true)<=0){
		msgOut(MSG_DEBUG, "Farm "+i2s(getAgentUniqueID())+" left the model for shortage of owned capital.. bye, bye..");
		return AGEXIT_NOCAPITAL;
	} else if (liquidity < - ( getCapital(true) * (1-minEqC)/minEqC) ) {
		msgOut(MSG_DEBUG, "Farm "+i2s(getAgentUniqueID())+" left the model for shortage of liquidity.. bye, bye..");
		return AGEXIT_DEBS;
	} else {
		double realProfit = netProductionProfit + netTotalPremium - sunkCostsPaid;
		liquidityShPrice = max(((double)0), liquidity)* OPT.getActivityObjValue("sellLiquidity");
		labourShPrice =  getResource("labour") * OPT.getActivityObjValue("offFarmLabour");
		landShPrice = 0 ;
		vector <double> rentalPrices = MTHREAD->RD->getDoubleVectorSetting("agrRentalPrices");
		vector <int> agrLandTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");

		
		for (uint i=0;i<ownedPlots.size();i++){
			// SHADOW VALUE CALIBRATED ON FIRST YEAR AND THEN RETRIEVED FROM AVG RENTAL PRICE
			int type= ((int)ownedPlots[i]->getDoubleValue("landUse"));
			for (uint j=0;j<agrLandTypes.size();j++){
				if (type == agrLandTypes[j]){
					double  plotShadowPrice = rentalPrices.at(j)*ownedPlots[i]->getShRentalPriceCoeff();
					landShPrice += plotShadowPrice; // /3 is a good number... no, it was for a bug. now doesn't needed
					break;
				}
			}
		}

		double premiumLeaving =0; // premium the farmer receive even in the case of leaving
		if (!forceFarmingForCashPremium){
			premiumLeaving += applyModulation(grossDecoupledPayment);
		}

		if (realProfit < (MTHREAD->RD->getDoubleSetting("oppCostsCoeff") * (liquidityShPrice+labourShPrice+landShPrice) ) + premiumLeaving ) {
			msgOut(MSG_DEBUG, "Farm "+i2s(getAgentUniqueID())+" left the model for better shadow prices of owned factors.. bye, bye..");
			return AGEXIT_SHPRICES;
		}
	}
	return AGEXIT_FALSE;
	
}

double
Agent_farmer::getDecoupledPayments(){
	
	double decPayments = initialDecPayment;
	vector<RegActivities*> actMoulds = MTHREAD->RD->getRegActivities();
	vector <string> policyPremiums = MTHREAD->RD->getResourceNames(RESTYPE_POLICYPREMIUM);
	int nPolicyPremiums = policyPremiums.size();

	int debugYear = MTHREAD->SCD->getYear();
	int farmerID = getAgentUniqueID(); 

	if(!MTHREAD->RD->getBoolSetting("regionalisation", DATA_NOW)){  // history-based
		for(uint i=0;i<actMoulds.size();i++){
			for(uint y=0; y<nPolicyPremiums;y++){
				if (actMoulds[i]->getDecouplingOption(y) >= 0) { // we are NOT in a "registration mode"
					decPayments += actMoulds[i]->getDecouplingOption(y)*decRights.at(i).at(y);
				}
			}
		}
	} else { // regionalisation
		double totHa = 0;
		for(uint i=0;i<actMoulds.size();i++){
			totHa += dVSum(decHa[i]);
		}
		decPayments += ((Manager_farmers*)manager) -> getAverageRightsByAct() * totHa;
	}

	return decPayments;
}

