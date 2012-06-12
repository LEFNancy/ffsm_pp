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
#include "Manager_space.h"
#include "Agent_space.h"
#include "Pixel.h"
#include "ThreadManager.h"

//constructor
Manager_space::Manager_space(ThreadManager* MTHREAD_h, string name_h): Manager_base(MTHREAD_h, name_h){

}

//destructor
Manager_space::~Manager_space(){

}

/**
Retrive the pixels owned and rented by each farms and call on each of them initObjects().
*/
void
Manager_space::assignSpatialObjectsToAgents(){
	msgOut(MSG_DEBUG,"Manager_space::assignSpatialObjectsToAgents()");
	/*for (uint i=0;i<managedAgents.size();i++){
		Agent_space * currentAgent = ((Agent_space*)managedAgents.at(i));
		vector <Pixel *> pixels = currentAgent->getPlots();
		for (uint j=0;j<pixels.size();j++){
			pixels.at(j)->initObjects();
		}
	}*/
	// 20080201, changed Antonello: really spatial objects are assigned to individual plots, not to their agents. It is wrong to iterate trough agents.. plots that are not rented nor owned by any farmer in this way arenot initializzated.. resulting e.g. in lot of vineyards on wine suitable plots on the first year
	vector <Pixel *> pixels = MTHREAD->GIS->getAllPlots();
	for(uint i=0;i<pixels.size();i++){
		pixels.at(i)->initObjects();
	}
}

