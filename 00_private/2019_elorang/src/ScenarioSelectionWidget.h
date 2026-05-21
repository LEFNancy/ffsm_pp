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
#ifndef SCENARIOSELECTIONWIDGET_H
#define SCENARIOSELECTIONWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>
//#include <QtGui>

class QComboBox;

/**
Simple widget to show the available scenarios so that the user can choose one.

  @author Antonello Lobianco <antonello@regmas.org>
*/
class ScenarioSelectionWidget: public QDialog{
  Q_OBJECT

public:
  ScenarioSelectionWidget(QWidget *parent = 0);
  void receiveScenarioOptions(const QVector<QString> &scenarios_h);
  QComboBox                  *scenarioSelector;

private:
  QLabel                                *label;


  ~ScenarioSelectionWidget();

/*
signals:
  void selectedScenarioName(const QString &scenario_h);

public slots:
  void processSelectedScenario(const QString &scenario_h);
*/

};

#endif
