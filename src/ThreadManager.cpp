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
#include <iostream>

#include <QtCore>
//#include <QMutex>
#include <QMutexLocker>

#include "ThreadManager.h"
#include "BaseClass.h"
#include "MainProgram.h"
#include "MainWindow.h"

using namespace std;

ThreadManager::ThreadManager(){
	running=false;
	stopped=false;
	layerQueryPos = -1;

	// initializing pointers...
	RD = NULL;
	GIS = NULL;
	SAM = NULL;
	INIT = NULL;
	SCD = NULL;
	DO = NULL;
	
	GUI = false;

	scenarioName="";
	inputFileName="";
	baseDirectory="";

}

void
ThreadManager::setMessage(const QString &message){
    messageStr = message;
}

void ThreadManager::run(){
	running=true;
	stopped=false;

	srand(1);
	GUI=true;

	emit upgradeLogArea("**INFO: Start running the model...");

	MainProgram* myProgram;
	try{
		deleteDeadOldPointers();
		emit resetGUIForNewSimulation();


		QFileInfo file(inputFileName);
		QDir baseDir = file.absoluteDir();
		baseDirectory = baseDir.absolutePath()+"/";
		myProgram = new MainProgram(this);

		//myProgram->setBaseDirectory(baseDirectory);
	
		vector<string> scenarios = RD->getScenarios();
		QVector<QString> qscenarios;
		for(uint i=0;i<scenarios.size();i++){
			qscenarios.push_back(scenarios.at(i).c_str());
		}
		running = false;
		emit sendScenarioOptionsToGUI(qscenarios);
		refreshGUI();

		myProgram->run();

		// Here the model has come to an end...
		running=false;
		stopped=true;
		delete myProgram;
		refreshGUI();
		
	}catch (...) { // TODO.. perform a better exception handing..
		emit upgradeLogArea("**INFO: Model has stopped or rised an error (read previous line).");
	}	
	emit upgradeLogArea("**INFO: Model has ended.");
	
}

void
ThreadManager::retrieveScenarioNameFromGUI(const QString &scenarioName_h){
	scenarioName = scenarioName_h;
	msgOut(MSG_INFO, "Selected scenario: "+scenarioName.toStdString());
	cout << "Selected scenario: "+scenarioName.toStdString() << endl;
	resume();
}

void
ThreadManager::runFromConsole(QString inputFileName_h, QString scenarioName_h){
		GUI = false;
		scenarioName = scenarioName_h;
		inputFileName = inputFileName_h;
		QFileInfo file(inputFileName);
		QDir baseDir = file.absoluteDir();
		baseDirectory = baseDir.absolutePath()+"/";
		cout <<"Using base directory: "<< baseDirectory.toStdString() << endl;


		MainProgram* myProgram = new MainProgram(this);

		if( scenarioName_h == ""){ // if the scenario option has not been shoosed, go for the first one!
			vector<string> scenarios = RD->getScenarios();
			scenarioName = scenarios.at(0).c_str();
		}

		//myProgram->setBaseDirectory(baseDirectory);
		myProgram->run();
}

void
ThreadManager::setInputFileName(QString inputFileName_h){
	inputFileName= inputFileName_h;
	QFileInfo file(inputFileName);
	QDir baseDir = file.absoluteDir();
	baseDirectory = baseDir.absolutePath()+"/";
}

/**
Delete the pointers (e.g. GIS) eventually remained from a previous run.
<br>This function is called at the START of a new simulation, and it will check if model pointers (e.g. GIS) exist , and if so it will delete them.
<br>This is useful when we keep the MainWindow open but we run the model for a second time.
<br>Why we don't delete them at the end of a simulation, instead of deleting them on a new run? That's because we want let the user to interface with the model even when this is ended, w.g. for query the map.
*/
void
ThreadManager::deleteDeadOldPointers(){
	if (DO) {delete DO; DO=0;}
	if (SAM) {delete SAM; SAM=0;}
	if (INIT) {delete INIT; INIT=0;}
	if (SCD) {delete SCD; SCD=0;}
	if (GIS) {delete GIS; GIS=0;}
	if (RD) {delete RD; RD=0;}
}


void
ThreadManager::stop(){
    stopped = true;
	emit upgradeLogArea("STOP cliccked stopping");
}

void
ThreadManager::pauseOrResume(){
	if(!stopped){
		if(running){
			running= false;
			emit upgradeLogArea("PAUSE cliccked PAUSING");
		}
		else {
			running=true;
			emit upgradeLogArea("PAUSE cliccked RESUMING");
			emit setGUIUnsavedStatus(true);
		}
	}
	return;
}

void
ThreadManager::pause(){
	if(!stopped){
		if(running){
			running= false;
		}
		else {
			return;
		}
	}
	return;
}

void
ThreadManager::resume(){
	if(!stopped){
		if(running){
			return;
		}
		else {
			running=true;
			emit setGUIUnsavedStatus(true);
		}
	}
	return;
}

