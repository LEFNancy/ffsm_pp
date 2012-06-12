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
#include "Opt.h"
#include <stdlib.h>

//#include <glpk.h>

//#include <typeinfo>

#include "Agent_base.h"
#include "Agent_space.h"
#include "RegData.h"
#include "Pixel.h"
#include "ModelObject.h"
#include "ThreadManager.h"
#include "Scheduler.h"

using namespace std;

Opt::Opt(ThreadManager* MTHREAD_h, Agent_base* AGENT_h){
	MTHREAD=MTHREAD_h;
	AGENT=AGENT_h;
	z=0;
	stat=0;
}

Opt::~Opt(){

}

/**
update() can be called at the initialization or later on. It completelly re-wrote the OPT objects (activities, resourceNames and resourceValues) collecting the data from the current situation of the agent.
*/
void
Opt::update(){
	updateActivities();
	updateResourceNames();
	updateResourceValues();
	//updateGlpkProblem();
	refreshGUI();
}

/**
Init or completelly re-write the activity objects of OPT.\n
The function loops over the activity moulds and, if the current one is spatially explicit, loops over the agent owned and rented plots to create a plot-dedicated activity (e.g. durumWheat_on_plot_7321).\n
Before beeing added to the activity vector, each activity is filtered by the agent.\n

- FUNCTION PSEUDOCODE:\n

- plots:     farmer's plots (both owned and rented)
- actMoulds: list of available activities (from input file)
- objMoulds: list of available objects (from input file)\n

- FOR each actMould
	- IF actMould is spatially explicit
		- FOR each plot
			- IF plot's landcode is whitin those required by actMould
				- IF actMould does not require a specific object OR plot has such object
					- create the activity based on actMould
				- END IF object requirement
			- END IF landcode requirement
		- END FOR each plot
	- ELSE
		- create the activity based on actMould
- END FOR each actMould
- FOR each objMould
	- IF objMould is spatially explicit
		- FOR each plot
			- IF plot's landcode is whitin those required by objMould
				- IF plot doesn't already have objMould
					- create the activity based on objMould
				- END IF checking object already present
			- END IF landcode requirement
		- END FOR each plot
	- ELSE
		- create the activity based on objMould
- END FOR each objMould

*/
void
Opt::updateActivities(){
	int nPlots;
	Agent_space* currentAgent;

	vector <Pixel* > plots;
	currentAgent = dynamic_cast<Agent_space*> (AGENT);
	//cout << typeid(AGENT).name()<<endl;
	if (currentAgent){
		plots = currentAgent->getPlots(PLOTS_ALL);
		nPlots = plots.size();
	}
	else{
		nPlots=0;
	}
	
	//int debugID = currentAgent->getAgentUniqueID();

	vector<RegActivities*> actMoulds = MTHREAD->RD->getRegActivities();
	vector<ModelObject*> objects     = MTHREAD->RD->getObjectsDefinitionVector();

	ACTS.clear(); //remove all elements

	// ACTIVITIES....
	for (uint i=0;i<actMoulds.size();i++){
		if(actMoulds.at(i)->getSpatiallyExplicit()){
			vector <int> requiredLandUseCodes = actMoulds.at(i)->getRequiredLandUseCodes();         // required any of them...
			string requiredObjectOnPlot = actMoulds.at(i)->getRequiredObjectOnPlot();
			string landUseCodesToken;
			//string requiredObjectsToken;
			for (uint j=0;j<requiredLandUseCodes.size();j++){
				landUseCodesToken = landUseCodesToken + " " + i2s(requiredLandUseCodes.at(j));
			}
			// setting activities with spatial requirement...
			for (uint j=0;j<plots.size();j++){
				unsigned int z;
				//double debugID = plots[j]->getID();
				string lusecode = d2s(plots.at(j)->getDoubleValue("landUse"));
				bool fulfilledObjectRequirement = false;
				z = landUseCodesToken.find(lusecode); //search the current pixel land use code if it is whitin the ones required by the current activity
				// we already know this activity mould is spacial explicit and so requiredLandUseCodes.size() is > 0
				if (z!=string::npos){ // z in not at the end of the string, means land use code founded!
					// this pixel is one of those supporting this activity. Now we see if the pixel has all the required objects...
					if (requiredObjectOnPlot != ""){ // -> this activity HAS object requirements...
						vector <string> pixelsObjects = plots.at(j)->getIncludedObjectNames();
						for (uint y=0;y<pixelsObjects.size();y++){
							if ( pixelsObjects.at(y) == requiredObjectOnPlot) {
								fulfilledObjectRequirement = true;
								break;
							}
						}
					}
					else {
						fulfilledObjectRequirement = true;
					}
					if (fulfilledObjectRequirement){
						// OK, we add the activity mould on this plot !!
						matrixActivities ACT;
						ACT.name=actMoulds.at(i)->getName();
						double plotID=plots.at(j)->getID();
						string plotIDs = d2s(plotID);
						ACT.fullName=actMoulds.at(i)->getName()+"_on_"+plotIDs;
						//ACT.isSpatial=true;
						ACT.isInvest = false;
						ACT.q = 0;
						ACT.objValue = actMoulds.at(i)->getMatrixGrossMargin();
						vector <double> matrixCoefficients = actMoulds.at(i)->getMatrixCoefficients();
						ACT.values = matrixCoefficients;
						for (uint y=0;y<plots.size();y++){
							ACT.values.push_back(0);
						}
						ACT.values.at(matrixCoefficients.size()+j)=1;
						ACT.plotHost=plots.at(j);
						ACT.actMould = actMoulds.at(i);
						ACT.objMould = 0;
						bool test=AGENT->filterActivity(&ACT); //filter the activity eg. inserting trasnport costs in the gm
						if(test) ACTS.push_back(ACT);
					}
				}
			}
		}
		else {
			// setting activities without space requirements..
			matrixActivities ACT;
			ACT.name=actMoulds.at(i)->getName();
			ACT.fullName=actMoulds.at(i)->getName();
			ACT.isInvest = false;
			ACT.q = 0;
			ACT.objValue = actMoulds.at(i)->getMatrixGrossMargin();
			vector <double> matrixCoefficients = actMoulds.at(i)->getMatrixCoefficients();
			ACT.values = matrixCoefficients;
			for (uint y=0;y<plots.size();y++){
				ACT.values.push_back(0);
			}
			ACT.plotHost=0;
			ACT.actMould = actMoulds.at(i);
			ACT.objMould = 0;
			bool test=AGENT->filterActivity(&ACT);//filter the activity eg. inserting trasnport costs in the gm
			if(test) ACTS.push_back(ACT);
		}
	}

	// NEW OBJECTS....
	for (uint i=0;i<objects.size();i++){
		if(objects.at(i)->getSpatiallyExplicit()){
			vector <int> requiredLandUseCodes = objects.at(i)->getRequiredLandUseCodes();          // required any of them...
			string landUseCodesToken;
			//string requiredObjectsToken;
			for (uint j=0;j<requiredLandUseCodes.size();j++){
				landUseCodesToken = landUseCodesToken + " " + i2s(requiredLandUseCodes.at(j));
			}
			// setting activities with spatial requirement...
			for (uint j=0;j<plots.size();j++){
				//double debugID = plots[j]->getID();
				unsigned int z;
				string lusecode = d2s(plots.at(j)->getDoubleValue("landUse"));
				z = landUseCodesToken.find(lusecode); //search the current pixel land use code if it is whitin the ones required by the current activity
				// we already know this activity mould is spacial explicit and so requiredLandUseCodes.size() is > 0
				if (z!=string::npos){ // z in not at the end of the string, means land use code founded!
					// we check that the plot doesn't already have the investment object we want offer:
					vector <string> pixelsObjects = plots.at(j)->getIncludedObjectNames();
					bool fulfilledObjectRequirement = true;
					for (uint y=0;y<pixelsObjects.size();y++){
						if ( pixelsObjects.at(y) == objects.at(i)->getName()) {
							fulfilledObjectRequirement = false;
							break;
						}
					}
					if (fulfilledObjectRequirement){
						// OK, we add the activity mould on this plot !!
						matrixActivities ACT;
						ACT.name=objects.at(i)->getName();
						ACT.fullName=objects.at(i)->getName()+"_on_"+d2s(plots.at(j)->getID());
						ACT.isInvest = true;
						ACT.q = 0;
						ACT.objValue = objects.at(i)->getMatrixGrossMargin();
						vector <double> matrixCoefficients = objects.at(i)->getMatrixCoefficients();
						ACT.values = matrixCoefficients;
						for (uint y=0;y<plots.size();y++){
							ACT.values.push_back(0);
						}
						ACT.values.at(matrixCoefficients.size()+j)=1;
						ACT.plotHost=plots.at(j);
						ACT.actMould = 0;
						ACT.objMould = objects.at(i);
						bool test=AGENT->filterActivity(&ACT);//filter the activity eg. inserting trasnport costs in the gm
						if(test) ACTS.push_back(ACT);
					}
				}
			}
		}
		else {
			// setting activities without space requirements..
			matrixActivities ACT;
			ACT.name=objects.at(i)->getName();
			ACT.fullName=objects.at(i)->getName();
			ACT.isInvest = true;
			ACT.q = 0;
			ACT.objValue = objects.at(i)->getMatrixGrossMargin();
			vector <double> matrixCoefficients = objects.at(i)->getMatrixCoefficients();
			ACT.values = matrixCoefficients;
			for (uint y=0;y<plots.size();y++){
				ACT.values.push_back(0);
			}
			ACT.plotHost=0;
			ACT.actMould = 0;
			ACT.objMould = objects.at(i);
			bool test=AGENT->filterActivity(&ACT);//filter the activity eg. inserting trasnport costs in the gm
			if(test) ACTS.push_back(ACT);
		}
	}
};

