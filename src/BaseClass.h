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
#ifndef BASECLASSBASECLASS_H
#define BASECLASSBASECLASS_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>

// regmas headers...

class ThreadManager;

using namespace std;

enum {

	// DON'T USE NEGATIVE NUMBERS, as often different meaning !
	PLOTS_ALL             = 0,
	PLOTS_OWNED           = 1,
	PLOTS_RENTED          = 2,

	STAGE_INIT            = 0,
	STAGE_NORMAL          = 1,

	MSG_NO_MSG            = 0,
	MSG_DEBUG             = 1,
	MSG_INFO              = 2,
	MSG_WARNING           = 3,
	MSG_ERROR             = 4,
	MSG_CRITICAL_ERROR    = 5,

	RESSOURCE_ZERO         =0,
	RESSOURCE_FIXED        =1,
	RESSOURCE_CALCULATED   =2,
	RESSOURCE_OBJECTS      =3,
	RESSOURCE_PIXEL        =4,

	RESTYPE_GENERIC        =0,
	RESTYPE_FINANCIAL      =1,
	RESTYPE_POLICYPREMIUM  =2,
	RESTYPE_LABOUR         =3,
	RESTYPE_STABLES        =4,
	RESTYPE_MACHINARY      =5,
	RESTYPE_FEED           =6,

	TYPE_INT               =0,
	TYPE_DOUBLE            =1,
	TYPE_STRING            =2,
	TYPE_BOOL              =3,

	DATA_INDIVIDUAL        =0,
	DATA_VECTOR            =1,
	DATA_NOW               =-1, // asking for data (settings) at current time
	DATA_ERROR             = -99999999999, // error in retrieving data
	DATA_INIT              = -2, // data referring to the init years

	OUTVL_NONE             =0, //output verbosity level
	OUTVL_MAPS             =5,
	OUTVL_AGGREGATED       =10,
	OUTVL_DETAILED         =15,
	OUTVL_BINMAPS          =20,
	OUTVL_ALL              =25,

	DOM_PRI_PR             =1, // domain of variables and constrains: primary, secondary, all products or all products over r2 couple regions (in-country commercial flows)
	DOM_SEC_PR             =2,
	DOM_ALL_PR             =3,
	DOM_R2_PRI_PR          =4,
	DOM_R2_SEC_PR          =5,
	DOM_R2_ALL_PR          =6,
	DOM_SCALAR             =7, // NOTUSED

	// domain of constrains variables (squares means a vector, r2n means neighbour -same r1- regions)
	// attention for the not used, as algorithm to handle them may not have been implemented or -as minimum- tested!!
	CDOM_SCALAR            =1,             // r1,r2,p
	CDOM_SCALAR_DOUBLE     =2,             // r1,r2,p,r2n and r1,r2n,p,r2
	CDOM_R2TO              =3,  // NOTUSED // r1,r2,p,[r2n]
	CDOM_R2FROM            =4,  // NOTUSED // r1,[r2n],p,r2
	CDOM_R2FROMTO          =5,             // r1,[r2n],p,[r2n]
	CDOM_PRI_PR            =6,             // r1,r2,[p_pr]
	CDOM_PRI_PR_AND_SCALAR =7,             // r1,r2,[p_pr]&&p
	CDOM_SEC_PR            =8,  // NOTUSED // r1,r2,[p_sec]
	CDOM_ALL_PR            =9,  // NOTUSED // r1,r2,[prd]
	CDOM_ALL               =10, // NOTUSED // [r1],[r2],[p]
	CDOM_ALLR2             =11, // NOTUSED // [r1],[r2],[p],[r2n]


	CONSTR_EQ              =1, // constrain of type equality
	CONSTR_LE0             =2, // constrain of type lower or equal than 0
	CONSTR_GE0             =3,  // constrain of type greater or equal 0

	// operations possible in certain contexts
	OP_SUM                 =1,
	OP_AVG                 =5,

	VAR_VOL                =1,
	VAR_AREA               =2,
	VAR_IN                 =3

};

// mathematical defines (not correctly implemented in some compilers, namely MS VisualStudio..)

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
 
#ifndef M_LN2
#define M_LN2 0.69314718055994530941723212145818
#endif
 
#ifndef M_LN10
#define M_LN10 2.3025850929940456840179914546844
#endif

// some macro for specific keywords in the model
#ifndef PROD_ALL
#define PROD_ALL "PROD_ALL" //< all primary and transformed products
#endif
#ifndef PROD_PRI
#define PROD_PRI "PROD_PRI" //< primary products
#endif
#ifndef PROD_SEC
#define PROD_SEC "PROD_SEC" //< secondary products
#endif
#ifndef DIAM_ALL
#define DIAM_ALL "DIAM_ALL" //< all diameter classes
#endif
#ifndef DIAM_PROD
#define DIAM_PROD "DIAM_PROD" //< diameter classes used for production (e.g. excluded the first one)
#endif
#ifndef DIAM_FIRST
#define DIAM_FIRST "DIAM_FIRST_CLASS" //< first diameter class (NOT used for production)
#endif
#ifndef FT_ALL
#define FT_ALL "FT_ALL" //< all forest types
#endif

/// Base class for the regmas application.
/**
This class is the base class for all classes in regmas. \\
It provides common methods in all parts of the application for printing the output, converting strings vs. values or regularly "ping" the GUI. \\ 
@author Antonello Lobianco
*/

class BaseClass{

public:
	                    BaseClass();
	                   ~BaseClass();

