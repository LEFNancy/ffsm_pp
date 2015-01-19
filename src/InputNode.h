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
#ifndef INPUTNODE_H
#define INPUTNODE_H

#include <iostream>
#include <cstdlib>

#include <string>
#include <sstream>
#include <stdexcept>
#include <list>
#include <vector>

#include <QDomElement>

//regmas headers...
#include "BaseClass.h"

using namespace std;

//class QDomElement;

///Wrapper around the underlying library for reading DOM elements (nodes).

/**
A small wrapper class using an underlying library (currently QtXml) to read DOM nodes.
<br>This class works with the individual nodes (DOM Elements), while the companion class InputDocument wrapper the whole document (DOM Document).
<br>Note: In the DOM terminology "Elements" are a subset of the more general "nodes" (that include comments and other typologies..)
@author Antonello Lobianco
*/
class InputNode: public BaseClass{

public:
                      InputNode();
                      InputNode(QDomElement domElement_h){domElement=domElement_h;}; //<Constructor
                     ~InputNode();
  bool                setWorkingFile    (std::string  filename_h); ///< Load the file on memory. Return false if no success.
  int                 getIntContent();    ///< Get the content between its tagName as integer
  double              getDoubleContent(); ///< Get the content between its tagName as double
  string              getStringContent(); ///< Get the content between its tagName as std::string
  bool                getBoolContent();   ///< Get the content between its tagName as bool 
  int                 getIntAttributeByName(string attributeName_h);    ///< Get an attribute by name as integer
  double              getDoubleAttributeByName(string attributeName_h); ///< Get an attribute by name as double
  string              getStringAttributeByName(string attributeName_h); ///< Get an attribute by name as string
  bool                hasAttributeByName(string attributeName_h);     ///< Check if an attribute with a certain name exist
  InputNode           getNodeByName     (string nodeName_h, int debugLevel=MSG_CRITICAL_ERROR, bool childFlag=false);  ///< return 0-or-1 nodes by name.
  vector <InputNode>  getNodesByName    (string nodeName_h, int debugLevel=MSG_WARNING, bool childFlag=false);  ///< return 0-to-n nodes by name
  /// Retrieve a child node with gived name and optionally with gived attribute or gived pair attribute/value. It raises an error if more than one.
  //InputNode           getNode(string nodeName_h, string attributeName_h="", string attributeValue_h="", int debugLevel=MSG_WARNING);
  /// Retrieve all child nodes with gived name and optionally with gived attribute or gived pair attribute/value. It raises an error if more than one.
  //vector <InputNode>          getNodes(string nodeName_h, string attributeName_h="", string attributeValue_h="", int debugLevel=MSG_WARNING);

  
  vector <InputNode>  getChildNodes();                            ///< Filtered to return only child <b>Elements</b>
  bool                hasChildNode(string name_h);                ///< True if it has specified child node
  int                 getChildNodesCount();                       ///< Only <b>Elements</b>
  string              getNodeName();

private:
  QDomElement                       domElement; ///< The underlying library-depending DOM rappresentation of the element

};

#endif
