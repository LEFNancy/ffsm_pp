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

#include "Manager_farmers.h"
#include "RegData.h"
#include "Agent_farmer.h"
#include "SuperAgentManager.h"
//#include "Pixel.h"
#include "Opt.h"
#include "Init.h"
#include "ThreadManager.h"
#include "Output.h"
#include "Scheduler.h"


//constructor
Manager_farmers::Manager_farmers(ThreadManager* MTHREAD_h, string name_h) : Manager_space(MTHREAD_h, name_h){
	msgOut(MSG_DEBUG,"Manager_farmers::Manager_farmers");
	collectorAgent = new Agent_farmer(MTHREAD_h, -managerID-1, ((Manager_base*) this));
}

//destructor
Manager_farmers::~Manager_farmers(){
	delete collectorAgent;
}

//functions:
/**
This function load the agent mould from a file and place them in the vector agentsMoulds.
@see Manager_base::setAgentMoulds()

*/
void
Manager_farmers::setAgentMoulds(){

	msgOut(MSG_DEBUG,"Setting farmer moulds..");
	LLData table = MTHREAD->RD->getTable("farmers");

	for (int i=0; i<table.nrecords();i++){
		Agent_farmer* FARMER = new Agent_farmer(MTHREAD, this);
		FARMER->setMouldLocalID(getNewMouldLocalID());
		FARMER->setComments(table.getData(i,"comments"));
		FARMER->setFadnID(table.getData(i,"fadnID"));
		FARMER->setMouldCoeff(s2i(table.getData(i,"upsCoeff")));
		FARMER->setFarmType(table.getData(i,"farmType"));
		FARMER->setHomeLandUseCode(s2i(table.getData(i,"homeLandUseCode")));
		FARMER->setLandCapital(s2d(table.getData(i,"landCapital")));
		FARMER->setInitialDecPayment(s2d(table.getData(i,"initialDecPayment")));
		if (MTHREAD->RD->getBoolSetting("useAgrRegions")){
			FARMER->setAgrRegionNumber(s2i(table.getData(i,"agrRegionNumber")));
		} else {
			FARMER->setAgrRegionNumber(0);
		}
		vector <string> resources = MTHREAD->RD->getResourceNames();
		for (uint j=0;j<resources.size();j++){
			double value = s2d(table.getData(i,resources.at(j)));
			FARMER->initialResources.insert(pair<string, double>(resources.at(j),value));
		}
		vector <int> agrLandTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
		for (uint z=0;z<agrLandTypes.size();z++){
			string rentedName = "land_r_"+i2s(agrLandTypes.at(z));
			string ownerName = "land_o_"+i2s(agrLandTypes.at(z));
			double rentedValue = s2d(table.getData(i,rentedName));
			double ownedValue = s2d(table.getData(i,ownerName));
			FARMER->initialRentedPlots.insert(pair<int, double>(agrLandTypes.at(z),rentedValue));
			FARMER->initialOwnedPlots.insert(pair<int, double>(agrLandTypes.at(z),ownedValue));
		}
		agentsMoulds.push_back(FARMER);
	}

	/*

	string directory;
	string filename;
	string filename_complete;
	vector<IFiles> iFilesVector = MTHREAD->RD->getIFilesVector();
	for (uint i=0; i<iFilesVector.size(); i++){
		if (iFilesVector.at(i).type == "farmers"){
			directory=iFilesVector.at(i).directory;
			filename=iFilesVector.at(i).name;
			break;
		}
	}
	string baseDirectory=MTHREAD->RD->getBaseDirectory();
	filename_complete = baseDirectory+directory+filename;
	InputNode objectDocument;
	msgOut(MSG_DEBUG, filename_complete);
	bool test=objectDocument.setWorkingFile(filename_complete);
	if (!test){msgOut(MSG_CRITICAL_ERROR, "Error opening the agents definition file "+filename_complete);}

	vector<InputNode> nodes = objectDocument.getNodesByName("record");

	for (uint i=0; i<nodes.size();i++){
		Agent_farmer* FARMER = new Agent_farmer(MTHREAD, this);
		FARMER->setMouldLocalID(getNewMouldLocalID());
		FARMER->setComments(nodes.at(i).getNodeByName("comments").getStringContent());
		FARMER->setFadnID(nodes.at(i).getNodeByName("fadnID").getStringContent());
		FARMER->setMouldCoeff(nodes.at(i).getNodeByName("upsCoeff").getIntContent());
		FARMER->setFarmType(nodes.at(i).getNodeByName("farmType").getStringContent());
		FARMER->setHomeLandUseCode(nodes.at(i).getNodeByName("homeLandUseCode").getIntContent());
		FARMER->setLandCapital(nodes.at(i).getNodeByName("landCapital").getDoubleContent());
		FARMER->setInitialDecPayment(nodes.at(i).getNodeByName("initialDecPayment").getDoubleContent());

		vector <string> resources = MTHREAD->RD->getResourceNames();
		for (uint j=0;j<resources.size();j++){
			double value = nodes.at(i).getNodeByName(resources.at(j)).getDoubleContent();
			FARMER->initialResources.insert(pair<string, double>(resources.at(j),value));
		}
		vector <int> agrLandTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
		for (uint z=0;z<agrLandTypes.size();z++){
			string rentedName = "land_r_"+i2s(agrLandTypes.at(z));
			string ownerName = "land_o_"+i2s(agrLandTypes.at(z));
			double rentedValue = nodes.at(i).getNodeByName(rentedName).getDoubleContent();
			double ownedValue = nodes.at(i).getNodeByName(ownerName).getDoubleContent();
			FARMER->initialRentedPlots.insert(pair<int, double>(agrLandTypes.at(z),rentedValue));
			FARMER->initialOwnedPlots.insert(pair<int, double>(agrLandTypes.at(z),ownedValue));
		}
		agentsMoulds.push_back(FARMER);
	}
	*/
}

