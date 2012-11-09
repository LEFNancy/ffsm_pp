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
#include <algorithm>

#include <cmath>
#include <algorithm>
#include <map>

#include <Eigen/Dense>

#include "Sandbox.h"
#include "ThreadManager.h"
#include "ModelData.h"
#include "Gis.h"



// Testing zip library...
//#include "zip.h"
//#include "unzip.h"
//#include <QFile>
//#include <QFileInfo>
//#include <QString>
//#include <QStringList>
//#include <QList>
//#include <iostream>
//#include <iomanip>

#include "IpIpoptApplication.hpp"
#include "IpSolveStatistics.hpp"

#include "Ipopt_nlp_problem_debugtest.h"
#include "Adolc_debugtest.h"


typedef map<string, string> TStrStrMap;
typedef pair<string, string> TStrStrPair;

using namespace std;

Sandbox::Sandbox(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
}

Sandbox::Sandbox(){

}


Sandbox::~Sandbox(){

}

// ---------------------------------------------
struct GccTest
{

	GccTest(string name_h){
		nameMember = name_h;
	};

	string nameMember;

   operator string ()
   {
	
		cout << "the first function\n";
		cout << nameMember << endl;
		return "42";
   }

   operator int ()
   {
		cout << "its \"underload\"\n";
      return 42;
   }

	operator vector<int> ()
	{
		cout << "within vector <int>" << endl;
		vector <int> toReturn;
		toReturn.push_back(3);
		toReturn.push_back(4);
		toReturn.push_back(5);
		return toReturn;
	}

};

// --------------------------------------
void
Sandbox::basicTest(){

	/*
	// testing findMap
	map<int,double> testMap;
	for (uint i=0;i<5;i++){
		pair<int,double> mypair(i,i*2.5);
		testMap.insert(mypair);
	}
	double result = findMap(testMap,3,MSG_NO_MSG);
	double result2 = findMap(testMap,1,MSG_ERROR);
	double result3 = findMap(testMap,7,MSG_DEBUG);
	cout << "Done" << endl;
	map<int, vector <double> > testMap2;
	for (uint i=0;i<5;i++){
		vector <double> myvector;
		for(uint j=0;j<10;j++) {
			myvector.push_back(i*100+j);
		}
		pair<int,vector <double> > mypair2(i,myvector);
		testMap2.insert(mypair2);
	}
	vector <double> resultb = findMap(testMap2,3,MSG_NO_MSG);
	vector <double> resultb2 = findMap(testMap2,1,MSG_ERROR);
	vector <double> resultb3 = findMap(testMap2,7);
	cout << "Done2" << endl;
	exit(1);
	*/



	/*
	// Testing vSum
	vector <int> ivector(5,5);
	vector <double> dvector(5,1.5);
	vector < vector <int> > ivector2;
	vector <vector <double > > dvector2;


	for(uint i=0;i<5;i++){
		ivector2.push_back(ivector);
		dvector2.push_back(dvector);
	}

	int iSum = vSum(ivector);
	int iSum2 = vSum(ivector2[2]);
	double dSum = vSum(dvector);
	double dSum2 = vSum(dvector2[1]);
	int iSum3 = vSum(ivector2);
	double dSum3 = vSum(dvector2);

	cout << "hi there" << endl;
	*/

	/*
	// Testing Eigen
	using Eigen::MatrixXd;
	MatrixXd m(2,2);
	m(0,0) = 4;
	m(1,0) = 2.5;
	m(0,1) = -1;
	m(1,1) = m(1,0) + m(0,1);
	std::cout << m << std::endl;
	exit(0);
	*/

	/*
	// Test on two different type of partial matching over map values
	testPartMatching2();
	testPartMatching();
	*/

	/*
	// -------------------------------------------------------------------
	// Testing how to erase elements from a vector according to conditions
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	vector<string> myvector;
	myvector.push_back("a");
	myvector.push_back("b");
	myvector.push_back("c");
	myvector.push_back("d");
	myvector.push_back("e");

	for (uint i=0; i<myvector.size();i++){
		cout << "i:" << i << " myvector[i]: " << myvector[i] << endl;
		if(myvector[i]== "c" || myvector[i]=="d"){
			cout << " -- TBR: " << "i:" << i << " myvector[i]: " << myvector[i] << endl;
			myvector.erase (myvector.begin()+i);
			i--;
		}
	}

	cout << "Myvector now contains:" << endl;
	for (int i=0; i<myvector.size(); i++) {
		cout << "i: " << i << " myvector[i]: " << myvector[i] << endl;
	}
	exit (0);
	*/


}

