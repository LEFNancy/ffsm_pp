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

#include "ModelRegion.h"

/**
The constructor of REGION instances want:
@param MTHREAD_h Pointer to the main thread manager
*/
ModelRegion::ModelRegion(ThreadManager* MTHREAD_h){
    MTHREAD=MTHREAD_h;
}

ModelRegion::~ModelRegion(){
}

/**
Childrens are all the lvel-1 region that are parts of this region.
In case that more levels will be developed in future version of this program, this still give only the direct children.
*/
void
setChildren(){

};

double
getVolumes(){
	//TODO;
	return 0;
};

vector<double>
getVolumes(int fType_h){
	//TODO;
	vector<double> toReturn;
	return toReturn;
};

vector < vector <double> >
getVolumes(int fType_h, string dClass_h){
	//TODO;
	vector < vector <double> > toReturn;
	return toReturn;
};

