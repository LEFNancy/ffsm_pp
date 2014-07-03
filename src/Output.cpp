/***************************************************************************
 *   Copyright (C) 2012 by Laboratoire d'Economie Forestière               *
 *   http://www.nancy.inra.fr/lef_eng                                      *
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
#include <fstream>
 
#include "Output.h"
#include "ThreadManager.h"
#include "Scheduler.h"
#include "ModelData.h"
#include "Gis.h"
#include "Carbon.h"

typedef map<string, vector <double> > DataMap;
typedef pair<string, vector <double> > DataPair;


Output::Output(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
}

Output::~Output(){
}

// ---- functions ... ----------------------


void
Output::initOutput(){
	commonInit();
	initOutputMaps();
  initDebugOutput();
	initOutputForestData();
	initOutputProductData();
	initOptimisationLog();
  initCarbonBalance();
}


void
Output::commonInit(){
	oLevel       = MTHREAD->MD->getIntSetting("outputLevel");
	d            = getOutputFieldDelimiter();
	inYear       = MTHREAD->MD->getIntSetting("initialYear");
	nYears       = MTHREAD->MD->getIntSetting("simulationYears");
	baseDir      = MTHREAD->MD->getBaseDirectory();
	oDir         = MTHREAD->MD->getOutputDirectory();
//    bool initSeed = MTHREAD->MD->getBoolSetting("newRandomSeed");
//    if (initSeed){
//        uniform_int_distribution<> d(1, 1000000);
//        int random = d(*MTHREAD->gen);
//        scenarioName = MTHREAD->getScenarioName()+"_"+i2s(random);
//    } else {
//        scenarioName = MTHREAD->getScenarioName();
//    }
  scenarioName = MTHREAD->getScenarioName();
  //scenarioName = "debug2";
	oFileExt     = MTHREAD->MD->getStringSetting("outputFileExtension");
	oHRedeable   = MTHREAD->MD->getBoolSetting("outputHumanReadable");
	oSingleFile  = MTHREAD->MD->getBoolSetting("outputSingleFile");
	oYears       = MTHREAD->MD->getIntVectorSetting("outYears");
	wRegId_l1    = MTHREAD->MD->getIntSetting("worldCodeLev1");
	wRegId_l2    = MTHREAD->MD->getIntSetting("worldCodeLev2");
	outForVariables          = MTHREAD->MD->getStringVectorSetting("outForVariables");
	outProdVariables         = MTHREAD->MD->getStringVectorSetting("outProdVariables");
	dClasses                 = MTHREAD->MD->getStringVectorSetting("dClasses");
  pDClasses.insert(pDClasses.end(), dClasses.begin()+1, dClasses.end() ); // production diameter classes
  dClasses.push_back(""); // needed for reporting of variables without diameter attribute
	outStepRange             = MTHREAD->MD->getIntSetting("outStepRange");
	forestDiamDetailedOutput = MTHREAD->MD->getBoolSetting("forestDiamDetailedOutput");
	fTypes                   = MTHREAD->MD->getForTypeIds();

	priPr    = MTHREAD->MD->getStringVectorSetting("priProducts");
	secPr    = MTHREAD->MD->getStringVectorSetting("secProducts");
	allPr    = priPr;
	allPr.insert( allPr.end(), secPr.begin(), secPr.end() );
	nPriPr   = priPr.size();
	nSecPr   = secPr.size();
	nAllPr   = allPr.size();
  l1regIds = MTHREAD->MD->getRegionIds(1, true);
  nL2r     = MTHREAD->MD->getRegionIds(2, true).size();
  spMode   = MTHREAD->MD->getBoolSetting("usePixelData");
    //if(spMode) {
    //   pxIds =   getXyNPixels();
    //}


	for(uint i=0;i<l1regIds.size();i++){
		std::vector<int> l2ChildrenIds;
		ModelRegion* l1Region = MTHREAD->MD->getRegion(l1regIds[i]);
		std::vector<ModelRegion*> l2Childrens = l1Region->getChildren(true);
		for(uint j=0;j<l2Childrens.size();j++){
			l2ChildrenIds.push_back(l2Childrens[j]->getRegId());
		}
		if(l2ChildrenIds.size()){
			l2r.push_back(l2ChildrenIds);
		}
	}

}

void
Output::initOptimisationLog(){
	if(oLevel<OUTVL_AGGREGATED) return;

    if (oSingleFile){
         logFilename = baseDir+oDir+"optimisationLogs/optimisationLogs.txt";

    } else {
         logFilename = baseDir+oDir+"optimisationLogs/"+scenarioName+".txt";
    }


	ifstream in(logFilename.c_str(), ios::in);
	if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
		in.close();
		cleanScenario(logFilename, scenarioName, d);
		ofstream out(logFilename.c_str(), ios::app);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+logFilename+" for writing.");}
		time_t now;
		time(&now);
		struct tm *current = localtime(&now);
		string timemessage = i2s(current->tm_hour)+":"+i2s(current->tm_min)+":"+ i2s(current->tm_sec);
		out << scenarioName << d << "0000" << d << timemessage << d << d << d <<"\n";
		out.close();
		return;
	} else { // file doesn't exist
		in.close();
		ofstream out(logFilename.c_str(), ios::out);
		if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+logFilename+" for writing.");}
		out << "scenario" << d << "year" << d << "time" << d << "opt flag" << d << "iterations" << d <<"\n";
		time_t now;
		time(&now);
		struct tm *current = localtime(&now);
		string timemessage = i2s(current->tm_hour)+":"+i2s(current->tm_min)+":"+ i2s(current->tm_sec);
		out << scenarioName << d << "0000" << d << timemessage << d << d << d <<"\n";
		out.close();
	}
}

void
Output::initDebugOutput(){
    if(oLevel<OUTVL_ALL) return;

    // init debugging the expected returns...
    if(spMode) return;
    expReturnsDebugVariables.push_back("hVol_byUPp");
    expReturnsDebugVariables.push_back("hV_byFT");
    expReturnsDebugVariables.push_back("finalHarvestFlag");
    expReturnsDebugVariables.push_back("pondCoeff");
    expReturnsDebugVariables.push_back("pW");
    expReturnsDebugVariables.push_back("cumTp");
    expReturnsDebugVariables.push_back("vHa");
    expReturnsDebugVariables.push_back("expectedReturns");
    expReturnsDebugVariables.push_back("weightedAvgCompModeFlag");

    if (oSingleFile){
         debugFilename = baseDir+oDir+"debugs/debugOut.csv";
    } else {
         debugFilename = baseDir+oDir+"debugs/debugOut_"+scenarioName+".csv";
    }

    ifstream in(debugFilename.c_str(), ios::in);
    if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
        in.close();
        cleanScenario(debugFilename, scenarioName, d);
        return;
    } else { // file doesn't exist
        in.close();
        ofstream out(debugFilename.c_str(), ios::out);
        if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+debugFilename+" for writing.");}
        out << "scenario" << d << "year" << d << "region or pixel" << d << "forType" << d << "freeDim" << d << "prod" << d << "parName" << d << "value" << d <<"\n";
        out.close();
    }
}


void
Output::initOutputForestData(){
	if(oLevel<OUTVL_AGGREGATED) return;

	if(oSingleFile){
		outFileName = baseDir+oDir+"results/forestData"+oFileExt;
		ifstream in(outFileName.c_str(), ios::in);
		if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
			in.close();
			cleanScenario(outFileName, scenarioName, d);
			return;
		} else {
			in.close();
		}
	} else {
		outFileName = baseDir+oDir+"results/forestData_"+scenarioName+oFileExt;
	}

	ofstream out(outFileName.c_str(), ios::out);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
    out << "scen" << d << "parName" << d << "country" << d << "region" << d << "forType" << d << "freeDim" << d;
	if(oHRedeable){
		for(int i=0;i<nYears;i++){
			out << i+inYear << d;
		}
	} else {
		out << "year" << d << "value" << d;
	}
	out << "\n";
	out.close();
}

void
Output::initOutputProductData(){
	if(oLevel<OUTVL_AGGREGATED) return;

	if(oSingleFile){
		outFileName = baseDir+oDir+"results/productData"+oFileExt;
		ifstream in(outFileName.c_str(), ios::in);
		if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
			in.close();
			cleanScenario(outFileName, scenarioName, d);
			return;
		} else {
			in.close();
		}
	} else {
		outFileName = baseDir+oDir+"results/productData_"+scenarioName+oFileExt;
	}

	ofstream out(outFileName.c_str(), ios::out);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
	out << "scen" << d << "parName" << d << "country" << d << "region" << d << "prod" << d << "freeDim" << d;
	if(oHRedeable){
		for(int i=0;i<nYears;i++){
			out << i+inYear << d;
		}
	} else {
		out << "year" << d << "value" << d;
	}
	out << "\n";
	out.close();
}

void
Output::initCarbonBalance(){

  if(oSingleFile){
    outFileName = baseDir+oDir+"results/carbonBalance"+oFileExt;
    ifstream in(outFileName.c_str(), ios::in);
    if(in.is_open()) { // file exist, no need to initializate it, but we are gonna clean it of previous data of the same scenario if present...
      in.close();
      cleanScenario(outFileName, scenarioName, d);
      return;
    } else {
      in.close();
    }
  } else {
    outFileName = baseDir+oDir+"results/carbonBalance_"+scenarioName+oFileExt;
  }

  ofstream out(outFileName.c_str(), ios::out);
  if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for reading.");}
    out << "scen" << d << "country" << d << "region" << d << "balItem" << d;
  //if(oHRedeable){
  //	for(int i=0;i<nYears;i++){
  //		out << i+inYear << d;
  //	}
  //} else {
    out << "year" << d << "value" << d;
  //}
  out << "\n";
  out.close();
}


/**
Resetting the list of printed layers and the scenario name..
<br>Printing scenario name for post-processing scripts
*/
void
Output::initOutputMaps(){
	if(oLevel<OUTVL_MAPS) return;
	string mapBaseDirectory = baseDir+oDir+"maps/";
    string filenameToSaveScenarioName = mapBaseDirectory+"scenarioNames/"+scenarioName;
    string filenameListIntLayers = mapBaseDirectory+"integerListLayers/"+scenarioName;
    string filenameListFloatLayers = mapBaseDirectory+"floatListLayers/"+scenarioName;

	// printing the scenario name in the "scenarioName file"...
	ofstream outSN(filenameToSaveScenarioName.c_str(), ios::out);
	if (!outSN){ msgOut(MSG_ERROR,"Error in opening the file "+filenameToSaveScenarioName+".");}
	outSN << scenarioName << "\n";
	outSN.close();
	// cleaning the "integerListLayers" and "floatListLayers" file...
	ofstream outi(filenameListIntLayers.c_str(), ios::out);
	outi.close();
	ofstream outf(filenameListFloatLayers.c_str(), ios::out);
	outf.close();
}

