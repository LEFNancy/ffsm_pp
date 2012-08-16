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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include <string>
#include <sstream>

#include <QMainWindow>
#include <QTextEdit>
#include <QLabel>

#include "ui_MainWindow.h"

// regmas headers..
#include "ThreadManager.h"
#include "ScenarioSelectionWidget.h"

using namespace std;

//class ScenarioSelectionWidget;

/// Main GUI interface

/**
MainWindow derive from both the generic Qt QMainWindow and from Ui::MainWindow (the latter being the autmatically generated C++ code from QtDesigner).
<br>It implements code and functionality that can not be done in the QtDesigner.
*/

class MainWindow : public QMainWindow, public Ui::MainWindow {
	Q_OBJECT

public:
	                    MainWindow(); ///< Constructor	

	void                setCurrentLogFileName(const QString &fileName);
	void                setCurrentModelFileName(const QString &fileName);
	bool                saveLogFile(const QString &logFileName);
	QString             strippedName(const QString &fullFileName);
	
	QString             getModelFileName(){return curModelFileName;};
	void                setModelFileName(const QString curModelFileName_h){curModelFileName=curModelFileName_h;};

public slots:
	void                setUnsavedStatus(bool unsavedStatus_h){unsavedStatus = unsavedStatus_h;};
	void                setOutputDirName(string outputDirName_h){outputDirName = outputDirName_h.c_str();};
	void                addLayer(QString layerName_h, QString layerLabel_h);
	void                switchToLayer(QString layerName_h);
	void                updatePixel(QString layerName_h, int x_h, int y_h, QColor color_h);
	void                updateImage(QString layerName_h, const QImage &image_h);
	void                switchToLayerFromLayerSelector(int layerIndex_h);
	/// Change value to an existing item in the Status Viewer
	void                treeViewerItemChangeValue(string itemID, string newValue);
	void                treeViewerItemRemove(string itemID);
	void                treeViewerAddItem(string text, string itemID, string parentID); ///< e.g. manager_farmer_manager agents or agent_12345_ownedHa
	void                processLogArea(const QString& message_h);
	void                resetGUIForNewSimulation(); ///< Reset the graphical elements for a new simulation
	///// Send the request of getting the pixel info to the main thread
	//void                sendQueryToMainThread(int px_ID);
	void                receiveScenarioOptions(const QVector<QString> &scenarios_h);


signals:
	void                currentModelFilenameChanged (QString);
	void                selectedScenarioName(const QString &scenarioName_h);

protected:
	void                closeEvent(QCloseEvent *event); ///< Manage the event of closing the application

private slots:
	void                open();
	bool                save();
	bool                saveAs();
	void                startModelMainThread();
	void                stopModelMainThread();
	void                pauseOrResumeModelMainThread();
	void                openRecentFile(); //already in the ui file ????
	void                hideDebugMsgs(bool hide);
	void                about();
	void                showDocumentation();

private:

	ThreadManager                modelMainThread;
	QLabel*                          yearSBLabel; ///< Status bar current year label
	QLabel*                          mainSBLabel; ///< Status bar main label
	bool                           unsavedStatus;
	QString                        outputDirName;
	QString                       curLogFileName;
	QString                     curModelFileName;
	QString                     curBaseDirectory;
	QStringList                      recentFiles;
	enum { MaxRecentFiles = 5 };
	QAction *  recentFileActions[MaxRecentFiles];
	QAction *                    separatorAction;
	bool                         debugMsgsEnable; ///< Allow debug messages to be show in the logArea
	ScenarioSelectionWidget      *scenarioWidget;
	/**
	Ids are based on the name of the item:
	- general
	- general_{name}
	- managers
	- manager_{managerID}
	- manager_{managerID}_{name}
	- agents
	- agent_{agentUniqueID}
	- agent_{agentUniqueID}_{name}
	*/
	map <string, QTreeWidgetItem*>       svIndex; ///< Map containing the ID and the pointers to the status viewer

	void                       createStatusBar();
	bool                          okToContinue();
	void                          readSettings();
	void                         writeSettings();
	void               updateRecentFileActions();
};

#endif
