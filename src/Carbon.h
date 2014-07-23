
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
#ifndef CARBON_H
#define CARBON_H

// Core C++ headers
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <time.h>

//regmas headers
#include "BaseClass.h"

/// Class responsable to keep the logbook of the Carbon Balance
/**
@author Antonello Lobianco

A single istance of this class exists and is available trought the global MTHREAD->CBAL pointer.

It consits of functions to track a carbon-related event and store the information in STL maps that either register the events (for the stocks) or contain the cumulated carbon (for emission flows).

Carbon pools are stored as Mm^3 wood while and emission cumulated counters are directly in Mt CO2.

getStock() and getCumSavedEmissions() are then used to report the current levels of carbon in the stock or emitted/substituted.
*/

class Carbon: public BaseClass{
public:
                        Carbon(ThreadManager* MTHREAD_h); ///< Constructor
                       ~Carbon();


  double                getStock(const int & regId, const int & stock_type) const;                          ///< Returns the current stock of carbon [Mt CO2]
  double                getCumSavedEmissions(const int & regId, const int & em_type) const;                 ///< Returns the current cumulative saved emissions by type [Mt CO2]

  void                  registerHarvesting(const double & value, const int & regId, const string &fType);   ///< Registers the harvesting of trees increasing the value of cumEmittedForOper
  void                  registerDeathBiomass(const double &value, const int & regId, const string &fType);  ///< Registers the "death" of a given amount of biomass, storing it in the deathBiomass map
  void                  registerProducts(const double &value, const int & regId, const string &productName);///< Registers the production of a given amount of products, storing it in the products maps
  void                  registerTransports(const double &distQ, const int & regId);                         ///< Registers the quantities emitted by transport of wood FROM a given region
  void                  initialiseDeathBiomassStocks(const vector<double> & deathByFt, const int & regId);  ///< Initialise the stocks of death biomass for the avgLive_* years before the simulation starts
  void                  initialiseProductsStocks(const vector<double> & qByProduct, const int &regId);      ///< Initialise the stocks of products for the avgLive_* years before the simulation starts
  void                  initialiseEmissionCounters();                                                       ///< Initialise the emission counters to zero.

private:
  void                  addSavedEmissions(const double & value, const int & regId, const int & em_type);    ///< Increases the value to the saved emissions for a given type and region


  map<iiskey, double >              deathBiomass; ///< Map that register the death of biomass by year, l2_region and forest type [Mm^3 wood]
  map<iiskey, double >                  products; ///< Map that register the production of a given product by year, l2_region and product [Mm^3 wood]
  map<int,double>           cumSubstitutedEnergy; ///< Map that store the cumulative CO2 substituted for energy consumption, by l2_region [Mt CO2]
  map<int,double>         cumSubstitutedMaterial; ///< Map that store the cumulative CO2 substituted using less energivory material, by l2_region [Mt CO2]
  map<int,double>              cumEmittedForOper; ///< Map that store emissions for forest operations, including transport, by l2_region [Mt CO2]



};                 

#endif // CARBON_H
