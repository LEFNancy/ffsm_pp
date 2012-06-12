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
#ifndef MANAGERSPACE_H
#define MANAGERSPACE_H

// standard headers
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>

// regmas headers...
#include "Manager_base.h"

class Agent_space;

/// Derived class for managers of agents WITH space dimension

/**
This is the interface agent_side. All (or most.. now we'll see) function should be "virtual", as they should be
coded according with the specific agent type. 
<br>This is the base class for all the manager objects with space geolocalization: 
<br>Manager_base --> Manager_nospace  --> ...
<br>Manager_base --> manager_space    --> ... 
@author Antonello Lobianco
*/
class Manager_space : public Manager_base {

public:
	                    Manager_space(ThreadManager* MTHREAD_h, string name_h);
	virtual            ~Manager_space();
	void                assignSpatialObjectsToAgents();
	virtual void        setAgentMoulds()=0;
	/// Assign a plot to a specific agent. @param assignmentType: plot type: PLOTS_OWNED, PLOTS_RENTED(d)
	virtual void        assignPlot(Pixel* plot_h, Agent_space* agent_h, double rentalCost=0, int assignmentType=PLOTS_RENTED)=0;
	virtual void        landAllocation()=0; /// renting decisions

protected:
};

#endif