void
Sandbox::fullTest(){


	/*
	// testing Pixel::getMultiplier (const string& multiplierName, const string& forName, int year)
	Pixel* px = MTHREAD->GIS->getPixel(0);
	double debug1 = px->getMultiplier("tp_multiplier","broadL_highF",2012);
	double debug2 = px->getMultiplier("tp_multiplier","broadL_highF",2008);
	double debug3 = px->getMultiplier("tp_multiplier","broadL_highF",2009);
	double debug4 = px->getMultiplier("tp_multiplier","broadL_highF",2010);
	double debug5 = px->getMultiplier("mortCoeff_multiplier","broadL_highF",2012);
	double debug6 = px->getMultiplier("mortCoeff_multiplier","con_copp",2012);
	double debug7 = px->getMultiplier("blaaaa","broadL_highF",2012);

	double debug10 = debug1;
*/

	/*
	// testing reading a directory
	string dir = MTHREAD->MD->getBaseDirectory()+MTHREAD->MD->getStringSetting("spatialDataSubfolder");
	vector<string> files = vector<string>();

	MTHREAD->MD->getFilenamesByDir (dir,files, ".grd");

	for (unsigned int i = 0;i < files.size();i++) {
		cout << files[i] << endl;
	}
	*/

	/*
	// testing ModelData:: ModelData::calculateAnnualisedEquivalent(double amount_h, int years_h)
	cout << "Value:" << MTHREAD->MD->calculateAnnualisedEquivalent(500.,4) << endl;
	cout << "Value:" << MTHREAD->MD->calculateAnnualisedEquivalent(500.,30) << endl;
	cout << "Value:" << MTHREAD->MD->calculateAnnualisedEquivalent(107.035040105,10) << endl;
	cout << "Value:" << MTHREAD->MD->calculateAnnualisedEquivalent(8.91507,1) << endl;
	cout << "Done" << endl;
	exit(0);
	*/

	/*
	// testing snprintf
	vector <int> myintegers;
	vector <double> mydoubles;
	char  szResult[24];

	myintegers.push_back(1);
	myintegers.push_back(202);
	myintegers.push_back(3003);
	myintegers.push_back(400004);
	myintegers.push_back(50000005);
	myintegers.push_back(6000000006);
	mydoubles.push_back(1.1234567890);
	mydoubles.push_back(12345678.9);
	mydoubles.push_back(12345678.90123456);
	mydoubles.push_back(6000000006.123456789012);
	for(uint i=0;i<myintegers.size();i++){
		snprintf ( szResult, sizeof(szResult), "%d", myintegers[i] );  // "safe" version
		cout << "int/string: " << myintegers[i] << "  /  " << szResult << endl;
	}
	for(uint i=0;i<mydoubles.size();i++){
			snprintf ( szResult, sizeof(szResult), "%f", mydoubles[i] );  // "safe" version
		cout << "double/string: " << mydoubles[i] << "  /  " << szResult << endl;
	}
	exit(0);
	*/

	/*
	// testing stod() ..
	// this is giving different results if gui or console mode !!
	vector<string> numbers;
	numbers.push_back("123.1234567890");
	numbers.push_back("123.1234");
	numbers.push_back("123,1234567890");
	numbers.push_back("123,1234");
	double outd;
	for(uint i=0;i<numbers.size();i++){
		try {
			outd =  stod(numbers[i]);
			cout << "Conversion passed: " << numbers[i] << "  -  " << outd << endl;
		} catch (...) {
			cout << "Conversion DID NOT passed: " << numbers[i] << "  -  " <<endl;
		}
	}
	exit(0);
	*/

/*
// ------------------------------------
// Testing makeKeyProdData() and unpackKeyProdData()
	string parName = "za";
	int regId = 20000;
	string prod = "wood";
	string freeDim = "";
	string key = MTHREAD->MD->makeKeyProdData(parName,i2s(regId),prod,freeDim);
	cout << "key: " << key << endl;
	MTHREAD->MD->unpackKeyProdData(key,parName,regId,prod,freeDim);
	cout << "parName: " << parName << endl;
	cout << "regId: " << regId << endl;
	cout << "prod: " << prod << endl;
	cout << "freeDim: " << freeDim << endl;
	exit(0);
*/

/*
// ---------------------------------------------
// checking the functions dataMapCheckExist() and dataMapGetValue() works well
typedef map<string, vector <double> > DataMap;
typedef pair<string, vector <double> > DataPair;

vector <double> abaa (5, 1.);
vector <double> abcc (5,10);
vector <double> anbb (5,100);
vector <double> andd (5,5);
vector <double> anff (5,3);
vector <double> ag (5,2);
vector <double> agii (5,7);



DataMap dM;
dM.insert(DataPair("abaa", abaa));
dM.insert(DataPair("abcc", abcc));
dM.insert(DataPair("anbb", anbb));
dM.insert(DataPair("andd", andd));
dM.insert(DataPair("anff", anff));
dM.insert(DataPair("ag", ag));
dM.insert(DataPair("agii", agii));

vector<string> tests;
tests.push_back("ab");
tests.push_back("anbb");
tests.push_back("ane");
tests.push_back("an");
tests.push_back("ac");
tests.push_back("ag");
tests.push_back("agii");
tests.push_back("al");


bool found;
double value;

for(uint i=0;i<tests.size();i++){
	found = MTHREAD->MD->dataMapCheckExist(dM, tests[i]);
	value = MTHREAD->MD->dataMapGetValue(dM, tests[i],2010);
	cout << tests[i] << ": " << b2s(found) << " value: "<< value << endl;
}

exit(0);
*/


	/*
	// testing how to search on a vector using the find algorithm

	vector<string> names;
	names.push_back("pippo");
	names.push_back("topolino");
	names.push_back("minni");
	names.push_back("paperino");

	string toSearch1 = "minni";
	string toSearch2 = "zio paperone";

	if( find(names.begin(), names.end(), toSearch1)!= names.end() ){
		cout << "minni trovata" << endl;
	}
		if( find(names.begin(), names.end(), toSearch2)!= names.end() ){
		cout << "zio paperone trovato" << endl;
	}
	cout << "test on find ended." << endl;
	exit(0);
	*/

// ------------------------------------------------------------------


	/*
	int a;
	a = getSetting<int>("myIntData", TYPE_INT);

	string b;
	b = getSetting<string>("myStringData", TYPE_STRING);

	bool c;
	c = getSetting<bool>("myBoolData", TYPE_BOOL);


	cout << "A is: " << a << endl;

	cout << "B is: " << b << endl;

	cout << "C is: " << c << endl;

	//vector<string> getVectorSetting <string> ("test", TYPE_STRING);
	//template <class T> vector <T> getVectorSetting(string name_h, int type);

	//vector <string> myStrings = getVectorSetting <vector<string> > ("test", TYPE_STRING);

	string s = GccTest("test");
	int i = GccTest("test");
	vector <int> iVector = GccTest("test");

	for (int i=0; i< iVector.size(); i++){
		cout << "iVector: " << iVector.at(i) << endl;
	}
	*/
	
	// ------------------------------------------------------------------

	/* // I learned: how to access elements - both objects and pointers - of a vector using pointers
	// testing how to operate with iterators over a pointer element in an array:

	cout << "Starting iterator test..." << endl;

	TestStructure a,b,c,d;
	a.i=0; b.i=1; c.i=2; d.i=3;
	TestStructure* ap;
	TestStructure* bp;
	TestStructure* cp;
	TestStructure* dp;

	ap = &a;
	bp = &b;
	cp = &c;
	dp = &d;

	vector <TestStructure>  objects;
	vector <TestStructure*> pointers;

	objects.push_back(a);
	objects.push_back(b);
	objects.push_back(c);
	objects.push_back(d);

	pointers.push_back(ap);
	pointers.push_back(bp);
	pointers.push_back(cp);
	pointers.push_back(dp);
	
	vector<TestStructure>::iterator pi;
	vector<TestStructure*>::iterator pp;

	 //ok it works
	//for ( pi = objects.begin() ; pi != objects.end();){
	//	if(pi->i==2){
	//		objects.erase(pi);
	//	}
	//	else {
	//		++pi;
	//	}
	//}

	//for (int j=0;j<objects.size();j++){
	//	cout << j << " object is: " << objects[j].i << endl;
	//}
	

	// works as well ;-))
	for ( pp = pointers.begin() ; pp != pointers.end();){
		if( (*pp)->i==2){
			//delete (*pp);
			pointers.erase(pp);
		}
		else {
			++pp;
		}
	}

	for (int j=0;j<pointers.size();j++){
		cout << j << " pointers is: " << pointers[j]->i << endl;
	}

	// c is not destructed if we don't explicitelly call delete over the pointer...
	cout << c.i << endl; // this go in seg-frag if we call delete (*pp)..
	*/

	// ------------------------------------------------------------------
	/* test on how to remove from a map.. deletable
	map <int, string> test;
	test.insert(pair<int, string>(2, "pippo"));
	test.insert(pair<int, string>(1, "pluto"));
	test.insert(pair<int, string>(5, "minni"));
	test.insert(pair<int, string>(3, "topolino"));


	map <int, string>::iterator p;
	p=test.find(3);
	if(p != test.end()){
		cout << p->second <<endl;
		test.erase(p);
	}
	else {
		cout << "not found " << endl;
	}

	map <int, string>::iterator p2;
	p2=test.find(3);
	if(p2 != test.end()){
		cout << p2->second <<endl;
		test.erase(p2);
	}
	else {
		cout << "not found " << endl;
	}
	*/

 	/*vector<int> test;
 	for (int i=0;i<5;i++) test.push_back(i);
 	cout << "test.." << endl;
	for (uint i=0;i<test.size();i++){
 		cout << "Test "<<i<<": "<<test.at(i) << endl;
	}
	//test.erase(2);

	vector<int>::iterator p;
	for ( p = test.begin() ; p != test.end();){
		if(*p == 1 || *p == 2 || *p==4){
			test.erase(p);
		}
		else {
			++p;
		}
	}
	

	for (uint i=0;i<test.size();i++){
 		cout << "Test "<<i<<": "<<test.at(i) << endl;
	}

//	test.erase(remove_if(test.begin(), test.end(), FindMatchingString(&fs))

//	for (int i=0;i<test.size();i++) cout << "TEST: "<<i<< " " << test.at(i) << endl;
	*/

	/*
	// On this test I am showing how to "move" one pointer from a vector of pointers to an other one. The real case is used to move Agent_farmer* pointers from the managedAgents vector to the removedVector.

	double* myDouble1 = new double(1);
	double* myDouble2 = new double(2);
	double* myDouble3 = new double(3);
	
	vector <double*> origin;
	vector <double*> destination;

	origin.push_back(myDouble1);
	origin.push_back(myDouble2);
	origin.push_back(myDouble3);

	cout << "MyDouble2: "<< *myDouble2 << endl;
	vector<double*>::iterator doublePIterator;

	for (int i=0;i<origin.size();i++){
		cout << i << " origin is: " << *origin[i] << endl;
	}

	for ( doublePIterator = origin.begin() ; doublePIterator !=origin.end();){
		if(*doublePIterator == myDouble2){
			destination.push_back(myDouble2);
			origin.erase(doublePIterator);
		}
		else {
			++doublePIterator;
		}
	}

	for (int i=0;i<origin.size();i++){
		cout << i << " origin is now: " << *origin[i] << endl;
	}

	for (int i=0;i<destination.size();i++){
		cout << i << " destination is: " << *destination[i] << endl;
	} */
	
	// ------------------------------------------------------------------
	/*
	// Test on how to return a vector of pointers from a member vector of data
	TestStructure a,b,c,d;
	a.i=0; b.i=1; c.i=2; d.i=3;
	testVector.push_back(a);
	testVector.push_back(b);
	testVector.push_back(c);
	testVector.push_back(d);

	vector<TestStructure*>  myVector=getTestStructure();

	for(uint i=0;i<myVector.size();i++){
		msgOut(MSG_DEBUG, i2s(myVector[i]->i));
	}	
	*/

	/*
	// Deleting an object and inserting a new one on a vector of objects.. it doesn't works.. problems with the last element..
	vector<BasicData>::iterator p;
	for(p=programSettingsVector.begin();p!=programSettingsVector.end();p++){
		if(p->name == SETT.name){
			programSettingsVector.erase(p);
			programSettingsVector.insert(p,1,SETT);
			cout << SETT.name <<endl;
			break;
		}
	}
	*/

	/*double test = -987654321.987654321;
	double result;
	result = fabs(test);
	cout << "Test: "<< result << endl;*/


	/*
	// Testing the zip library:

	cout <<"Hello world Zip!" << endl;

	QString file = "data/testInput.ods";
	QString out = "data/tempInput";
	QString pwd = "";
	if (!QFile::exists(file))
	{
		cout << "File does not exist." << endl << endl;
		//return false;
	}

	UnZip::ErrorCode ec;
	UnZip uz;

	if (!pwd.isEmpty())
		uz.setPassword(pwd);

	ec = uz.openArchive(file);
	if (ec != UnZip::Ok)
	{
		cout << "Failed to open archive: " << uz.formatError(ec).toAscii().data() << endl << endl;
		//return false;
	}

	ec = uz.extractAll(out);
	if (ec != UnZip::Ok)
	{
		cout << "Extraction failed: " << uz.formatError(ec).toAscii().data() << endl << endl;
		uz.closeArchive();
		//return false;
	}
	*/

	/*
	// How to : delete an element from an array from its position
	cout << "How to : delete an element from an array from its position" << endl;

	vector <string> headers;
	vector < vector <string> > records;
	vector <string> firstrecord;
	vector <string> secondrecord;
	records.push_back(firstrecord);
	records.push_back(secondrecord);

	headers.push_back("a");
	headers.push_back("b");
	headers.push_back("");
	headers.push_back("d");
	headers.push_back("e");
	headers.push_back("");
	
	records[0].push_back("0");
	records[0].push_back("1");
	records[0].push_back("2");
	records[0].push_back("3");
	records[0].push_back("4");
	records[0].push_back("5");
	records[1].push_back("00");
	records[1].push_back("11");
	records[1].push_back("22");
	records[1].push_back("33");
	records[1].push_back("44");
	records[1].push_back("55");

	for (int i=headers.size()-1;i>=0;i--){
		if(headers[i] == ""){
			headers.erase(headers.begin()+i);
			for (int j=0;j<records.size();j++){
				records[j].erase(records[j].begin()+i);
			}
		}
	}
	for(uint i=0;i<headers.size();i++){
		cout << headers.at(i) << " - " << records[0].at(i) << " - " << records[1].at(i) << endl;
	}
	cout << "done!" << endl;
	*/

	//testThreads();
	/*vector<double> numbers;
	double cumNumbers = 0.00;
	numbers.push_back(0.40);
	numbers.push_back(0.10);
	numbers.push_back(0.20);
	numbers.push_back(0.08);
	numbers.push_back(0.22);
	
	for (uint i=0;i<numbers.size();i++){
		cumNumbers += numbers[i];
	}
	
	if (cumNumbers <= 0.99999999 || cumNumbers >= 1.00000001) {
		cout <<"Bastardo!"<<endl;
	} else {
		cout <<"qui funzia!"<<endl;
	}*/

}