/**
This reimplemented function create the agents from their mould, applying to each of the mould their coefficient and place the (actual) agents to the vector managedAgents.
@see Manager_base::riseMyAgents()
*/
void
Manager_farmers::riseMyAgents(){
 	msgOut(MSG_DEBUG,"Rising farmer agents..");
	// about 5700 agents creted here..
	for (uint i=0;i<agentsMoulds.size();i++){
		Agent_farmer * currentAgent = ((Agent_farmer*)agentsMoulds.at(i));
		for (int z=0;z<currentAgent->getMouldCoeff();z++){ // the mould coefficient change if we are in the subregion mode !!!
			Agent_farmer* FARMER= new Agent_farmer(MTHREAD, MTHREAD->SAM->getNewUniqueAgentID(), this);
			FARMER->setAgentLocalID(getNewAgentLocalID());
			FARMER->setMouldLocalID(currentAgent->getMouldLocalID());
			FARMER->setFadnID(currentAgent->getFadnID());
			FARMER->setFarmType(currentAgent->getFarmType());
			FARMER->setHomeLandUseCode(currentAgent->getHomeLandUseCode());
			FARMER->setLandCapital(currentAgent->getLandCapital());
			FARMER->setInitialDecPayment(currentAgent->getInitialDecPayment());
			FARMER->setInitialResources(currentAgent->getInitialResources());
			FARMER->setInitialRentedPlots(currentAgent->getInitialRentedPlots());
			FARMER->setInitialOwnedPlots(currentAgent->getInitialOwnedPlots());
			FARMER->setMouldLocalID(currentAgent->getMouldLocalID());
			FARMER->setMouldCoeff(-1);
			FARMER->setLiquidity(currentAgent->getInitialResourceByName("liquidity"));
			FARMER->setAgrRegionNumber(currentAgent->getAgrRegionNumber());
			FARMER->setCategory(this->getName());
			//here things to differenciate agents... e.g. age..
			managedAgents.push_back(FARMER);	
		}
	} 
	MTHREAD->treeViewerManagerPropertyChangeValue(getName(),"managed agents",i2s(managedAgents.size()));
	/*for (uint i=0;i<managedAgents.size();i++){
		cout <<"ID: "<<managedAgents[i]->getAgentUniqueID() << " MouldID: " << managedAgents[i]->getMouldLocalID() << endl;
	}*/
}

