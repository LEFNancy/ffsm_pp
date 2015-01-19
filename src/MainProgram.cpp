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
#include <iostream>
#include <clocale>

#include "MainProgram.h"
#include "ThreadManager.h"
#include "Opt.h"




//constructor
MainProgram::MainProgram(ThreadManager* MTHREAD_h)
{
  //input_filename = input_filename_h;
  MTHREAD = MTHREAD_h;
  // Creating objects for the program flow:
  // the regional data object..
  ModelData *MD   = new ModelData(MTHREAD);
  MTHREAD->setMDPointer(MD);
  MTHREAD->MD->setBaseDiretory(MTHREAD->getBaseDirectory());
  MTHREAD->MD->loadInput(); // Unzip the ooffice input file and load it into memory

}

//distructor
MainProgram::~MainProgram(){

}

/**
This is the main call of the program.
<br>It firstly create the objects (and keep track of them trough pointers) of the main functional objects of the program.
<br>Then it call the INIT object to do its jobs and when it ends, it gives control to SCD (Scheduler) for the year loops.
<br>Finally it clean-up and returns.
*/
void
MainProgram::run(){

  setlocale(LC_ALL, "C"); // force to use the dot as digital separator also if we are running under the GUI

  // GIS information and methods..
  Gis *GIS  = new Gis(MTHREAD);
  MTHREAD->setGISPointer(GIS);
  // a test object for various 0-effects tests (sandbox)..
  Sandbox* TEST = new Sandbox(MTHREAD);
  MTHREAD->setTestPointer(TEST);
  // the Init object, it schedule the pre-simulation phase..
  Init *INIT = new Init(MTHREAD);
  MTHREAD->setINITPointer(INIT);
  // the scheduler object. It manage the simulation loops..
  Scheduler *SCD  = new Scheduler(MTHREAD);
  MTHREAD->setSCDPointer(SCD);
  // the core of the model
  ModelCore *CORE = new ModelCore(MTHREAD);
  MTHREAD->setCOREPointer(CORE);
  // the core of the model (spatial version)
  ModelCoreSpatial *SCORE = new ModelCoreSpatial(MTHREAD);
  MTHREAD->setSCOREPointer(SCORE);
  // the market optimisation algorithm
  Opt *OPT = new Opt(MTHREAD);
  MTHREAD->setOPTPointer(OPT);
  // manage the printing of data needed for scenario-analisys. The "message output" (needed to see "what is it happening?" are instead simply printed with msgOut()..
  Output *DO = new Output(MTHREAD);
  MTHREAD->setDOPointer(DO);
  // the carbon balance
  Carbon *CBAL = new Carbon(MTHREAD);
  MTHREAD->setCBALPointer(CBAL);

  // Creating an istance of INIT and delegating to it the Initialization phase..
  MTHREAD->INIT->setInitLevel(1); // Initial environment setting and agent rising
  refreshGUI();
  MTHREAD->INIT->setInitLevel(3); // assigning resources to agents and evenutal env reallocation
  refreshGUI();
  MTHREAD->INIT->setInitLevel(5); // starting simulations. Once INIT has ended it is the turn of SCD (Scheduler) to manage the simulation...
  refreshGUI();
  MTHREAD->INIT->setInitLevel(6); // ending simulations
  refreshGUI();

  // Deleting the pointers...
  // 20070102: if I delete the pointers I can not access the legend after simulation has ended
  // 20070109: pointers (e.g. INIT) are deleted in ThreadManager when a new simulation start
}

