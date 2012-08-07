/***************************************************************************
 *   Copyright (C) 2006-2008 by Antonello Lobianco                         *
 *   http://regmas.org                                                     *
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

	OUTVL_NONE             =0, //output verbosity level
	OUTVL_MAPS             =3,
	OUTVL_AGGREGATED       =3,
	OUTVL_DETAILED         =5,
	OUTVL_BINMAPS          =6,
	OUTVL_ALL              =7,

	VDOM_PRI_PR                 =1, // domain of the variable: primary, secondary, all products or all products over r2 couple regions (in-country commercial flows)
	VDOM_SEC_PR                 =2,
	VDOM_ALL_PR                 =3,
	VDOM_R2_ALL_PR              =4

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

	void                msgOut(int msgCode_h, string msg_h, bool refreshGUI_h=true) const; ///< Overloaded function to print the output log
	void                msgOut(int msgCode_h, int msg_h, bool refreshGUI_h=true) const;    ///< Overloaded function to print the output log
	void                msgOut(int msgCode_h, double msg_h, bool refreshGUI_h=true) const; ///< Overloaded function to print the output log	

	int                 s2i ( string string_h)         const; ///<  string  to integer conversion
	double              s2d (string string_h)          const; ///<  string  to double  conversion
	bool                s2b (string string_h)          const; ///<  string  to bool    conversion
	string              i2s (int int_h)                const; ///<  integer to string  conversion
	string              d2s (double double_h)          const; ///<  double  to string  conversion
	string              b2s (bool bool_h)              const; ///<  bool    to string  conversion
	vector<int>         s2i ( vector<string> string_h) const; ///<  string  to integer conversion (vector)
	vector<double>      s2d (vector<string> string_h)  const; ///<  string  to double  conversion (vector)
	vector<bool>        s2b (vector<string> string_h)  const; ///<  string  to bool    conversion (vector)
	vector<string>      i2s (vector<int> int_h)        const; ///<  integer to string  conversion (vector)
	vector<string>      d2s (vector<double> double_h)  const; ///<  double  to string  conversion (vector)
	vector<string>      b2s (vector<bool> bool_h)      const; ///<  bool    to string  conversion (vector)

	/**
	Return a type according to enum TYPE_* from a string (eg: "string" -> TYPE_STRING (2)).
	<br>If we pass it an unknow type it generates an error.
	*/
	int                 getType(string &type_h);

	void                refreshGUI() const;  ///< Ping to periodically return the control to the GUI
	
	//string intToString(int x);
	template<typename T> string toString(const T& x);      // string s = toString(x);
	template<typename T> T stringTo(const std::string& s); // e.g. int x = stringTo<int>("123");

	int                 iVSum (const vector<int> &intVector_h);       ///< integer vector sum - sum of an vector of integer;
	double              dVSum (const vector<double> &doubleVector_h); ///< double vector sum - sum of a vectr of double;

	/// Tokenize a string using a delimiter (default is space)
	void                tokenize(const string& str, vector<string>& tokens, const string& delimiter = " ");

protected:

	/**
	Pointer to the Thread Manager. \\	
	Through this pointer each deribed subclass (the vast maiority of those used on Pythia) can "ask" for sending signals to the GUI, like append the log or modify the map.
	*/
	ThreadManager*                       MTHREAD;                       ///< Pointer to the Thread manager.

private:
	void msgOut2(int msgCode_h, string msg_h, bool refreshGUI_h) const; ///< Do the job of the overloaded functions

};

#endif
