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
#include "Output.h"
#include "Agent_base.h"
#include "Agent_space.h"
#include "SuperAgentManager.h"
#include "ThreadManager.h"
#include "Scheduler.h"
#include "Manager_base.h"
//#include "RegData.h"

Output::Output(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
}

Output::~Output(){
}

// ---- functions ... ----------------------

void
Output::collectData(){
	if(MTHREAD->RD->getIntSetting("outputLevel")<=OUTVL_NONE) return;
	vector <Agent_base*> agents          = MTHREAD-> SAM ->getAgentsByType(false);
	vector<int>          agrSoilTypes    = MTHREAD-> RD  ->getIntVectorSetting("agrLandTypes");
	vector<Pixel*>       agrPixels       = MTHREAD-> GIS ->getAllPlotsByValue("landUse", agrSoilTypes);
	double               noValue         = MTHREAD-> GIS ->getNoValue();
	int                  idleCode        = MTHREAD-> RD  ->getActGisCodeByName("idleLand");

	// COLLECTING AGENT BASED DATA...
	for (uint i=0;i<agents.size();i++){
		AgentSurvey SURVEY;
		agents.at(i)->answerStats(SURVEY);
		surveys.push_back(SURVEY);
	}
	
	// COLLECTING LAND BASED DATA...
	abandonedAgrLandByType.clear();
	abandonedAgrLandByType.resize(agrSoilTypes.size(),0);
	unpossessedAgrLandByType.clear();
	unpossessedAgrLandByType.resize(agrSoilTypes.size(),0);
	idleAgrLandByType.clear();
	idleAgrLandByType.resize(agrSoilTypes.size(),0);
	MTHREAD->GIS->resetLayer("agrUsage");

	for(uint i=0;i<agrPixels.size();i++){
		int pxActValue = ((int)agrPixels[i]->getDoubleValue("farmActivities"));
		int pxLandType = ((int)agrPixels[i]->getDoubleValue("landUse"));
		for(uint j=0;j<agrSoilTypes.size();j++){
			if(pxLandType == agrSoilTypes[j]){
				if(pxActValue == noValue){
					abandonedAgrLandByType.at(j)++;
				}
				if(pxActValue == idleCode){
					idleAgrLandByType.at(j)++;
				}
				if(!agrPixels[i]->getTenant()) {
					if (!agrPixels[i]->getOwner() || agrPixels[i]->getOwner()->getAgentUniqueID() <0){
						unpossessedAgrLandByType.at(j)++;
					}
				}

				break;
			}
		}
		// updating agrUsage layer..
		if (pxActValue==noValue){
			agrPixels[i]->changeValue("agrUsage",0);
		} else if (pxActValue==idleCode) {
			agrPixels[i]->changeValue("agrUsage",1);
		} else {
			agrPixels[i]->changeValue("agrUsage",2);
		}
	}
	MTHREAD->GIS->updateImage("agrUsage");
}

void
Output::initOutput(){
	initOutputAggregatedData();
	initOutputMaps();
	initOutputDetailedData();
	initOutputLeavingAgents();
}