void
Manager_farmers::locateMyAgents(){

	msgOut(MSG_DEBUG,"Locating farmers..");

	MTHREAD->GIS->addLayer("farmsteads", "Farmstead position", true, true);
	MTHREAD->GIS->addLayer("farmOwnedPlots", "Farmers ID (owned plots)", true, true);
	MTHREAD->GIS->addLayer("farmRentedPlots", "Farmers ID (rented plots)", true, true);

	string managerIDString = i2s(-managerID-1);
	MTHREAD->GIS->addLegendItem("farmOwnedPlots", -managerID-1, managerIDString, 0, 0, 0, -managerID-1, -managerID-1); // adding a black legend item for pixels owned by dead agents


	vector <int> agrLandTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	vector <vector <Pixel*> > freePlotsByLandTypes;
	for (uint i=0;i<agrLandTypes.size();i++){
		vector <Pixel * > freePlots;
		freePlots = MTHREAD->GIS->getAllPlotsByValue("landUse",agrLandTypes.at(i),true, MSG_CRITICAL_ERROR); // filtering only the free plots
		freePlotsByLandTypes.push_back(freePlots);
	}

	// locating the farmstead...
	msgOut(MSG_INFO,"Assigning location to each managed agent..");
	for (uint i=0;i<managedAgents.size();i++){
		Agent_farmer * currentAgent = ((Agent_farmer*) managedAgents.at(i));

		//remainingRentingYears = minAgrContract +( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(maxAgrContract-minAgrContract+1); // [minAgrContract, maxAgrContract]
		//int random = min + ( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(max-min+1);
		// 20080117: RGB colors restricted to the [50,255] interval to avoid too dark pixels (and because black is used for collectorAgent)
		int randomRed = int (50+( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(255-50+1)); // randomRed is [50,255] Don't use "randomNumber % range" !!
		int randomGreen = int (50+( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(255-50+1));
		int randomBlue = int (50+( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(255-50+1));

		int ID = managedAgents.at(i)->getAgentLocalID();
		string ID_str = i2s(ID);
		MTHREAD->GIS->addLegendItem("farmsteads", ID, ID_str, randomRed, randomGreen, randomBlue, ID, ID);
		MTHREAD->GIS->addLegendItem("farmOwnedPlots", ID, ID_str, randomRed, randomGreen, randomBlue, ID, ID);
		MTHREAD->GIS->addLegendItem("farmRentedPlots", ID, ID_str, randomRed, randomGreen, randomBlue, ID, ID);

		Pixel* assignedPlot;
		int homeLandUseCode = currentAgent->getHomeLandUseCode();
		int idx =-1;
		for (uint j=0;j<agrLandTypes.size();j++){
			if(homeLandUseCode == agrLandTypes.at(j)){
				idx = j;
				break;
			}
		}
		if (idx<0) {
			msgOut(MSG_CRITICAL_ERROR, "Looks like farmer "+i2s(ID)+" (mould ID " + i2s(currentAgent->getMouldLocalID())+  ") has a homeLandUseCode that is not within the agricultural sets!");
		}
		if (freePlotsByLandTypes.at(idx).size()<1){
			msgOut(MSG_CRITICAL_ERROR, "Trying to assign a plot from an empty vector.");
		}

		/*int vectorSize = freePlotsByLandTypes.at(idx).size();
		assignedPlot = freePlotsByLandTypes.at(idx).at(vectorSize-1);
		freePlotsByLandTypes.at(idx).pop_back();
		assignedPlot->changeValue("farmsteads", ID);
		currentAgent->setHomePlot(assignedPlot);*/

		int vectorSize = freePlotsByLandTypes.at(idx).size();
		vector <Pixel*> tempVector;

		bool boolFlagAssigned = 0;
		if(MTHREAD->RD->getBoolSetting("useAgrRegions")){
			for(uint j=0;j<vectorSize;j++){
				Pixel* tempPixel = freePlotsByLandTypes.at(idx).at(j);
				/*int debugAgrRegion = tempPixel->getDoubleValue("agrRegions");
				int debugAgentAgrRegion = currentAgent->getAgrRegionNumber();
				int debugPxLandUseCode = tempPixel->getDoubleValue("landUse");*/
				if (tempPixel->getDoubleValue("agrRegions") == currentAgent->getAgrRegionNumber()
						&& (! boolFlagAssigned)
				){
					assignedPlot = tempPixel;
					assignedPlot->changeValue("farmsteads", ID);
					currentAgent->setHomePlot(assignedPlot);
					boolFlagAssigned = 1;
				}
				else {
					tempVector.push_back(tempPixel);
				}
			}
		}
		if(!boolFlagAssigned){
			assignedPlot = freePlotsByLandTypes.at(idx).at(vectorSize-1);
			freePlotsByLandTypes.at(idx).pop_back();
			assignedPlot->changeValue("farmsteads", ID);
			currentAgent->setHomePlot(assignedPlot);
		} else {
			freePlotsByLandTypes.at(idx).clear();
			freePlotsByLandTypes.at(idx).resize(tempVector.size());
			copy(tempVector.begin(), tempVector.end(),freePlotsByLandTypes.at(idx).begin() );
		}

		if (currentAgent->uncompletePlotDotation(homeLandUseCode,PLOTS_OWNED)){
			assignPlot(assignedPlot, currentAgent, 0, PLOTS_OWNED); // assign as a owned plot
		}
		else if (currentAgent->uncompletePlotDotation(homeLandUseCode,PLOTS_RENTED)){
			double startRentalCosts = assignedPlot->getStartingRentalCosts();
			assignPlot(assignedPlot, currentAgent, startRentalCosts, PLOTS_RENTED);  // assigned as a rented plot
			assignedPlot->setRemainingRentingTerm(999); // hack, if the farmstead is on a rented plot - it means this farm doesn't have owned land - the rental contract is perpetual.
		}
		else {
			msgOut(MSG_CRITICAL_ERROR, "Looks like farmer "+i2s(ID)+" (mould ID " + i2s(currentAgent->getMouldLocalID())+  ")doesn't have any initial plot to place the farmstead!");
		}
		refreshGUI();

		/*int debug1 = currentAgent->getAgentUniqueID();
		int debug2 = currentAgent->getHomeLandUseCode();
		int debug3 = currentAgent->getAgrRegionNumber();
		int debug4 = currentAgent->getHomePlot()->getID();
		int debug5 = currentAgent->getHomePlot()->getDoubleValue("landUse");
		int debug6 = currentAgent->getHomePlot()->getDoubleValue("agrRegions");

		cout <<"Farmer ID/mould ID/home land use code/Agr.R.Num: " << 
		       currentAgent->getAgentUniqueID() << " - " <<
		       currentAgent->getMouldLocalID() << " - " <<
		       currentAgent->getHomeLandUseCode() << " - " <<
		       currentAgent->getAgrRegionNumber() << " Host plot's ID/land use code/region number/: " <<
		       currentAgent->getHomePlot()->getID() << " - " <<
		       currentAgent->getHomePlot()->getDoubleValue("landUse") << " - " <<
		       currentAgent->getHomePlot()->getDoubleValue("agrRegions") << endl;*/
		//DONE: the problem was in the maps with wrong regionalNumer, not in the code. land allocation by agr region doesn't work! 
	}
	refreshGUI();

	MTHREAD->GIS->updateImage("farmsteads");

	// assigning plots to the closer farm with free slots...
	msgOut(MSG_INFO, "Assigning plots to each farm..");

	// caching the farmstead owner on each plots...
	msgOut(MSG_DEBUG, "Cashing pixels with their farmstead owners...");
	double nPixels = MTHREAD->GIS->getXyNPixels();
	double noValue = MTHREAD->GIS->getNoValue();
	double farmID;
	for (double i=0; i<nPixels; i++){
		Pixel* currentPixel= MTHREAD->GIS->getPixel(i);
		farmID = currentPixel->getDoubleValue("farmsteads");
		currentPixel->clearCache();
		if (farmID != noValue){
			Agent_farmer * currentFarmer = ((Agent_farmer*) getAgentByLocalID(farmID));
			currentPixel->setCachedAgent(currentFarmer);
		}
	}

	// merging and mixing the remained freeplots...	
	vector <Pixel*> freePlots;
	for (uint i=0;i<agrLandTypes.size();i++){
		for (uint j=0;j<freePlotsByLandTypes.at(i).size();j++){
			freePlots.push_back(freePlotsByLandTypes.at(i).at(j));
		}
	}
	random_shuffle(freePlots.begin(), freePlots.end());
	
	int maxRange =   MTHREAD->RD->getIntSetting("initialLandAllocationMaxRange"); // range to search for free plots to match the initial farm capacity (in plots)
	int assignedCount = 0;

	msgOut(MSG_DEBUG, "Started the plot assignment algorithm (this may take some minutes)..");
	msgOut(MSG_DEBUG, "Total pixels to assign: "+i2s(freePlots.size()));

	/*
	Looking over concentical neighbour plots loop for farmers. When a farmer is found, if it is still on a "uncomplete" state assign the plot to her.
	*/
	for (uint i=0;i<freePlots.size();i++){
		// it crash at i==19717.. that was for right-bottom corner error in getPixelsAtDistLevel(j), solved on 20070719
		int landUseCode = ((int)freePlots.at(i)->getDoubleValue("landUse"));
		bool assigned = false;
		for (int j=1;j<maxRange;j++){
			if (assigned) break;
			vector <Pixel *> neighbourPlots = freePlots[i]->getPixelsAtDistLevel(j);
			for (uint z=0;z<neighbourPlots.size();z++){
				if (neighbourPlots[z]->getCachedAgent()){
					Agent_farmer * currentFarmer = ((Agent_farmer*) neighbourPlots[z]->getCachedAgent() );
					if (currentFarmer->uncompletePlotDotation(landUseCode,PLOTS_OWNED)){
						assignPlot(freePlots[i], currentFarmer, 0, PLOTS_OWNED); // assign as a owned plot
						assigned = true;
						assignedCount ++;
						break;
					}
					else if (currentFarmer->uncompletePlotDotation(landUseCode,PLOTS_RENTED)){
						double startRentalCosts = freePlots[i]->getStartingRentalCosts();
						assignPlot(freePlots[i], currentFarmer, startRentalCosts, PLOTS_RENTED); // assigned as a rented plot
						assigned = true;
						assignedCount ++;
						break;
					}
				}
			}
		}
		refreshGUI();
	}

	msgOut(MSG_DEBUG,"Total assigned pixels: "+i2s(assignedCount));

	MTHREAD->GIS->updateImage("farmOwnedPlots");
	MTHREAD->GIS->updateImage("farmRentedPlots");
	
	if(MTHREAD->RD->getBoolSetting("agentsView")){
		double haByPlot = MTHREAD->GIS->getHaByPixel();
		for(uint i=0;i<managedAgents.size();i++){
			int ownedPlots = ((Agent_space*) managedAgents.at(i)) -> getNPlots(PLOTS_OWNED);
			int rentedPlots = ( (Agent_space*) managedAgents.at(i)) -> getNPlots(PLOTS_RENTED);
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"owned land", d2s(( (double)ownedPlots)*haByPlot));
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"rented land", d2s(( (double)rentedPlots)*haByPlot));
		}
	}

}

void
Manager_farmers::createBehaviours(){
	msgOut(MSG_INFO, "Creating farmers bahaviours (decision matrix)..");
	for (uint i=0;i<managedAgents.size();i++){
		Agent_farmer * currentAgent = ((Agent_farmer*) managedAgents.at(i));
		currentAgent->initMIP();
	}
	msgOut(MSG_DEBUG, "Ended initialization of the MIP.");
	if(MTHREAD->RD->getBoolSetting("agentsView")){
		for(uint i=0;i<managedAgents.size();i++){
			double liquidity = ((Agent_space*) managedAgents.at(i)) -> getLiquidity();
			double capital   = ( (Agent_space*) managedAgents.at(i)) -> getCapital();
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"liquidity", d2s(liquidity));
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"capital", d2s(capital));
		}
	}
}