template <class T> vector <T> getVectorSetting(string name_h, int type) {

		vector <string> myStringDatas;
		myStringDatas.push_back("aaaaa");
		myStringDatas.push_back("bbbbb");
		myStringDatas.push_back("ccccc");	
		vector <T> xVector;

		for (int i=0;i<myStringDatas.size();i++){
			istringstream iss(myStringDatas[i]);
			T x;
			iss >> x;
			xVector.push_back(x);
		}

		return xVector;
}




template <class T> T
Sandbox::getSetting(string name_h, int type){

	string myIntData;
	myIntData = "34";
	string myStringData;
	myStringData = "abcdefg";

	string myBoolData;
	myBoolData = "false";
	
	if(type==TYPE_INT){
		istringstream iss(myIntData);
		T x;
		iss >> x;
		return x;
	}

	if(type==TYPE_STRING){
		istringstream iss(myStringData);
		T x;
		iss >> x;
		return x;
	}

	if(type==TYPE_BOOL){
		string tempBoolString;
		if (myBoolData == "1" || myBoolData == "true" || myBoolData == "True" || myBoolData == "TRUE" || myBoolData == "vero" || myBoolData == "Vero"|| myBoolData == "VERO"){
			tempBoolString = "1";
		}
		else if (myBoolData == "0" || myBoolData == "false" || myBoolData == "False" || myBoolData == "FALSE" || myBoolData == "falso" || myBoolData == "falso"|| myBoolData == "FALSO"){
			tempBoolString = "0";
		}
		else {
			msgOut(MSG_CRITICAL_ERROR, "Impossible conversion of "+myBoolData+" to bool!. Aborted.");
		}
		istringstream iss(tempBoolString);
		T x;
		iss >> x;
		return x;
	}


}