	void                msgOut(const int& msgCode_h, const string& msg_h, const bool& refreshGUI_h=true) const; ///< Overloaded function to print the output log
	void                msgOut(const int& msgCode_h, const int& msg_h, const bool& refreshGUI_h=true)    const; ///< Overloaded function to print the output log
	void                msgOut(const int& msgCode_h, const double& msg_h, const bool& refreshGUI_h=true) const; ///< Overloaded function to print the output log

	int                 s2i (const string& string_h)         const; ///<  string  to integer conversion
	double              s2d (const string& string_h) const; ///<  string  to double  conversion
	double              s2d (const string& string_h, const bool& replaceComma) const; ///<  string  to double  conversion
	bool                s2b (const string& string_h)          const; ///<  string  to bool    conversion
	// as of 20120909 c++11 to_string(), stoi and stod functions not working in MinGw, as bug http://gcc.gnu.org/bugzilla/show_bug.cgi?id=52015
	// reverting to old code :-(
	//string              i2s (const int& int_h)       const {return to_string(int_h);};    ///<  integer to string  conversion
	//string              d2s (const double& double_h) const {return to_string(double_h);}; ///<  double  to string  conversion
	string              i2s (const int& int_h)               const; ///<  integer to string  conversion
	string              d2s (const double& double_h)         const; ///<  double  to string  conversion
	string              b2s (const bool& bool_h)             const; ///<  bool    to string  conversion

	vector<int>         s2i (const vector<string>& string_h) const; ///<  string  to integer conversion (vector)
	vector<double>      s2d (const vector<string>& string_h, const bool& replaceComma = false) const; ///<  string to double conversion (vector)
	vector<bool>        s2b (const vector<string>& string_h) const; ///<  string  to bool    conversion (vector)
	vector<string>      i2s (const vector<int>& int_h)       const; ///<  integer to string  conversion (vector)
	vector<string>      d2s (const vector<double>& double_h) const; ///<  double  to string  conversion (vector)
	vector<string>      b2s (const vector<bool>& bool_h)     const; ///<  bool    to string  conversion (vector)

	/**
	Return a type according to enum TYPE_* from a string (eg: "string" -> TYPE_STRING (2)).
	<br>If we pass it an unknow type it generates an error.
	*/
	int                 getType(const string &type_h) const;

	void                refreshGUI() const;  ///< Ping to periodically return the control to the GUI

	//string intToString(int x);
	template<typename T> string toString(const T& x) const;      // string s = toString(x);
	template<typename T> T stringTo(const std::string& s) const; // e.g. int x = stringTo<int>("123");

	// vector and vector of vector sums
	int                   vSum(const vector<int>& vector_h) const {return accumulate(vector_h.begin(),vector_h.end(),0);};
	double                vSum(const vector<double>& vector_h) const {return accumulate(vector_h.begin(),vector_h.end(),0.);};
	int                   vSum(const vector< vector <int> >& vector_h) const;
	double                vSum(const vector< vector <double> >& vector_h) const;

	/// Tokenize a string using a delimiter (default is space)
	void                tokenize(const string& str, vector<string>& tokens, const string& delimiter = " ") const;

    /// Lookup a map for a value. Return the value starting from the key
	template <typename K, typename V> V findMap(const map <K, V> &mymap, const K& key, const int& error_level=MSG_CRITICAL_ERROR){
		typename map<K, V>::const_iterator p;
		p=mymap.find(key);
		if(p != mymap.end()) {
			return p->second;
		}
		else {
			msgOut(error_level, "Error in finding a value in a map (no value found)");
		}
	}

    /// Change the value stored in a map given the key and the new value
    template <typename K, typename V> void changeMapValue(map <K, V> &mymap, const K& key, const V& value, const int& error_level=MSG_CRITICAL_ERROR){
        typename map<K, V>::iterator p;
        p=mymap.find(key);
        if(p != mymap.end()) {
            p->second = value;
            return;
        }
        else {
            msgOut(error_level, "Error in finding a value in a map (no value found)");
        }
    }
    /// Increment a value stored in a map of the specified value, given the key
    template <typename K, typename V> void incrMapValue(map <K, V> &mymap, const K& key, const V& value, const int& error_level=MSG_CRITICAL_ERROR){
        typename map<K, V>::iterator p;
        p=mymap.find(key);
        if(p != mymap.end()) {
            p->second = p->second + value;
            return;
        }
        else {
            msgOut(error_level, "Error in finding a value in a map (no value found)");
        }
    }

    /// Reset all values stored in a map to the specified one
    template <typename K, typename V> void resetMapValues(map <K, V> &mymap, const V& value){
        typename map<K, V>::iterator p;
        for(p=mymap.begin(); p!=mymap.end(); p++) {
            p->second =value;
        }
    }

    /// Return a map built using the given vector and the given (scalar) value as keys/values pairs
    template <typename K, typename V> map <K, V> vectorToMap(const vector <K>& keys, const V& value=0.0){
        map<K,V> returnMap;
        for(uint i=0; i<keys.size();i++){
            pair<K,V> apair(keys[i],value);
            returnMap.insert(apair);
        }
        return returnMap;
    }


protected:

	/**
	Pointer to the Thread Manager. \\	
	Through this pointer each deribed subclass (the vast maiority of those used on FFSM) can "ask" for sending signals to the GUI, like append the log or modify the map.
	*/
	ThreadManager*                         MTHREAD; ///< Pointer to the Thread manager.
	// ATTENTION
	// I can't create member variables to host return values as these would break all the const mechanism..

private:
	void msgOut2(const int& msgCode_h, const string& msg_h, const bool& refreshGUI_h) const; ///< Do the job of the overloaded functions

};

#endif
