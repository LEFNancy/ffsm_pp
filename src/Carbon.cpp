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

#include "Carbon.h"
#include "ThreadManager.h"
#include "ModelData.h"
#include "Scheduler.h"



Carbon::Carbon(ThreadManager* MTHREAD_h){
  MTHREAD=MTHREAD_h;
}

Carbon::~Carbon(){
}


// ################# GET FUNCTIONS ################
/**
 * @param reg
 * @param stock_type
 * @return the Carbon stocked in a given sink
 *
 * For product sink:
 * - for primary products it includes the primary products exported out of the country, but not those exported to other regions or used in the region as
 *   these are assumed to be totally transformed to secondary products;
 * - for secondary products it includes those produced in the region from locally or regionally imported primary product plus those secondary products
 *   imported from other regions, less those exported to other regions. It doesn't include the secondary products imported from abroad the country.
 */
double
Carbon::getStock(const int & regId, const int & stock_type) const{
  double toReturn = 0.0;
  int currentYear = MTHREAD->SCD->getYear();
  switch (stock_type){
    case STOCK_PRODUCTS: {
      vector <string> priProducts = MTHREAD->MD->getStringVectorSetting("priProducts");
      vector <string> secProducts = MTHREAD->MD->getStringVectorSetting("secProducts");
      vector <string> allProducts = priProducts;
      allProducts.insert( allProducts.end(), secProducts.begin(), secProducts.end() );
      for(uint i=0;i<allProducts.size();i++){
        double coeff = MTHREAD->MD->getProdData("co2content_products",regId,allProducts[i],DATA_NOW,""); //  [kg CO2/m^3 wood]
        double life  = MTHREAD->MD->getProdData("avgLife_products",regId,allProducts[i],DATA_NOW,"");
        for(int y=currentYear;y>currentYear-life;y--){ // ok
          iiskey key(y,regId,allProducts[i]);
          toReturn += findMap(products,key,MSG_NO_MSG)*coeff/1000;
        }
      }
      break;
    }
    case STOCK_INV:{
    // TO.DO !!! No, It is already done (hint: missing the break!)

    }
    case STOCK_EXTRA:{
      vector <string> fTypes = MTHREAD->MD->getForTypeIds();
      for(uint i=0;i<fTypes.size();i++){
        // units:
        // co2content_inventary: [Kg CO2 / m^3 wood]
        // co2content_extra:     [Kg CO2 / m^3 inventaried wood]
        double coeff = (stock_type==STOCK_INV)? MTHREAD->MD->getForData("co2content_inventary",regId,fTypes[i],"",DATA_NOW):MTHREAD->MD->getForData("co2content_extra",regId,fTypes[i],"",DATA_NOW); //  [kg CO2/m^3 wood]
        double life  = MTHREAD->MD->getForData("avgLive_deathBiomass",regId,fTypes[i],"",DATA_NOW);
        // PART A: from death biomass..
        for(int y=currentYear;y>currentYear-life;y--){ // ok
          iiskey key(y,regId,fTypes[i]);
          toReturn += findMap(deathBiomass,key,MSG_NO_MSG)*coeff/1000;
        }
        // PART B: from inventary volumes
        toReturn += MTHREAD->MD->getForData("vol",regId,fTypes[i],DIAM_ALL,DATA_NOW)*coeff/1000;
      }
      break;
    }
    default:
      msgOut(MSG_CRITICAL_ERROR,"Unexpected stock_type in function getStock");
  }
  return toReturn;
}


double
Carbon::getCumSavedEmissions(const int & regId, const int & em_type) const{
  switch (em_type){
    case EM_ENSUB:
      return findMap(cumSubstitutedEnergy, regId);
      break;
    case EM_MATSUB:
      return findMap(cumSubstitutedMaterial, regId);
      break;
    case EM_FOROP:
      return -findMap(cumEmittedForOper, regId);
      break;
    default:
      msgOut(MSG_CRITICAL_ERROR,"Unexpected em_type in function getCumSavedEmissions");
  }
  return 0.0;
}

// ################# INITIALISE FUNCTIONS ################

void
Carbon::initialiseEmissionCounters(){
  vector<int> regIds = MTHREAD->MD->getRegionIds(2);
  for (uint i=0;i<regIds.size();i++){
    pair<int,double> mypair(regIds[i],0.0);
    cumSubstitutedEnergy.insert(mypair);
    cumSubstitutedMaterial.insert(mypair);
    cumEmittedForOper.insert(mypair);
  }
}

