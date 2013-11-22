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
//#include <QtGui>   // Qt4
#include <QtWidgets> // Qt5
#include <QtXml>

#include "InputNode.h"
//#include "InputDocument.h"


InputNode::InputNode(){
}

InputNode::~InputNode(){
}

bool
InputNode::setWorkingFile(std::string filename_h){

	QString errorStr;
	int errorLine;
	int errorColumn;
	
	QFile file(filename_h.c_str());
	QIODevice* device;
	device = &file;
		
	QDomDocument doc;
	if (!doc.setContent(device, true, &errorStr, &errorLine, &errorColumn)) {
		string message = "XML error on file "+ filename_h + " at line ";
		message.append(i2s(errorLine));
		message.append(" column ");
		message = message.c_str() + i2s(errorColumn);
		message = message + ": ";
		message = message + errorStr.toStdString();
		msgOut(MSG_WARNING, message.c_str());
		return false;
    }
	domElement = doc.documentElement();
	return true;
}

// *******************************************************************************
int
InputNode::getIntContent(){
	return domElement.text().toInt();
}

double
InputNode::getDoubleContent(){
	return domElement.text().toDouble(); // This is a Qt function that works both with dot and comma separators !
}

std::string
InputNode::getStringContent(){
	return domElement.text().toStdString();
}

bool
InputNode::getBoolContent(){
	string content = domElement.text().toStdString();
	if (content == "false" || content == "falso" || content == "FALSE" || content == "0")
		return false;
	else if (content == "true" || content == "vero" || content == "TRUE" || content == "1")
		return true;
	msgOut(MSG_WARNING, "Sorry, I don't know how to convert "+content+" to a bool value. I return true... hope for the best");
	return true;
}

int 
InputNode::getIntAttributeByName(std::string attributeName_h){
	if (domElement.hasAttribute(attributeName_h.c_str())){
		return domElement.attribute(attributeName_h.c_str()).toInt();
	}else{
		msgOut(MSG_ERROR, "Element doens't have attribute " + attributeName_h );
		return 0;
	}
}

double 
InputNode::getDoubleAttributeByName(std::string attributeName_h){
	if (domElement.hasAttribute(attributeName_h.c_str())){
		return domElement.attribute(attributeName_h.c_str()).toDouble();
	}else{
		msgOut(MSG_ERROR, "Element doens't have attribute " + attributeName_h );
		return 0;
	}
}

string 
InputNode::getStringAttributeByName(std::string attributeName_h){
	if (domElement.hasAttribute(attributeName_h.c_str())){
		return domElement.attribute(attributeName_h.c_str()).toStdString();
	}else{
		msgOut(MSG_ERROR, "Element doens't have attribute " + attributeName_h );
		return "";
	}
}

bool 
InputNode::hasAttributeByName(std::string attributeName_h){
	if (domElement.hasAttribute(attributeName_h.c_str())){
		return 1;
	}else{
		return 0;
	}
}

InputNode
InputNode::getNodeByName(string nodeName_h, int debugLevel, bool childFlag){
	/*
	QDomNodeList myElementList = domElement.elementsByTagName ( nodeName_h.c_str() );
	if (myElementList.size()>1){
		msgOut(debugLevel, "Too many elements. Expected only one of type "+nodeName_h);
	}
	if (myElementList.isEmpty()){
		msgOut(debugLevel, "No elements in the XML file. Expected 1 of type "+nodeName_h);
	}
	QDomElement myElement = myElementList.item(0).toElement() ;
	InputNode myInputNode(myElement);
	return myInputNode; */
	vector<InputNode> myNodes = getNodesByName(nodeName_h, debugLevel, childFlag);
	if (myNodes.size()>1){
		msgOut(debugLevel, "Too many elements. Expected only one of type "+nodeName_h);
		return myNodes[0];
	}
	if (myNodes.size() == 0){
		msgOut(debugLevel, "No elements in the XML file. Expected 1 of type "+nodeName_h+". Returning emty node!!");
		InputNode toReturn;
		return toReturn;
	}
	return myNodes[0];
}