void
Output::initOutputAggregatedData(){
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_AGGREGATED) return;


	char                 d               = getOutputFieldDelimiter();
	vector <string>      agentCategories = MTHREAD->SAM->getAgentTypeNames();
	vector <Agent_base*> agents          = MTHREAD->SAM->getAgentsByType(false);
	vector<int> landTypes;
	if(!MTHREAD->RD->getBoolSetting("compactOutput")){
		landTypes = MTHREAD->RD->getIntVectorSetting("landTypes");
	} else {
		landTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	}
	vector<string>       objNames        = MTHREAD->RD->getObjectNames();
	vector<string>       actNames        = MTHREAD->RD->getActivityNames();
	vector<int>          agrSoilTypes    = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	vector<string>       agrSoilLabels   = MTHREAD->RD->getStringVectorSetting("agrLandTypesShortLabels");

	vector< vector<int> > mouldsByCategory;
	vector <Manager_base*> listOfManagers = MTHREAD->SAM->getManagers();
	for(uint i=0;i<listOfManagers.size();i++){
		vector<int> localMoulds = listOfManagers[i]->getMouldIDs();
		mouldsByCategory.push_back(localMoulds);
	}

	string outFileName;


	if(MTHREAD->RD->getBoolSetting("outputAgentAggregatedHumanReadable")) {	
		outFileName = MTHREAD->RD->getBaseDirectory() 
		              + MTHREAD->RD->getOutputDirectory()
		              + "results/aggregatedOutput_"
		              + MTHREAD->getScenarioName()
		              + MTHREAD->RD->getStringSetting("outputFileExtension");
		ofstream out(outFileName.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
		out << "YEAR"                << d << "\n";
		out << "AGENT COUNT"         << d << "\n";
		for (uint i=0;i<agentCategories.size();i++){
			out << " - " << agentCategories.at(i)  <<" TOTAL"<< d << "\n";
			for (uint j=0;j<mouldsByCategory.at(i).size();j++){
				out << "  -- mould " << mouldsByCategory.at(i).at(j) << d << "\n";
			}
		}
		out << "avg. capital"              << d << "\n"
			<< "avg. landCapital"          << d << "\n"
			<< "avg. objectsCapital"       << d << "\n"
			<< "avg. ownLabour"            << d << "\n"
			<< "avg. companyLabour"        << d << "\n"
			<< "avg. liquidity"            << d << "\n"
			<< "avg. netProductionProfit"  << d << "\n"
			<< "avg. coupledPremiums"      << d << "\n"
			<< "avg. decoupledPayment"     << d << "\n"
			<< "avg. lastSunkCostsPaid"    << d << "\n"
			<< "avg. companyProfit"        << d << "\n"
			<< "avg. companyProfitWSupport"<< d << "\n"
			<< "avg. incomes"              << d << "\n";

		out << "RENTAL PRICE BY (AGR) TYPE"         << d << "\n";
		for (uint i=0;i<agrSoilTypes.size();i++){
			out << " - " << agrSoilLabels.at(i)  << d << "\n";
		}
		out << "Precised abandoned land" <<d << "\n"; // obtained from unpossessed plots plus agent activities rather than going to see the values on the layer that is approximate as each plot has a value used/not used

		// abandoned plots are those that are not used in any production activities, even not in the "idle" mode. They can be even not be owned or rented by anybody.
		out << "ENTIRELY ABAND.ED PLOTS BY (AGR) TYPE"         << d << "\n";
		for (uint i=0;i<agrSoilTypes.size();i++){
			out << " - " << agrSoilLabels.at(i)  << d << "\n";
		}
		out << "IDLE PLOTS BY (AGR) TYPE"         << d << "\n";
		for (uint i=0;i<agrSoilTypes.size();i++){
			out << " - " << agrSoilLabels.at(i)  << d << "\n";
		}
		out << "TOTAL_PLOTS" << d << "\n";
		out << "OWNED PLOTS BY TYPE"  << d << "\n";
		for(uint i=0;i<landTypes.size();i++){
			out << " - " << landTypes[i]  << d << "\n";
		}
		out << "RENTED PLOTS BY TYPE"  << d << "\n";
		for(uint i=0;i<landTypes.size();i++){
			out << " - " << landTypes[i]  << d << "\n";
		}
		out << "AVAILABLE OBJECT BY TYPE"  << d << "\n";
		for(uint i=0;i<objNames.size();i++){
			out << " - " << objNames[i]  << d << "\n";
		}
		out << "NEWLY AV. OBJECT BY TYPE"  << d << "\n";
		for(uint i=0;i<objNames.size();i++){
			out << " - " << objNames[i]  << d << "\n";
		}
		out << "ACTIVITIES BY TYPE"  << d << "\n";
		for(uint i=0;i<actNames.size();i++){
			out << " - " << actNames[i]  << d << "\n";
		}
		out.close();	
	} else { // output is for database processing...
		outFileName = MTHREAD->RD->getBaseDirectory() 
		              + MTHREAD->RD->getOutputDirectory()
		              + "results/aggregatedOutput"
		              //+ MTHREAD->getScenarioName()
		              + MTHREAD->RD->getStringSetting("outputFileExtension");
		ifstream in(outFileName.c_str(), ios::in);
		if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
			in.close();
			cleanScenario(outFileName, MTHREAD->getScenarioName(), d);
			return;
		} else {
			in.close();
		}
		ofstream out(outFileName.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
		out << "SCENARIO" << d ;
		out << "YEAR"                << d ;
		out << "AGENT_COUNT"         << d ;
		for (uint i=0;i<agentCategories.size();i++){
			out << agentCategories.at(i)  <<"_total"<< d;
			for (uint j=0;j<mouldsByCategory.at(i).size();j++){
				out << agentCategories.at(i)  << "_mould_" << mouldsByCategory.at(i).at(j) << d;
			}
		}
		out << "avg_capital"                << d
			<< "avg_landCapital"            << d
			<< "avg_objectsCapital"         << d
			<< "avg. ownLabour"             << d
			<< "avg. companyLabour"         << d
			<< "avg_liquidity"              << d
			<< "avg_netProductionProfit"    << d
			<< "avg_coupledPremiums"        << d
			<< "avg_decoupledPayment"       << d
			<< "avg_lastSunkCostsPaid"      << d
			<< "avg. companyProfit"         << d
			<< "avg. companyProfitWSupport" << d
			<< "avg. incomes"               << d;


		// abandoned plots are those that are not used in any production activities, even not in the "idle" mode. They can be even not be owned or rented by anybody.
		out << "RENTAL_PRICE_BY_AGR_TYPE"         << d;
		for (uint i=0;i<agrSoilTypes.size();i++){
			out << "rPrice_" << agrSoilLabels.at(i)  << d;
		}
		out << "Precise abandoned land" <<d << "\n"; // obtained from unpossessed plots plus agent activities rather than going to see the values on the layer that is approximate as each plot has a value used/not used

		// abandoned plots are those that are not used in any production activities, even not in the "idle" mode. They can be even not be owned or rented by anybody.
		out << "ABANDONED_PLOTS_BY_AGR_TYPE"         << d;
		for (uint i=0;i<agrSoilTypes.size();i++){
			out << "abPlots_" << agrSoilLabels.at(i)  << d;
		}
		out << "IDLE_PLOTS_BY_AGR_TYPE"         << d;
		for (uint i=0;i<agrSoilTypes.size();i++){
			out << "idlePlots_" << agrSoilLabels.at(i)  << d;
		}
		out << "TOTAL_PLOTS" << d;
		out << "OWNED_PLOTS_BY_TYPE"  << d;
		for(uint i=0;i<landTypes.size();i++){
			out << "oPlots_" << landTypes[i]  << d;
		}
		out << "RENTED_PLOTS_BY_TYPE"  << d;
		for(uint i=0;i<landTypes.size();i++){
			out << "rPlots_" << landTypes[i]  << d;
		}
		out << "AVAILABLE_OBJECT_BY_TYPE"  << d;
		for(uint i=0;i<objNames.size();i++){
			out << "avObjs_" << objNames[i]  << d;
		}
		out << "NEWLY_AV_OBJECT_BY_TYPE"  << d;
		for(uint i=0;i<objNames.size();i++){
			out << "newObjs_" << objNames[i]  << d;
		}
		out << "ACTIVITIES_BY_TYPE"  << d;
		for(uint i=0;i<actNames.size();i++){
			out << "acts_" << actNames[i]  << d;
		}
		out << "\n";
		out.close();
	}





}
/**
Resetting the list of printed layers and the scenario name..
<br>Printing scenario name for post-processing scripts
*/
void
Output::initOutputMaps(){
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_MAPS) return;
	string mapBaseDirectory = MTHREAD->RD->getBaseDirectory()+MTHREAD->RD->getOutputDirectory()+"maps/";	
	string filenameToSaveScenarioName = mapBaseDirectory+"scenarioName";
	string filenameListIntLayers = mapBaseDirectory+"integerListLayers";
	string filenameListFloatLayers = mapBaseDirectory+"floatListLayers";

	// printing the scenario name in the "scenarioName file"...
	ofstream outSN(filenameToSaveScenarioName.c_str(), ios::out);
	if (!outSN){ msgOut(MSG_ERROR,"Error in opening the file "+filenameToSaveScenarioName+".");}
	outSN << MTHREAD->getScenarioName() << "\n";
	outSN.close();
	// cleaning the "integerListLayers" and "floatListLayers" file...
	ofstream outi(filenameListIntLayers.c_str(), ios::out);
	outi.close();
	ofstream outf(filenameListFloatLayers.c_str(), ios::out);
	outf.close();

	// init a new layers: agrUsage, overall agricultural usage, to keep track of abandoned and idle plots
	MTHREAD->GIS->addLayer("agrUsage","Overall agricultural usage",true, true);
	MTHREAD->GIS->addLegendItem("agrUsage", 0, "Abandoned plots", 255, 0, 0, ((double)0), ((double)0));
	MTHREAD->GIS->addLegendItem("agrUsage", 1, "Idle plots", 120, 120, 0, ((double)0), ((double)0));
	MTHREAD->GIS->addLegendItem("agrUsage", 2, "Normal agr. usage", 0, 255, 0, ((double)0), ((double)0));
	
}
void
Output::initOutputDetailedData(){
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_DETAILED) return;

	vector <Agent_base*> agents = MTHREAD->SAM->getAgentsByType(false);
	char d = getOutputFieldDelimiter();
	vector<int> landTypes;
	if(!MTHREAD->RD->getBoolSetting("compactOutput")){
		landTypes = MTHREAD->RD->getIntVectorSetting("landTypes");
	} else {
		landTypes = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	}
	vector<string> objNames = MTHREAD->RD->getObjectNames();
	vector<string> actNames = MTHREAD->RD->getActivityNames();

	if(MTHREAD->RD->getBoolSetting("outputAgentDetailsHumanReadable")){
		for (uint i=0;i<agents.size();i++){
			string agName= i2s(agents[i]->getAgentUniqueID());
			string outFileName =     MTHREAD->RD->getBaseDirectory() 
			                       + MTHREAD->RD->getOutputDirectory()
			                       + "results/ag_"
			                       + agName
			                       + "_"
			                       + MTHREAD->getScenarioName()
			                       + MTHREAD->RD->getStringSetting("outputFileExtension");
			ofstream out(outFileName.c_str(), ios::out);
			if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+".");}
			out << "RECORDID"             << d << "\n"
			    << "YEAR"                 << d << "\n"
			    << "agentType"            << d << "\n"
			    << "agentID"              << d << "\n"
			    << "agentMouldID"         << d << "\n"
			    << "spatial"              << d << "\n"
			    << "hostPlotID"           << d << "\n"
			    << "hostPlotX"            << d << "\n"
			    << "hostPlotY"            << d << "\n"
			    << "capital"              << d << "\n"
			    << "landCapital"          << d << "\n"
			    << "objectsCapital"       << d << "\n"
			    << "ownLabour"            << d << "\n" // eg farm family labour
			    << "companyLabour"        << d << "\n" // e.g. farm labour (+aquired labour, -off-farm labour)
			    << "liquidity"            << d << "\n"
			    << "netProductionProfit"  << d << "\n"
			    << "coupledPremiums"      << d << "\n"
			    << "decoupledPayment"     << d << "\n"
			    << "lastSunkCostsPaid"    << d << "\n"
			    << "companyProfit"        << d << "\n" // farm profit
			    << "companyProfitWSupport"<< d << "\n" // farm profit with CAP
			    << "incomes"              << d << "\n";
			out << "TOTAL PLOTS"  << d << "\n";
			out << "OWNED PLOTS BY TYPE"  << d << "\n";
			for(uint i=0;i<landTypes.size();i++){
				out << " - " << landTypes[i]  << d << "\n";
			}
			out << "RENTED PLOTS BY TYPE"  << d << "\n";
			for(uint i=0;i<landTypes.size();i++){
				out << " - " << landTypes[i]  << d << "\n";
			}
			out << "OWNED OBJECT BY TYPE"  << d << "\n";
			for(uint i=0;i<objNames.size();i++){
				out << " - " << objNames[i]  << d << "\n";
			}
			out << "AVAILABLE OBJECT BY TYPE"  << d << "\n";
			for(uint i=0;i<objNames.size();i++){
				out << " - " << objNames[i]  << d << "\n";
			}
			out << "NEWLY AV. OBJECT BY TYPE"  << d << "\n";
			for(uint i=0;i<objNames.size();i++){
				out << " - " << objNames[i]  << d << "\n";
			}
			out << "ACTIVITIES BY TYPE"  << d << "\n";
			for(uint i=0;i<actNames.size();i++){
				out << " - " << actNames[i]  << d << "\n";
			}
			out << "UNUSED O+R LAND"  << d << "\n";
		    out.close();
		}
	} else { // output is on one big file....
		string outFileName =     MTHREAD->RD->getBaseDirectory() 
			                       + MTHREAD->RD->getOutputDirectory()
			                       + "results/agentsDetails"
			                       //+ MTHREAD->getScenarioName()
			                       + MTHREAD->RD->getStringSetting("outputFileExtension");
		ifstream in(outFileName.c_str(), ios::in);
		if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
			in.close();
			cleanScenario(outFileName, MTHREAD->getScenarioName(), d);
			return;
		} else {
			in.close();
		}
		ofstream out(outFileName.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+".");}
		out << "SCENARIO"             << d ;
		out << "RECORDID"             << d ;
		out << "YEAR"                 << d
		    << "agentType"            << d
		    << "agentID"              << d
		    << "agentMouldID"         << d
		    << "spatial"              << d
		    << "hostPlotID"           << d
		    << "hostPlotX"            << d
		    << "hostPlotY"            << d
		    << "capital"              << d
		    << "landCapital"          << d
		    << "objectsCapital"       << d
		    << "ownLabour"            << d
		    << "companyLabour"        << d
		    << "liquidity"            << d
		    << "netProductionProfit"  << d
		    << "coupledPremiums"      << d
		    << "decoupledPayment"     << d
		    << "lastSunkCostsPaid"    << d
		    << "companyProfit"        << d
		    << "companyProfitWSupport"<< d
		    << "incomes"              << d;

		out << "TPLOTS"  << d  ;
		out << "OPLOTS"  << d  ;
		for(uint i=0;i<landTypes.size();i++){
			out << "oPlots_" << landTypes[i]  << d ;
		}
		out << "RPLOTS"  << d ;
		for(uint i=0;i<landTypes.size();i++){
			out << "rPlots_" << landTypes[i]  << d ;
		}
		out << "OOBJS"  << d ;
		for(uint i=0;i<objNames.size();i++){
			out << "oObjs_" << objNames[i]  << d ;
		}
		out << "AVOBJS"  << d ;
		for(uint i=0;i<objNames.size();i++){
			out << "avObjs_" << objNames[i]  << d ;
		}
		out << "NEWOBJS"  << d ;
		for(uint i=0;i<objNames.size();i++){
			out << "newObjs_" << objNames[i]  << d ;
		}
		out << "ACTS"  << d ;
		for(uint i=0;i<actNames.size();i++){
			out << "acts_" << actNames[i]  << d ;
		}
		out << "UNUSED O+R LAND"  << d ;
		out << "\n" ;
		out.close();
	}
}

