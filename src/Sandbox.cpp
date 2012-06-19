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

#include "Sandbox.h"
#include "ThreadManager.h"
#include <cmath>



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


using namespace std;

Sandbox::Sandbox(ThreadManager* MTHREAD_h){
	MTHREAD=MTHREAD_h;
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
Sandbox::test(){


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

