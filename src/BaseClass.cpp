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

// other ways of converting string to integers :
// -- 1 (not tested) :
// (int)strtol(token_start, (char **)NULL, 10);
// -- 2 (old used method, everyone say slow) :
// int valueAsInteger;
// string valueAsString = string_h;
// istringstream totalSString( valueAsString );
// totalSString >> valueAsInteger;
// return valueAsInteger;
// -- 3 (C++11) :
// String to integer: int n = std::stoi(s);
// Integer to string: std::string s = std::to_string(n);
// the if string_h=="" return 0 is no longer needed.


/// Includes comma to dot conversion if needed.
double
BaseClass::s2d (const  string& string_h, const bool& replaceComma) const {
	if(replaceComma){
		string valueAsString = string_h;
		// replace commas with dots. This is not needed when directly reading the input nodes as double, as the Qt function to Double does the same.
		replace(valueAsString.begin(), valueAsString.end(), ',', '.');
		return atof(valueAsString.c_str());
	}
	return atof(string_h.c_str());
}

/// Includes conversion checks.
bool
BaseClass::s2b (const string& string_h) const {
	if (string_h == "true" || string_h == "vero" || string_h == "TRUE" || string_h == "1" || string_h == "True")
		return true;
	else if (string_h == "false" || string_h == "falso" || string_h == "FALSE" || string_h == "0" || string_h == "")
		return false;

	msgOut(MSG_ERROR, "Sorry, I don't know how to convert "+string_h+" to a bool value. I return true... hope for the best");
	return true;
}

//string
//BaseClass::i2s (const int& int_h) const{
	//ostringstream out;
	//out<<int_h;
	//return out.str();

	// return to_string(int_h);

	//char* result;
	//sprintf( result, "%d", int_h );
	//string str(result);
	//return str;

//}

//string
//BaseClass::d2s (const double& double_h) const{
//	ostringstream out;
//	out<<double_h;
//	return out.str();
//}

string
BaseClass::b2s (const bool& bool_h) const{
	if (bool_h) return "true";
	else return "false";
}

vector <int>
BaseClass::s2i(const  vector <string>& string_h) const{
	vector <int> valuesAsInteger;
	for (uint i=0;i<string_h.size();i++){
		valuesAsInteger.push_back(atof(string_h[i].c_str()));
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
			valuesAsDouble.push_back(::atof(valueAsString.c_str()));
		} else {
			valuesAsDouble.push_back(::atof(string_h[i].c_str()));
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
		valuesAsString.push_back(to_string(int_h[i]));
	}
	return valuesAsString;
}

vector <string>
BaseClass::d2s (const vector <double> &double_h) const{
	vector <string> valuesAsString;
	for (uint i=0;i<double_h.size();i++){
		valuesAsString.push_back(to_string(double_h[i]));
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

template<typename T> T 
BaseClass::stringTo(const std::string& s) const{
  std::istringstream iss(s);
  T x;
  iss >> x;
  return x;
}


int
BaseClass::iVSum (const vector<int> &intVector_h) const{
	int toReturn=0;
	for(uint i=0;i<intVector_h.size();i++){
		toReturn += intVector_h[i];
	}
	return toReturn;
}

double
BaseClass::dVSum (const vector<double> &doubleVector_h) const{
	double toReturn=0;
	for(uint i=0;i<doubleVector_h.size();i++){
		toReturn += doubleVector_h[i];
	}
	return toReturn;
}


void
BaseClass::tokenize(const string& str, vector<string>& tokens, const string& delimiter) const{
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