void
Output::initOutputLeavingAgents(){
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_DETAILED) return;

	string outFileName =     MTHREAD->RD->getBaseDirectory() 
	                       + MTHREAD->RD->getOutputDirectory()
	                       + "results/leavingAgents"
	                       //+ "_"
	                       //+ MTHREAD->getScenarioName()
	                       + MTHREAD->RD->getStringSetting("outputFileExtension");

	char d = getOutputFieldDelimiter();

	ifstream in(outFileName.c_str(), ios::in);
	if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
		in.close();
		cleanScenario(outFileName, MTHREAD->getScenarioName(), d);
		return;
	} else {
		in.close();
	}



	ofstream out(outFileName.c_str(), ios::out);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+".");}

	out << "SCENARIO"  << d ;
	out << "YEAR"      << d << "ID"         << d << "TYPE"       << d
	    << "MOULD"     << d << "MOTIVATION" << d << "TotPLOTS"   << d << "OwPLOTS"    << d << "RePLOTS"     << d << "CAP" << d
	    << "LIQ"       << d << "MAXDEB"     << d << "PRODPROFIT" << d << "COUPPREMIUMS" << d << "DECPAYMENT" << d
	    << "SUNKCOSTS" << d << "LIQSHPR"    << d << "LABSHPR"    << d
	    << "LANDSHPR"  << d << "REALPROFIT" << d << "TOTSHPR"    << d << "\n";

	out.close();
}

