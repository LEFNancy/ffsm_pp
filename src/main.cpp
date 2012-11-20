/***************************************************************************
 *   Copyright (C) 2012 by Laboratoire d'Economie Forestière               *
 *   antonello@regmas.org                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <iostream>
#include <string>

#include "anyoption.h"


#include <QApplication>

#include "Sandbox.h"
#include "MainWindow.h"
#include "ThreadManager.h"

// HTML code for the home page of the doxygen-generated documentation (Reference Manual)...
#include "../doc/referenceManual/mainPage.h"

using namespace std;

int main(int argc, char *argv[]){


	cout << endl;
	cout << "*******************************************************************" << endl;
	cout << "***    !! Welcome to FFSM - The Forest Sector Simulator !!    ***" << endl;
	cout << "***      For info & doc: http://www.ffsm-project.org/doc      ***" << endl;
	cout << "***         Compiled on: " << __DATE__ << " - " << __TIME__ << "                 ***" << endl;
	cout << "*******************************************************************" << endl<<endl;

	// Running "simple testing" that can be done at this early time
	Sandbox TEST;
	int debug=0;
	TEST.basicTest(); // normally this is an empty function, used only to place temporary in-developmentr tests
	//TEST.runSimpleTests();
	// TEST.testIpopt();
	//debug = TEST.testAdolc();
	//cout << "Early debug value: " << debug << endl;


	QDir dir;
	QString currentDir = dir.currentPath();
	QString inputFileName = "";
	QString scenarioName = "";


	// 1. CREATE AN OBJECT
	AnyOption *opt = new AnyOption();


	// 2. SET PREFERENCES
	//opt->setVerbose(); // print warnings about unknown options
	//opt->autoUsagePrint(true); // print usage for bad options

	// 3. SET THE USAGE/HELP
	opt->addUsage( "*** FFSM - Forest Sector Simulator ***" );
	opt->addUsage( "Usage: " );
	opt->addUsage( "" );
	opt->addUsage( " -h  --help			Prints this help " );
	opt->addUsage( " -c  --console 			Run in console mode (no gui, default: false) " );
	opt->addUsage( " -i  --input_file [input_file_name]	Input file (relative path, default: 'data/ffsmInput.ods') " );
	opt->addUsage( " -s  --scenario [scenario_name]	Scenario name (default: the first defined in the input file) " );
	opt->addUsage( "" );
	opt->addUsage( "Notes:");
	opt->addUsage( "	- input_file and scenario options have no effect in GUI mode;" );
	opt->addUsage( "	- the working directory is the base path relative to the input file." );
	opt->addUsage( "" );
	opt->addUsage( "Read installed documentation or browse it at http://www.ffsm-project.org/doc." );
	opt->addUsage( "" );

	// 4. SET THE OPTION STRINGS/CHARACTERS
	opt->setFlag(  "help", 'h' );
	opt->setFlag(  "console", 'c' );
	opt->setOption(  "input_file", 'i' );
	opt->setOption(  "scenario", 's' );

	// 5. PROCESS THE COMMANDLINE
	opt->processCommandArgs( argc, argv );

	// 6. GET THE VALUES
	if( opt->getFlag( "help" ) || opt->getFlag( 'h' ) || opt->getArgc() >0 ) {
		opt->printUsage();
		delete opt;
		return EXIT_FAILURE;
	}

	if( opt->getValue( 'i' ) != NULL  || opt->getValue( "input_file" ) != NULL  ){
			QString tempdata(opt->getValue( 'i' ));
			inputFileName = currentDir + "/" + tempdata;
	}
	else {
			inputFileName = currentDir + "/data/ffsmInput.ods";
	}

	if( opt->getValue( 's' ) != NULL  || opt->getValue( "scenario" ) != NULL  ){
			scenarioName = opt->getValue( 's' );
	}

	if( opt->getFlag( 'c' ) || opt->getFlag( "console" ) ){
		ThreadManager  modelMainThread;
		modelMainThread.runFromConsole(inputFileName,scenarioName);
	}
	else {
		QApplication app(argc, argv);
		MainWindow mainWin;
		mainWin.show();
		return app.exec();
	}
	delete opt;
}