/**
Init or completelly re-write the resource names of OPT.
<br>The function loops over the resource moulds and, if the current one is spatially explicit, loops over the agent owned and rented plots to create a plot-dedicated resource name (e.g. plot_7321).
*/
void
Opt::updateResourceNames(){

	resourceNames.clear();
	resourceNames= MTHREAD->RD->getResourceNames();
	
	int nPlots;
	Agent_space* currentAgent;
	vector <Pixel* > plots;
	currentAgent = dynamic_cast<Agent_space*> (AGENT);
	if (currentAgent){
		plots=currentAgent->getPlots(PLOTS_ALL);
		nPlots=plots.size();
	}
	else{
		nPlots=0;
	}
	for (uint i=0;i<plots.size();i++){
		string name = "Plot_"+d2s(plots.at(i)->getID());
		resourceNames.push_back(name);
	}
};

/**
Init or completelly re-write the level of the resources available to the agent in the MIP.

Resource can be of four different types according on their SOURCE:
- RESSOURCE_ZERO Their value is always zero, as there will be some activity filling it during the MIP solving algorithm;
- RESSOURCE_FIXED The value is fixed at data-input stage and doen't change during the model. E.g. family labour
- RESOURCE_CALCULATED The individual agents are provided with a specific function to calculate this resources, e.g. liquidity and debsLevel
- RESSOURCE_OBJECTS: this resource is provided by the object owned by the agent (e.g. animal stables)

At the end of the "normal" resources, each pixel owned/rented by the agent is added on the list with value "1"
*/
void
Opt::updateResourceValues(){

	resourceValues.clear();
	resourceValues.resize(resourceNames.size(),0);

	// ..then setting initial resources..
	for (int i=0;i<MTHREAD->RD->getResourceSize();i++){
		resourceValues.at(i) = AGENT->getResource(resourceNames.at(i));
	}
	//placing 1 for pixel resources...
	for (uint i=MTHREAD->RD->getResourceSize();i<resourceNames.size();i++){
		resourceValues[i] = 1;
	}	

};

