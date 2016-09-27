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
#include <stdio.h>
#include <algorithm>
#include <numeric>

#include "BaseClass.h"
#include "ThreadManager.h"

using namespace std;

BaseClass::BaseClass()
{
  MTHREAD=NULL;
}

BaseClass::~BaseClass()
{

}

/**
Overloaded method for the output log:

@param msgCode_h:    MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR, MSG_CRITICAL_ERROR
@param msg_h:        message text (string)
@param refreshGUI_h: use this call to "ping" the GUI (optional, default=true)

*/
void
BaseClass::msgOut(const int& msgCode_h, const string& msg_h, const bool& refreshGUI_h) const {

  msgOut2(msgCode_h, msg_h, refreshGUI_h);

}

/**
Overloaded method for the output log:

@param msgCode_h:    MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR, MSG_CRITICAL_ERROR
@param msg_h:        message text (int)
@param refreshGUI_h: use this call to "ping" the GUI (optional, default=true)

*/
void
BaseClass::msgOut(const int& msgCode_h, const int& msg_h, const bool& refreshGUI_h) const {
  msgOut2(msgCode_h, i2s(msg_h), refreshGUI_h);
}

/**
Overloaded method for the output log:

@param msgCode_h:    MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR, MSG_CRITICAL_ERROR
@param msg_h:        message text (double)
@param refreshGUI_h: use this call to "ping" the GUI (optional, default=true)

*/
void
BaseClass::msgOut(const int& msgCode_h, const double& msg_h, const bool& refreshGUI_h) const {
  msgOut2(msgCode_h, d2s(msg_h), refreshGUI_h);

}

/**
Convenient (private) function to actually do the job of the overloaded functions

*/
void
BaseClass::msgOut2(const int& msgCode_h, const string& msg_h, const bool& refreshGUI_h) const {

  string prefix;
  switch (msgCode_h){
  case MSG_NO_MSG:
    return;
  case MSG_DEBUG:
    prefix="*DEBUG: ";
    break;
  case MSG_INFO:
    prefix="**INFO: ";
    break;
  case MSG_WARNING:
    prefix="**WARNING: ";
    break;
  case MSG_ERROR:
    prefix="***ERROR: ";
    break;
  case MSG_CRITICAL_ERROR:
    prefix="****CRITICAL ERROR: ";
    break;
  default:
    cerr<<"I got an unknow error code: "<<msgCode_h<<" ("<<msg_h<<")"<<endl;
    exit(EXIT_FAILURE);
  }

  string message = prefix+msg_h;
  if (MTHREAD && MTHREAD->usingGUI()){
    MTHREAD->msgOut(msgCode_h, message);
  }
  else {
    string totalMsg = prefix+msg_h;
    cout<< totalMsg <<endl;
  }

  if(refreshGUI_h) {refreshGUI();}

  if(msgCode_h==MSG_CRITICAL_ERROR){
    if (MTHREAD && MTHREAD->usingGUI()){
      throw(2);
    }
    else {
      //throw(2);
      exit(EXIT_FAILURE);
    }
  }
}

void
BaseClass::refreshGUI()const{
  if (MTHREAD && MTHREAD->usingGUI()){
    MTHREAD->refreshGUI();
  }
}

int
BaseClass::s2i ( const string &string_h) const {
  if (string_h == "") return 0;
  int valueAsInteger;
  stringstream ss(string_h);
  ss >> valueAsInteger;
  return valueAsInteger;
  /*
  // I can't use stoi as of bug in MinGW
  try {
    return stoi(string_h);
  } catch (...) {
    if (string_h == "") return 0;
    else {
      msgOut(MSG_CRITICAL_ERROR,"Conversion string to integer failed. Some problems with the data? (got\""+string_h+"\")");
    }
  }
  return 0;
  */

}

double
BaseClass::s2d (const string& string_h) const {
  if (string_h == "") return 0.;
  double valueAsDouble;
  istringstream totalSString( string_h );
  totalSString >> valueAsDouble;
  return valueAsDouble;
  /*
  if (string_h == "") return 0.;
  try {
    return stod(string_h); // stod want dot as decimal separator in console mode and comma in gui mode. Further the decimal digits left are only 2 !!
  } catch (...) {
    if (string_h == "") return 0.;
    else {
      msgOut(MSG_CRITICAL_ERROR,"Conversion string to double failed. Some problems with the data? (got\""+string_h+"\")");
    }
  }
  return 0.;
  */
}