void
Output::print(){
	int cYear = MTHREAD->SCD->getYear();
	int initialSimulationYear = MTHREAD->MD->getIntSetting("initialOptYear");

	if (outStepRange != -1 && (cYear-initialSimulationYear)%outStepRange != 0 && cYear != (initialSimulationYear+nYears)-1 ) {
		cout << cYear << " not printed" << endl;
		return;
	}
	bool printThisYear = false;
	for(uint i=0;i<oYears.size();i++){
		if (outStepRange == -1 && oYears[i] == cYear) printThisYear = true;
	}
	if(outStepRange == -1 && !printThisYear) return;


	cout << "printing " << cYear << endl;
	printMaps();
	MTHREAD->MD->setErrorLevel(MSG_NO_MSG);
	printForestData(false);
	printProductData(false);
  printCarbonBalance();
  printDebugOutput();
	MTHREAD->MD->setErrorLevel(MSG_ERROR);
}

void
Output::printMaps(){
	if(oLevel<OUTVL_MAPS) return;
	MTHREAD->GIS->printLayers();
	if(oLevel<OUTVL_BINMAPS) return;
	MTHREAD->GIS->printBinMaps();
}

void
Output::printFinalOutput(){
	// we do this only if we choosed the outputHumanReadable settings, as we flush the data all in ones at the end.
	// oterwise we flush data every year
	if(oHRedeable){
		MTHREAD->MD->setErrorLevel(MSG_NO_MSG);
		printForestData(true);
    printProductData(true);
		MTHREAD->MD->setErrorLevel(MSG_ERROR);
	}
}