/*
// removed 20090701
void
Opt::updateGlpkProblem(){
	// ia[] -> row indexes; ja[]->column indexes; ar[]->corresponding values
	
	// As in Windows I can't allocate more than 120000 elements in the arrys (bho, why ????) I am first checking and emitting a warning if the array to allocate is so big
	int maxNZSpace = 2; //shuld be 1, 2 just to be sure...
	for (uint j=1;j<ACTS.size()+1;j++){
		for (uint i=1;i<resourceNames.size()+1;i++){
			if (ACTS[j-1].values[i-1] != 0){
				maxNZSpace++;
			}
		}
	}	
	if(maxNZSpace >=120000){
		msgOut(MSG_WARNING, "Agent "+i2s(AGENT->getAgentUniqueID())+" has a huge matrix.. on MS Windows system this could lead to a crash.. u are advised!");
	}

	//int maxNZSpace= ACTS.size()*resourceNames.size()+1;
	int ia[maxNZSpace], ja[maxNZSpace];  // cool! Borland compiler didn't allow me array initialization from a variable !!
	double ar[maxNZSpace]; //array of the coefficients

	// deleting all rows and columns...
	int nRowsCurrentProblem = glp_get_num_rows(lpglpk);
	int nColsCurrentProblem = glp_get_num_cols(lpglpk);
	
	if(nRowsCurrentProblem>0){
		int num[nRowsCurrentProblem];
		for(int i=1;i<nRowsCurrentProblem+1;i++){
			num[i]=i;
		}
		glp_del_rows(lpglpk, nRowsCurrentProblem, num);
	}
	if(nColsCurrentProblem>0){
		int num[nColsCurrentProblem];
		for(int i=1;i<nColsCurrentProblem+1;i++){
			num[i]=i;
		}
		glp_del_cols(lpglpk, nColsCurrentProblem, num);
	}

	// adding rows and placing bounds:
	glp_add_rows(lpglpk, resourceNames.size());
	// checking if this year we are for forced to use ALL the land we have (cross-complain)
	bool fixedLand = MTHREAD->RD->getBoolSetting("fullLandUsage", DATA_NOW);
	for (uint i=1;i<resourceNames.size()+1;i++){
		if (((int)i)<MTHREAD->RD->getResourceSize()+1 || !fixedLand ){
			glp_set_row_bnds(lpglpk, i, GLP_UP, 0.0, resourceValues[i-1]); // <= setting the constrain upper limit
		}
		else {
			// the resource is a plot and we are forced to use it all..
			glp_set_row_bnds(lpglpk, i, GLP_FX, resourceValues[i-1], resourceValues[i-1]); // <= setting the constrain limit as fixed
		}
	}


	// adding columns, placing bounds and obj coefficients:
	glp_add_cols(lpglpk, ACTS.size());
	for (uint i=1;i<ACTS.size()+1;i++){
		glp_set_obj_coef(lpglpk, i, ACTS[i-1].objValue);
		// Investments are not made on a separate MIP. They are done in the same normal production MIP, in one go.
		// If we go for two separate MIPS, we would need to fix investments decision quantities to 0:
		//if (prod) {  // production lp, no investments
		//	if ((i-1)< prodcols)
		//		glp_set_col_bnds(lpglpk, i, GLP_LO, 0.0, 0.0); // proction: 0->+inf
		//	else
		//		glp_set_col_bnds(lpglpk, i, GLP_FX, 0.0, 0.0); // investments: fixed to 0
		//}
		//else { // normal lp
		glp_set_col_bnds(lpglpk, i, GLP_LO, 0.0, 0.0);
		//}
	}
	
	// creating the three arrays needed for lpx_load_matrix(): rows, column, value
	// zero coefficients are not allowed
	int nz_counter=1; // nz_conter stand for "non-zero counter"
	for (uint j=1;j<ACTS.size()+1;j++){
		for (uint i=1;i<resourceNames.size()+1;i++){
			if (ACTS[j-1].values[i-1] != 0){
				ia[nz_counter] = i;
				ja[nz_counter] = j;
				ar[nz_counter] = ACTS[j-1].values[i-1];
				nz_counter++;
			}
		}
	}
	// loading the constrain coefficient matrix..
	glp_load_matrix(lpglpk, nz_counter-1, ia, ja, ar);

	// setting integer parameters
	for (uint i=0;i<ACTS.size();i++){
		if (ACTS[i].isInvest){
			glp_set_col_kind(lpglpk, i+1, GLP_IV);
		}
	}
};
*/

