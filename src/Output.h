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

// Core C++ headers
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <sstream>

//regmas headers
#include "BaseClass.h"


class Pixel;


/// %Output methods

/**
Class responsable to output  the data, both as all kind of log as well as georeferenciated one.
@author Antonello Lobianco
*/
class Output: public BaseClass{
public:
	                    Output(ThreadManager* MTHREAD_h); ///< Constructor
	                   ~Output();

	void                initOutput();
		void                initOutputAggregatedData();
		void                initOutputMaps();
		void                initOutputDetailedData();
	void                collectData();
	void                print();
		void                printAggregatedData();
		void                printMaps();
		void                printDetailedData();
		void                printDebugMatrices();
	char                getOutputFieldDelimiter();
	void                cleanScenario(string fileName, string scenarioName, char d);

private:
	
};

#endif