void
Output::printForestData(bool finalFlush){

	if(oLevel<OUTVL_DETAILED) return;
	if(oHRedeable && !finalFlush) return;

	msgOut(MSG_INFO, "Printing forest data..");
	int currentYear = MTHREAD->SCD->getYear();
	if(oSingleFile){
		outFileName = baseDir+oDir+"results/forestData"+oFileExt;
	} else {
		outFileName = baseDir+oDir+"results/forestData_"+scenarioName+oFileExt;
	}
	ofstream out (outFileName.c_str(), ios::app);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for writing.");}
	double outvalue;
	for(uint v=0;v<outForVariables.size();v++){
		for (uint r1=0;r1<l2r.size();r1++){
			for (uint r2=0;r2<l2r[r1].size();r2++){
				for(uint ft=0;ft<fTypes.size();ft++){
					if(forestDiamDetailedOutput){
						for(uint dc=0;dc<dClasses.size();dc++){
							out << scenarioName << d;
							out << outForVariables[v] << d;
							out << l1regIds.at(r1) << d;
							out << l2r[r1][r2] << d;
							out << fTypes[ft] << d;
							out << dClasses[dc] << d;
							if (oHRedeable){
								for(int y=0;y<nYears;y++){
									outvalue = MTHREAD->MD->getForData(outForVariables[v],l2r[r1][r2],fTypes[ft],dClasses[dc],y+inYear);
									out << outvalue << d;
								}
								out << "\n";
							} else {
								outvalue = MTHREAD->MD->getForData(outForVariables[v],l2r[r1][r2],fTypes[ft],dClasses[dc]);
								out << currentYear << d;
								out << outvalue << d;
								out << "\n";
							}
						}
					} else {
						out << scenarioName << d;
						out << outForVariables[v] << d;
						out << l1regIds.at(r1) << d;
						out << l2r[r1][r2] << d;
						out << fTypes[ft] << d;
						out << d;
						if (oHRedeable){
							for(int y=0;y<nYears;y++){
								outvalue = MTHREAD->MD->getForData(outForVariables[v],l2r[r1][r2],fTypes[ft],DIAM_ALL,y+inYear);
								out << outvalue << d;
							}
							out << "\n";
						} else {
							outvalue = MTHREAD->MD->getForData(outForVariables[v],l2r[r1][r2],fTypes[ft],DIAM_ALL);
							out << currentYear << d;
							out << outvalue << d;
							out << "\n";
						}
					}
				}
			}
		}
	}
	/*
	DataMap::const_iterator i;
	string key;
	vector <double> values;
	string parName;
	int regId;
	string forType;
	string diamClass;
	for(i=MTHREAD->MD->forDataMap.begin();i!=MTHREAD->MD->forDataMap.end();i++){
		key = i->first;
		values = i->second;
		MTHREAD->MD->unpackKeyForData(key, parName, regId, forType, diamClass);
		ModelRegion* REG = MTHREAD->MD->getRegion(regId);
		// we don't want to output data from residual region unless it's the world region we are speaking of
		if(REG->getIsResidual() && !(regId==wRegId_l1 || regId==wRegId_l2)) continue;
		out << scenarioName << d;
		out << parName << d;
		if (REG->getRegLevel()==2){
			ModelRegion* pREG = MTHREAD->MD->getRegion(REG->getParRegId());
			out << pREG->getRegSName() << d;
			out << REG->getRegSName() << d;
		} else if (REG->getRegLevel()==1){
			out << REG->getRegSName() << d;
			out << d;
		} else {
			out << d << d;
		}
		out << forType << d;
		out << diamClass << d;
		if (oHRedeable){
			for(int y=0;y<nYears;y++){
				out << MTHREAD->MD->getTimedData(values,y+inYear) << d;
			}
			out << "\n";
		} else {
			out << currentYear << d;
			out << MTHREAD->MD->getTimedData(values,currentYear) << d;
			out << "\n";
		}
	}
	*/
	out.close();
}