void
Opt::debug(string debugMessage, bool useCallCounter){
	int callCounter=0; // 20090701 removed static !
	string directoryToSave = MTHREAD->RD->getBaseDirectory()+MTHREAD->RD->getOutputDirectory()+"debugMatrices/";
	string filenameToSave = "";
	if (useCallCounter){
		filenameToSave  = "ag"+i2s(AGENT->getAgentUniqueID())+"_-_"+i2s(MTHREAD->SCD->getYear())+"_-_"+i2s(callCounter)+"_-_"+debugMessage+".csv";
	} else {
		filenameToSave  = "ag"+i2s(AGENT->getAgentUniqueID())+"_-_"+i2s(MTHREAD->SCD->getYear())+"_-_"+debugMessage+".csv";
	}
	string fullFilename = directoryToSave+filenameToSave;
	ofstream out;
	out.open(fullFilename.c_str(), ios::out); //ios::app to append..
	if (!out){ msgOut(MSG_ERROR,"Error in opening the file "+fullFilename+".");}
	out << "*** Debug Matrix ***"<<  "\n\n";
	out << "Optimal Value (Z);Stat;Year;Agent;Mould;Msg" << "\n";
	out << z << ";" << stat << ";" << MTHREAD->SCD->getYear() << ";" << AGENT->getAgentUniqueID() << ";" << AGENT->getMouldLocalID() << ";" << debugMessage << "\n\n";
	
	// printing activity names..
	out << ";RESOURCES;;";
	for (uint i=0;i<ACTS.size();i++){
		out << ACTS.at(i).fullName << ";" ;
	}
	out << "\n";
	// printing activity type (continuous/intger)..
	out << "type   -->;;;";
	for (uint i=0;i<ACTS.size();i++){
		out << ACTS.at(i).isInvest << ";" ;
	}
	out << "\n";
	// printing target function coefficients..
	out << "gm     -->;;;";
	for (uint i=0;i<ACTS.size();i++){
		out << ACTS.at(i).objValue << ";" ;
	}
	out << "\n";
	// printing (solved) optimal quantities..
	out << "opt q  -->;;;";
	for (uint i=0;i<ACTS.size();i++){
		out << ACTS.at(i).q << ";" ;
	}
	out << "\n";
	out << "Resources:;"<<"\n";

	for (uint i=0;i<resourceNames.size();i++){
		out << resourceNames.at(i) << ";" ;
		out << resourceValues.at(i) << ";;" ;
		for (uint j=0;j<ACTS.size();j++){
			out << ACTS.at(j).values.at(i) << ";" ;
		}
		out << "\n";
	}
	out << "\n\n";

	out.close();
	callCounter++;
};

/**
The function call the underlying solver library (currently glpk) to "solve" the current state of the OPT problem.

The value of the objective function is stored on OPT.z, the optimal quantities are stored on the individual OPT.ACTS.at(i).q variables and the solver exit code on OPT.stat.
*/
void
Opt::solve(){
	
	solveGlpk();
	return;

}