/// Includes comma to dot conversion if needed.
double
BaseClass::s2d (const  string& string_h, const bool& replaceComma) const {
  if(replaceComma){
    string valueAsString = string_h;
    // replace commas with dots. This is not needed when directly reading the input nodes as double, as the Qt function to Double does the same.
    replace(valueAsString.begin(), valueAsString.end(), ',', '.');
    return s2d(valueAsString);
  }
  return s2d(string_h);
  msgOut(MSG_CRITICAL_ERROR, "debug me please!");
  return 0.;
}

/// Includes conversion checks.
bool
BaseClass::s2b (const string& string_h) const {
  if (string_h == "true" || string_h == "vero" || string_h == "TRUE" || string_h == "1" || string_h == "True")
    return true;
  else if (string_h == "false" || string_h == "falso" || string_h == "FALSE" || string_h == "0" || string_h == "" || string_h == "False")
    return false;

  msgOut(MSG_CRITICAL_ERROR,"Conversion string to bool failed. Some problems with the data? (got\""+string_h+"\")");
  return true;
}

string
BaseClass::i2s (const int& int_h) const{
  //ostringstream out;
  //out<<int_h;
  //return out.str();
  char  outChar[24];
  snprintf ( outChar, sizeof(outChar), "%d", int_h );
  return string(outChar);
}

string
BaseClass::d2s (const double& double_h) const{
  //ostringstream out;
  //out<<double_h;
  //return out.str();
  char  outChar[24];
  snprintf ( outChar, sizeof(outChar), "%f", double_h );
  return string(outChar);
}

string
BaseClass::b2s (const bool& bool_h) const{
  if (bool_h) return "true";
  else return "false";
}

vector <int>
BaseClass::s2i(const  vector <string>& string_h) const{
  vector <int> valuesAsInteger;
  for (uint i=0;i<string_h.size();i++){
    valuesAsInteger.push_back(s2i(string_h[i]));
  }
  return valuesAsInteger;
}

/// Includes comma to dot conversion if needed.
vector <double>
BaseClass::s2d (const vector <string>& string_h, const bool& replaceComma) const{
  vector <double> valuesAsDouble;
  for (uint i=0;i<string_h.size();i++){
    if(replaceComma){
      string valueAsString = string_h[i];
      // replace commas with dots. This is not needed when directly reading the input nodes as double, as the Qt function to Double does the same.
      replace(valueAsString.begin(), valueAsString.end(), ',', '.');
      valuesAsDouble.push_back(s2d(valueAsString));
    } else {
      valuesAsDouble.push_back(s2d(string_h[i]));
    }
  }
  return valuesAsDouble;
}

/// Includes conversion checks.
vector <bool>
BaseClass::s2b(const vector <string> &string_h) const{
  vector <bool> valuesAsBool;
  for (uint i=0;i<string_h.size();i++){
    valuesAsBool.push_back(s2b(string_h[i]));
  }
  return valuesAsBool;
}

vector <string>
BaseClass::i2s (const vector <int> &int_h) const{
  vector <string> valuesAsString;
  for (uint i=0;i<int_h.size();i++){
        valuesAsString.push_back(i2s(int_h[i]));
  }
  return valuesAsString;
}

vector <string>
BaseClass::d2s (const vector <double> &double_h) const{
  vector <string> valuesAsString;
  for (uint i=0;i<double_h.size();i++){
        valuesAsString.push_back(d2s(double_h[i]));
  }
  return valuesAsString;
}

vector <string>
BaseClass::b2s (const vector <bool> &bool_h) const{
  vector <string> valuesAsString;
  for (uint i=0;i<bool_h.size();i++){
    if(bool_h[i]) valuesAsString.push_back("true");
    else valuesAsString.push_back("false");
  }
  return valuesAsString;
}


