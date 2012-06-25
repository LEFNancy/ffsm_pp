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
#include "BaseClass.h"
#include "ModelData.h"
#include "Gis.h"

class Gis;        //forward declaration

/// Pixel-level class

/**
This class manage the info at the pixel level. A vector of pixel objects is owned by the class Gis.
@author Antonello Lobianco
*/
class Pixel: public BaseClass{
	
public:
	                    Pixel(double ID_h, ThreadManager* MTHREAD_h);
	                   ~Pixel();

	/// Return the value for a specific layer
	double              getDoubleValue ( string layerName_h , bool returnZeroForNoValue = false) const;
	void                newYear();


	// space..
	double              getID() const {return ID;}  ;
	int                 getX() const {return pxX;} ;
	int                 getY() const {return pxY;};
	/// Return a vector of pixels at the specified distance (in levels, not in physical units)
	vector <Pixel *>    getPixelsAtDistLevel (int distLevel_h) const;

	string              getPxComments() const {return pxComments;};
	double              getCachedDouble() const {return cachedDouble;};	


	/// Insert a new layer and its value
	void                setValue ( string layerName_h, double value_h );
	/// Change the value of an existing layer
	void                changeValue ( string layerName_h, double value_h, bool setNoValueForZero=false );
	void                setCoordinates ( int x_h, int y_h ) {pxX=x_h; pxY=y_h;};;
	void                setPxComments ( std::string pxComments_h ) {pxComments = pxComments_h;};
	void                setCachedDouble(double cachedDouble_h){cachedDouble=cachedDouble_h;};
	void                clearCache(){cachedDouble=0;};

private:	
	map<string, double>            values;  ///< Map of values for each layer
	double                             ID;
	int                               pxX;
	int                               pxY;
	string                     pxComments;
	double                   cachedDouble;  ///< Cachable double used in some optimized algorithms

};

#endif
