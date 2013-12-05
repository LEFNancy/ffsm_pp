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
#ifndef THREAD_H
#define THREAD_H

#include <iostream>
#include <string>
#include <sstream>
#include <random>
#include <cmath>

#include <QtCore>
#include <QThread>
#include <QString>
#include <QColor>
#include <QImage>
#include <QMutex>

#include "IpIpoptApplication.hpp"
#include "IpSolveStatistics.hpp"
#include "IpSmartPtr.hpp"

// regmas includes..
#include "BaseClass.h"

class MainWindow;
class ModelData;
class Gis;
class Init;
class Scheduler;
class Output;
class ModelCore;
class ModelCoreSpatial;
class Opt;
class Sandbox;

using namespace std;

/// Thread manager. Responsable to manage the main thread and "speak" with the GUI

/**
ThreadManager is responsable for the actions on the main thread (run/pause/resume/stop) and to speack with the GUI using the signal/slot tecniques.
@author Antonello Lobianco
*/
class ThreadManager : public QThread, public BaseClass
{
    Q_OBJECT

public:
	                    ThreadManager();
	// pointers..
	ModelData*          MD;    ///< the model data object
	Gis*                GIS;   ///< GIS information and methods
	Init*               INIT;  ///< the Init object (pre-simulation scheduler)
	Scheduler*          SCD;   ///< the scheduler object (simulation-loops scheduler)
	Output*             DO;    ///< data output
	ModelCore*          CORE;  ///< Core of the model
	ModelCoreSpatial*   SCORE; ///< Core of the model (spatial version)
	Sandbox*            TEST;  ///< Various debugging code for development
	Ipopt::SmartPtr <Ipopt::TNLP>      OPT;  ///< Market optimisation
    //std::random_device*  randev; ///< used in the sampling from normal distribution
    std::mt19937*        gen;  ///< used in the sampling from normal distribution

	void                setMessage(const QString &message);
	void                stop();
	void                deleteDeadOldPointers(); ///< Useful for several model running without leaving the GUI
	void                pauseOrResume();
	void                pause();
	void                resume();
	void                refreshGUI();
	void                msgOut(const int msgCode_h, const string message_h);
	void                addLayer(string layerName_h, string layerLabel_h);
	void                updatePixel(string layerName_h, int x_h, int y_h, QColor color);
	void                updateImage(string layerName_h, const QImage &image_h);
	void                upgradeMainSBLabel(const string message_h);
	void                upgradeYearSBLabel(int year);
	string              getBaseDirectory(){return baseDirectory.toStdString();};
	string              getInputFileName(){return inputFileName.toStdString();};
	string              getScenarioName() {return scenarioName.toStdString();};
	void                setOutputDirName(string outputDirname_h);

	/// the regional data object..
	void                setMDPointer(ModelData *MD_h){MD=MD_h;};
	/// GIS information and methods..
	void                setGISPointer(Gis *GIS_h){GIS=GIS_h;};
	/// the Init object, it schedule the pre-simulation phase..
	void                setINITPointer(Init *INIT_h){INIT=INIT_h;};
	/// the sandbox object for within-development quick tests
	void                setTestPointer(Sandbox *TEST_h){TEST=TEST_h;};
	/// the scheduler object. It manage the simulation loops..
	void                setSCDPointer(Scheduler *SCD_h){SCD=SCD_h;};
	/// manage the printing of data needed for scenario-analisys. The "message output" (needed to see "what is it happening?" are instead simply printed with msgOut()..
	void                setDOPointer(Output *DO_h){DO=DO_h;};
	/// Perform the algorithms of the model
	void                setCOREPointer(ModelCore* CORE_h){CORE=CORE_h;};
		/// Perform the algorithms of the model
	void                setSCOREPointer(ModelCoreSpatial* SCORE_h){SCORE=SCORE_h;};
	/// Perform the market optimisation
	void                setOPTPointer(Ipopt::SmartPtr<Ipopt::TNLP> OPT_h){OPT=OPT_h;};