/**
Initialise the problem, solve it and load the results using the glpk library

Function updated on 2009.07.01 for multithreading (previously it was splitted in several parts for optimisation)
*/
void
Opt::solveGlpk(){
    msgOut(MSG_ERROR,"Finito",true);
    /*
	bool fixedLand;
	int resourceSize;
	//QMutex localMutex1, localMutex2, localMutex3; // 20090703 mutexes must be shared along several instances !This is wrong!
	//localMutex1.lock();
	// checking if this year we are for forced to use ALL the land we have (cross-complain)
	fixedLand = MTHREAD->RD->getBoolSetting("fullLandUsage", DATA_NOW);
	resourceSize = MTHREAD->RD->getResourceSize();
	//localMutex1.unlock();

	// ###### FROM OLD HEADER/CONSTRUCTOR... #######
	glp_prob*  lpglpk; // pointer to the glpk problem
	glp_iocp*  parm; // parameter structur for glp_intopt routine (from glpk v. >= 4.20)
	lpglpk = glp_create_prob();
	glp_set_prob_name(lpglpk, "farmerProblem");
	glp_set_obj_dir(lpglpk, GLP_MAX);
	// Suppress Output from glpk
	//lpx_set_int_parm(lpglpk, LPX_K_MSGLEV, 0); //deprecated
	glp_term_out(GLP_OFF); //new, it also delete the "crashing..." messages that is a glpk terminology meaning totally safe and different thing than normal "crashing"
	// for glp_intopt() function, from glpk >= 4.20
	parm = new glp_iocp;
	//parm = NULL;
	glp_init_iocp(parm);
	// Suppress Output from glpk
	parm->msg_lev = GLP_MSG_OFF;



	// ###### FROM OLD updateGlpkProblem.. ######
	// ia[] -> row indexes; ja[]->column indexes; ar[]->corresponding values
	
	// As in Windows I can't allocate more than 120000 elements in the arrys (bho, why ????) I am first checking and emitting a warning if the array to allocate is so big
	int maxNZSpace = 2; //shuld be 1, 2 just to be sure...
	for (uint j=1;j<ACTS.size()+1;j++){
		for (uint i=1;i<resourceNames.size()+1;i++){
			if (ACTS[j-1].values[i-1] != 0){
				maxNZSpace++;
			}
		}
	}	
	//localMutex2.lock();
	if(maxNZSpace >=120000){
		msgOut(MSG_WARNING, "Agent "+i2s(AGENT->getAgentUniqueID())+" has a huge matrix.. on MS Windows system this could lead to a crash.. u are advised!");
	}
	//localMutex2.unlock();

	//int maxNZSpace= ACTS.size()*resourceNames.size()+1;
	int ia[maxNZSpace], ja[maxNZSpace];  // cool! Borland compiler didn't allow me array initialization from a variable !!
	double ar[maxNZSpace]; //array of the coefficients

	// deleting all rows and columns...
	int nRowsCurrentProblem = glp_get_num_rows(lpglpk);
	int nColsCurrentProblem = glp_get_num_cols(lpglpk);
	
	if(nRowsCurrentProblem>0){
		int num[nRowsCurrentProblem];
		for(int i=1;i<nRowsCurrentProblem+1;i++){
			num[i]=i;
		}
		glp_del_rows(lpglpk, nRowsCurrentProblem, num);
	}
	if(nColsCurrentProblem>0){
		int num[nColsCurrentProblem];
		for(int i=1;i<nColsCurrentProblem+1;i++){
			num[i]=i;
		}
		glp_del_cols(lpglpk, nColsCurrentProblem, num);
	}

	// adding rows and placing bounds:
	glp_add_rows(lpglpk, resourceNames.size());
	

	for (uint i=1;i<resourceNames.size()+1;i++){
		if (((int)i)<resourceSize+1 || !fixedLand ){
			glp_set_row_bnds(lpglpk, i, GLP_UP, 0.0, resourceValues[i-1]); // <= setting the constrain upper limit
		}
		else {
			// the resource is a plot and we are forced to use it all..
			glp_set_row_bnds(lpglpk, i, GLP_FX, resourceValues[i-1], resourceValues[i-1]); // <= setting the constrain limit as fixed
		}
	}


	// adding columns, placing bounds and obj coefficients:
	glp_add_cols(lpglpk, ACTS.size());
	for (uint i=1;i<ACTS.size()+1;i++){
		glp_set_obj_coef(lpglpk, i, ACTS[i-1].objValue);
		// Investments are not made on a separate MIP. They are done in the same normal production MIP, in one go.
		// If we go for two separate MIPS, we would need to fix investments decision quantities to 0:
		//if (prod) {  // production lp, no investments
		//	if ((i-1)< prodcols)
		//		glp_set_col_bnds(lpglpk, i, GLP_LO, 0.0, 0.0); // proction: 0->+inf
		//	else
		//		glp_set_col_bnds(lpglpk, i, GLP_FX, 0.0, 0.0); // investments: fixed to 0
		//}
		//else { // normal lp
		glp_set_col_bnds(lpglpk, i, GLP_LO, 0.0, 0.0);
		//}
	}
	
	// creating the three arrays needed for lpx_load_matrix(): rows, column, value
	// zero coefficients are not allowed
	int nz_counter=1; // nz_conter stand for "non-zero counter"
	for (uint j=1;j<ACTS.size()+1;j++){
		for (uint i=1;i<resourceNames.size()+1;i++){
			if (ACTS[j-1].values[i-1] != 0){
				ia[nz_counter] = i;
				ja[nz_counter] = j;
				ar[nz_counter] = ACTS[j-1].values[i-1];
				nz_counter++;
			}
		}
	}
	// loading the constrain coefficient matrix..
	glp_load_matrix(lpglpk, nz_counter-1, ia, ja, ar);

	// setting integer parameters
	for (uint i=0;i<ACTS.size();i++){
		if (ACTS[i].isInvest){
			glp_set_col_kind(lpglpk, i+1, GLP_IV);
		}
	}

	// ###### FROM OLD SOLVE FUNCTION... ######
	//recreate the standard bases..
	//glp_adv_basis(lpglpk,0); //no longer needed

	// solving the problem with only continuous variables..
	stat=lpx_simplex(lpglpk);

	// setting the problem as integer
	//lpx_set_class(lpglpk, LPX_MIP);

	// solving the problem with the integer variables
	stat=lpx_integer(lpglpk); // old 11.? seconds . Now only 8.45 sec

	//stat=glp_intopt(lpglpk, parm); // 8.60 seconds, like lpx_integer()

	//stat=lpx_intopt(lpglpk); I can't shut down the messages, so very slow

	 //lpx_intopt(lpglpk);  // as of version 4.9 lpx_intopt is buggy (we discover it ;-) )
	z = glp_mip_obj_val(lpglpk);

	// retrive activity levels...
	for (uint i=0; i<ACTS.size(); i++){
		ACTS[i].q = glp_mip_col_val(lpglpk, i+1);  // I need to be fast here !!
	}

	//refreshGUI();

	// ###### FROM OLD DESTRUCTOR... ######
	glp_delete_prob(lpglpk);
	delete parm;
    */

}