void
Output::print(){

	printAggregatedData();
	printMaps();
	printDetailedData();
	printDebugMatrices();

	//cleaning-up...
	surveys.clear();
}

void
Output::printAggregatedData(){
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_AGGREGATED) return;

	string inOutFileName;

	char                 d               = getOutputFieldDelimiter();
	vector <int>         agentCounts     = MTHREAD->SAM->getAgentCountByType();
	vector <Agent_base*> agents          = MTHREAD->SAM->getAgentsByType(false);
	vector<int>          landTypes       = MTHREAD->RD->getIntVectorSetting("landTypes");
	vector<string>       objNames        = MTHREAD->RD->getObjectNames();
	vector<string>       actNames        = MTHREAD->RD->getActivityNames();
	vector<int>          agrSoilTypes    = MTHREAD->RD->getIntVectorSetting("agrLandTypes");
	vector<double>       agrRPrices      = MTHREAD->RD->getDoubleVectorSetting("agrRentalPrices");
	vector <Manager_base*> listOfManagers = MTHREAD->SAM->getManagers();


	string out;
	vector<string> data; // vector of the newly added data
	int agSize = agents.size();

	double          SUMcapital=0;
	double          SUMlandCapital=0;
	double          SUMobjectsCapital=0;
	double          SUMownLabour=0;
	double          SUMcompanyLabour=0;
	double          SUMliquidity=0;
	double          SUMnetProductionProfit=0;
	double          SUMcoupledPremiums=0;
	double          SUMdecoupledPayment=0;
	double          SUMlastSunkCostsPaid=0;
	double          SUMcompanyProfit=0;
	double          SUMcompanyProfitWSupport=0;
	double          SUMincomes=0;
	double          SUMsurveyUnusedLand=0;

	vector <int>    SUMownedPlotsByAllTypes        (landTypes.size(), 0);
	vector <int>    SUMrentedPlotsByAllTypes       (landTypes.size(), 0);
	vector <int>    SUMavailableObjectsByType      (objNames.size(),  0);
	vector <int>    SUMnewlyAvailableObjectsByType (objNames.size(),  0);
	vector <double> SUMactivitiesByType            (actNames.size(),  0);
	vector < vector <int> >    SUMagentsByTypeAndMould;

	for(uint i=0;i<listOfManagers.size();i++){
		int nMoulds = listOfManagers[i]->getMouldLocalIDCounter();
		vector <int> moulds (nMoulds,0);
		SUMagentsByTypeAndMould.push_back(moulds);
	}

	

	for(uint i=0;i<surveys.size();i++){
		SUMcapital              += surveys[i].capital;
		SUMlandCapital          += surveys[i].landCapital;
		SUMobjectsCapital       += surveys[i].objectsCapital;
		SUMownLabour            += surveys[i].labour;
		SUMcompanyLabour        += surveys[i].labour + getSurveyActivityByName(surveys[i], "hiredLabour") - getSurveyActivityByName(surveys[i], "offFarmLabour");
		SUMliquidity            += surveys[i].liquidity;
		SUMnetProductionProfit  += surveys[i].netProductionProfit;
		SUMcoupledPremiums      += surveys[i].coupledPremiums;
		SUMdecoupledPayment     += surveys[i].decoupledPayment;
		SUMlastSunkCostsPaid    += surveys[i].lastSunkCostsPaid;
		SUMcompanyProfit        += surveys[i].netProductionProfit
		                         - getSurveyActivityByName(surveys[i], "offFarmLabour")* MTHREAD->RD->getDoubleSetting("labourShadowPrice", DATA_NOW)
		                         - surveys[i].lastSunkCostsPaid;
		SUMcompanyProfitWSupport += surveys[i].netProductionProfit
		                          - getSurveyActivityByName(surveys[i], "offFarmLabour")* MTHREAD->RD->getDoubleSetting("labourShadowPrice", DATA_NOW)
		                          - surveys[i].lastSunkCostsPaid
		                          + surveys[i].coupledPremiums
		                          + surveys[i].decoupledPayment;
		SUMincomes               += surveys[i].netProductionProfit
		                          + surveys[i].coupledPremiums
		                          + surveys[i].decoupledPayment
		                          - surveys[i].lastSunkCostsPaid;


		for(uint j=0;j<landTypes.size();j++){
			SUMownedPlotsByAllTypes.at(j) += surveys[i].ownedPlotsByAllTypes.at(j);
			SUMrentedPlotsByAllTypes.at(j) += surveys[i].rentedPlotsByAllTypes.at(j);
		}
		for(uint j=0;j<objNames.size();j++){
			SUMavailableObjectsByType.at(j) += surveys[i].availableObjectsByType.at(j);
			SUMnewlyAvailableObjectsByType.at(j) += surveys[i].newlyAvailableObjectsByType.at(j);
		}
		for(uint j=0;j<actNames.size();j++){
			SUMactivitiesByType.at(j) += surveys[i].activitiesByType.at(j);
		}
		for(uint j=0;j<listOfManagers.size();j++){
			int managerID =listOfManagers[j]->getManagerID();
			if(surveys[i].managerID == managerID){
				SUMagentsByTypeAndMould.at(j).at(surveys[i].agentMouldID)++;
				break;
			}
		}
		SUMsurveyUnusedLand += getSurveyUnusedLand(surveys[i]); // unused land
	}
	
	data.push_back(i2s(MTHREAD->SCD->getYear()));
	data.push_back("");
	for (uint i=0;i<agentCounts.size();i++){
		data.push_back(i2s(agentCounts.at(i)));
		for(uint j=0;j<SUMagentsByTypeAndMould.at(i).size();j++){
			data.push_back(i2s(SUMagentsByTypeAndMould.at(i).at(j)));
		}
	}
	data.push_back(d2s(SUMcapital/agSize));
	data.push_back(d2s(SUMlandCapital/agSize));
	data.push_back(d2s(SUMobjectsCapital/agSize));
	data.push_back(d2s(SUMownLabour/agSize));
	data.push_back(d2s(SUMcompanyLabour/agSize));
	data.push_back(d2s(SUMliquidity/agSize));
	data.push_back(d2s(SUMnetProductionProfit/agSize));
	data.push_back(d2s(SUMcoupledPremiums/agSize));
	data.push_back(d2s(SUMdecoupledPayment/agSize));
	data.push_back(d2s(SUMlastSunkCostsPaid/agSize));
	data.push_back(d2s(SUMcompanyProfit/agSize));
	data.push_back(d2s(SUMcompanyProfitWSupport/agSize));
	data.push_back(d2s(SUMincomes/agSize));

	data.push_back("");
	for (uint i=0;i<agrSoilTypes.size();i++){
		data.push_back(d2s(agrRPrices.at(i))); // rental prices
	}

	data.push_back(d2s(SUMsurveyUnusedLand + ((double) iVSum(unpossessedAgrLandByType)) * MTHREAD->GIS->getHaByPixel() ));

	data.push_back(i2s(iVSum(abandonedAgrLandByType)));
	for (uint i=0;i<agrSoilTypes.size();i++){
		data.push_back(i2s(abandonedAgrLandByType.at(i))); // abandoned agr plots
	}
	data.push_back(i2s(iVSum(idleAgrLandByType)));
	for (uint i=0;i<agrSoilTypes.size();i++){
		data.push_back(i2s(idleAgrLandByType.at(i))); // idle agr plots
	}
	data.push_back(i2s(iVSum(SUMownedPlotsByAllTypes)+iVSum(SUMrentedPlotsByAllTypes))); // total plots
	data.push_back(i2s(iVSum(SUMownedPlotsByAllTypes)));
	for (uint i=0;i<landTypes.size();i++){
		if(MTHREAD->GIS->isAgrCode(landTypes[i]))
			data.push_back(i2s(SUMownedPlotsByAllTypes.at(i)));
	}
	data.push_back(i2s(iVSum(SUMrentedPlotsByAllTypes)));
	for (uint i=0;i<landTypes.size();i++){
		if(MTHREAD->GIS->isAgrCode(landTypes[i]))
			data.push_back(i2s(SUMrentedPlotsByAllTypes.at(i)));
	}
	data.push_back(i2s(iVSum(SUMavailableObjectsByType)));;
	for (uint i=0;i<objNames.size();i++){
		data.push_back(i2s(SUMavailableObjectsByType.at(i)));
	}
	data.push_back(i2s(iVSum(SUMnewlyAvailableObjectsByType)));
	for (uint i=0;i<objNames.size();i++){
		data.push_back(i2s(SUMnewlyAvailableObjectsByType.at(i)));
	}
	data.push_back("");
	for (uint i=0;i<actNames.size();i++){
		data.push_back(d2s(SUMactivitiesByType.at(i)));
	}

	if (MTHREAD->RD->getBoolSetting("outputAgentAggregatedHumanReadable")){
		inOutFileName = MTHREAD->RD->getBaseDirectory() 
		                + MTHREAD->RD->getOutputDirectory()
		                + "results/aggregatedOutput_"
		                + MTHREAD->getScenarioName()
		                + MTHREAD->RD->getStringSetting("outputFileExtension");
		ifstream inFile (inOutFileName.c_str(), ios::in);
		if (!inFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for reading.");}
		for(uint i=0;i<data.size();i++) {
			string tempString;
			getline (inFile,tempString);
			out.append(tempString);
			out.append(data[i]+d);
			out.append("\n");
		}
		inFile.close();

		ofstream outFile (inOutFileName.c_str(), ios::out);
		if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}
		outFile << out ;
		outFile.close();
	} else { // outpout for DB...

		inOutFileName = MTHREAD->RD->getBaseDirectory() 
		                + MTHREAD->RD->getOutputDirectory()
		                + "results/aggregatedOutput"
		                //+ MTHREAD->getScenarioName()
		                + MTHREAD->RD->getStringSetting("outputFileExtension");

		ofstream outFile (inOutFileName.c_str(), ios::app);
		if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}
		outFile << MTHREAD->getScenarioName() << d ;
		for (uint i=0;i<data.size();i++){
				outFile << data[i] << d ;
		}
		outFile << "\n" ;
		outFile.close();
	}
}

