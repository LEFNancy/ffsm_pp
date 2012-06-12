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
#ifndef PIXELS_H
#define PIXELS_H

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>

// regmas headers...
#include "BaseRegmas.h"
#include "RegData.h"
#include "Gis.h"
#include "ModelObject.h"

class Agent_space; //forward declaration
class Gis;        //forward declaration

/// Pixel-level class

/**
This class manage the info at the pixel level. A vector of pixel objects is owned by the class Gis.
@author Antonello Lobianco
*/
class Pixel: public BaseRegmas{
	
public:
	                    Pixel(double ID_h, ThreadManager* MTHREAD_h);
	                   ~Pixel();

	/// Return the value for a specific layer
	double              getDoubleValue ( string layerName_h ) const;
	void                initObjects(); ///< Initialize the pixel with a suitable object
	/// Associate a new object to the agent. @param case_h stage (STAGE_INIT or STAGE_NORMAL)
	void                acquireObject(ModelObject acquiredObject_h, int case_h=STAGE_NORMAL);
	void                newYear();
	/// Set a new, random-duration, rental contract. For now only "agrContract" is implemented. Take account of objects included in the plots.
	void                setRandomRenting(string contractType);
	bool                isAgricultural() const;
	/// Return the initial renting costs for this plot type (exogenous)
	double              getStartingRentalCosts() const;
	/// Destroy the rental contract when it goes out-of-scope or when the farm leave freeing the land
	void                destroyRentalContract();

	/// last year before the rental contract will be ended
	int                 getLastRentingYear() const {return startRentingYear+rentingTerm-1;};
	int                 getStartRentingYear() const {return startRentingYear;};
	int                 getRentingTerm() const {return rentingTerm;};
	int                 getRentingAge() const;
	int                 getRemainingRentingAge() const {return getRentingTerm()-getRentingAge();};


	// space..
	double              getID() const {return ID;}  ;
	int                 getX() const {return pxX;} ;
	int                 getY() const {return pxY;};
	/// Return a vector of pixels at the specified distance (in levels, not in physical units)
	vector <Pixel *>    getPixelsAtDistLevel (int distLevel_h) const;
	Agent_space*        getOwner() {return pxOwner;};
	Agent_space*        getTenant() {return pxTenant;};
	string              getPxComments() const {return pxComments;};
	Agent_space *       getCachedAgent(){return cachedAgent;};	
	double              getCachedDouble() const {return cachedDouble;};	
	/// Return pointers to objects located on this pixel
	vector <ModelObject*> getIncludedObjects();
	vector <string>     getIncludedObjectNames() const;
	int                 countMyObjects() const {return myObjects.size();};
	double              getRentalCosts() const {return rentalCosts;};
	double              getShRentalPriceCoeff() const {return shRentalPriceCoeff;};


	/// Insert a new layer and its value
	void                setValue ( string layerName_h, double value_h );
	/// Change the value of an existing layer
	void                changeValue ( string layerName_h, double value_h );
	void                setCoordinates ( int x_h, int y_h ) {pxX=x_h; pxY=y_h;};
	void                setOwner ( Agent_space* owner_h ) {pxOwner=owner_h;};
	void                setTenant ( Agent_space* tenant_h ) {pxTenant=tenant_h;};
	void                setPxComments ( std::string pxComments_h ) {pxComments = pxComments_h;};
	void                setCachedAgent(Agent_space * cachedAgent_h){cachedAgent=cachedAgent_h;};
	void                setCachedDouble(double cachedDouble_h){cachedDouble=cachedDouble_h;};
	void                clearCache(){cachedAgent=0;cachedDouble=0;};
	void                setRemainingRentingTerm (int years_h);
	void                setRentingTerm(int years){rentingTerm=years;};
	void                setRentalCosts(double rentalCosts_h){rentalCosts=rentalCosts_h;};
	void                setShRentalPriceCoeff(double shRentalPriceCoeff_h){shRentalPriceCoeff = shRentalPriceCoeff_h;};



private:	
	map<string, double>            values;  ///< Map of values for each layer
	double                             ID;
	int                               pxX;
	int                               pxY;
	string                     pxComments;
	Agent_space*                  pxOwner;
	Agent_space*                 pxTenant;
	Agent_space*              cachedAgent;  ///< Cachable pointer to Agent_space used in some optimized algorithms
	double                   cachedDouble;  ///< Cachable double used in some optimized algorithms
	vector <ModelObject>        myObjects;  ///< Vector of spacial objects guested on this plot 

	/// how many years the object can live. rentingTerm= lastYear-startYear  The object start with age=0 and is destroied at the end of year when it reach lifeTerm-1.. currentAge never go to lifeTerm
	int                       rentingTerm;
	/// year when the object was created
	int                  startRentingYear;
	double                    rentalCosts;
	double             shRentalPriceCoeff; ///< Ratio between the shadow price of this plot in comparition with the avg rental price of its type (calculted on the first year)

};

#endif