int
BaseClass::getType(const string &type_h) const{
  int toReturn=0;
  if (type_h == "int")         toReturn = TYPE_INT;
  else if (type_h == "double") toReturn = TYPE_DOUBLE;
  else if (type_h == "string") toReturn = TYPE_STRING;
  else if (type_h == "bool")   toReturn = TYPE_BOOL;
  else msgOut(MSG_CRITICAL_ERROR, "Unknow type "+type_h+".");
  return toReturn;
}


template<typename T> std::string
BaseClass::toString(const T& x) const {
  std::ostringstream oss;
  oss << x;
  return oss.str();
}


double
BaseClass::normSample (const double& avg, const double& stdev, const double& minval, const double& maxval) const{
  if(minval != NULL  && maxval != NULL){
    if (maxval <= minval){
      msgOut(MSG_CRITICAL_ERROR,"Error in normSample: the maxvalue is lower than the minvalue");
    }
  }
  for(;;){
    normal_distribution<double> d(avg,stdev);
    double c = d(*MTHREAD->gen);
    if( (minval == NULL || c >= minval) && (maxval == NULL || c <= maxval) ){
      return c;
    }
  }
  return minval;
}


template<typename T> T 
BaseClass::stringTo(const std::string& s) const {
  std::istringstream iss(s);
  T x;
  iss >> x;
  return x;
}

int
BaseClass::vSum (const vector <vector<int> > &vector_h) const{
  int toReturn = 0;
  for(vector < vector<int> >::const_iterator j=vector_h.begin();j!=vector_h.end();++j){
    toReturn += accumulate(j->begin(),j->end(),0);
  }
  return toReturn;
}

double
BaseClass::vSum (const vector<vector<double> > &vector_h) const{
  double toReturn = 0.0;
  for(vector < vector<double> >::const_iterator j=vector_h.begin();j!=vector_h.end();++j){
    toReturn += accumulate(j->begin(),j->end(),0.0);
  }
  return toReturn;
}

void
BaseClass::tokenize(const string& str, vector<string>& tokens, const string& delimiter) const {
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiter, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiter, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiter, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiter, lastPos);
    }
}

void
BaseClass::untokenize(string &str, vector<string>& tokens, const string& delimiter) const {
  // add initial loken in str is not empty
  if(str != ""){
    str += delimiter;
  }
  for(int i=0;i<tokens.size();i++){
    str += tokens[i];
    // don't add final delimiter
    if(i != (tokens.size()-1)){
       str += delimiter;
    }
  }
}

///////////////////////////////////// OTHER CLASSES THAN BASECLASS //////////////////
/// iskey class ///
iskey::iskey(){
 i = 0;
 s = "";
}
iskey::iskey(int i_h, string s_h){
 i = i_h;
 s = s_h;
}

iskey::~iskey(){

}

bool
iskey::operator == (const iskey & op2) const{
  if(op2.i == i && op2.s == s){
    return true;
  }
  return false;
}

bool
iskey::operator != (const iskey & op2) const{
  if(op2.i == i && op2.s == s){
    return false;
  }
  return true;
}

bool
iskey::operator < (const iskey & op2) const{
  if (i < op2.i ) return true;
  if (i == op2.i) {
    if (s < op2.s) return true;
  }
  return false;
}

bool
iskey::operator > (const iskey & op2) const{
  if (i > op2.i ) return true;
  if (i == op2.i) {
    if (s > op2.s) return true;
  }
  return false;
}

bool
iskey::operator <= (const iskey & op2) const{
  if (i < op2.i ) return true;
  if (i == op2.i) {
    if (s <= op2.s) return true;
  }
  return false;
}

bool
iskey::operator >= (const iskey & op2) const{
  if (i > op2.i ) return true;
  if (i == op2.i) {
    if (s >= op2.s) return true;
  }
  return false;
}

/// iiskey class (note the double ii) ///
iiskey::iiskey(){
 i = 0;
 i2 = 0;
 s = "";
}
iiskey::iiskey(int i_h, int i2_h, string s_h){
 i  = i_h;
 i2 = i2_h;
 s  = s_h;
}

iiskey::~iiskey(){

}

bool
iiskey::operator == (const iiskey & op2) const{
  if(op2.i == i && op2.i2 == i2 && op2.s == s){
    return true;
  }
  return false;
}

bool
iiskey::operator != (const iiskey & op2) const{
  if(op2.i == i && op2.i2 == i2 && op2.s == s){
    return false;
  }
  return true;
}

