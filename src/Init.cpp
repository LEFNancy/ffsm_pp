/***************************************************************************
 *   Copyright (C) 2012 by Laboratoire d'Economie Forestière               *
 *   http://www.nancy.inra.fr/lef_eng                                      *
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
#include "ModelCore.h"
#include "Opt.h"
#include "Sandbox.h"

//using namespace std;

Init::Init(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
	InitState=0;
}

Init::~Init()
{
}

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
}

void
Init::setInitLevel0(){
	//unused now
	InitState=0;
}

/**
Setting up the space
<br>Level 1 :
 - set the environment (settings, available resource name, possible activities)
 - init the space
@see ModelData::setDefaultSettings();
@see Gis::setSpace()
@see Manager_farmers::setAgentMoulds()

*/
void
Init::setInitLevel1(){
	//Loading data from file.
	InitState=1;
	msgOut(MSG_DEBUG,"Entering Init state "+i2s(InitState));
	time(&now);
	current = localtime(&now);
	string timemessage = "Local time is "+i2s(current->tm_hour)+":"+i2s(current->tm_min)+":"+ i2s(current->tm_sec);
	msgOut(MSG_INFO, timemessage);
	MTHREAD->MD->setDefaultSettings();
	MTHREAD->MD->setScenarioSettings();
	MTHREAD->TEST->fullTest(); // normally empty function
	MTHREAD->MD->createRegions();
	MTHREAD->MD->setDefaultForData();
	MTHREAD->MD->setScenarioForData();
	MTHREAD->MD->setDefaultProdData();
	MTHREAD->MD->setScenarioProdData();
	MTHREAD->MD->setForestTypes();
	MTHREAD->MD->setReclassificationRules();
	MTHREAD->MD->applyOverrides();
	MTHREAD->MD->setProductResourceMatrixLink();
	MTHREAD->MD->applyDebugMode();


	if(MTHREAD->MD->getBoolSetting("randomSimulation")){
		srand(time(NULL));
	}
	MTHREAD->GIS->setSpace();
	MTHREAD->GIS->applyForestReclassification();


}

void
Init::setInitLevel2(){
	InitState=2;
}

/**
Init 3 run the simulation/assign the values for the pre-optimisation year(s)
*/
void
Init::setInitLevel3(){
	InitState=3;
	MTHREAD->DO->initOutput();       // initialize the output files
	MTHREAD->CORE->runInitPeriod();
}

void
Init::setInitLevel4(){
	InitState=4;
}

/**
Init level 5 pass the controll to the Scheduler object for the running of the simulations.
*/
void
Init::setInitLevel5(){
	InitState=5;
	MTHREAD->SCD->run(); // !!!! go "bello" !!!! start the simulation !!!!!
}

void
Init::setInitLevel6(){
	InitState=6;
	MTHREAD->DO->printFinalOutput();
	msgOut(MSG_INFO, "Model has ended scheduled simulation in a regular way.");
	time(&now);
	current = localtime(&now);
	string timemessage = "Local time is "+i2s(current->tm_hour)+":"+i2s(current->tm_min)+":"+ i2s(current->tm_sec);
	msgOut(MSG_INFO, timemessage);
}








