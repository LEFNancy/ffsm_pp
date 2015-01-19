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
#ifndef MAINPROGRAM_H
#define MAINPROGRAM_H

// standard include
#include <cstdlib>
#include <string>

// regmas headers...
#include "BaseClass.h"
#include "ModelData.h"
#include "Gis.h"
#include "Init.h"
#include "Scheduler.h"
#include "Sandbox.h"
#include "Output.h"
#include "ModelCore.h"
#include "ModelCoreSpatial.h"
#include "Carbon.h"

/// Main program scheleton. It control the flow of the program.

/**
There is only one istance of this class. It is responsable to load the setting files, call the Init class, "speack" with the Scheduler and finally end the program.
@author Antonello Lobianco
*/
class MainProgram: public BaseClass {

public:
                      MainProgram(ThreadManager* MTHREAD);
             ~MainProgram();
  void                run(); ///< Run the program

};

#endif
