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

//#include "Set.h"

using namespace std;

// http://stackoverflow.com/questions/28942758/
namespace std
{
    template<typename T>
    struct rank< std::vector<T>> : public std::integral_constant<std::size_t, rank<T>::value + 1>
    {};
}

template <class T>
Set<T>::Set (const string & name_h, T data_h) {
  data=data_h;
  int dim = std::rank< decltype(data_h)>::value;
  name = name_h;
};


template <class T>
void Set<T>::doSomething(T param){
    cout << "Hello World" << endl;
    cout << param << endl;
    exit(0);
};