void
ThreadManager::refreshGUI(){
		checkQuery(0,0,false);
		while (!running){
			if(stopped){
				break;
			}
		}
		if (stopped){
			emit upgradeLogArea("Model has been stopped.");
			running= false;
			throw(2);
		}
}

void
ThreadManager::msgOut(const int msgCode_h, const string message_h){
	QString message = message_h.c_str();
	emit upgradeLogArea(message);
	if (msgCode_h == 2){
		emit upgradeMainSBLabelToGui(message);
	}
}

void
ThreadManager::setOutputDirName(string outputDirname_h){
	emit setOutputDirNameToGui(outputDirname_h);
}

void 
ThreadManager::addLayer(string layerName_h, string layerLabel_h){
	QString layerName = layerName_h.c_str();
	QString layerLabel = layerLabel_h.c_str();
	emit addLayerToGui(layerName, layerLabel);
}

void
ThreadManager::updatePixel(string layerName_h, int x_h, int y_h, QColor color_h){
	emit updatePixelToGui(layerName_h.c_str(), x_h, y_h, color_h);
}

void
ThreadManager::updateImage(string layerName_h, const QImage &image_h){
	emit updateImageToGui(layerName_h.c_str(), image_h);
}

void
ThreadManager::upgradeMainSBLabel(const string message_h){
	emit upgradeMainSBLabelToGui(message_h.c_str());
}

void
ThreadManager::upgradeYearSBLabel(int year){
	QString temp;
	temp= i2s(year).c_str();
	emit upgradeYearSBLabelToGui(temp);
}

/**
checkQuery() is a function that can be called my the GUI trough a signal or from the running thread under refreshGUI(), and it is protected with a mutex.
<br>It's role is to control the status of pxQueryID and layerQueryPos member variables.
<br>If the call come from the GUI, it is a new request and we set them to the new values, otherwise we gonna see if they are just beed changed and if so (layerQueryPos>=0) we call computeQuery().
*/
void
ThreadManager::checkQuery(int px_ID, int currentLayerIndex, bool newRequest){
	QMutexLocker locker(&mutex);
	if(newRequest){
		pxQueryID = px_ID;
		layerQueryPos = currentLayerIndex;
		if(stopped){computeQuery(pxQueryID, layerQueryPos);layerQueryPos = -1;} // model is stopped, no way the model thread will do the query work
		else{emit publishQueryResults("<i>..wait.. processing query..</i>");} // model is running.. it will be the model thread to execute the query		
		return;
	} else {
		if(layerQueryPos<0){
			return;
		} else {
			computeQuery(pxQueryID, layerQueryPos);
			layerQueryPos = -1;
			return;
		}
	}
}

void
ThreadManager::computeQuery(int px_ID, int currentLayerIndex){
	
	// IMPORTANT: this function is called at refreshGUI() times, so if there are output messages, call them with the option to NOT refresh the gui, otherwise we go to an infinite loop...

	vector<Layers*> layers;
	try {
		layers = GIS->getLayerPointers();
	}catch (...) {
		emit activateTab(2); // tell the gui to activate the 3rd page, those with the pixel info
		emit publishQueryResults("GIS pointer is dead.. maybe simulation has ended???");
		return;
	}
	QString result= "";
	int realID = GIS->sub2realID(px_ID);
	if (realID<0) {
		emit publishQueryResults("Query result: Spatial data is not yet ready in the model. Please click again later.");
		return; // on early stage we may have errors, and here we prevent this error to have further consequences.
	}
	Pixel* px;
	try {
		px = GIS->getPixel(realID);
	}catch (...) {
		emit activateTab(2); // tell the gui to activate the 3rd page, those with the pixel info
		emit publishQueryResults("Query result: Spatial data is not yet ready in the model. Please click again later.");
		return;
	}	
	result += "Pixel: ";
	result += i2s(realID).c_str();
	result += " (";
	result += i2s(px->getX()).c_str();
	result += ",";
	result += i2s(px->getY()).c_str();
	result += ")";
	result +="<p><table>";
	for (uint i=0;i<layers.size();i++){
		QString boldStart="";
		QString boldEnd = "";
		if (i == currentLayerIndex){
			boldStart = "<b>";
			boldEnd   = "</b>";
		}
		result += "<tr>";
		string layerName = layers[i]->getName();
		double value = px->getDoubleValue(layerName);
		string category = layers[i]->getCategory(value);
		//QColor color = layers[i]->getColor(value);
		result += "<td>";
		result += boldStart;
		result += layerName.c_str();
		result += boldEnd;
		result += "</td><td>";
		result += boldStart;
		result += category.c_str();
		result += boldEnd;
		result += "</td>";
		result += "</tr>";
	}
	result += "</table>";
	emit activateTab(2); // tell the gui to activate the 3rd page, those with the pixel info
	emit publishQueryResults(result);
}