/**
This function save to a temporary cache the value of the target function reached by the last call to solve() and the optimal quantities od ACTS.

It is used when planning solve() is needed for e.g. calculate the shadow price of plots.

*/
void
Opt::saveToCache(){
	cachedZ = z;
	for (uint i=0;i<ACTS.size();i++){
		ACTS[i].cachedQ = ACTS[i].q; // I need to be fast here !!
	}
	cachedStat = stat;
}

/**
This function retrieve from the temporary cache the value of the target function and the optimal quantities od ACTS.

It is used when planning solve() is needed for e.g. calculate the shadow price of plots to reset the OPT object to its original conditions.

*/
void
Opt::restoreFromCache(){
	z = cachedZ;
	for (uint i=0;i<ACTS.size();i++){
		ACTS[i].q = ACTS[i].cachedQ;  // I need to be fast here !!
	}
	stat = cachedStat;
}

/**
The function upgrade the OPT object when the agent has availability of a new plot.

The nice thing is that it does that without re-writing the whole matrix, but just adding the relevant activities and resources. And even more nicely, it keeps track of that using the actsAddedOnLastPixel counter, so we can later on remove the last added pixel using Opt::removeLastAddedPixel() ;-)

This function is mainly used to calculate the shadow price of a new plot.
*/

void
Opt::addPixel(const Pixel* px_h){

	Agent_space* currentAgent;
	currentAgent = dynamic_cast<Agent_space*> (AGENT);
	//QMutex localMutex;


	if (currentAgent){ // if our agent is not spatially explicit we don't have anything to do ;-)
		//localMutex.lock();
		double plotId;
		vector<RegActivities*> actMoulds;
		vector<ModelObject*> objects;
		double landUse;
		string lusecode;


		plotId=px_h->getID();
		actMoulds = MTHREAD->RD->getRegActivities();
		objects     = MTHREAD->RD->getObjectsDefinitionVector();
		lusecode = d2s(px_h->getDoubleValue("landUse"));
		//localMutex.unlock();
		
		actsAddedOnLastPixel=0;
		resourceNames.push_back("Plot_"+d2s(plotId));
		resourceValues.push_back(1);

		for (uint i=0;i<ACTS.size();i++){
			ACTS.at(i).values.push_back(0);
		}
		
		/*if (MTHREAD->RD->getBoolSetting("fullLandUsage", DATA_NOW)){ // we are in a "forced to use all land" mode..
			addResourceToGlpk(GLP_FX, 1, 1);
		} else {
			addResourceToGlpk(GLP_UP, 0, 1);
		}*/




		// adding spatial activities...
		for (uint i=0;i<actMoulds.size();i++){
			if(actMoulds.at(i)->getSpatiallyExplicit()){
				vector <int> requiredLandUseCodes = actMoulds.at(i)->getRequiredLandUseCodes();         // required any of them...
				string requiredObjectOnPlot = actMoulds.at(i)->getRequiredObjectOnPlot();    // required ALL of them...
				string landUseCodesToken;
				//string requiredObjectsToken;
				for (uint j=0;j<requiredLandUseCodes.size();j++){
					landUseCodesToken = landUseCodesToken + " " + i2s(requiredLandUseCodes.at(j));
				}
				unsigned int z;
				bool fulfilledObjectRequirement = false; // bug, 20071123, Antonello
				z = landUseCodesToken.find(lusecode); //search the pixel land use code if it is whitin the ones required by the current activity
				// we already know this activity mould is spacial explicit and so requiredLandUseCodes.size() is > 0
				if (z!=string::npos){ // z in not at the end of the string, means land use code founded!
					// this pixel is one of those supporting this activity. Now we see if the pixel has all the required objects...
					if (requiredObjectOnPlot != ""){
						vector <string> pixelsObjects = px_h->getIncludedObjectNames();
						for (uint y=0;y<pixelsObjects.size();y++){
							if ( pixelsObjects.at(y) == requiredObjectOnPlot) {
								fulfilledObjectRequirement = true;
								break;
							}
						}
					}
					else {
						fulfilledObjectRequirement = true;
					}
					if (fulfilledObjectRequirement){
						// OK, we add the activity mould on this plot !!
						matrixActivities ACT;
						ACT.name=actMoulds.at(i)->getName();
						ACT.fullName=actMoulds.at(i)->getName()+"_on_"+d2s(px_h->getID());
						ACT.isInvest = false;
						ACT.q = 0;
						ACT.objValue = actMoulds.at(i)->getMatrixGrossMargin();
						vector <double> matrixCoefficients = actMoulds.at(i)->getMatrixCoefficients();
						ACT.values = matrixCoefficients;
						for (int y=0;y<currentAgent->getNPlots();y++){
							ACT.values.push_back(0);
						}
						ACT.values.push_back(1);
						ACT.plotHost=px_h;
						ACT.actMould = actMoulds.at(i); // bug, 20071123, Antonello
						ACT.objMould = 0;
						bool test=AGENT->filterActivity(&ACT); //filter the activity eg. inserting trasnport costs in the gm
						if(test) ACTS.push_back(ACT);
						actsAddedOnLastPixel++;
						//addActivityToGlpk(&ACT);
					}
				}
			}
		}

		// adding spatial investment objects
		//DONE: add spatial investment objects
		for (uint i=0;i<objects.size();i++){
			if(objects.at(i)->getSpatiallyExplicit()){
				vector <int> requiredLandUseCodes = objects.at(i)->getRequiredLandUseCodes();          // required any of them...
				string landUseCodesToken;
				//string requiredObjectsToken;
				for (uint j=0;j<requiredLandUseCodes.size();j++){
					landUseCodesToken = landUseCodesToken + " " + i2s(requiredLandUseCodes.at(j));
				}
				// setting activities with spatial requirement...
				unsigned int z;
				string lusecode = d2s(px_h->getDoubleValue("landUse"));
				z = landUseCodesToken.find(lusecode); //search the current pixel land use code if it is whitin the ones required by the current activity
				// we already know this activity mould is spacial explicit and so requiredLandUseCodes.size() is > 0
				if (z!=string::npos){ // z in not at the end of the string, means land use code founded!
					// we check that the plot doesn't already have the investment object we want offer:
					vector <string> pixelsObjects = px_h->getIncludedObjectNames();
					bool fulfilledObjectRequirement = true;
					for (uint y=0;y<pixelsObjects.size();y++){
						if ( pixelsObjects.at(y) == objects.at(i)->getName()) {
							fulfilledObjectRequirement = false;
							break;
						}
					}
					if (fulfilledObjectRequirement){
						// OK, we add the activity mould on this plot !!
						matrixActivities ACT;
						ACT.name=objects.at(i)->getName();
						ACT.fullName=objects.at(i)->getName()+"_on_"+d2s(px_h->getID());
						ACT.isInvest = true;
						ACT.q = 0;
						ACT.objValue = objects.at(i)->getMatrixGrossMargin();
						vector <double> matrixCoefficients = objects.at(i)->getMatrixCoefficients();
						ACT.values = matrixCoefficients;
						for (int y=0;y<currentAgent->getNPlots();y++){
							ACT.values.push_back(0);
						}
						ACT.values.push_back(1);
						ACT.plotHost=px_h;
						ACT.actMould = 0;
						ACT.objMould = objects.at(i);
						bool test=AGENT->filterActivity(&ACT);//filter the activity eg. inserting trasnport costs in the gm
						if(test) ACTS.push_back(ACT);
						actsAddedOnLastPixel++;
						//addActivityToGlpk(&ACT);
					}
				}
			}	
		}
		//localMutex.unlock();
	}
};