void
Carbon::initialiseDeathBiomassStocks(const vector<double> & deathByFt, const int & regId){
  // it must initialize in the past the death biomass taking the value of the first year
  vector <string> fTypes = MTHREAD->MD->getForTypeIds();
  if(fTypes.size() != deathByFt.size()) {msgOut(MSG_CRITICAL_ERROR,"deathByFt and fTypes have different lenght!");}
  int currentYear = MTHREAD->SCD->getYear();
  for(uint i=0;i<fTypes.size();i++){
    double life  = MTHREAD->MD->getForData("avgLive_deathBiomass",regId,fTypes[i],"",DATA_NOW);
    for(int y=currentYear;y>currentYear-life;y--){
      iiskey key(y,regId,fTypes[i]);
      pair<iiskey,double> mypair(key,deathByFt.at(i));
      deathBiomass.insert(mypair);
    }
  }
}

void
Carbon::initialiseProductsStocks(const vector<double> & qByProduct, const int & regId){
  // it must initialize in the past the products taking the value of the first year
  vector <string> priProducts = MTHREAD->MD->getStringVectorSetting("priProducts");
  vector <string> secProducts = MTHREAD->MD->getStringVectorSetting("secProducts");
  vector <string> allProducts = priProducts;
  allProducts.insert( allProducts.end(), secProducts.begin(), secProducts.end() );
  if(allProducts.size() != qByProduct.size()) {msgOut(MSG_CRITICAL_ERROR,"allProducts and qByProduct have different lenght!");}
  int currentYear = MTHREAD->SCD->getYear();
  for(uint i=0;i<allProducts.size();i++){
    double life  = MTHREAD->MD->getProdData("avgLife_products",regId,allProducts[i],DATA_NOW);
    for(int y=currentYear;y>currentYear-life;y--){
      iiskey key(y,regId,allProducts[i]);
      pair<iiskey,double> mypair(key,qByProduct.at(i));
      products.insert(mypair);
     }
  }
  cout << "" << endl;
}

// ################# REGISTER FUNCTIONS ################
void
Carbon::registerHarvesting(const double & value, const int & regId, const string & fType){
  double convCoeff = MTHREAD->MD->getForData("forOperEmissions",regId,fType,""); //  Kg of CO2 emitted per cubic meter of forest operations
  // units:
  // value: Mm^3
  // convCoeff: Kg CO2/m^3 wood
  // desidered output: Mt CO2
  // ==> I must divide by 1000
  addSavedEmissions(-convCoeff*value/1000,regId,EM_FOROP);
}


void
Carbon::registerDeathBiomass(const double &value, const int & regId, const string & fType){
  int year = MTHREAD->SCD->getYear();
  iiskey key(year,regId,fType);
  pair<iiskey,double> mypair(key,value);
  deathBiomass.insert(mypair);
}

void
Carbon::registerProducts(const double &value, const int & regId, const string & productName){
  // Registering the CO2 stock embedded in the product...
  int year = MTHREAD->SCD->getYear();
  iiskey key(year,regId,productName);
  pair<iiskey,double> mypair(key,value);
  products.insert(mypair);
  // registering the substituted CO2 for energy and material..
  double subEnergyCoeff   = MTHREAD->MD->getProdData("co2sub_energy",regId,productName,DATA_NOW,"");
  double subMaterialCoeff = MTHREAD->MD->getProdData("co2sub_material",regId,productName,DATA_NOW,"");
  // units:
  // value: Mm^3
  // subEnergyCoeff and subMaterialCoeff:  [kgCO2/m^3 wood]
  // desidered output: Mt CO2
  // ==> I must divide by 1000
  addSavedEmissions(subEnergyCoeff*value/1000,regId,EM_ENSUB);
  addSavedEmissions(subMaterialCoeff*value/1000,regId,EM_MATSUB);
}



void
Carbon::registerTransports(const double &distQ, const int & regId){
  // units:
  // distQ: km*Mm^3
  // transportEmissionsCoeff:  [Kg CO2 / (km*m^3) ]
  // desidered output: Mt CO2
  // ==> I must divide by 1000
  double transportEmissionsCoeff = MTHREAD->MD->getDoubleSetting("transportEmissionsCoeff");
  addSavedEmissions(-transportEmissionsCoeff*distQ/1000,regId,EM_FOROP);
}


// ################# UTILITY (PRIVATE) FUNCTIONS ################

void
Carbon::addSavedEmissions(const double & value, const int & regId, const int & em_type){
  switch (em_type){
    case EM_ENSUB:
      incrMapValue(cumSubstitutedEnergy, regId, value);
      break;
    case EM_MATSUB:
      incrMapValue(cumSubstitutedMaterial, regId, value);
      break;
    case EM_FOROP:
      incrMapValue(cumEmittedForOper, regId, -value);
      break;
    default:
      msgOut(MSG_CRITICAL_ERROR,"Unexpected em_type in function getCumSavedEmissions");
  }
}