void
Output::printMaps(){
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_MAPS) return;
	MTHREAD->GIS->printLayers();
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_BINMAPS) return;
	MTHREAD->GIS->printBinMaps();
	
}

void
Output::printDetailedData(){
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_DETAILED) return;

	char           d         = getOutputFieldDelimiter();
	vector<int>    landTypes    = MTHREAD->RD->getIntVectorSetting("landTypes");
	
	vector<string> objNames  = MTHREAD->RD->getObjectNames();
	vector<string> actNames  = MTHREAD->RD->getActivityNames();

	for(uint i=0;i<surveys.size();i++){
		string out;
		string agName= i2s(surveys[i].agentID);
		string surveyID;
		surveyID = i2s(MTHREAD->RD->getScenarioIndex())+i2s(surveys[i].year)+i2s(surveys[i].agentID);

		double companyLabour = surveys[i].labour + getSurveyActivityByName(surveys[i], "hiredLabour") - getSurveyActivityByName(surveys[i], "offFarmLabour");
		double companyProfit =
			  surveys[i].netProductionProfit
			- getSurveyActivityByName(surveys[i], "offFarmLabour")* MTHREAD->RD->getDoubleSetting("labourShadowPrice", DATA_NOW)
			- surveys[i].lastSunkCostsPaid;
		double companyProfitWithSupport = 
			  companyProfit
			+ surveys[i].coupledPremiums
			+ surveys[i].decoupledPayment;
		double incomes = 
			  surveys[i].netProductionProfit
			+ surveys[i].coupledPremiums
			+ surveys[i].decoupledPayment
			- surveys[i].lastSunkCostsPaid;

		vector <string> sData; //surveyData
		sData.push_back(surveyID);
		sData.push_back(i2s(surveys[i].year));
		sData.push_back(surveys[i].agentType);
		sData.push_back(i2s(surveys[i].agentID));
		sData.push_back(i2s(surveys[i].agentMouldID));
		sData.push_back(b2s(surveys[i].spatial));
		sData.push_back(d2s(surveys[i].hostPlotID));
		sData.push_back(i2s(surveys[i].hostPlotX));
		sData.push_back(i2s(surveys[i].hostPlotY));
		sData.push_back(d2s(surveys[i].capital));
		sData.push_back(d2s(surveys[i].landCapital));
		sData.push_back(d2s(surveys[i].objectsCapital));
		sData.push_back(d2s(surveys[i].labour));
		sData.push_back(d2s(companyLabour));
		sData.push_back(d2s(surveys[i].liquidity));
		sData.push_back(d2s(surveys[i].netProductionProfit));
		sData.push_back(d2s(surveys[i].coupledPremiums));
		sData.push_back(d2s(surveys[i].decoupledPayment));
		sData.push_back(d2s(surveys[i].lastSunkCostsPaid));

		sData.push_back(d2s(companyProfit));
		sData.push_back(d2s(companyProfitWithSupport));
		sData.push_back(d2s(incomes));

		sData.push_back(i2s(iVSum(surveys[i].ownedPlotsByAllTypes)+iVSum(surveys[i].rentedPlotsByAllTypes) ));	// total plots
		sData.push_back(i2s(iVSum(surveys[i].ownedPlotsByAllTypes)));		
		for(uint j=0;j<landTypes.size();j++){
			if(MTHREAD->GIS->isAgrCode(landTypes[j]))
				sData.push_back(i2s(surveys[i].ownedPlotsByAllTypes.at(j)));
		}

		sData.push_back(i2s(iVSum(surveys[i].rentedPlotsByAllTypes)));
		for(uint j=0;j<landTypes.size();j++){
			if(MTHREAD->GIS->isAgrCode(landTypes[j]))
				sData.push_back(i2s(surveys[i].rentedPlotsByAllTypes.at(j)));
		}
		sData.push_back(i2s(iVSum(surveys[i].ownedObjectsByType)));
		for(uint j=0;j<objNames.size();j++){
				sData.push_back(i2s(surveys[i].ownedObjectsByType.at(j)));
		}
		sData.push_back(i2s(iVSum(surveys[i].availableObjectsByType)));
		for(uint j=0;j<objNames.size();j++){
				sData.push_back(i2s(surveys[i].availableObjectsByType.at(j)));
		}
		sData.push_back(i2s(iVSum(surveys[i].newlyAvailableObjectsByType)));
		for(uint j=0;j<objNames.size();j++){
				sData.push_back(i2s(surveys[i].newlyAvailableObjectsByType.at(j)));
		}
		sData.push_back("");
		for(uint j=0;j<actNames.size();j++){
				sData.push_back(d2s(surveys[i].activitiesByType.at(j)));
		}
		sData.push_back(d2s(getSurveyUnusedLand(surveys[i]))); // unused land

		if(MTHREAD->RD->getBoolSetting("outputAgentDetailsHumanReadable")){

			string inOutFileName =     MTHREAD->RD->getBaseDirectory() 
			                       + MTHREAD->RD->getOutputDirectory()
			                       + "results/ag_"
		    	                   + agName
			                       + "_"
			                       + MTHREAD->getScenarioName()
			                       + MTHREAD->RD->getStringSetting("outputFileExtension");

			ifstream inFile (inOutFileName.c_str(), ios::in);
			if (!inFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for reading.");}
			for(uint i=0;i<sData.size();i++) {
				string tempString;
				getline (inFile,tempString);
				out.append(tempString);
				out.append(sData[i]+d);
				out.append("\n");
			}
			inFile.close();

			ofstream outFile (inOutFileName.c_str(), ios::out);
			if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}
			outFile << out ;
			outFile.close();
		} else {  // one big file...
			string outFileName =     MTHREAD->RD->getBaseDirectory() 
			                       + MTHREAD->RD->getOutputDirectory()
			                       + "results/agentsDetails"
			                       //+ MTHREAD->getScenarioName()
			                       + MTHREAD->RD->getStringSetting("outputFileExtension");
			ofstream outFile (outFileName.c_str(), ios::app);
			if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+".");}
			outFile << MTHREAD->getScenarioName() << d ;
			for (uint i=0;i<sData.size();i++){
				outFile << sData[i] << d ;
			}
			outFile << "\n" ;
			outFile.close();
		}
	}
}