/*
// removed 20090701
void
Opt::addResourceToGlpk(int glpkBoundType, double minValue, double maxValue){
	int newAddedRowNumber = glp_add_rows(lpglpk,1);
	glp_set_row_bnds(lpglpk, newAddedRowNumber, glpkBoundType, minValue, maxValue);
};

void
Opt::addActivityToGlpk(const matrixActivities* ACT){
	int newAddedColumnNumber = glp_add_cols(lpglpk, 1);
	glp_set_obj_coef(lpglpk, newAddedColumnNumber, ACT->objValue);
	glp_set_col_bnds(lpglpk, newAddedColumnNumber, GLP_LO, 0.0, 0.0);

	int maxSize = ACT->values.size()+1;
	int lpCounters[maxSize+1];
	double lpValues[maxSize+1];

	int nz_counter=1; // nz_conter stand for "non-zero counter"
	for(int i=1;i<maxSize;i++){
		if(ACT->values[i-1] != 0){
			lpCounters[nz_counter] = i;
			lpValues[nz_counter] = ACT->values[i-1];
			nz_counter++;
		}
	}
	nz_counter--;
	glp_set_mat_col(lpglpk, newAddedColumnNumber, nz_counter, lpCounters, lpValues);
	/// setting integer constraint if investment...
	if (ACT->isInvest){
		glp_set_col_kind(lpglpk, newAddedColumnNumber, GLP_IV);
	}
};
*/