void
Output::printProductData(bool finalFlush){

	if(oLevel<OUTVL_DETAILED) return;
	if(oHRedeable && !finalFlush) return;

	msgOut(MSG_INFO, "Printing market data..");
	int currentYear = MTHREAD->SCD->getYear();

	if(oSingleFile){
		outFileName = baseDir+oDir+"results/productData"+oFileExt;
	} else {
		outFileName = baseDir+oDir+"results/productData_"+scenarioName+oFileExt;
	}
	ofstream out (outFileName.c_str(), ios::app);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for writing.");}


  //11042	hardWSawnW	11083	0.00230651
  //11042	hardWSawnW	11082	0.0390874

  //if(MTHREAD->SCD->getYear() == 2007){
//  double test  = MTHREAD->MD->getProdData("rt",11042,"hardWSawnW",DATA_NOW);
//  double test2 = MTHREAD->MD->getProdData("rt",11042,"hardWSawnW",DATA_NOW,"11083");
//  double test3 = MTHREAD->MD->getProdData("rt",11042,"hardWSawnW",DATA_NOW,"11082");
//  cout << test << '\t' << test2 << '\t' << test3 << endl;
//  exit(0);
//  }

	double outvalue;
	for(uint v=0;v<outProdVariables.size();v++){
		for (uint r1=0;r1<l2r.size();r1++){
			for (uint r2=0;r2<l2r[r1].size();r2++){
				for(uint p=0;p<allPr.size();p++){

          if(outProdVariables[v]=="rt"){
            for(uint r2b=0;r2b<l2r[r1].size();r2b++){
              out << scenarioName << d;
              out << outProdVariables[v] << d;
              out << l1regIds.at(r1) << d;
              out << l2r[r1][r2] << d;
              out << allPr[p] << d;
              out << l2r[r1][r2b] << d;
              if (oHRedeable){
                for(int y=0;y<nYears;y++){
                  outvalue = MTHREAD->MD->getProdData(outProdVariables[v],l2r[r1][r2],allPr[p],y+inYear,i2s(l2r[r1][r2b]));
                  out << outvalue << d;
                }
                out << "\n";
              } else {
//                if(MTHREAD->SCD->getYear() == 2007 && l2r[r1][r2] == 11042 && allPr[p] == "hardWSawnW" && (l2r[r1][r2b]== 11083 || l2r[r1][r2b]== 11082 )){
//                outvalue = MTHREAD->MD->getProdData(outProdVariables[v],l2r[r1][r2],allPr[p],currentYear,i2s(l2r[r1][r2b]));
//                cout << outvalue << endl;
//                }
                outvalue = MTHREAD->MD->getProdData(outProdVariables[v],l2r[r1][r2],allPr[p],currentYear,i2s(l2r[r1][r2b]));
                out << currentYear << d;
                out << outvalue << d;
                out << "\n";
              }
            }
          } else {
            out << scenarioName << d;
            out << outProdVariables[v] << d;
            out << l1regIds.at(r1) << d;
            out << l2r[r1][r2] << d;
            out << allPr[p] << d;
            out << d;
            if (oHRedeable){
              for(int y=0;y<nYears;y++){
                outvalue = MTHREAD->MD->getProdData(outProdVariables[v],l2r[r1][r2],allPr[p],y+inYear);
                out << outvalue << d;
              }
              out << "\n";
            } else {
              outvalue = MTHREAD->MD->getProdData(outProdVariables[v],l2r[r1][r2],allPr[p]);
              out << currentYear << d;
              out << outvalue << d;
              out << "\n";
            }

          }
				}
			}
		}
	}




/*
	DataMap::const_iterator i;
	string key;
	vector <double> values;
	string parName;
	int regId;
	string prod;
	string freeDim;
	for(i=MTHREAD->MD->prodDataMap.begin();i!=MTHREAD->MD->prodDataMap.end();i++){
		key = i->first;
		values = i->second;
		MTHREAD->MD->unpackKeyProdData(key, parName, regId, prod, freeDim);
		ModelRegion* REG = MTHREAD->MD->getRegion(regId);
		// we don't want to output data from residual region unless it's the world region we are speaking of
		if(REG->getIsResidual() && !(regId==wRegId_l1 || regId==wRegId_l2)) continue;
		out << scenarioName << d;
		out << parName << d;
		if (REG->getRegLevel()==2){
			ModelRegion* pREG = MTHREAD->MD->getRegion(REG->getParRegId());
			out << pREG->getRegSName() << d;
			out << REG->getRegSName() << d;
		} else if (REG->getRegLevel()==1){
			out << REG->getRegSName() << d;
			out << d;
		} else {
			out << d << d;
		}
		out << prod << d;
		out << freeDim << d;
		if (oHRedeable){
			for(int y=0;y<nYears;y++){
				out << MTHREAD->MD->getTimedData(values,y+inYear) << d;
			}
			out << "\n";
		} else {
			out << currentYear << d;
			out << MTHREAD->MD->getTimedData(values,currentYear) << d;
			out << "\n";
		}
	}

*/
	out.close();
}