/**
This function is used to perform any various task to be done before the simulation start
 
*/
void
Manager_farmers::endInit(){

	// initialising the arrays of decoupling rights for dead farmers...
	int nActivities = MTHREAD->RD->getActivitiesDefinitionVectorSize();
	int nPolicyPremiums = MTHREAD->RD->getResourceSize(RESTYPE_POLICYPREMIUM);
	for (int i=0; i< nActivities; i++){
		vector<double> tempVector(nPolicyPremiums,0);
		for(uint y=0;y<nPolicyPremiums;y++){
			sumOfRightsOfDeadAgents.push_back(tempVector);
			sumOfHaOfDeadAgents.push_back(tempVector);
		}
	}
	 //calibrateRentalCoefficients();

}

void
Manager_farmers::prepare(){
	msgOut(MSG_INFO, "Preparing of the period has started..");
	
	for (uint i=0;i<managedAgents.size();i++){
		Agent_farmer * currentAgent = ((Agent_farmer*) managedAgents.at(i));
		currentAgent->payInitialYearCosts();
	}

	for(uint i=0;i<managedAgents.size();i++){
		Agent_farmer * currentAgent = ((Agent_farmer*) managedAgents.at(i));
		currentAgent->initMIP();
	}

	landAllocation();
	if(MTHREAD->SCD->getYear() == MTHREAD->RD->getIntSetting("initialYear")){
		calibrateLandValues(); // needed for calculating shadow price of owned land when farms must decide if continue
	}

	// 2007.11.05 Payment of rent for newly rented plots is now in the assignPlot() function (after each individual assignation)

}

/**
	landAllocation is one of the most important functions.
	<br>Here we allocate the free plots to the individual farmers.
	<br>We collect the X closest farms to the free plot and we gonna ask each to bid for it. Then we collect the results and assign the plot to the highest bidder at the second-best bid.
*/