template<typename T> T 
Sandbox::test2(const std::string& s) {
  std::istringstream iss(s);
  T x;
  iss >> x;
  return x;
}


vector <TestStructure*>
Sandbox::getTestStructure(){
	vector <TestStructure*> toReturn;
	for (uint i=0;i<testVector.size();i++){
		//TestStructure* tempTest = new TestStructure;
		toReturn.push_back(&testVector[i]);
	}
	return toReturn;

}



void
Sandbox::testThreads(){

				/*
				PSEUDOCODE
				- attivo i vari thread
				- per ogni closestAgent itero fra i vari thread e se "è libero" gli assegno il closestAgent
				- quando ho finito i closestAgent aspetto che tutti i threads abbiano finito il lavoro
				- chiudo i threads
				- vado avanti
				*/
				int nAgents= 50;
				vector<TestStructure*> myAgents;
				vector<double> myResults (nAgents, (double) 0);
				//int nThreads = MTHREAD->MD->getIntSetting("nThreads");
				int nThreads= 5;

				for (int i=0; i < nAgents; i++){
					TestStructure* myAgent = new TestStructure;
					myAgent->i = i;
					myAgent->random =  (0+( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(10-0+1))/ (double)100;
					myAgents.push_back(myAgent);
				}

				vector <testThread*> myThreads ;

				for (int i=0; i < nThreads; i++){
					testThread* myThread = new testThread;
					myThreads.push_back(myThread);
				}

				for (uint i=0;i<myAgents.size();i++){
					bool assigned = false;
					while(!assigned) {
						for (uint j=0;j<myThreads.size();j++){
							if (!myThreads[j]->isRunning()){
								cout << "Assigning agent " << i << " to thread " << j << endl;
								myThreads[j]->assignJob(myAgents[i]);
								myThreads[j]->start();
								assigned = true;
								break;
							}
							else {
								cout << "Thread " << j << " is busy" << endl;
							}
						}		
					}	
				}
				/*
				volatile bool somethingStopping = true;
				while (somethingStopping){
					somethingStopping = false;
					for (uint i=0;i<myThreads.size();i++){
						if(myThreads[i]->isRunning()){
							somethingStopping = true;
							//cout << "somethingStopping is true" << endl;
						}
					}
				}

				if (somethingStopping) {
					cout << "somethingStopping is true" << endl;
				}
				else {
					cout << "somethingStopping is false" << endl;
				}
				cout << "pinco pallo sono nel mezzo dei threads..."<<endl;
				*/
				for (int i=0; i < nThreads; i++){
					myThreads[i]->wait();
				}


				for (int i=0; i < nThreads; i++){
					delete myThreads[i];
				}

				for (uint i=0;i<myAgents.size();i++){
					//cout <<myAgents[i]->cachedOffer<<endl;

					double random =  (0+( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(10-0+1))/ (double)100;

					// important !
					// for random integer see also std::uniform_int_distribution :
					// http://stackoverflow.com/questions/7780918/stduniform-int-distributionint-range-in-g-and-msvc
					// in regmas:
					// int randomRed = int (50+( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(255-50+1)); // randomRed is [50,255] Don't use "randomNumber % range" !!

					//cout <<random<<endl;
				}

				//thread1.stop();
				cout << "FINITO"<<endl;
				

}

testThread::testThread(){

}

void
testThread::run(){

	cout << agent->i << endl;

	double randChange =  (0+( (double)rand() / ((double)(RAND_MAX)+(double)(1)) )*(10-0+1))/ (double)100; //rand() must be not thread safe !!!!

	int justn = 10000;
	vector <double> takeTimeVector (justn, 0);
	for (int i =0; i< justn;i++){
		takeTimeVector.at(i)=i*2;
	}
	agent->cachedOffer = agent->random;
}

void
testThread::assignJob(TestStructure* agent_h){
	agent = agent_h;
	agent->cachedOffer = 0;
}

void
Sandbox::testIpopt(){


    using namespace Ipopt;

      // Create a new instance of your nlp
      //  (use a SmartPtr, not raw)
      SmartPtr<TNLP> mynlp = new Ipopt_nlp_problem_debugtest();

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

}

int
Sandbox::testAdolc(){

  using namespace Ipopt;
 // Create an instance of your nlp...
	 SmartPtr<TNLP> myadolc_nlp = new MyADOLC_NLP();
	//SmartPtr<TNLP> myadolc_nlp = new MyADOLC_sparseNLP();

  // Create an instance of the IpoptApplication
  SmartPtr<IpoptApplication> app = new IpoptApplication();

  // Initialize the IpoptApplication and process the options
  ApplicationReturnStatus status;
  status = app->Initialize();
  if (status != Solve_Succeeded) {
    printf("\n\n*** Error during initialization!\n");
    return (int) status;
  }

  status = app->OptimizeTNLP(myadolc_nlp);

  if (status == Solve_Succeeded) {
    // Retrieve some statistics about the solve
    Index iter_count = app->Statistics()->IterationCount();
    printf("\n\n*** The problem solved in %d iterations!\n", iter_count);

    Number final_obj = app->Statistics()->FinalObjective();
    printf("\n\n*** The final value of the objective function is %e.\n", final_obj);
  }

  return (int) status;
}

// ------------------------------------------------------------------
// How to partial matching the key of a map

/*TStrStrMap::iterator
Sandbox::FindPrefix(const TStrStrMap& map, const string& search_for) {
		TStrStrMap::iterator i = map.lower_bound(search_for);
		if (i != map.end()) {
				const string& key = i->first;
				if (key.compare(0, search_for.size(), search_for) == 0) // Really a prefix?
						return i;
		}
		return map.end();
}
*/

/*
void
Sandbox::testSearchMap(const TStrStrMap& map, const string& search_for) {
	 cout << search_for;
		TStrStrMap::iterator i = FindPrefix(map, search_for);
		if (i != map.end())
				cout << '\t' << i->first << ", " << i->second;
		cout << endl;

}
*/

void
Sandbox::testSearchMap(const TStrStrMap& map, const string& search_for) {
	TStrStrMap::const_iterator i = map.lower_bound(search_for);
	for(;i != map.end();i++){
		const string& key = i->first;
		if (key.compare(0, search_for.size(), search_for) == 0) {// Really a prefix?
				cout << i->first << ", " << i->second << endl;
		} else {
			break;
		}
	}

}


void
Sandbox::testPartMatching(){

		TStrStrMap tMap;

		tMap.insert(TStrStrPair("John", "AA"));
		tMap.insert(TStrStrPair("Mary", "BBB"));
		tMap.insert(TStrStrPair("Mother", "A"));
		tMap.insert(TStrStrPair("Moliere", "D"));
		tMap.insert(TStrStrPair("Marlon", "C"));

		testSearchMap(tMap, "Marl");
		testSearchMap(tMap, "Mo");
		testSearchMap(tMap, "ther");
		testSearchMap(tMap, "Mad");
		testSearchMap(tMap, "Mom");
		testSearchMap(tMap, "Perr");
		testSearchMap(tMap, "Jo");

	exit(0);
		return;
}

void
Sandbox::testSearchMap2(const TStrStrMap& map_h, const string& search_for) {
	TStrStrMap::const_iterator i = map_h.upper_bound(search_for);
	if(i!= map_h.begin())	i--;
	const string& key = i->first;
	string search_base = search_for.substr(0,search_for.size()-4);
	if (key.compare(0, search_base.size(), search_base) == 0){
		cout << "MATCH: " << search_for <<", "<< i->first << ", " << i->second << endl;
	} else {
		cout << "NOTM:  " << search_for <<", "<< i->first << endl;
	}

}

void
Sandbox::testPartMatching2(){

		TStrStrMap tMap;


	tMap.insert(TStrStrPair("mortCoeff_multiplier#broadL_highF##2005", "2005"));
	tMap.insert(TStrStrPair("regLev_1", "-9999"));
	tMap.insert(TStrStrPair("regLev_2", "-9999"));
	tMap.insert(TStrStrPair("tp_multiplier#broadL_copp##2005", "-9999"));
	tMap.insert(TStrStrPair("tp_multiplier#broadL_highF##2005", "50"));
	tMap.insert(TStrStrPair("tp_multiplier#broadL_highF##2010", "2010"));
	tMap.insert(TStrStrPair("tp_multiplier#broadL_mixedF##2005", "-9999"));
	tMap.insert(TStrStrPair("tp_multiplier#con_copp##2005", "-9999"));
	tMap.insert(TStrStrPair("tp_multiplier#con_highF##2005", "-9999"));
	tMap.insert(TStrStrPair("tp_multiplier#con_mixedF##2005", "aa"));

	TStrStrMap::const_iterator i;

	for(i=tMap.begin();i!=tMap.end();i++){
		cout << i->first << ", " << i->second << endl;
	}
	cout << endl;

		testSearchMap2(tMap, "mortCoeff_multiplier#broadL_highF##2006");
		testSearchMap2(tMap, "tp_multiplier#broadL_highF##2008");
		testSearchMap2(tMap, "aaaaaa");
		testSearchMap2(tMap, "zzzzzz");

	exit(0);
		return;
}