bool
iiskey::operator < (const iiskey & op2) const{
  if (i < op2.i ) {return true;}
  if (i == op2.i) {
    if (i2 < op2.i2 ) {return true;}
    if (i2 == op2.i2){
      if (s < op2.s) {return true;}
    }
  }
  return false;
}

bool
iiskey::operator > (const iiskey & op2) const{
  if (i > op2.i ) {return true;}
  if (i == op2.i) {
    if (i2 > op2.i2 ) {return true;}
    if (i2 == op2.i2){
      if (s > op2.s) {return true;}
    }
  }
  return false;
}

bool
iiskey::operator <= (const iiskey & op2) const{
  if (i < op2.i ) {return true;}
  if (i == op2.i) {
    if (i2 < op2.i2 ) {return true;}
    if (i2 == op2.i2){
      if (s <= op2.s) {return true;}
    }
  }
  return false;
}

bool
iiskey::operator >= (const iiskey & op2) const{
  if (i > op2.i ) {return true;}
  if (i == op2.i) {
    if (i2 > op2.i2 ) {return true;}
    if (i2 == op2.i2){
      if (s >= op2.s) {return true;}
    }
  }
  return false;
}

/// iisskey class (note the double ii and double ss) ///
iisskey::iisskey(){
 i = 0;
 i2 = 0;
 s = "";
 s2= "";
}
iisskey::iisskey(int i_h, int i2_h, string s_h, string s2_h){
 i  = i_h;
 i2 = i2_h;
 s  = s_h;
 s2 = s2_h;
}

iisskey::~iisskey(){

}

bool
iisskey::operator == (const iisskey & op2) const{
  if(op2.i == i && op2.i2 == i2 && op2.s == s && op2.s2 == s2){
    return true;
  }
  return false;
}

bool
iisskey::operator != (const iisskey & op2) const{
  if(op2.i == i && op2.i2 == i2 && op2.s == s && op2.s2 == s2){
    return false;
  }
  return true;
}

bool
iisskey::operator < (const iisskey & op2) const{
  if (i < op2.i ) {return true;}
  if (i == op2.i) {
    if (i2 < op2.i2 ) {return true;}
    if (i2 == op2.i2){
      if (s < op2.s) {return true;}
      if (s == op2.s){
        if (s2 < op2.s2) {return true;}
      }
    }
  }
  return false;
}

bool
iisskey::operator > (const iisskey & op2) const{
  if (i > op2.i ) {return true;}
  if (i == op2.i) {
    if (i2 > op2.i2 ) {return true;}
    if (i2 == op2.i2){
      if (s > op2.s) {return true;}
      if (s == op2.s){
        if (s2 > op2.s2) {return true;}
      }
    }
  }
  return false;
}

bool
iisskey::operator <= (const iisskey & op2) const{
  if (i < op2.i ) {return true;}
  if (i == op2.i) {
    if (i2 < op2.i2 ) {return true;}
    if (i2 == op2.i2){
      if (s < op2.s) {return true;}
      if (s == op2.s){
        if (s2 <= op2.s2) {return true;}
      }
    }
  }
  return false;
}

bool
iisskey::operator >= (const iisskey & op2) const{
  if (i > op2.i ) {return true;}
  if (i == op2.i) {
    if (i2 > op2.i2 ) {return true;}
    if (i2 == op2.i2){
      if (s > op2.s) {return true;}
      if (s == op2.s){
        if (s2 >= op2.s2) {return true;}
      }
    }
  }
  return false;
}

bool
iisskey::filter(const iisskey & key_h) const{
  if( (key_h.i == NULL  || key_h.i==i)    &&
      (key_h.i2 == NULL || key_h.i2==i2)  &&
      (key_h.s == ""  || key_h.s==s)    &&
      (key_h.s2 == "" || key_h.s2==s2)     ) return true;
  return false;
}

string
iisskey::print() const{
    char  outChar1[24];
    char  outChar2[24];
    snprintf ( outChar1, sizeof(outChar1), "%d", i);
    snprintf ( outChar2, sizeof(outChar2), "%d", i2);
    return string(outChar1)+'\t'+string(outChar2)+'\t'+s+'\t'+s2;

}
