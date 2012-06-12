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
#include <iostream>

#include "IpIpoptApplication.hpp"

#include "MainProgram.h"
//#include "InputDocument.h"
//#include "RegData.h"
#include "ThreadManager.h"
#include "ipopt_nlp_problem.h"



//constructor
MainProgram::MainProgram(ThreadManager* MTHREAD_h)
{
	//input_filename = input_filename_h;
	MTHREAD = MTHREAD_h;
	// Creating objects for the program flow:
	// the regional data object..
	RegData *RD   = new RegData(MTHREAD);
	MTHREAD->setRDPointer(RD);
	MTHREAD->RD->setBaseDiretory(MTHREAD->getBaseDirectory());	
	MTHREAD->RD->loadInput(); // Unzip the ooffice input file and load it into memory
}

//distructor
MainProgram::~MainProgram()
{
}

/**
This is the main call of the program.
<br>It firstly create the objects (and keep track of them trough pointers) of the main functional objects of the program.
<br>Then it call the INIT object to do its jobs and when it ends, it gives control to SCD (Scheduler) for the year loops.
<br>Finally it clean-up and returns.
*/
void
MainProgram::run(){



using namespace Ipopt;

  // Create a new instance of your nlp
  //  (use a SmartPtr, not raw)
  SmartPtr<TNLP> mynlp = new ipopt_nlp_problem();

  // Create a new instance of IpoptApplication
  //  (use a SmartPtr, not raw)
  // We are using the factory, since this allows us to compile this
  // example with an Ipopt Windows DLL
  SmartPtr<IpoptApplication> app = IpoptApplicationFactory();

  // Change some options
  // Note: The following choices are only examples, they might not be
  //       suitable for your optimization problem.
  app->Options()->SetNumericValue("tol", 1e-7);
  app->Options()->SetStringValue("mu_strategy", "adaptive");
  app->Options()->SetStringValue("output_file", "ipopt.out");
  //app->Options()->SetStringValue("hessian_approximation", "limited-memory");
  //app->Options()->SetStringValue("derivative_test", "second-order");
  //app->Options()->SetStringValue("derivative_test_print_all", "yes");


  // The following overwrites the default name (ipopt.opt) of the
  // options file
  // app->Options()->SetStringValue("option_file_name", "hs071.opt");

  // Intialize the IpoptApplication and process the options
  ApplicationReturnStatus status;
  status = app->Initialize();
  if (status != Solve_Succeeded) {
    std::cout << std::endl << std::endl << "*** Error during initialization!" << std::endl;
    //return (int) status; // here the abort
  }

  // Ask Ipopt to solve the problem
  status = app->OptimizeTNLP(mynlp);

  if (status == Solve_Succeeded) {
    std::cout << std::endl << std::endl << "*** The problem solved!" << std::endl;
  }
  else {
    std::cout << std::endl << std::endl << "*** The problem FAILED!" << std::endl;
  }

  
  exit(0);










	// GIS information and methods..
	Gis *GIS  = new Gis(MTHREAD);
	MTHREAD->setGISPointer(GIS);
	// a test object for various 0-effects tests (sandbox)..
	Sandbox* TEST = new Sandbox(MTHREAD);
	TEST->test();
	// the agent super manager, will deal with all the manager objects..
	SuperAgentManager *SAM  = new SuperAgentManager(MTHREAD);
	MTHREAD->setSAMPointer(SAM);
	// the Init object, it schedule the pre-simulation phase..
	Init *INIT = new Init(MTHREAD);
	MTHREAD->setINITPointer(INIT);
	// the scheduler object. It manage the simulation loops..
	Scheduler *SCD  = new Scheduler(MTHREAD);
	MTHREAD->setSCDPointer(SCD);
	// manage the printing of data needed for scenario-analisys. The "message output" (needed to see "what is it happening?" are instead simply printed with msgOut()..
	Output *DO = new Output(MTHREAD);
	MTHREAD->setDOPointer(DO);
	
	// Creating an istance of regional data and assigning the first data (the input_file /output_directory)
	//msgOut(MSG_DEBUG, input_filename);

	//MTHREAD->RD->parseInitialConfigFile();




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