	//public slots:
	void                setInputFileName(QString inputFileName_h);
	//void                setBaseDirectory(QString baseDirectory_h){baseDirectory =  baseDirectory_h;};

	// tree viewer operations...
	/*
	void               treeViewerAddManager(string name){emit treeViewerAddItemToGui("Manager "+name, "manager_"+name, "managers");};
	void               treeViewerAddAgent(int uniqueID){emit treeViewerAddItemToGui(i2s(uniqueID), "agent_"+i2s(uniqueID), "agents"); };
	void               treeViewerAddManagerProperty(string managerName, string propertyName){
	                        emit treeViewerAddItemToGui(propertyName, "manager_"+managerName+"_"+propertyName, "manager_"+managerName);};
	void	           treeViewerAddAgentProperty(int uniqueID, string propertyName){
	                        emit treeViewerAddItemToGui(propertyName, "agent_"+i2s(uniqueID)+"_"+propertyName, "agent_"+i2s(uniqueID));};
	void               treeViewerManagerPropertyChangeValue(string managerName, string propertyName, string newValue){
	                        emit treeViewerItemChangeValueToGui("manager_"+managerName+"_"+propertyName, newValue);};
	void               treeViewerAgentPropertyChangeValue(int uniqueID, string propertyName, string newValue){
	                        emit treeViewerItemChangeValueToGui("agent_"+i2s(uniqueID)+"_"+propertyName, newValue);};
	void               treeViewerRemoveManager(string name){emit treeViewerItemRemoveToGui("manager_"+name);};
	void               treeViewerRemoveAgent(int uniqueID){emit treeViewerItemRemoveToGui("agent_"+i2s(uniqueID));};
	*/
	void               treeViewerChangeGeneralPropertyValue(string propertyName, string newValue){
	                        emit treeViewerItemChangeValueToGui("general_"+propertyName, newValue);};


	void               fitInWindow() {emit fitInWindowToGui();}; ///< Re-draw the map making it to fit (with the right proportions) to the widget
	void                runFromConsole(QString inputFileName_h, QString scenarioName_h);
	bool                usingGUI(){return GUI;};

signals:
	void                upgradeLogArea(const QString &logMessage);
	void                upgradeMainSBLabelToGui(const QString &logMessage);
	void                upgradeYearSBLabelToGui(const QString &logMessage);
	void                addLayerToGui(QString layerName, QString layerLabel);
	void                updatePixelToGui(QString layerName_h, int x_h, int y_h, QColor color);
	void                updateImageToGui(QString layerName_h, QImage image_h);
	void                setOutputDirNameToGui(string outputDirname_h);
	void                setGUIUnsavedStatus(bool status_h);
	void                setGUIMapDimension(int x_h, int y_h);
	void                treeViewerItemChangeValueToGui(string itemID, string newValue);
	void                treeViewerItemRemoveToGui(string itemID);
	void                treeViewerAddItemToGui(string text, string itemID, string parentID);
	void                fitInWindowToGui();
	void                queryRequestOnPx(int px_ID, int currentLayerIndex);
	void                publishQueryResults(const QString &results);
	void                activateTab(int pos_h);
	void                resetGUIForNewSimulation();
	void                sendScenarioOptionsToGUI(const QVector<QString> &scenarios_h);


public slots:
	/// Switch and control the access to pxQueryID and layerQueryPos members
	void                checkQuery(int px_ID, int currentLayerIndex, bool newRequest=true);
	/// Compute the pixel query and return it to the GUI (with a signal)
	void                computeQuery(int px_ID, int currentLayerIndex);
	void                retrieveScenarioNameFromGUI(const QString &scenarioName_h);

protected:
	void                run();

private:
	QString                           messageStr;
	volatile bool                        stopped;
	volatile bool                        running;
	QString                        inputFileName;
	QString                        baseDirectory;
	QString                         scenarioName;
	volatile int                       pxQueryID;
	volatile int                   layerQueryPos;
	QMutex                                 mutex;
	bool                                     GUI;

};

#endif