void
Output::printDebugMatrices(){
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_ALL) return;
	vector <Agent_base*> agents = MTHREAD->SAM->getAgentsByType(false);
	for (uint i=0;i<agents.size();i++){
		agents.at(i)->debugOptProblem();
	}
}

void
Output::printLeavingAgent(Agent_base* agent, int cause){
	if(MTHREAD->RD->getIntSetting("outputLevel")<OUTVL_DETAILED) return;

	string outFileName =     MTHREAD->RD->getBaseDirectory() 
	                       + MTHREAD->RD->getOutputDirectory()
	                       + "results/leavingAgents"
	                       //+ "_"
	                       //+ MTHREAD->getScenarioName()
	                       + MTHREAD->RD->getStringSetting("outputFileExtension");
	char d = getOutputFieldDelimiter();
	string causeStr;
	double minEqC = MTHREAD->RD->getDoubleSetting("minEC");

	double netProductionProfit   = agent->getNetProductionProfit();
	double grossCoupledPremiums  = agent->getGrossCoupledPremiums();
	double grossDecoupledPayment = agent->getGrossDecoupledPayment();
	double netTotalPremium       = agent->getNetTotalPremium();

	double netCoupledPremiums    = grossCoupledPremiums  * netTotalPremium / (grossCoupledPremiums+grossDecoupledPayment);
	double netDecoupledPayment   = grossDecoupledPayment * netTotalPremium / (grossCoupledPremiums+grossDecoupledPayment);

	double sunkCostsPaid    = agent->getSunkCostsPaid();
	double liquidityShPrice = agent->getLiquidityShPrice();
	double labourShPrice    = agent->getLabourShPrice();
	double landShPrice      = agent->getLandShPrice();

	switch (cause) {
		case AGEXIT_FALSE:
			msgOut(MSG_CRITICAL_ERROR, "Why we are printing as leaving the model the record of an agent that is NOT leaving? Something wrong in the code!");
			break;
		case AGEXIT_NOCAPITAL:
			causeStr = "NOCAPITAL";
			break;
		case AGEXIT_DEBS:
			causeStr = "DEBS";
			break;
		case AGEXIT_SHPRICES:
			causeStr = "SHPRICES";
			break;
		default:
			msgOut(MSG_ERROR, "You have specified an unknow cause of agent leaving the model.");
			causeStr = "UNKNOW";
	}
	ofstream out(outFileName.c_str(), ios::app);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+".");}
	out << MTHREAD->getScenarioName()  << d ;
	out << MTHREAD->SCD->getYear()                        << d
	    << agent->getAgentUniqueID()                      << d
	    << agent->getCategory()                           << d
	    << agent->getMouldLocalID()                       << d
	    << causeStr                                       << d
	    << agent->getNPlots()                             << d
	    << agent->getNPlots(PLOTS_OWNED)                  << d
	    << agent->getNPlots(PLOTS_RENTED)                 << d
	    << agent->getCapital()                            << d
	    << agent->getLiquidity()                          << d
	    << ( agent->getCapital() * (1-minEqC)/minEqC)     << d
	    << netProductionProfit                            << d
	    << netCoupledPremiums                             << d
	    << netDecoupledPayment                            << d
	    << sunkCostsPaid                                  << d
	    << liquidityShPrice                               << d
	    << labourShPrice                                  << d
	    << landShPrice                                    << d
	    << netProductionProfit + netCoupledPremiums + netDecoupledPayment - sunkCostsPaid << d
	    << liquidityShPrice + labourShPrice + landShPrice << d << "\n";

	out.close();
}

