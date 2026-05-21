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
#ifndef LAYERS_H
#define LAYERS_H
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>

#include <QColor>

// regmas headers...
#include "BaseClass.h"

using namespace std;

struct LegendItems;
struct ReclassRules;

/// Define layer objects at the regional level
/**
Layer class (setting, legend...)
<br>This class define layer objects, including: 
 - a set of layer proprieties (name(ID), label, associated dataset, typology (integer or double) 
 - a vector of legend items, associating one color to each value or interval 
 - a vector of reclassification rule, when we need to work with a level of depth different of those coming with the dataset
@author Antonello Lobianco <antonello@regmas.org>
*/
class Layers : public BaseClass{

public:
  /// In the constructor we set the main layer properties
                      Layers(  ThreadManager*     MTHREAD_h,
                               string                name_h,
                               string               label_h,
                               bool             isInteger_h,
                               bool        dynamicContent_h,
                               string        fullFilename_h,
                               bool               display_h=true);
                     ~Layers();
  /// Add a legend item. @see LegendItems
  void                addLegendItem( int             ID_h,
                                     string       label_h,
                                     int         rColor_h,
                                     int         gColor_h,
                                     int         bColor_h,
                                     double    minValue_h,
                                     double    maxValue_h   );
  void                addLegendItems(vector <LegendItems> legendItems_h);
  vector<LegendItems> getLegendItems(){return legendItems;};

  /// Evaluates all the legend items to find the one that match the input code, and return its color as a QColor
  QColor              getColor(double ID_h);
  /// Evaluates all the legend items to find the one that match the input code, and return its label
  string              getCategory(double ID_h);
  /// Used to reclassify the land use map for "generic" categories
  double              filterExogenousDataset(double code_h);
  /// Count the pixels going to each legend item and print them if debug==true
  void                countMyPixels(bool debug=false);
  /// For some sensitivity analisys, random the values for this layer for not-empty values (only integer layers)
  void                randomShuffle();
  /// Return if the layer is integer or not (If integer on each legend item: minValue==maxValue==ID)
  bool                getIsInteger() {return isInteger;}
  /// Print the layer content as an ASCII grid map with its companion files (classification and colors). It always print the whole region, even when subregion is actived.
  void                print();
  /// Print a binary reppresentation of the data (a standard image, e.g. a .png file). It prints only the subregion if this is active.
  void                printBinMap();

  string              getName() const {return name;}
  /// Return the filename of the associated dataset
  string              getFilename(){return fullFileName;}
  /// Return true if the content may change during simulation period
  bool                getDynamicContent(){return dynamicContent;}
  bool                getDisplay(){return display;}


private:
  string                                  name; ///< ID of the layer (no spaces allowed)
  string                                 label; ///< Label of the layer (spaces allowed)
  bool                               isInteger; ///< Type of the layer (true==integer, false==double. If true, on each legend item: minValue==maxValue==ID)
  bool                          dynamicContent; ///< True if the content may change during simulation year
  bool                                 display; ///< Normally true, but some layers used to just keep data shoudn't be normally processed
  string                          fullFileName; ///< Filename of the associated dataset (map)  
  vector<LegendItems>              legendItems; ///< Vector of legend items. @see LegendItems
  vector<ReclassRules>      reclassRulesVector; ///< Vector of initial reclassification rules. @see ReclassRules
};

/// Legend items

/**
Struct containing data about the programm settings.
<br>The minValue and the maxValue are used to compare one record value and return the right color. If the layer is of type integer (isInteger==true), minValue==maxValue==ID.
@author Antonello Lobianco
*/
struct LegendItems {
  int                                       ID;
  string                                 label;
  int                                   rColor;
  int                                   gColor;
  int                                   bColor;
  double                              minValue;
  double                              maxValue;
  int                              cashedCount; ///< count the pixels whitin a item range
};

/// Initial reclassification rules (dataset filters)

/**
A structure for easy reclassification of "mixed" categories in some layers.
<br>The reclassification can be made to both <i>increase</i> depth or <i>decrease</i> depth to the original dataset.
<br>Eg, if in our model we don't differ between coniferous and hardwood forests, we can set all them to be "forest".
<br>At the opposite, if our model require more detail than the map provide, e.g. irrigable arable VS dry arable, we can set the generic "arable land" of becoming "arable" or "dry" according with a regional-defined probability (getted from other sources, e.g. census data).
@author Antonello Lobianco
*/
struct ReclassRules{
  int                                   inCode;
  int                                  outCode;
  /// Probability that one pixel of code inCode will become of code outCode. 1 for fixed transformation.
  double                                     p;
};

#endif