// new
void
Manager_farmers::landAllocation(){

	vector <Pixel*> freePlots;
	int counterAssigned;
	int loopCounter;
	vector <int> agrLandTypes;
	vector <string> agrLandTypesShortLabels;
	int bidSize;
	double highestBid;
	double secondHighestBid;
	Agent_farmer* currentWinner;
	int freePlotsCount;
	double bid=0;
	vector <Agent_space* > closestAgents;
	int closestAgentCount;
	//int nThreads = MTHREAD->RD->getIntSetting("nThreads");
	int nThreads = 1; //TODO: make again nthread loadable in input
	//QMutex* mutex = new QMutex;

	agrLandTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	vector <double> sumOfRentalExpendituresBySoilType (agrLandTypes.size(), ((double)0));
	vector <int>    assignedPlotsBySoilType (agrLandTypes.size(), 0);
	vector <double> avgPricesBySoilType (agrLandTypes.size(), ((double)0));
	
	agrLandTypesShortLabels = MTHREAD->RD->getStringVectorSetting("agrLandTypesShortLabels");
	bidSize = MTHREAD->RD->getIntSetting("askBidsArraySize");

	if(MTHREAD->RD->getBoolSetting("subRegionMode")){ // in case of a sub-region we take less bids, but at least 3
		bidSize = ((int)round(bidSize * MTHREAD->RD->getDoubleSetting("scaleCoeff")));
		bidSize = max(3, bidSize);
	}
	QThreadPool::globalInstance()->setMaxThreadCount(nThreads);
	vector <Manager_farmers_threads*> wThreads;

	//for (int i=0; i < nThreads; i++){
	//	Manager_farmers_threads* wThread = new Manager_farmers_threads;
	//	wThreads.push_back(wThread);
	//}
	
	for (uint i=0;i< agrLandTypes.size(); i++){
		sumOfRentalExpendituresBySoilType[i] = 0;
		assignedPlotsBySoilType[i] = 0;

		freePlots =  MTHREAD->GIS->getAllPlotsByValue("landUse", agrLandTypes.at(i), true, MSG_DEBUG);
		counterAssigned=0;
		loopCounter=0;
		msgOut(MSG_INFO, "Renting plots of type "+i2s(agrLandTypes.at(i))+". "+i2s(freePlots.size())+" plots to assign (this can take a while)..");
		freePlotsCount = freePlots.size();
		for (int j=0;j<freePlotsCount;j++){
			highestBid           = 0;
			secondHighestBid     = 0;
			currentWinner = 0;

			for(uint ii=0;ii<2;ii++){ //we try two times before we end up declaring a plot unused..
				if(ii==0){ // first, normal attempt
					closestAgents = MTHREAD->GIS->getClosestAgents(freePlots[j], bidSize, name);
				}
				else { // we ask to bid to more farms...
					closestAgents = MTHREAD->GIS->getClosestAgents(freePlots[j], bidSize*3, this->getName());
				}
				closestAgentCount = closestAgents.size();

				vector<double> offers (closestAgentCount, ((double)0));

				
				//META CODE:
				//For each agents to ask price
				//	For each thread
				//		We ask the thread if it is running
				//		If not, while it is not running, we pass the thread some infos it will need to elaborate and then we start the thread
				//ATTENCTION! This code require a specifically patched glpk library that, even if not fully //reentrant, doesn't make the application crash in presence of several threads
				


				for (uint y=0;y<closestAgents.size();y++){
					Manager_farmers_threads* runElement = new Manager_farmers_threads;
					runElement->setAutoDelete(FALSE);
					runElement->assignJob(closestAgents.at(y),freePlots.at(j));
					wThreads.push_back(runElement);
					QThreadPool::globalInstance()->start(runElement);
				}


				// Be sure that all thread have ended their last job before continue..
				//for (int z=0; z < nThreads; z++){
				//	wThreads[z]->wait();
				//}
				QThreadPool::globalInstance()->waitForDone();
				for ( int z=0; z< closestAgentCount; z++){
					offers[z] = ( (Agent_farmer*) closestAgents[z])->getCachedOffer();
				}

				for (uint i=0;i<wThreads.size();i++){
					delete wThreads[i];
				}
				wThreads.clear();

				for ( int z=0; z< closestAgentCount; z++){
					if (offers[z]>highestBid){
						secondHighestBid = highestBid;
						highestBid = offers[z];
						currentWinner = ( (Agent_farmer*) closestAgents[z]) ;
					} else if (bid > secondHighestBid) {
						secondHighestBid = offers[z];
					}
				}
				if (currentWinner){
					assignPlot(freePlots[j], currentWinner, secondHighestBid, PLOTS_RENTED);
					counterAssigned++;
					sumOfRentalExpendituresBySoilType[i] += secondHighestBid ;
					assignedPlotsBySoilType[i] ++ ;
					currentWinner->produce(); // 20071106 BUM!!! This is needed as she getShadowPrice() is called without first solving the matrix 
					break;
				}
				else {
					if(ii==1){
						msgOut(MSG_DEBUG,"Plot "+d2s( freePlots.at(j)->getID())+" ("+i2s( freePlots.at(j)->getX())+","+i2s( freePlots.at(j)->getY())+") unrented on the first tempt.. asking to bid to more farms..");
					}
					else {
						msgOut(MSG_DEBUG, "Plot "+d2s( freePlots.at(j)->getID())+" ("+i2s( freePlots.at(j)->getX())+","+i2s( freePlots.at(j)->getY())+")  definitively unrented.. gived up..");
					}
				}
			}
			if(loopCounter%100==0){
				msgOut(MSG_DEBUG, i2s(loopCounter)+" plots done...");
			}
			loopCounter ++;
			refreshGUI();
		}
		if(assignedPlotsBySoilType[i]>5) {
			avgPricesBySoilType[i] = sumOfRentalExpendituresBySoilType[i] / ((double)assignedPlotsBySoilType[i]);
			MTHREAD->RD->setBasicData("agrRentalPrices", avgPricesBySoilType[i], i);
			msgOut(MSG_INFO, "Assigned "+i2s(counterAssigned)+" plots of type "+agrLandTypesShortLabels.at(i));
			msgOut(MSG_INFO, "Avg. price: "+d2s(avgPricesBySoilType[i])+" euro/ha"  );
		}
		else {
			avgPricesBySoilType[i] = 0;
			msgOut(MSG_INFO, "Assigned "+i2s(counterAssigned)+" plots of type "+agrLandTypesShortLabels.at(i));
			msgOut(MSG_INFO, "Avg. price not updated as too few assignments to calculate an average.");
		}
	}

	MTHREAD->GIS->updateImage("farmRentedPlots");

	//for (int i=0; i < nThreads; i++){
	//	delete wThreads[i]; // deleting the threads
	//}

	//delete mutex;

	if(MTHREAD->RD->getBoolSetting("agentsView")){
		double haByPlot = MTHREAD->GIS->getHaByPixel();
		for(uint i=0;i<managedAgents.size();i++){
			int ownedPlots = ((Agent_space*) managedAgents.at(i)) -> getNPlots(PLOTS_OWNED);
			int rentedPlots = ( (Agent_space*) managedAgents.at(i)) -> getNPlots(PLOTS_RENTED);
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"owned land", d2s(( (double)ownedPlots)*haByPlot));
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"rented land", d2s(( (double)rentedPlots)*haByPlot));
		}
	}
}
/*
//old:
void
Manager_farmers::landAllocation(){


	vector <Pixel*> freePlots;
	int counterAssigned;
	int loopCounter;
	vector <int> agrLandTypes;
	vector <string> agrLandTypesShortLabels;
	int bidSize;
	double highestBid;
	double secondHighestBid;
	Agent_farmer* currentWinner;
	int freePlotsCount;
	double bid;
	vector <Agent_space* > closestAgents;
	int closestAgentCount;

	agrLandTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	vector <double> sumOfRentalExpendituresBySoilType (agrLandTypes.size(), ((double)0));
	vector <int>    assignedPlotsBySoilType (agrLandTypes.size(), 0);
	vector <double> avgPricesBySoilType (agrLandTypes.size(), ((double)0));
	
	agrLandTypesShortLabels = MTHREAD->RD->getStringVectorSetting("agrLandTypesShortLabels");
	bidSize = MTHREAD->RD->getIntSetting("askBidsArraySize");

	if(MTHREAD->RD->getBoolSetting("subRegionMode")){ // in case of a sub-region we take less bids, but at least 3
		bidSize = ((int)round(bidSize * MTHREAD->RD->getDoubleSetting("scaleCoeff")));
		bidSize = max(3, bidSize);
	}
	
	for (uint i=0;i< agrLandTypes.size(); i++){
		sumOfRentalExpendituresBySoilType[i] = 0;
		assignedPlotsBySoilType[i] = 0;

		freePlots =  MTHREAD->GIS->getAllPlotsByValue("landUse", agrLandTypes.at(i), true, MSG_DEBUG);
		counterAssigned=0;
		loopCounter=0;
		msgOut(MSG_INFO, "Renting plots of type "+i2s(agrLandTypes.at(i))+". "+i2s(freePlots.size())+" plots to assign (this can take a while)..");
		freePlotsCount = freePlots.size();
		for (int j=0;j<freePlotsCount;j++){
			highestBid           = 0;
			secondHighestBid     = 0;
			currentWinner = 0;


			for(uint ii=0;ii<2;ii++){ //we try two times before we end up declaring a plot unused..
				if(ii==0){ // first, normal attempt
					closestAgents = MTHREAD->GIS->getClosestAgents(freePlots[j], bidSize, name);
				}
				else { // we ask to bid to more farms...
					closestAgents = MTHREAD->GIS->getClosestAgents(freePlots[j], bidSize*3, this->getName());
				}
				closestAgentCount = closestAgents.size();
				for ( int z=0; z< closestAgentCount; z++){
					bid = ( (Agent_farmer*) closestAgents[z])->offerRentalPrice(freePlots[j]);
					if (bid>highestBid){
						secondHighestBid = highestBid;
						highestBid = bid;
						currentWinner = ( (Agent_farmer*) closestAgents[z]) ;
					} else if (bid > secondHighestBid) {
						secondHighestBid = bid;
					}
				}
				if (currentWinner){
					assignPlot(freePlots[j], currentWinner, secondHighestBid, PLOTS_RENTED);
					counterAssigned++;
					sumOfRentalExpendituresBySoilType[i] += secondHighestBid ;
					assignedPlotsBySoilType[i] ++ ;
					currentWinner->produce(); // 20071106 BUM!!! This is needed as she getShadowPrice() is called without first solving the matrix 
					break;
				}
				else {
					if(ii==1){
						msgOut(MSG_DEBUG,"Plot "+d2s( freePlots.at(j)->getID())+" ("+i2s( freePlots.at(j)->getX())+","+i2s( freePlots.at(j)->getY())+") unrented on the first tempt.. asking to bid to more farms..");
					}
					else {
						msgOut(MSG_DEBUG, "Plot "+d2s( freePlots.at(j)->getID())+" ("+i2s( freePlots.at(j)->getX())+","+i2s( freePlots.at(j)->getY())+")  definitively unrented.. gived up..");
					}
				}
			}
			if(loopCounter%100==0){
				msgOut(MSG_DEBUG, i2s(loopCounter)+" plots done...");
			}
			loopCounter ++;
			refreshGUI();
		}
		if(assignedPlotsBySoilType[i]>5) {
			avgPricesBySoilType[i] = sumOfRentalExpendituresBySoilType[i] / ((double)assignedPlotsBySoilType[i]);
			MTHREAD->RD->setBasicData("agrRentalPrices", avgPricesBySoilType[i], i);
			msgOut(MSG_INFO, "Assigned "+i2s(counterAssigned)+" plots of type "+agrLandTypesShortLabels.at(i));
			msgOut(MSG_INFO, "Avg. price: "+d2s(avgPricesBySoilType[i])+" euro/ha"  );
		}
		else {
			avgPricesBySoilType[i] = 0;
			msgOut(MSG_INFO, "Assigned "+i2s(counterAssigned)+" plots of type "+agrLandTypesShortLabels.at(i));
			msgOut(MSG_INFO, "Avg. price not updated as too few assignments to calculate an average.");
		}
	}

	MTHREAD->GIS->updateImage("farmRentedPlots");

	if(MTHREAD->RD->getBoolSetting("agentsView")){
		double haByPlot = MTHREAD->GIS->getHaByPixel();
		for(uint i=0;i<managedAgents.size();i++){
			int ownedPlots = ((Agent_space*) managedAgents.at(i)) -> getNPlots(PLOTS_OWNED);
			int rentedPlots = ( (Agent_space*) managedAgents.at(i)) -> getNPlots(PLOTS_RENTED);
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"owned land", d2s(( (double)ownedPlots)*haByPlot));
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"rented land", d2s(( (double)rentedPlots)*haByPlot));
		}
	}
}
*/


