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

/**
 * \file BaseClass.h
 * \brief This file is the header of BaseClass and it is included by ALL compiled code.
 *
 * It contains also global enum and macro definitions that can be used anywhere in the code.
 * If the code require some "case" parameter, put the cases in the enum here.
 * DON'T USE NEGATIVE NUMBERS in the enums, as often negative numbers have a different meaning !
 *
 */

#ifndef BASECLASSBASECLASS_H
#define BASECLASSBASECLASS_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <limits>
#include <cstddef>
#include <fenv.h> // for division by zero runtime error

// regmas headers...

class ThreadManager;

using namespace std;

/// Type of message to be printed
enum messageType{

  MSG_NO_MSG            = 0,             ///< Do not actually output any message
  MSG_DEBUG             = 1,             ///< Print a debug message, normally filtered out
  MSG_INFO              = 2,             ///< Print an INFO message
  MSG_WARNING           = 3,             ///< Print a WARNING message
  MSG_ERROR             = 4,             ///< Print an ERROR message, but don't stop the model
  MSG_CRITICAL_ERROR    = 5,             ///< Print an error message and stop the model
};
/// Type of data requested
enum dataType{
  TYPE_INT               =0,             ///< The required data is an integer
  TYPE_DOUBLE            =1,             ///< The required data is a double
  TYPE_STRING            =2,             ///< The required data is a string
  TYPE_BOOL              =3,             ///< The required data is a bool
};
/// A generic enum to deal with data requests
enum dataRequest {
  DATA_NOW               =-1,            ///< The required data is for the current year
  DATA_INIT              = -2,           ///< Setting a data request for the init period
  DATA_ERROR             = -99999999999, ///< There is an error in retrieving the data
  // operations possible in certain contexts
  OP_SUM                 =1,             ///< Perform a SUM operation
  OP_AVG                 =5,             ///< Perform an AVERAGE operation
};
/// Verbosity level of the output
enum outputVerbosity {
  OUTVL_NONE             =0,             ///< Output verbosity level none
  OUTVL_MAPS             =5,             ///< Output verbosity level print the maps
  OUTVL_AGGREGATED       =10,            ///< Output verbosity level print (also) aggregated output
  OUTVL_DETAILED         =15,            ///< Output verbosity level print (also) detailed output
  OUTVL_BINMAPS          =20,            ///< Output verbosity level print (also) binary (png) maps
  OUTVL_ALL              =25,            ///< Output verbosity level print everything
};
/// Domain associated to a variable or a constrain in the optimisation of the market module
enum domains {
  DOM_PRI_PR             =1,             ///< Primary products // domain of variables and constrains: primary, secondary, all products or all products over r2 couple regions (in-country commercial flows)
  DOM_SEC_PR             =2,             ///< Secondary products
  DOM_ALL_PR             =3,             ///< All products (primary+secondary)
  DOM_R2_PRI_PR          =4,             ///< Primary products over r2 couple regions (in-country commercial flows)
  DOM_R2_SEC_PR          =5,             ///< Secondary products over r2 couple regions (in-country commercial flows)
  DOM_R2_ALL_PR          =6,             ///< All products over r2 couple regions (in-country commercial flows)
  DOM_SCALAR             =7,             ///< Scalar variable (not used)
  DOM_PRI_PR_ALLCOMBS    =8,             ///< All possible combinations of primary products (2^ number of primary products)
};
/// Carbon stocks
enum carbonStocks {
  STOCK_INV              =1,             ///< Invetoried biomass (live and death tree logs)
  STOCK_EXTRA            =2,             ///< Extra biomass (soils, branches..)
  STOCK_PRODUCTS         =3,             ///< Biomass in forest products (sawns, pannels..)
};
/// Emission types
enum emissionType {
  EM_ENSUB               =4,             ///< Energy substitution
  EM_MATSUB              =5,             ///< Material substitution
  EM_FOROP               =6,             ///< Flow from forest operations
};

