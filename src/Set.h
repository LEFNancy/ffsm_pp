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

#ifndef OPTSET_H
#define OPTSET_H

#include <cstdlib>
#include <list>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <sstream>

// regmas headers..



using namespace std;
/*
template <class K >
class Set {

public:
  //template <class K> optset(string name, int dims, vector<K> data, );
  //template <class K>
  //                     Set();
  Set(K data_h);
  string                                    name;
  int                                       dims;
  K                                         data;
  void setName(const string & name_h){name = name_h;};
  //template <class K>  setData(const K & data_h) {data=

};

*/
template <class T>
class Set {
public:
    Set (const string& name_h, T data_h);
    void doSomething(T param);
    T getData(){return data;};

    T      data;
    string name;
    int     dim;
};

#include "Set.cpp"


#endif // OPTSET_H
