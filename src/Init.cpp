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
#include <time.h> // we only use this to seed the random number generator

#include "Init.h"
//#include "Pixel.h"
#include "Scheduler.h"
#include "ThreadManager.h"
#include "Output.h"
#include "agents/Manager_farmers.h"

//using namespace std;

Init::Init(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
	InitState=0;
};

Init::~Init()
{
};

void
Init::setInitLevel(int level_h){

	switch (level_h){
		case 0:
			this->setInitLevel0();
			break;
		case 1:
			this->setInitLevel1();
			break;
		case 2:
			this->setInitLevel2();
			break;
		case 3:
			this->setInitLevel3();
			break;
		case 4:
			this->setInitLevel4();
			break;
		case 5:
			this->setInitLevel5();
			break;
		case 6:
			this->setInitLevel6();
			break;
		default:
			msgOut(MSG_ERROR,"unexpected Init level");
		}	
};

void
Init::setInitLevel0(){
	//unused now
	InitState=0;
};

/**
Setting up the space, the model objects and the agents (definitions only)
<br>Level 1 :
 - set the environment (settings, available resource name, possible activities)
 - set the object definitions
 - init the space
 - set the agent "moulds" (agent prototypes that will be used to generate the real agents)
 - populate the agents (without yet object nor space assignment)
@see RegData::setObjectsDefinitions()
@see Gis::setSpace()
@see Manager_farmers::setAgentMoulds()
@see Manager_farmers::riseMyAgents()
*/
void
Init::setInitLevel1(){
	//Loading data from file.
	InitState=1;
	ostringstream out;
	out<<"OK, Entering Init state "<<InitState<<"...";
	msgOut(MSG_DEBUG,out.str());
	MTHREAD->RD->setDefaultSettings();
	MTHREAD->RD->setScenarioSettings();
	if(MTHREAD->RD->getBoolSetting("randomSimulation")){
		srand(time(NULL));
	}
	MTHREAD->RD->setResources();
	MTHREAD->RD->setDefaultActivities();
	MTHREAD->RD->setScenarioActivities();
	MTHREAD->RD->setObjectsDefinitions();
	MTHREAD->RD->initializeResourcesObjectsMatch(); // cache values
	MTHREAD->RD->initializeSpacialObjectsMatch();   // cache values
	MTHREAD->GIS->setSpace();

	/** ************************ ADD HERE YOUR MANAGERS ***************** **/
	
	// IMPORTANT: For each new agent_type, remember to load it in the vector of managers!!!
	Manager_farmers* FARMERM = new Manager_farmers(MTHREAD, "farmer"); // the manager for farm agents
	MTHREAD->SAM->addManager(FARMERM);

	// ***************************************************************** //

	MTHREAD->SAM->setAgentMoulds();
	if(MTHREAD->RD->getBoolSetting("subRegionMode")){
		MTHREAD->SAM->rescaleMoulds(MTHREAD->RD->getDoubleSetting("scaleCoeff"));
	}
	MTHREAD->SAM->populateAgents();
};

void
Init::setInitLevel2(){
	InitState=2;
};

/**
Init 3 assign objects to the agents and finally locate both objects and agents along the space dimension.
@see Manager_farmers::assignObjectsToAgents
*/
void
Init::setInitLevel3(){
	InitState=3;
	MTHREAD->SAM->locate();
	MTHREAD->SAM->assignObjectsToAgents();        // this is made my manager_base
	MTHREAD->SAM->assignSpatialObjectsToAgents(); // this is  specific of manager_space managers
	MTHREAD->SAM->createBehaviours();             // create the farmer behaviours
	MTHREAD->DO->initOutput();                    // initialize the output files
	MTHREAD->SAM->endInit();                      // if u have to do something before the simulation start, do it now!
};

void
Init::setInitLevel4(){
	InitState=4;
};

/**
Init level 5 pass the controll to the Scheduler object for the running of the simulations.

*/
void
Init::setInitLevel5(){
	InitState=5;

	vector<double> landStats = MTHREAD->GIS->getAgrLandStats();
	MTHREAD->treeViewerChangeGeneralPropertyValue("total agr land", d2s(landStats.at(0)));
	MTHREAD->treeViewerChangeGeneralPropertyValue("owned agr land", d2s(landStats.at(1)));
	MTHREAD->treeViewerChangeGeneralPropertyValue("rented agr land", d2s(landStats.at(2)));

	MTHREAD->SCD->run(); // !!!! go "bello" !!!! start the simulation !!!!!
};

void
Init::setInitLevel6(){
	InitState=6;
	msgOut(MSG_INFO, "Model has ended scheduled simulation in a regular way.");
};








