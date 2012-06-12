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
#ifndef MANAGERBASE_H
#define MANAGERBASE_H

// standard headers
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>

// regmas headers...
#include "Agent_base.h"

using namespace std;
class SuperAgentManager; //forward declaration

/// Abstract base class for the managers of agents (interface agent-side)

/**
This is the interface agent_side. All (or most.. now we'll see) function should be "virtual", as they should be
coded according with the specific agent type.
<br>This is the base class for all the manager objects:
<br>Manager_base --> manager_nospacial          --> ...
<br>Manager_base --> manager_spaciallyExplicit  --> ...
<br>Regional data is declared "const", leading to a read-only way. To write back to the environment must be used specialized functions.
@author Antonello Lobianco
*/
class Manager_base: public BaseRegmas{

public:
	                    Manager_base(ThreadManager* MTHREAD_h, string name_h);
	virtual            ~Manager_base();

	/**
	This abstract virtual function when applyed to the various manager, make them to rise their agents from their moulds according to their specific code.
	*/
	virtual void        riseMyAgents()=0;                 ///< Rise the agents (mostly from the moulds)
	/// Create new layers, locate the managed agents in the space and assign the plots to agents;
	virtual void        locateMyAgents()=0;
	/**
	Abstract virtual function to be ran on init time to assign objects to each agents according to their mould and each manager rules.
	*/
	virtual void        assignObjectsToAgents();            ///< Assign non-spacial objects to agents
	virtual void        assignSpatialObjectsToAgents(){};   ///< Assign spacial objects to agents. Doesn't do anything if agent is not spatial.
	virtual void        createBehaviours()=0;
	virtual void        endInit(){};                        ///< Individual managers can skip this function if they don't have anything to set before simulation
	virtual void        prepare()=0;
	virtual void        act()=0;
	virtual void        update()=0;
	virtual void        planNext()=0;

	// data member access..
	int                 getNewMouldLocalID(){int toReturn=mouldLocalIDCounter; mouldLocalIDCounter++; return toReturn;}
	int                 getNewAgentLocalID(){int toReturn=agentLocalIDCounter; agentLocalIDCounter++; return toReturn;}
	string              getName(){return name;};
	vector <Agent_base*>getMyAgents(){return managedAgents;}; ///< Return all managed agents
	int                 countMyAgents(){return managedAgents.size();};

	Agent_base *        getAgentByLocalID(double localID_h); ///< Return the pointer to a managed agent gived its ID;
	vector <Agent_base*>getRandomAgents(int size_h); ///< Randomly return size_h pointers to its managed agents;
	/**
	This abstract virtual function when applyed to the various manager, make them to "create" the moulds for their agents according to their specific code	
	*/
	virtual void        setAgentMoulds()=0;               ///< Init the moulds
	void                rescaleMoulds(double factor_h);   ///< Rescale the mould coefficients of a constant factor (e.g. for the subregion)
	void                setName(string name_h){name=name_h;};
	vector<int>         getMouldIDs();
	int                 getMouldLocalIDCounter(){return mouldLocalIDCounter;};
	int                 getManagerID(){return managerID;};

protected:
	vector <Agent_base*>        agentsMoulds; ///< Vector of pointers to their managed agents (moulds)
	vector <Agent_base*>       managedAgents; ///< Vector of pointers to their managed agents (actual agents)
	vector <Agent_base*>       removedAgents; ///< Vector of pointers to agents "removed" from the model
	int                  mouldLocalIDCounter; ///< each mould has its own local ID (unique for each agent typology)
	int                  agentLocalIDCounter; ///< each agent has its own local ID (unique for each agent typology)
	string                              name; ///< name of the category of agents (eg. "farmers")
	int                            managerID; ///< Id of the manager
	Agent_base*               collectorAgent; ///< This agent collects (And become the owner) of all spatial objects left by the leaving agents. Is agent_base type (an not agent_space) as in the future it could be used for other things as well.

};
#endif