enum contrainDirection {
  CONSTR_EQ              =1, // constrain of type equality
  CONSTR_LE0             =2, // constrain of type lower or equal than 0
  CONSTR_GE0             =3, // constrain of type greater or equal 0
};

enum varType {
  VAR_VOL                =1,
  VAR_AREA               =2,
  VAR_IN                 =3
};

enum boundType {
  LBOUND                 =1,
  UBOUND                 =2
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
#define PROD_ALL "PROD_ALL"           ///< All primary and transformed products
#endif
#ifndef PROD_PRI
#define PROD_PRI "PROD_PRI"           ///< Primary products
#endif
#ifndef PROD_SEC
#define PROD_SEC "PROD_SEC"           ///< Secondary products
#endif
#ifndef DIAM_ALL
#define DIAM_ALL "DIAM_ALL"           ///< All diameter classes
#endif
#ifndef DIAM_PROD
#define DIAM_PROD "DIAM_PROD"         ///< Diameter classes used for production (e.g. excluded the first one)
#endif
#ifndef DIAM_FIRST
#define DIAM_FIRST "DIAM_FIRST_CLASS" ///< First diameter class (NOT used for production)
#endif
#ifndef FT_ALL
#define FT_ALL "FT_ALL"               ///< All forest types
#endif

// Bounds for optimisation
#ifndef LBOUND_MIN
#define LBOUND_MIN -20000000000000000000.0 ///< Lower bound in optimisation -10^19
#endif
#ifndef UBOUND_MAX
#define UBOUND_MAX 20000000000000000000.0  ///< Upper bound in optimisation 10^19
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

  int                 getType(const string &type_h) const; ///< Return a type according to enum TYPE_* from a string (eg: "string" -> TYPE_STRING (2))
  void                refreshGUI() const;                  ///< Ping to periodically return the control to the GUI

  //string intToString(int x);
  template<typename T> string toString(const T& x) const;      // string s = toString(x);
  template<typename T> T stringTo(const std::string& s) const; // e.g. int x = stringTo<int>("123");

  // vector and vector of vector sums
  int                   vSum(const vector<int>& vector_h) const {return accumulate(vector_h.begin(),vector_h.end(),0);};
  double                vSum(const vector<double>& vector_h) const {return accumulate(vector_h.begin(),vector_h.end(),0.);};
  int                   vSum(const vector< vector <int> >& vector_h) const;
  double                vSum(const vector< vector <double> >& vector_h) const;

  /// Tokenize a string using a delimiter (default is space)
  void                tokenize(const string& str, vector<string>& tokens, const string& delimiter = " ") const; // See also http://stackoverflow.com/questions/236129/split-a-string-in-c that could be faster
  void                untokenize(string &str, vector<string>& tokens, const string& delimiter = " ") const;