/**
if we just added a pixel with addPixel end we want remove that (e.g. because we was just calculating a shadow price, without a "real" addiction of the pixel to the agent), we call this function to reset the OPT object to their previous condition.

This function uses actsAddedOnLastPixel counter to do its job, and it doesn't require a whole re-writing of the OPT object. 

*/
void
Opt::removeLastAddedPixel(){

	int temp[1+1];
	temp[0] = resourceNames.size();
	temp[1] = resourceNames.size();
	//glp_del_rows(lpglpk, 1, temp); // removing last row from glpk object 
	resourceNames.pop_back();
	resourceValues.pop_back();

	//removing 1 row...
	for (uint i=0;i<ACTS.size();i++){
		ACTS.at(i).values.pop_back();
	}
	// removing actsAddedOnLastPixel columns...
	for (int i=0;i<actsAddedOnLastPixel;i++){
		temp[0] = ACTS.size();
		temp[1] = ACTS.size();
		//glp_del_cols(lpglpk,1,temp); // deleting the last one. next line assures that ACTS.size() change correctly 
		ACTS.pop_back();
	}
}

vector <matrixActivities*>
Opt::getActs(){
	vector <matrixActivities*> toReturn;
	for (uint i=0;i< ACTS.size();i++){
		toReturn.push_back(&ACTS[i]);
	}
	return toReturn;
}

void
Opt::test(){
	//test for GLPK
    /*
	glp_prob *lp;
	int ia[1+1000], ja[1+1000];
	double ar[1+1000], z;
	lp = glp_create_prob();
	glp_set_prob_name(lp, "sample");
	glp_set_obj_dir(lp, GLP_MAX);
	glp_add_rows(lp, 3);
	glp_set_row_name(lp, 1, "p");
	glp_set_row_bnds(lp, 1, GLP_UP, 0.0, 100.0);
	glp_set_row_name(lp, 2, "q");
	glp_set_row_bnds(lp, 2, GLP_UP, 0.0, 600.0);
	glp_set_row_name(lp, 3, "r");
	glp_set_row_bnds(lp, 3, GLP_UP, 0.0, 300.0);
	glp_add_cols(lp, 3);
	glp_set_col_name(lp, 1, "x1");
	glp_set_col_bnds(lp, 1, GLP_LO, 0.0, 0.0);
	glp_set_obj_coef(lp, 1, 10.0);
	glp_set_col_name(lp, 2, "x2");
	glp_set_col_bnds(lp, 2, GLP_LO, 0.0, 0.0);
	glp_set_obj_coef(lp, 2, 6.0);
	glp_set_col_name(lp, 3, "x3");
	glp_set_col_bnds(lp, 3, GLP_LO, 0.0, 0.0);
	glp_set_obj_coef(lp, 3, 4.0);
    ia[1] = 1, ja[1] = 1, ar[1] = 1.0; // a[1,1] = 1
    ia[2] = 1, ja[2] = 2, ar[2] = 1.0; // a[1,2] = 1
    ia[3] = 1, ja[3] = 3, ar[3] = 1.0; // a[1,3] = 1
    ia[4] = 2, ja[4] = 1, ar[4] = 10.0; // a[2,1] = 10
    ia[5] = 3, ja[5] = 1, ar[5] = 2.0; // a[3,1] = 2
    ia[6] = 2, ja[6] = 2, ar[6] = 4.0; // a[2,2] = 4
    ia[7] = 3, ja[7] = 2, ar[7] = 2.0; // a[3,2] = 2
    ia[8] = 2, ja[8] = 3, ar[8] = 5.0; // a[2,3] = 5
    ia[9] = 3, ja[9] = 3, ar[9] = 6.0; // a[3,3] = 6
	glp_load_matrix(lp, 9, ia, ja, ar);
	// Suppress Output from glpk
	lpx_set_int_parm(lp, LPX_K_MSGLEV, 0);
	lpx_simplex(lp);   // solve with simplex method

	// setting the problem as integer..
	lpx_set_class(lp, LPX_MIP);
	// setting 2nd row as integer
	lpx_set_col_kind(lp, 2, LPX_IV);

	// solving the problem with the integer variables
	lpx_integer(lp);
	z = lpx_mip_obj_val(lp);

	//lpx_interior(lp);  // solve with interior points method
	//z = glp_get_obj_val(lp); // simplex
	//z = glp_ipt_obj_val(lp); // interior points
	//x1 = glp_get_col_prim(lp, 1);
	//x2 = glp_get_col_prim(lp, 2);
	//x3 = glp_get_col_prim(lp, 3);
	//printf("\nz = %g; x1 = %g; x2 = %g; x3 = %g\n",
	//	z, x1, x2, x3);
	msgOut(MSG_DEBUG, "Optimal value is: "+d2s(z));
	glp_delete_prob(lp);
    */
}

double
Opt::getActivityObjValue(string name_h){
	for (uint i=0;i<ACTS.size();i++){
		if(ACTS.at(i).fullName == name_h){
			return ACTS.at(i).objValue;
		}
	}
	msgOut(MSG_ERROR, "Activity "+name_h+" not found. Returning 0");
	return 0;
}
