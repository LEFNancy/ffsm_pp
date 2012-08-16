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
#include "BaseClass.h"
#include "ThreadManager.h"
#include <algorithm>


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
BaseClass::msgOut(int msgCode_h, string msg_h, bool refreshGUI_h) const {

	msgOut2(msgCode_h, msg_h, refreshGUI_h);

}

/**
Overloaded method for the output log:

@param msgCode_h:    MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR, MSG_CRITICAL_ERROR
@param msg_h:        message text (int) 
@param refreshGUI_h: use this call to "ping" the GUI (optional, default=true)

*/
void
BaseClass::msgOut(int msgCode_h, int msg_h, bool refreshGUI_h) const {
	ostringstream out;
	out<<msg_h;
	msgOut2(msgCode_h, out.str(), refreshGUI_h);
}

/**
Overloaded method for the output log:

@param msgCode_h:    MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR, MSG_CRITICAL_ERROR
@param msg_h:        message text (double)
@param refreshGUI_h: use this call to "ping" the GUI (optional, default=true)

*/
void
BaseClass::msgOut(int msgCode_h, double msg_h, bool refreshGUI_h) const {

	ostringstream out;
	out<<msg_h;
	msgOut2(msgCode_h, out.str(), refreshGUI_h);

}

/**
Convenient (private) function to actually do the job of the overloaded functions

*/
void
BaseClass::msgOut2(int msgCode_h, string msg_h, bool refreshGUI_h) const {
	
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
BaseClass::s2i ( string string_h) const {
	if (string_h == "") return 0;
	int valueAsInteger;
	string valueAsString = string_h;
	istringstream totalSString( valueAsString );
	totalSString >> valueAsInteger;
	return valueAsInteger;
}

/// Includes comma to dot conversion if needed.
double
BaseClass::s2d ( string string_h) const {
	if (string_h == "") return 0;
	double valueAsDouble;
	string valueAsString = string_h;
	// replace commas with dots. This is not needed when directly reading the input nodes as double, as the Qt function to Double does the same.
	replace(valueAsString.begin(), valueAsString.end(), ',', '.');
	istringstream totalSString( valueAsString );
	totalSString >> valueAsDouble;
	return valueAsDouble;
}

/// Includes conversion checks.
bool
BaseClass::s2b ( string string_h) const {
	if (string_h == "true" || string_h == "vero" || string_h == "TRUE" || string_h == "1" || string_h == "True")
		return true;
	else if (string_h == "false" || string_h == "falso" || string_h == "FALSE" || string_h == "0" || string_h == "")
		return false;

	msgOut(MSG_ERROR, "Sorry, I don't know how to convert "+string_h+" to a bool value. I return true... hope for the best");
	return true;
}

string
BaseClass::i2s (int int_h) const{
	ostringstream out;
	out<<int_h;
	return out.str();
}

string
BaseClass::d2s (double double_h) const{
	ostringstream out;
	out<<double_h;
	return out.str();
}

string
BaseClass::b2s (bool bool_h) const{
	string out;
	if(bool_h)
		out = "true";
	else
		out = "false";
	return out;
}

vector <int>
BaseClass::s2i( vector <string> string_h) const{
	vector <int> valuesAsInteger;
	for (uint i=0;i<string_h.size();i++){
		if (string_h.at(i) == "") {
			valuesAsInteger.push_back(0);
			continue;
		}
		int valueAsInteger;
		istringstream totalSString( string_h.at(i));
		totalSString >> valueAsInteger;
		valuesAsInteger.push_back(valueAsInteger);
	}
	return valuesAsInteger;
}

/// Includes comma to dot conversion if needed.
vector <double>
BaseClass::s2d (vector <string> string_h) const{
	vector <double> valuesAsDouble;
	for (uint i=0;i<string_h.size();i++){
		if (string_h.at(i) == "") {
			valuesAsDouble.push_back(0);
			continue;
		}
		double valueAsDouble;
		// replace commas with dots. This is not needed when directly reading the input nodes as double, as the Qt function to Double does the same.
		replace(string_h.at(i).begin(), string_h.at(i).end(), ',', '.');
		istringstream totalSString( string_h.at(i));
		totalSString >> valueAsDouble;
		valuesAsDouble.push_back(valueAsDouble);
	}
	return valuesAsDouble;
}

/// Includes conversion checks.
vector <bool>
BaseClass::s2b( vector <string> string_h) const{
	vector <bool> valuesAsBool;
	for (uint i=0;i<string_h.size();i++){
		valuesAsBool.push_back(s2b(string_h.at(i)));
	}
	return valuesAsBool;
}

vector <string>
BaseClass::i2s (vector <int> int_h) const{
	vector <string> valuesAsString;
	for (uint i=0;i<int_h.size();i++){
		ostringstream valueAsString;
		valueAsString<<int_h.at(i);
		valuesAsString.push_back(valueAsString.str());
	}
	return valuesAsString;
}

vector <string>
BaseClass::d2s (vector <double> double_h) const{
	vector <string> valuesAsString;
	for (uint i=0;i<double_h.size();i++){
		ostringstream valueAsString;
		valueAsString<<double_h.at(i);
		valuesAsString.push_back(valueAsString.str());
	}
	return valuesAsString;
}

vector <string>
BaseClass::b2s (vector <bool> bool_h) const{
	vector <string> valuesAsString;
	for (uint i=0;i<bool_h.size();i++){
		string temp;
		if(bool_h.at(i)) temp = "true";
		else temp = "false";
		valuesAsString.push_back(temp);
	}
	return valuesAsString;
}


int
BaseClass::getType(string &type_h){
	int toReturn=0;
	if (type_h == "int")         toReturn = TYPE_INT;
	else if (type_h == "double") toReturn = TYPE_DOUBLE;
	else if (type_h == "string") toReturn = TYPE_STRING;
	else if (type_h == "bool")   toReturn = TYPE_BOOL;
	else msgOut(MSG_CRITICAL_ERROR, "Unknow type "+type_h+".");
	return toReturn;
}


template<typename T> std::string
BaseClass::toString(const T& x) {
  std::ostringstream oss;
  oss << x;
  return oss.str();
}

template<typename T> T 
BaseClass::stringTo(const std::string& s) {
  std::istringstream iss(s);
  T x;
  iss >> x;
  return x;
}


int
BaseClass::iVSum (const vector<int> &intVector_h){
	int toReturn=0;
	for(uint i=0;i<intVector_h.size();i++){
		toReturn += intVector_h[i];
	}
	return toReturn;
}

double
BaseClass::dVSum (const vector<double> &doubleVector_h){
	double toReturn=0;
	for(uint i=0;i<doubleVector_h.size();i++){
		toReturn += doubleVector_h[i];
	}
	return toReturn;
}


void
BaseClass::tokenize(const string& str, vector<string>& tokens, const string& delimiter){
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
