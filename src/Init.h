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
#ifndef STDINIT_H
#define STDINIT_H

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>

// RegMAS include stuff..
#include "BaseClass.h"
#include "RegData.h"
#include "Gis.h"

/// %Init the environment, the objects and the agents of the model
/**
The Init class is responsable to ask to the various objects to Init themself, in a 7-steps procedures.
<br>The basic idea is to first init the environment: options, settings and space.
<br>Then objects and agents are mould up, objects are assigned to agents and finally agents and objects are collocated in the space.
@author Antonello Lobianco
*/
class Init: public BaseClass{
	
public:
	
	                    Init(ThreadManager* MTHREAD_h);
	                   ~Init();
	/// Wrapper to the correct setInitLevelX()
	void                setInitLevel(int level_h);
	/// Unused, reserver for future use
	void                setInitLevel0();
	/// Setting up the space, the model objects and the agents (definitions only)
	void                setInitLevel1();
	/// Unused, reserver for future use
	void                setInitLevel2();
	/// Linking object to agents and assigning space proprieties to objects and agents 
	void                setInitLevel3();
	/// Unused, reserver for future use
	void                setInitLevel4();
	/// Simulation start
	void                setInitLevel5();
	/// End of simulation (e.g. print summary statistics)
	void                setInitLevel6();
	int                 getInitState(){return InitState;};

private:
	int                                InitState; ///< One of the 7 possible init states (0..6)
};

#endif
