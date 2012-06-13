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
#ifndef STDSUPERAGENTMANAGER_H
#define STDSUPERAGENTMANAGER_H

// standard headers
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>

// regmas headers...
#include "BaseClass.h"
#include "Gis.h"
#include "RegData.h"


class Manager_base;

/// Program-side interface to comunicate with the different agent managers.

/**
Main class for agents. It should be an interface for the I/O with the program (particularly, with the Scheduler) ON THE PROGRAM SIDE.
<br>This class has a unique istance and own a vector of Manager_base object (called without too much fantacy "managers").
<br>The idea is to have a parallel hierachical system for the agent managers and for the agents. The former are responsable to aggregate the agents they manage and produce aggregated autput, while the latter are the behaviour of the single agents.
<br>In the middle of the hiearchical system there is a division between agents with space-related carachteristics (e.g. farmers) and agents not georeferenced (e.g. market agents).

<p>this file           --> interface programmer side

<br>Manager_base        --> interface on the single agent side, base class for manager objects
<br>Manager_nospace     --> half-derived class for manager object of agent without explicit spatial rappresentation
<br>manager_space       --> the same for agents that require geolocalisation
<br>agents/manager_xxxs  --> the final manager object for the specific agent type

<br>Agent_base           --> desribing the behaviour of all modelled agents, base
<br>Agent_nospace        --> derived class for agetns without space requirements
<br>Agent_space          --> derived class for spaced related agents
<br>agents/agent_xxxx    --> describing the single agent behaviour

<p>For each new "agent type" there should so the need of only two files. The agents need to know only about their manager, and their manager need info only from their agents and the "SuperAgentManager" class.
@author Antonello Lobianco
*/
class SuperAgentManager: public BaseClass{
	
public:
	                    SuperAgentManager(ThreadManager* MTHREAD_h);
	                   ~SuperAgentManager();

	/// Return the category names of the agents (e.g. "Agent_farmer")
	vector<string>      getAgentTypeNames();
	/// Return the number of agents managed for each type
	vector<int>         getAgentCountByType();
	/// Return the vector of Managers (pointers)
	vector<Manager_base*> getManagers(){return managers;};

	/// Add the manager to the vector of managers
	void                addManager(Manager_base* manager_h){managers.push_back(manager_h);};
	/// Asks managers to setting their moulds. @see Manager_base::setAgentMoulds()
	void                setAgentMoulds();
	/// Asks managers to rise their actual agents. @see Manager_base::riseMyAgents()
	void                populateAgents();
	/// Asks managers to locate agents in the space and assign plots, eventually creating new layers. @see Manager_base::locateMyAgents();
	void                locate();
	/// Asks managers to assign objects to their agents.  @see Manager_base::assignObjectsToAgents()
	void                assignObjectsToAgents();
	/// Asks managers to assign spatial objects to their agent's plots.  @see Manager_space::assignObjectsToAgents()
	void                assignSpatialObjectsToAgents();
	int                 getNewUniqueAgentID(){int toReturn=uniqueAgentIDCounter; uniqueAgentIDCounter++; return toReturn;}
	int                 getNewUniqueManagerID(){int toReturn=uniqueManagerIDCounter; uniqueManagerIDCounter++; return toReturn;}
	/// Ask managers to rescale their agent's mould coefficients of a constant factor @see Manager_base::rescaleMoulds()
	void                rescaleMoulds(double factor_h);
	/// Ask managers to create the behaviour system on their agents @see Manager_farmer::createBehaviours()
	void                createBehaviours();
	/// If managers need to fix something before the rimulation really begane, they can do it here
	void                endInit();

	/// Ask managers to prepare the period @see Manager_farmer::prepare()
	void                prepare();
	/// Ask managers to actually take their decisions @see Manager_farmer::act()
	void                act();
	/// Ask managers to update the data of their agents as loop in ending @see Manager_farmer::update()
	void                update();
	/// Ask managers to plan for the following year, including decision to leave the sector @see Manager_farmer::planNext()
	void                planNext();

	vector <Agent_base*> getAgentsByType(bool onlySpatial, string category_h="");

private:
	vector <Manager_base*>              managers; ///< Vector of all the managers of the various type of agents
	int                     uniqueAgentIDCounter; ///< Counter unique for each agent in the model
	int                   uniqueManagerIDCounter; ///< Counter for each manager in the model
};

#endif
