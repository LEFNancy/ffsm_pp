/***************************************************************************
 *   Copyright (C) 2007-2008 by Antonello Lobianco                              *
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
#ifndef SANDBOX_H
#define SANDBOX_H

#include <QtCore>
#include <QThread>
#include <QString>

#include "BaseClass.h"


/**
This is a test class used when I need to test some C++ or other library functions. It can safelly be removed by the project if needed.

	@author Antonello Lobianco <antonello@regmas.org>
*/

struct TestStructure;

class Sandbox : public BaseClass {

public:
                             Sandbox(ThreadManager* MTHREAD_h);
							 Sandbox();
                            ~Sandbox();

	template <class T> T     getSetting(string name_h, int type);
	template <class T> vector <T> getVectorSetting(string name_h, int type);
	template <class T> T      test2(const std::string& s); // e.g. int x = test<int>("123");
	void                     printAString(string what){cout << "You printed: "<< what << endl;};
	vector <TestStructure*>  getTestStructure();
	void                     testThreads();
	void                     basicTest(); ///< Simple tests that doesn't require anything else (are encapsulated) and so they can be run at the beginning of the program. Normally empty
	void                     fullTest(); ///< Tests that require a full sandbox object including MTHREAD. Normally empty
	void                     testIpopt();
	int                      testAdolc();
	void                     testPartMatching(); ///< How to partial matching the key of a map


private:
	vector <TestStructure>  testVector;
	// How to partial matching the key of a map
//	map<string, string>::iterator FindPrefix(const map<string, string>& map, const string& search_for);
	void                       testSearchMap(const map<string, string>& map, const string& search_for);

};

struct TestStructure {

	int                                        i;
	string                                     s;
	double                           cachedOffer;
	double                                random;
};


class testThread : public QThread {
	Q_OBJECT

public:
	testThread();
	void                assignJob(TestStructure* agent_h);

protected:
	void run();

private:
	volatile TestStructure* agent;
};



#endif
