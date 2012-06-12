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
#ifndef STDOUTPUT_H
#define STDOUTPUT_H

//regmas headers
#include "BaseRegmas.h"

struct AgentSurvey;
class ModelObject;
class Agent_base;
class Pixel;


/// %Output methods

/**
Class responsable to output the data, both as all kind of log as well as georeferenciated one.
@author Antonello Lobianco
*/
class Output: public BaseRegmas{
public:
	                    Output(ThreadManager* MTHREAD_h); ///< Constructor
	                   ~Output();

	void                initOutput();
		void                initOutputAggregatedData();
		void                initOutputMaps();
		void                initOutputDetailedData();
		void                initOutputLeavingAgents();
	void                collectData();
	void                print();
		void                printAggregatedData();
		void                printMaps();
		void                printDetailedData();
		void                printDebugMatrices();
	char                getOutputFieldDelimiter();
	void                printLeavingAgent(Agent_base* agent, int cause);
	void                cleanScenario(string fileName, string scenarioName, char d);
	double              getSurveyActivityByName(const AgentSurvey& survey, const string& actName);
	double              getSurveyUnusedLand(const AgentSurvey& survey);

private:
	
	vector<AgentSurvey>                  surveys;
	vector<int>           abandonedAgrLandByType; ///< abandoned land is those land that is unowned, unrented or - even if with a possession title- unused
	vector<int>         unpossessedAgrLandByType; ///< unpossessed land is just those unowned and unrented
	vector<int>                idleAgrLandByType;
};

struct AgentSurvey {
	int                                     year;
	string                             agentType;
	int                                  agentID;
	int                             agentMouldID;
	int                                managerID;
	Agent_base*                            agent;
	bool                                 spatial;
	double                            hostPlotID;
	int                                hostPlotX;
	int                                hostPlotY;
	double                               capital;
	double                           landCapital;
	double                        objectsCapital;
	double                                labour;
	double                             liquidity;
	double                   netProductionProfit; ///< z of the production solve() minus the couled premium
	double                       coupledPremiums;
	double                      decoupledPayment;
	double                     lastSunkCostsPaid; ///< rental costs and other sunk costs (e.g. capital depreciation)
	vector<int>             ownedPlotsByAllTypes;
	vector<int>            rentedPlotsByAllTypes;
	vector<int>               ownedObjectsByType;
	vector<int>           availableObjectsByType;
	vector<int>      newlyAvailableObjectsByType;
	vector<double>              activitiesByType;
};

#endif