char
Output::getOutputFieldDelimiter(){
	int delimiterID = MTHREAD->RD->getIntSetting("outputFieldDelimiter");
	switch (delimiterID) {
		case 1:
			return ',';
			break;
		case 2:
			return ';';
			break;
		case 3:
			return ':';
			break;
		case 4:
			return '\t';
			break;
		case 5:
			return ' ';
			break;
		default:
			msgOut(MSG_ERROR, "You have specified an unknow output file field delimiter. Using \";\".");
			return ',';
	}
}

/**
This routine clean the output scenario from previous outputs of the defined scenario.
Other scenarios are untouched. The scenarioName must be in the first row.
@param filename Filename of the output file to clean
@param scenarioName Name of the scenario we are replacing
@param d Field delimiter. It must not be changed in the meantime (between the various scenarios)
*/
void
Output::cleanScenario(string fileName, string scenarioName, char d){
	string dStr(&d,1);
	vector <string> rows;
	string tempRow;
	ifstream inFile (fileName.c_str(), ios::in);
	if (!inFile){
		msgOut(MSG_ERROR,"Error in opening the file "+fileName+" for reading.");
		return;
	}
	while( getline (inFile,tempRow) ){
		vector<string> tokens;
		tokenize(tempRow,tokens,dStr);
		if(tokens[0] != scenarioName)
			rows.push_back( tempRow );
	}
	inFile.close();
	ofstream out(fileName.c_str(), ios::out);
	for(uint i=0;i<rows.size();i++){
		out << rows[i];
		out << "\n";
	}
	
	/*QFile infile(fileName.c_str());
	if (!infile.open(QFile::ReadOnly)) {
		//cerr << "Cannot open file for reading: "
		//	<< qPrintable(file.errorString()) << endl;
		//msgOut(MSG_ERROR, "Cannot open map file "+fileName+" for reading.");
		return;
	}
	QTextStream in(&file);
	int countRow = 0;
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList fields = line.split(' ');

	*/

	/*		for(uint i=0;i<sData.size();i++) {
				string tempString;
				getline (inFile,tempString);
				out.append(tempString);
				out.append(sData[i]+d);
				out.append("\n");
			}
			inFile.close();

			ofstream outFile (inOutFileName.c_str(), ios::out);
			if (!outFile){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+inOutFileName+" for writing.");}
			outFile << out ;
			outFile.close(); 
	*/
}

double
Output::getSurveyActivityByName(const AgentSurvey& survey, const string& actName){

	vector <string> actNames = MTHREAD->RD->getActivityNames();
	for (uint i=0;i<actNames.size();i++){
		if (actName == actNames[i]){
			return survey.activitiesByType.at(i);
		}
	}
	msgOut(MSG_CRITICAL_ERROR, "in function getSurveyActivityByName() activity named "+actName+" not found.");
	return 0;
}

double
Output::getSurveyUnusedLand(const AgentSurvey& survey){
	double usedLand = 0;
	vector <RegActivities*> activities = MTHREAD->RD->getRegActivities();
	for (uint i=0;i<activities.size();i++){
		if (activities[i]->getSpatiallyExplicit()){
			usedLand += survey.activitiesByType.at(i);
		}
	}
	return ( (double) iVSum(survey.ownedPlotsByAllTypes)+iVSum(survey.rentedPlotsByAllTypes)) - usedLand;

}