  /// Lookup a map for a value. Return the value starting from the key
  template <typename K, typename V> V findMap(const map <K, V> &mymap, const K& key, const int& error_level=MSG_CRITICAL_ERROR, const V &notFoundValue=numeric_limits<V>::min()) const{
    typename map<K, V>::const_iterator p;
    p=mymap.find(key);
    if(p != mymap.end()) {
      return p->second;
    }
    else {
      msgOut(error_level, "Error in finding a value in a map (no value found)");
      return notFoundValue;
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

  /// Increments a value stored in a map of the specified value, given the key
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

  /// Increments a value stored in a map of the specified value, given the key
  template <typename K, typename V> void incrOrAddMapValue(map <K, V> &mymap, const K& key, const V& value){
      typename map<K, V>::iterator p;
      p=mymap.find(key);
      if(p != mymap.end()) {
          // We found the key, we gonna add the value..
          p->second = p->second + value;
          return;
      }
      else {
          // We didn't find the key, we gonna add it together with the value
          pair<K,V> myPair(key,value);
          mymap.insert(myPair);
      }
  }

  /// Reset all values stored in a map to the specified one
  template <typename K, typename V> void resetMapValues(map <K, V> &mymap, const V& value){
      typename map<K, V>::iterator p;
      for(p=mymap.begin(); p!=mymap.end(); p++) {
          p->second =value;
      }
  }

  /// Returns a map built using the given vector and the given (scalar) value as keys/values pairs
  template <typename K, typename V> map <K, V> vectorToMap(const vector <K>& keys, const V& value=0.0){
      map<K,V> returnMap;
      for(unsigned int i=0; i<keys.size();i++){
          pair<K,V> apair(keys[i],value);
          returnMap.insert(apair);
      }
      return returnMap;
  }

  /// Returns the position of the maximum element in the vector (the last one in case of multiple equivalent maxima)
  template <typename K>  int getMaxPos (const vector <K> & v){
    return (minmax_element(v.begin(), v.end()).second - v.begin());
  }
  /// Returns the position of the minimum element in the vector (the first one in case of multiple equivalent minima)
  template <typename K>  int getMinPos (const vector <K> & v){
    return (minmax_element(v.begin(), v.end()).first - v.begin());
  }
  /// Returns the value of the maximum element in the vector (the last one in case of multiple equivalent maxima)
  template <typename K>  int getMax(const vector <K> & v){
    return *minmax_element(v.begin(), v.end()).second;
  }
  /// Returns the value of the minimum element in the vector (the first one in case of multiple equivalent minima)
  template <typename K>  int getMin (const vector <K> & v){
    return *minmax_element(v.begin(), v.end()).first;
  }

  /// Sample from a normal distribution with bounds. Slower (double time, but still you see the diff only after milion of loops).

  /// It doesn't require the normal_distribution to be passed to it, but due to including MTHREAD its definition can't be placed
  /// in the header and hence it can not be templated, so it works only with doubles.
  double normSample (const double& avg, const double& stdev, const double& minval=NULL, const double& maxval=NULL) const;

  /// Sample from a normal distribution with bounds. Faster (half time) as the normal_distribution is made only once.
  template <typename K> K normSample (normal_distribution<K>& d, std::mt19937& gen, const K& minval=NULL, const K& maxval=NULL) const {
    if(minval != NULL  && maxval != NULL){
      if (maxval <= minval){
        msgOut(MSG_CRITICAL_ERROR,"Error in normSample: the maxvalue is lower than the minvalue");
      }
    }
    for(;;){
      K c = d(gen);
      if( (minval == NULL || c >= minval) && (maxval == NULL || c <= maxval) ){
        return c;
      }
    }
    return minval;
  }




protected:

  /**
  * Through this pointer each derived subclass (the vast maiority of those used on FFSM) can "ask"
  * for sending signals to the GUI, like append the log or modify the map.
  */
  ThreadManager*                         MTHREAD; ///< Pointer to the Thread manager.
  // ATTENTION
  // I can't create member variables to host return values as these would break all the const mechanism..

private:
  void msgOut2(const int& msgCode_h, const string& msg_h, const bool& refreshGUI_h) const; ///< Do the job of the overloaded functions

};

/// Class to provide a simple integer-string key to be used in std maps
class iskey {
public:
                      iskey();
                      iskey(int i_h, string s_h);
                     ~iskey();
  bool                operator == (const iskey &op2) const;
  bool                operator != (const iskey &op2) const;
  bool                operator <  (const iskey &op2) const;
  bool                operator >  (const iskey &op2) const;
  bool                operator <= (const iskey &op2) const;
  bool                operator >= (const iskey &op2) const;
  int                                          i;
  string                                       s;
};

/// Class to provide a simple integer-integer-string key in std maps
class iiskey {
public:
                      iiskey();
                      iiskey(int i_h, int i2_h, string s_h);
                     ~iiskey();
  bool                operator == (const iiskey &op2) const;
  bool                operator != (const iiskey &op2) const;
  bool                operator <  (const iiskey &op2) const;
  bool                operator >  (const iiskey &op2) const;
  bool                operator <= (const iiskey &op2) const;
  bool                operator >= (const iiskey &op2) const;
  int                                          i;
  int                                          i2;
  string                                       s;

};

#endif