vector <InputNode>
InputNode::getNodesByName(string nodeName_h, int debugLevel, bool childFlag){
	vector <InputNode> myNodeVector;
	if (!childFlag){
		QDomNodeList myElementList = domElement.elementsByTagName ( nodeName_h.c_str() );
		for (int i=0;i<myElementList.size();i++){
			InputNode myInputNode(myElementList.item(i).toElement());
			myNodeVector.push_back(myInputNode);
		}
		
	}
	else {
		QDomNodeList myElementList = domElement.childNodes();
		for (int i=0;i<myElementList.size();i++){
			if (   myElementList.item(i).nodeType() == QDomNode::ElementNode
			    && myElementList.item(i).toElement().tagName().toStdString() == nodeName_h){
				InputNode myInputNode(myElementList.item(i).toElement());
				myNodeVector.push_back(myInputNode);
			}
		}
	}
	if (myNodeVector.size()==0){
		msgOut(debugLevel, "No elements in the XML file. Expected at least one of type "+nodeName_h);
	}
	//for (int i=0;i<myElementList.size();i++){
	//	InputNode myInputNode(myElementList.item(i).toElement());
	//	myNodeVector.push_back(myInputNode);

		/*InputNode myInputNode(myElementList.item(i).toElement());
		string firstNodeContent= myInputNode.getStringContent();
		// checking that the setting is not an empy line nor a comment (e.g. starting with "#")..
		if(firstNodeContent=="") continue;
		unsigned int z;
		z = firstNodeContent.find("#");
		if( z!=string::npos && z == 0) continue;
		// chacking also the "childs" as in the XMLs deriving from csv I want delete the whole "<record>" tree, including his childs (fields)
		vector <InputNode> childs = myInputNode.getChildNodes();
		if(childs.size()>0){
			string firstChildContent= childs[0].getStringContent();
			// checking that the setting is not an empy line nor a comment (e.g. starting with "#")..
			if(firstChildContent=="") continue;
			unsigned int y;
			y = firstChildContent.find("#");
			if( y!=string::npos && y == 0) continue;
		}
		myNodeVector.push_back(myInputNode);
		*/


	//}
	return myNodeVector;
}


/*
InputNode
InputNode::getNode(string nodeName_h, string attributeName_h, string attributeValue_h, int debugLevel){
	vector <InputNode> nodes = getNodes(nodeName_h, attributeName_h, attributeValue_h, debugLevel);
	if (nodes.size()>1){
		msgOut(debugLevel,"I got more than one node with specified carhacteristics. Returned the first one or aborting.");
		return nodes[0];
	} else if (nodes.size() == 0) {
		msgOut(debugLevel,"I don't have any node with the requested parameters. Returning an empty node.");
		InputNode toReturn;
		return toReturn;
	} else {
		return nodes[0];
	}
}

vector <InputNode>
InputNode::getNodes(string nodeName_h, string attributeName_h, string attributeValue_h, int debugLevel){
	vector <InputNode> nodes;

	return nodes;

}
*/


vector <InputNode>
InputNode::getChildNodes(){
	vector <InputNode> myNodeVector;
	QDomNodeList myElementList = domElement.childNodes();
	for (int i=0;i<myElementList.size();i++){
		if (myElementList.item(i).nodeType() == QDomNode::ElementNode ){
			InputNode myInputNode(myElementList.item(i).toElement());
			myNodeVector.push_back(myInputNode);
		}
	}
	return myNodeVector;
}

bool
InputNode::hasChildNode(string name_h){
	bool toReturn = false;
	QDomNodeList myElementList = domElement.childNodes();
	for (int i=0;i<myElementList.size();i++){
		if (myElementList.item(i).nodeType() == QDomNode::ElementNode ){
			if(myElementList.item(i).toElement().tagName().toStdString() == name_h) return true;
		}
	}
	return toReturn;
}

int
InputNode::getChildNodesCount(){
	int myElementListCountInt = 0;
	QDomNodeList myElementList = domElement.childNodes();
	for (int i=0;i<myElementList.size();i++){
		if (myElementList.item(i).nodeType() == QDomNode::ElementNode ){
			myElementListCountInt++ ;
		}
	}
	return myElementListCountInt;
}

string
InputNode::getNodeName(){
	return domElement.tagName().toStdString();
}