void
Output::printCarbonBalance(){

  int currentYear = MTHREAD->SCD->getYear();
  if (currentYear == inYear) {return;} // don't print carbon balance on first year, carbon balance containers has not yet been initialised

  msgOut(MSG_INFO, "Printing forest data..");

  if(oSingleFile){
    outFileName = baseDir+oDir+"results/carbonBalance"+oFileExt;
  } else {
    outFileName = baseDir+oDir+"results/carbonBalance_"+scenarioName+oFileExt;
  }
  ofstream out (outFileName.c_str(), ios::app);
  if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+outFileName+" for writing.");}
  double outvalue=0;

  vector<int> balItems {STOCK_INV,STOCK_EXTRA,STOCK_PRODUCTS,EM_ENSUB,EM_MATSUB,EM_FOROP};

  for (uint r1=0;r1<l2r.size();r1++){
    for (uint r2=0;r2<l2r[r1].size();r2++){
      int regId = l2r[r1][r2];
      for (uint b=0;b<balItems.size();b++){
        out << scenarioName << d;
        out << l1regIds.at(r1) << d;
        out << l2r[r1][r2] << d;
        string balItemString;
        switch(balItems[b]){
          case STOCK_INV: {
            balItemString = "STOCK_INV";
            outvalue = MTHREAD->CBAL->getStock(regId, balItems[b]);
            break;
          }
          case STOCK_EXTRA: {
            balItemString = "STOCK_EXTRA";
            outvalue = MTHREAD->CBAL->getStock(regId, balItems[b]);
            break;
          }
          case STOCK_PRODUCTS: {
            balItemString = "STOCK_PRODUCTS";
            outvalue = MTHREAD->CBAL->getStock(regId, balItems[b]);
            break;
          }
          case EM_ENSUB: {
            balItemString = "EM_ENSUB";
            outvalue = MTHREAD->CBAL->getCumSavedEmissions(regId, balItems[b]);
            break;
          }
          case EM_MATSUB: {
            balItemString = "EM_MATSUB";
            outvalue = MTHREAD->CBAL->getCumSavedEmissions(regId, balItems[b]);
            break;
          }
          case EM_FOROP: {
            balItemString = "EM_FOROP";
            outvalue = MTHREAD->CBAL->getCumSavedEmissions(regId, balItems[b]);
            break;
          }
        default:
            msgOut(MSG_CRITICAL_ERROR,"Unexpected balance item type in function printCarbonBalance");
        }
        out << balItemString << d;
        out << currentYear << d;
        out << outvalue << d;
        out << "\n";


      } // end bal items
    } // end r2
  } // end r1
  out.close();
}


