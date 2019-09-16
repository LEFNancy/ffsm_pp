// This is a skeleton of the structure of FFSM as a program (not as a model)


#include <iostream>

using namespace std;


// ************** CLASSES DECLARATION

class LinkManager;
class MainProgram;
class myClass1;
class myClass2;

class BaseClass{

public:
                      BaseClass();
                     ~BaseClass();
protected:
    LinkManager*                         MYLINK;
};

class MainProgram: public BaseClass {

public:
                      MainProgram(LinkManager* MYLINK);
             ~MainProgram();
  void                run();

};

class LinkManager : public BaseClass
{


public:
                      LinkManager();
  void                setPointer1(myClass1 *MC1_h){MC1=MC1_h;};
  void                setPointer2(myClass2 *MC2_h){MC2=MC2_h;};
  void                runProgram();
  void                deleteDeadOldPointers();

  // pointers..
  myClass1*           MC1;
  myClass2*           MC2;

};

class myClass1: public BaseClass{

public:
                      myClass1(LinkManager* MYLINK_h);
                     ~myClass1();
  int                 getYear(){return year;}
  int                 setYear(const int& year_h){year = year_h;}


private:
  int                 year;

};

class myClass2: public BaseClass{

public:
                      myClass2(LinkManager* MYLINK_h);
                     ~myClass2();
  void                runBuggyCode();

};


// ************** CONSTRUCTORS/DESTRUCTORS

BaseClass::BaseClass()
{
  MYLINK=NULL;
}

BaseClass::~BaseClass()
{

}


MainProgram::MainProgram(LinkManager* MYLINK_h)
{
  MYLINK=MYLINK_h;
}

MainProgram::~MainProgram(){

}


LinkManager::LinkManager(){

  // initializing pointers...
  MC1     = NULL;
  MC2     = NULL;
}


myClass1::myClass1(LinkManager* MYLINK_h){
  MYLINK=MYLINK_h;
  year = 2000;
}

myClass1::~myClass1(){
}

myClass2::myClass2(LinkManager* MYLINK_h){
  MYLINK=MYLINK_h;
}

myClass2::~myClass2(){
}


// ************** IMPLEMENTATION FUNCTIONS

void
LinkManager::runProgram(){

  MainProgram* myProgram;
  deleteDeadOldPointers();
  myProgram = new MainProgram(this);
  myProgram->run();
  delete myProgram;
}

void
MainProgram::run(){
  myClass1* MC1  = new myClass1(MYLINK);
  MYLINK->setPointer1(MC1);
  myClass2* MC2  = new myClass2(MYLINK);
  MYLINK->setPointer2(MC2);
  MYLINK->MC2->runBuggyCode();
}

void
myClass2::runBuggyCode(){
    MYLINK->MC1->setYear(2019);
    cout <<"Year: " << MYLINK->MC1->getYear() << endl;
    cout <<"All fine, program didn't crash" << endl;
}

void
LinkManager::deleteDeadOldPointers(){
  if (MC1) {delete MC1; MC1=0;}
  if (MC2) {delete MC2; MC2=0;}
}

int main() { // main function
  cout << "Hello World!!!" << endl;

   LinkManager  modelMainLink;
   modelMainLink.runProgram();


  return 0;
}