/**
On the first simulation year we calibrate the rental value of each owned plots as a coefficient in comparision of the average rental price.
We need rental value shadow price to value the shadow price of owned land for farmers that may wish to get rid of farming activities but it is too computationally expensive to calculate it each year so we do it only the firt year and then we base on the avg rental costs*this coefficient.
*/
void
Manager_farmers::calibrateLandValues(){
	msgOut(MSG_INFO, "Calibrating shadow prices of owned plots (this can take a while)..");
	vector <int> agrLandTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	vector <double> rentalPrices = MTHREAD->RD->getDoubleVectorSetting("agrRentalPrices");
	vector<Pixel*> ownedPlots;

	for (uint i=0;i<managedAgents.size();i++){
		Agent_farmer * currentAgent = ((Agent_farmer*) managedAgents.at(i));
		vector<Pixel*> agentOwnedPlots = currentAgent->getPlots(PLOTS_OWNED);
		copy (agentOwnedPlots.begin(), agentOwnedPlots.end(), back_inserter(ownedPlots));
	}

	msgOut(MSG_INFO, i2s(ownedPlots.size())+" plots to give estimate value..");
	for(uint i=0;i<ownedPlots.size();i++){
		double shPrice = quotePlot(ownedPlots[i]);
		int type= ((int)ownedPlots[i]->getDoubleValue("landUse"));
		double avgRentalPrice = 0;
		for (uint j=0;j<agrLandTypes.size();j++){
			if (type == agrLandTypes[j]){
				avgRentalPrice = rentalPrices.at(j);
				break;
			}
		}
		if(avgRentalPrice>0){
			ownedPlots[i]->setShRentalPriceCoeff(shPrice/avgRentalPrice);
		}
		else {
			ownedPlots[i]->setShRentalPriceCoeff(1);
		}
		if(i%100==0){
			msgOut(MSG_DEBUG, i2s(i)+" plots done...");
			refreshGUI();
		}
	}
	msgOut(MSG_INFO, "Done: estimated "+i2s(ownedPlots.size())+" plots.");
	
}