char
Output::getOutputFieldDelimiter(){
	int delimiterID = MTHREAD->MD->getIntSetting("outputFieldDelimiter");
	switch (delimiterID) {
		case 1:
			return ',';
			break;
		case 2:
			return ';';
			break;
		case 3:
			return ':';
			break;
		case 4:
			return '\t';
			break;
		case 5:
			return ' ';
			break;
		default:
			msgOut(MSG_ERROR, "You have specified an unknow output file field delimiter. Using \";\".");
			return ',';
	}
}

void
Output::printOptLog(bool optimal, int &nIterations, double &obj){
	if(oLevel<OUTVL_AGGREGATED) return;

	ofstream out(logFilename.c_str(), ios::app);
	if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+logFilename+" for writing.");}
	time_t now;
	time(&now);
	struct tm *current = localtime(&now);
	string timemessage = i2s(current->tm_hour)+":"+i2s(current->tm_min)+":"+ i2s(current->tm_sec);
	out << scenarioName << d << MTHREAD->SCD->getYear() << d << timemessage << d << optimal;
	out << d << nIterations << d << obj << "\n";
	out.close();

}

void
Output::printDebugOutput(){
  if(oLevel<OUTVL_ALL) return;

  // print debugging the expected returns...

  if (!spMode && !expReturnsDebug.empty()){
    ofstream out (debugFilename.c_str(), ios::app);
    if (!out){ msgOut(MSG_CRITICAL_ERROR,"Error in opening the file "+debugFilename+" for writing.");}
    int currentYear = MTHREAD->SCD->getYear();
    vector <int> regIds2 = MTHREAD->MD->getRegionIds(2);

        for (uint r2=0;r2<regIds2.size();r2++){
            for(uint ft=0;ft<fTypes.size();ft++){
                for(uint dc=0;dc<(dClasses.size()-1);dc++){
                    for(uint pp=0;pp<priPr.size();pp++){
                        for(uint dv=0;dv<expReturnsDebugVariables.size();dv++){
                          // vector <vector < vector <vector <vector <double> > > > > expReturnsDebug;
                          double outputValue = expReturnsDebug.at(r2).at(ft).at(dc).at(pp).at(dv);
                          out << scenarioName << d;
                          out << currentYear << d;
                          out << regIds2[r2] << d;
                          out << fTypes[ft] << d;
                          out << dClasses[dc] << d;
                          out << priPr[pp] << d;
                          out << expReturnsDebugVariables[dv] << d;
                          out << outputValue << d;
                          out << "\n";
                        }
                    }
                }
            }
        }

  } // end initial condition checks
}



/**
This routine clean the output scenario from previous outputs of the defined scenario.
Other scenarios are untouched. The scenarioName must be in the first row.
@param filename Filename of the output file to clean
@param scenarioName Name of the scenario we are replacing
@param d Field delimiter. It must not be changed in the meantime (between the various scenarios)
*/
void
Output::cleanScenario(string fileName, string scenarioName, char d){
	string dStr(&d,1);
	vector <string> rows;
	string tempRow;
	ifstream inFile (fileName.c_str(), ios::in);
	if (!inFile){
		msgOut(MSG_ERROR,"Error in opening the file "+fileName+" for reading.");
		return;
	}
	while( getline (inFile,tempRow) ){
		vector<string> tokens;
		tokenize(tempRow,tokens,dStr);
		if(tokens[0] != scenarioName)
			rows.push_back( tempRow );
	}
	inFile.close();
	ofstream out(fileName.c_str(), ios::out);
	for(uint i=0;i<rows.size();i++){
		out << rows[i];
		out << "\n";
	}
}

