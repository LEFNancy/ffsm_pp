/***************************************************************************
 *   Copyright (C) 2015 by Laboratoire d'Economie Forestière               *
 *   http://ffsm-project.org                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version, given the compliance with the     *
 *   exceptions listed in the file COPYING that is distribued together     *
 *   with this file.                                                       *
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
#ifndef STDSCHEDULER_H
#define STDSCHEDULER_H

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>

// regmas headers..
#include "BaseClass.h"
//#include "ModelData.h"

/// Manage the yearly  loops

/**
This class is responsable to manage the time-dimension of the program.
<br>It start its job when Init has ended and schedule the various operation to be done during the year loops.
@author Antonello Lobianco
*/
class Scheduler: public BaseClass{

public:
                      Scheduler(ThreadManager* MTHREAD_h);
                     ~Scheduler();
  void                run();
  int                 getIteration(){return iteration;};
  int                 getYear(){return year;}
  void                setYear(const int& year_h){year = year_h;}
  void                advanceYear(){year += 1;}

private:
  int                                iteration;
  int                                     year;

};

#endif