double
Manager_farmers::quotePlot(Pixel* plot){
	double highestBid           = 0;
	double secondHighestBid     = 0;
	double bid                  = 0;
	int bidSize                 = MTHREAD->RD->getIntSetting("askBidsArraySize");

	if(MTHREAD->RD->getBoolSetting("subRegionMode")){ // in case of a sub-region we take less bids, but at least 3
		bidSize = ((int)round(bidSize * MTHREAD->RD->getDoubleSetting("scaleCoeff")));
		bidSize = max(3, bidSize);
	}

	Agent_space* owner = plot->getOwner();
	vector <Agent_space*> closestAgents;
	Agent_farmer* currentWinner=NULL;


	closestAgents = MTHREAD->GIS->getClosestAgents(plot, bidSize, name);
	// remove the plot owner from the array..
	if(owner){
		vector <Agent_space*>::iterator p;
		for ( p = closestAgents.begin() ; p != closestAgents.end();){
			if (*p == owner){
				//delete *p .. don't use while() as in the book, otherwise the deletion of the pointer within the cycle will make a SIGFAUL
				closestAgents.erase(p);
			}
			else {
				++p;
			}
		}
	}
	int closestAgentCount = closestAgents.size();
	for ( int z=0; z< closestAgentCount; z++){
		bid = ( (Agent_farmer*) closestAgents[z])->offerRentalPrice(plot);
		if (bid>highestBid){
			secondHighestBid = highestBid;
			highestBid = bid;
			currentWinner = ( (Agent_farmer*) closestAgents[z]) ;
		}
	}
	if (currentWinner){
		return secondHighestBid;
	}
	else {
			return 0;
	}	
	return 0;
}

void
Manager_farmers::act(){
	// DONE: say the MIP to add activity only if enought rental years.. no not needed any longer.. it is the opposite.. if the farmer do permanent crops, the rental contract is posponed
	msgOut(MSG_INFO, "Period activity has started...");
	double debugLandUsed=0;
	MTHREAD->GIS->resetLayer("farmActivities"); // this is needed as otherwise we'll never report abandoned land, as we have the value on the plot of the previous years
	for (uint i=0;i<managedAgents.size();i++){
		Agent_farmer * currentAgent = ((Agent_farmer*) managedAgents.at(i));
		//int debugID = currentAgent->getAgentUniqueID();
		currentAgent->produce(); // just OPT.solve() ;-)))
		currentAgent->collectProductionEffects(); // add profit and subtract investment expenses and withdraw to liquidity. Plus add investment objects
		currentAgent->withdraw(); // calculate the money withdrawn from the bank for living
		debugLandUsed += currentAgent->debugLandUse;
	}
	MTHREAD->GIS->updateImage("farmActivities");
}

void
Manager_farmers::update(){
	msgOut(MSG_INFO, "Updating agents...");
	for (uint i=0;i<managedAgents.size();i++){
		Agent_farmer * currentAgent = ((Agent_farmer*) managedAgents.at(i));
		currentAgent->update();
	}
	MTHREAD->treeViewerManagerPropertyChangeValue(getName(),"managed agents",i2s(managedAgents.size()));
	if(MTHREAD->RD->getBoolSetting("agentsView")){
		for(uint i=0;i<managedAgents.size();i++){
			double liquidity = ((Agent_space*) managedAgents.at(i)) -> getLiquidity();
			double capital   = ( (Agent_space*) managedAgents.at(i)) -> getCapital(true);
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"liquidity", d2s(liquidity));
			MTHREAD->treeViewerAgentPropertyChangeValue(managedAgents.at(i)->getAgentUniqueID(),"capital", d2s(capital));
		}
	}
	//MTHREAD->GIS->resetLayer("farmActivities"); //20071202: commented so that we can check in the pixArea the meaning of the colors. TO DO: check that this do not lead to bugs.. I thing so, but we can move it on a different place, so that the period that there is no data is minimised.. IMPORTANT !!!! DONE The problem was that otherwise we never have abandoned land, as we have the production of previous years
}

void
Manager_farmers::planNext(){
	msgOut(MSG_INFO, "Planning for the next period has started...");
	for (uint i=0;i<managedAgents.size();i++){
		Agent_farmer * currentAgent = ((Agent_farmer*) managedAgents.at(i));
		int leaveActivity = currentAgent->leaveActivity();
		if (leaveActivity){
			MTHREAD->DO->printLeavingAgent(currentAgent, leaveActivity);
			removeFromModel(currentAgent);
		}
	}

	MTHREAD->treeViewerManagerPropertyChangeValue(getName(),"managed agents",i2s(managedAgents.size()));
}

void
Manager_farmers::assignPlot(Pixel* plot_h, Agent_space* agent_h, double rentalCosts, int assignmentType){

	if (assignmentType ==PLOTS_OWNED){
		if( MTHREAD->INIT->getInitState() > 4){ // we are no longer in the init stage
			msgOut(MSG_CRITICAL_ERROR, "Buying/selling plots is not yet implemented so here is one of that places where the program should has never gonna. Check what went wrong! Aborting..."); 
		}
		plot_h->changeValue("farmOwnedPlots", agent_h->getAgentLocalID());
		agent_h->addPlotToOwnedList(plot_h);
		plot_h->setOwner(agent_h);
	}
	else if (assignmentType==PLOTS_RENTED){

		plot_h->changeValue("farmRentedPlots", agent_h->getAgentLocalID());
		plot_h->setRandomRenting("agrContract"); // DONE: if we are in the init the random renting must be randomized even at a remaining level before the minimu contracts, otherwise the first X years no contracts ends !!!!
		plot_h->setRentalCosts(rentalCosts);

		if( MTHREAD->INIT->getInitState() > 4){ // we are no longer in the init stage
			agent_h->addPlotToMIP(plot_h);
			agent_h->pay(rentalCosts);
			agent_h->addToSunkCostsPaid(rentalCosts);
		}
		agent_h->addPlotToRentedList(plot_h); // don't invert !!! this MUST go after the adding to the MIP
		plot_h->setTenant(agent_h);
		
	}
	else {
		msgOut(MSG_ERROR, "Function assignPlot() called with an unknow type");
	}
}

