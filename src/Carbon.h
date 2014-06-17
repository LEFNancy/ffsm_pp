
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

/// %Output methods

/**
Class responsable to output  the data, both as all kind of log as well as georeferenciated one.
@author Antonello Lobianco

An istance of this class is owned by each region .. or better a single global one ??
*/

class Carbon: public BaseClass{
public:
                        Carbon(ThreadManager* MTHREAD_h);///< Constructor
                       ~Carbon();


  double                getStock(const int & regId, const int & stock_type) const;
  double                getCumSavedEmissions(const int & regId, const int & em_type) const;                ///< return the current cumulative saved emissions by type

  void                  registerHarvesting(const int & regId, const string & fType, const double & value); ///< register the harvesting of trees -> cumEmittedForOper
  void                  registerDeathBiomass(const double &value, const int & regId, const string &fType);
  void                  registerProducts(const double &value, const int & regId, const string &productName);
  void                  initialiseDeathBiomassStocks(const vector<double> & deathByFt, const int & regId); ///< Initialise the stocks of death biomass for the avgLive_* years before the simulation starts
  void                  initialiseProductsStocks(const vector<double> & qByProduct, const int &regId);                       ///< Initialise the stocks of products for the avgLive_* years before the simulation starts
  void                  initialiseEmissionCounters(); ///< Initialise the emission counters to zero.
//double                       transportCostsToCo2; // coefficient that transform the transport cost per m^3 in Kg Co2 emitted
/*vector<double>                  forOperEmissions; // emissions due to forest operations, by forest type [Kg C02 / m^3 harvested]
vector<double>              co2content_inventary; // carbon content in inventariated forest wood resources, by forest type [Kg CO2 / m^3 inventariated wood]
vector<double>                  co2content_extra; // carbon content in non-inventariated forest wood resources (branches, soil..), by forest type [Kg CO2 / m^3 of inventariated wood]
vector<double>              avgLife_deathBiomass; // average life before death biomass get oxidated, by forest type [years]
vector<double>                  avgLife_products;
vector<double>                     co2sub_energy;
vector<double>                   co2sum_material;*/
private:
  void                  addSavedEmissions(const double & value, const int & regId, const int & em_type); ///< add value to the saved emissions for a given type and region

  map<iiskey, double >              deathBiomass; ///< map that register the death of biomass by year, l2_region and forest type [Mm^3 wood]
  map<iiskey, double >                  products; ///< map that register the production of a given product by year, l2_region and product [Mm^3 wood]
  map<int,double>           cumSubstitutedEnergy; ///< map that store the cumulative CO2 substituted for energy consumption [Mt CO2]
  map<int,double>         cumSubstitutedMaterial; ///< map that store the cumulative CO2 substituted using less energivory material [Mt CO2]
  map<int,double>              cumEmittedForOper; ///< map that store emissions for forest operations [Mt CO2]



};                 


//typedef map<iskey, double > DataMap;
//typedef pair<iskey, double > DataPair;


#endif // CARBON_H
