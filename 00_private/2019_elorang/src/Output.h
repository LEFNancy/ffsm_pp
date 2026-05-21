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
#ifndef STDOUTPUT_H
#define STDOUTPUT_H

// Core C++ headers
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <time.h>
#include <tr1/array>

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
    void                commonInit();
    void                initOutputMaps();
    void                initOutputForestData();
    void                initOutputProductData();
    void                initOptimisationLog();
    void                initDebugOutput();
    void                initDebugPixelValues();
    void                initCarbonBalance();
  void                print(bool earlyPrint=true); ///< Print output. If earlyPrinting it doesn't print some stuff for which we don't yet have values
    void                printMaps();
    void                printForestData(bool finalFlush=false);
    void                printProductData(bool finalFlush=false);
    void                printCarbonBalance();
  void                printFinalOutput();
  void                printDebugOutput();
  void                printDetailedHV(map<tr1::array<string, 4>,double> hVol_byPrd);
  void                printDebugPixelValues();
  void                printOptLog(bool optimal, int &nIterations, double &obj);
  char                getOutputFieldDelimiter();
  void                cleanScenario(string fileName, string scenarioName, char d);

    vector <vector < vector <vector <vector <double> > > > > expReturnsDebug; ///< l2_region, for type, d.c., pr prod, variable name
    vector <string>       expReturnsDebugVariables;

private:
  int                                      oLevel;
  char                                          d;
  int                                      inYear;
  int                                      nYears;
  string                                  baseDir;
  string                                     oDir;
  string                             scenarioName;
  string                                 oFileExt;
  bool                                 oHRedeable;
  bool                                oSingleFile;
  vector<int>                              oYears; // list of output years for data
  vector<int>                          mapsOYears; // list of output years for maps
  int                                   wRegId_l1;
  int                                   wRegId_l2;
  string                              outFileName;
  vector <string>                 outForVariables;
  vector <string>                outProdVariables;
  bool                                   oForData;
  bool                                  oProdData;
  bool                                oCarbonData;
  int                                outStepRange;
  bool                   forestDiamDetailedOutput;
  vector<string>                           priPr;
  vector<string>                           secPr;
  vector<string>                           allPr;
  vector<int>                           l1regIds;
  vector < vector <int> >                    l2r;
  vector <string>                         fTypes;
  vector <string>                       dClasses; /// includes an empty string for variables without diameter attribute
  vector <string>                      pDClasses; ///< production diameter classes: exclude the fist diameter class below 15 cm
  int                                     nPriPr;
  int                                     nSecPr;
  int                                     nAllPr;
  int                                       nL2r;
  string                             logFilename;
  string                           debugFilename;
  string                   debugPxValuesFilename;
  bool                                    spMode; // spatial mode
};
#endif