void
Manager_farmers::removeFromModel(Agent_farmer* agent_h){
	agent_h->freeLand();
	vector<Agent_base*>::iterator iterator;
	int nPolicyPremiums = MTHREAD->RD->getResourceSize(RESTYPE_POLICYPREMIUM);
 
	for ( iterator = managedAgents.begin() ; iterator !=managedAgents.end();){
		if(*iterator == agent_h){
			// if we keep also the dead agents (and in particulat their OPT matrix) the memory explode. We are forced to delete them.
			
			Pixel* farmsteadPx = agent_h->getHomePlot();
			vector <Pixel*> ownedPlots = agent_h->getPlots(PLOTS_OWNED);
			vector <Pixel*> rentedPlots = agent_h->getPlots(PLOTS_RENTED);
			double noValue = MTHREAD->GIS->getNoValue();

			// deleting farmseads...
			farmsteadPx->changeValue("farmsteads",noValue);
			// assigning owned plots to collectorAgent...
			for (uint i=0;i<ownedPlots.size();i++){
				ownedPlots[i]->setOwner((( Agent_space*) collectorAgent));
				//int debug =  collectorAgent->getAgentUniqueID();
				ownedPlots[i]->changeValue("farmOwnedPlots",collectorAgent->getAgentUniqueID() );
			}
			// free the rented land...
			for (uint i=0;i<rentedPlots.size();i++){
				rentedPlots[i]->destroyRentalContract();
				rentedPlots[i]->changeValue("farmRentedPlots",noValue);
			}
			
			vector<RegActivities*> actMoulds = MTHREAD->RD->getRegActivities();
			
			for (uint j=0;j<actMoulds.size();j++){
				for(uint y=0;y<nPolicyPremiums;y++){
					if(actMoulds[j]->getDecouplingOption(y) > 0){ // this activity is not in registration mode...
						sumOfRightsOfDeadAgents.at(j).at(y) += actMoulds[j]->getDecouplingOption(y) * agent_h->getDecRightsByAct(j,y);
						sumOfHaOfDeadAgents.at(j).at(y) += agent_h->getDecHaByActMould(j,y);
					}
				}
			}


			delete agent_h;
			//removedAgents.push_back(agent_h);

			managedAgents.erase(iterator);
		}
		else {
			++iterator;
		}
	}
}

/**
Return the regional rights for decoupling by (optional) activity
Changed 2008.02.07: do not return the plenty rights, but those already filtered by the share of rights that are returned by each activity (e.g. 0.93)
@param actMouldCounter counter of the activity mould in the regional vector. Default to -1, means to all the moulds
*/

double
Manager_farmers::getAverageRightsByAct(int actMouldCounter){
	double sumOfRights=0;
	double sumOfHa=0;
	vector<RegActivities*> actMoulds = MTHREAD->RD->getRegActivities();
	vector <string> policyPremiums = MTHREAD->RD->getResourceNames(RESTYPE_POLICYPREMIUM);
	int nPolicyPremiums = policyPremiums.size();

	//TO.DO DONE: include also the dead agents.. this means to keep them and destroy only their OPT object instead of destroy them..
	//TODO: make cashed vaues to speed up this calculatio (it's the same for each agent!)
	// 2008.09.21, bug corrected.. as the average premium is then multiplied to all hectars, also the "average" should refer to the whole hecterage..  
	if(actMouldCounter != -1){// we are asked for a specific activity
		for(uint y=0;y< nPolicyPremiums;y++){
			if(actMoulds.at(actMouldCounter)->getDecouplingOption(y) >= 0){ //this activity, for this specific premium, is not in "registration rights" mode...
				for (uint i=0;i<managedAgents.size();i++){
					sumOfRights += actMoulds.at(actMouldCounter)->getDecouplingOption(y) *
					               ((Agent_farmer*)managedAgents[i])->getDecRightsByAct(actMouldCounter,y);
					sumOfHa += ((Agent_farmer*)managedAgents[i])->getDecHaByActMould(actMouldCounter,y);
				}
				sumOfRights += sumOfRightsOfDeadAgents.at(actMouldCounter).at(y);
				sumOfHa += sumOfHaOfDeadAgents.at(actMouldCounter).at(y);
			}
			else {
				for (uint i=0;i<managedAgents.size();i++){
					sumOfHa += ((Agent_farmer*)managedAgents[i])->getDecHaByActMould(actMouldCounter,y);
				}
				sumOfHa += sumOfHaOfDeadAgents.at(actMouldCounter).at(y);
			}
		}
	} else {
		for (uint j=0;j<actMoulds.size();j++){
			for (uint y=0;y< nPolicyPremiums;y++){
				if(actMoulds[j]->getDecouplingOption(y) >= 0){ // this activity is not in registration mode...
					for (uint i=0;i<managedAgents.size();i++){
						sumOfRights += actMoulds[j]->getDecouplingOption(y) * ((Agent_farmer*)managedAgents[i])->getDecRightsByAct(j,y);
						sumOfHa += ((Agent_farmer*)managedAgents[i])->getDecHaByActMould(j,y);
					}
					sumOfRights += sumOfRightsOfDeadAgents.at(j).at(y);
					sumOfHa += sumOfHaOfDeadAgents.at(j).at(y);
				}
				else {
					for (uint i=0;i<managedAgents.size();i++){
						sumOfHa += ((Agent_farmer*)managedAgents[i])->getDecHaByActMould(j,y);
					}
					sumOfHa += sumOfHaOfDeadAgents.at(j).at(y);
				}
			}
		}
	}

	if(sumOfHa>0){
		return sumOfRights/sumOfHa;
	} else {
		return 0;
	}
	return 0;
	
}

// ############################ CLASS  Manager_farmers_threads    ####################


Manager_farmers_threads::Manager_farmers_threads(){

}

void
Manager_farmers_threads::run(){

	//mutex->lock();
	//cout << agent->getAgentUniqueID()<< endl;

	//double randChange =  (0+( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(10-0+1))/ (double)100; //rand() must be not thread safe !!!!

	//int justn = 10000;
	//vector <double> takeTimeVector (justn, 0);
	//for (int i =0; i< justn;i++){
	//	takeTimeVector.at(i)=i*2;
	//}
	
		( (Agent_farmer*) agent)->cachedOffer = ( (Agent_farmer*) agent)->offerRentalPrice(plot);
	//( (Agent_farmer*) agent)->cachedOffer = ( (Agent_farmer*) agent)->getAgentUniqueID();
	//mutex->unlock();
}

void
Manager_farmers_threads::assignJob(Agent_space* agent_h, const Pixel* plot_h){
	agent = agent_h;
	plot = plot_h;
	//mutex = mutex_h;
	( (Agent_farmer*) agent) ->cachedOffer = 0;
}


