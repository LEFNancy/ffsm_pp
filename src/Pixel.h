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
#ifndef PIXELS_H
#define PIXELS_H

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>

#include <Eigen/Dense>

// regmas headers...
#include "BaseClass.h"
#include "ModelData.h"

class Gis;        //forward declaration

using namespace Eigen;

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
	double              getDoubleValue (const string& layerName_h, const  bool& returnZeroForNoValue = false) const;
	double              getDoubleValue (const string& parName, const string& forName, const string& dClass, const int& year, const  bool& returnZeroForNoValue = false);
	double              getMultiplier (const string& multiplierName, const string& forName, int year=DATA_NOW);
	void                correctInputMultiplier (const string& multiplierName, const string& forName, double coefficient=1); //< It apply a given coefficient to all the multipliers layers of a given ft
	void                newYear();
	double              getPastRegArea(const int& ft_idx, const int& year);
	void                setPastRegArea(const double& value, const int& ft_idx, const int& year);


	// space..
	double              getID() const {return ID;}  ;
	int                 getX() const {return pxX;} ;
	int                 getY() const {return pxY;};
	/// Return a vector of pixels at the specified distance (in levels, not in physical units)
	vector <Pixel *>    getPixelsAtDistLevel (int distLevel_h) const;

	string              getPxComments() const {return pxComments;};
	double              getCachedDouble() const {return cachedDouble;};


	/// Insert a new layer and its value
	void                setValue ( const string& layerName_h, const double& value_h ){values.insert(pair<string, double>(layerName_h, value_h));}
	//inline void         setValue ( const string& parName, const string& forName, const string& dClass, const int& year, const double& value_h); // never used ???
	/// Change the value of an existing layerMTHREAD->GIS->pack(parName, forName, dClass, year), value_h,
	void                changeValue (const string& layerName_h, const double& value_h, const bool& setNoValueForZero=false );
	//void                changeValue (const double& value_h, const string& parName, const string& forName, const string& dClass, const int& year, const bool& setNoValueForZero=false);
	void                setCoordinates ( int x_h, int y_h ) {pxX=x_h; pxY=y_h;};
	void                setPxComments ( std::string pxComments_h ) {pxComments = pxComments_h;};
	void                setCachedDouble(double cachedDouble_h){cachedDouble=cachedDouble_h;};
	void                clearCache(){cachedDouble=0;};
    void                setSpModifier(const double& value, const int& ftindex){spMods.at(ftindex)=value;};
    double              getSpModifier(const string& ft);
	void                swap(const int &swap_what);

	// matrices of (ft,dc)
	/*MatrixXd                                   vol;
	MatrixXd                                  area;
	MatrixXd                               regArea;
	MatrixXd                                  hVol;
	MatrixXd                                 vol_l;
	MatrixXd                                area_l;
	MatrixXd                             regArea_l;
	MatrixXd                                hVol_l;
	MatrixXd                                  beta;
	MatrixXd                                  mort;
	MatrixXd                                    tp;
	MatrixXd                                 cumTp;*/

	vector <vector <double> >                  vol; // by ft,dc
    //vector <vector <double> >                 area; // by ft,dc
    vector <vector <double> >                hArea; // by ft, dc  // possibly in ha, but to be check for 100% sure
	vector <vector <double> >                 hVol; // by ft.dc
	vector < vector <vector <double> > >hVol_byPrd; // by ft, dc, pp
	map <int, vector <double> >            regArea; // by year, ft
    //vector <double>                             in; // by pp
    //vector <double>                             hr; // by pp
	vector <double >                          vReg; // by ft
	vector <double>                expectedReturns; // by ft

	vector <vector <double> >                vol_l;

	vector <vector <double> >                 beta;
	vector <vector <double> >                 mort;
	vector <vector <double> >                   tp;
	vector <vector <double> >                cumTp; ///< This is time of passage to REACH a diameter class (while the exogenous tp by diameter class is the time of passage to LEAVE to the next d class)
	vector <vector <double> >                  vHa; ///< Volume at hectar by each diameter class [m^3/ha]
    vector <vector <double> >             cumAlive; ///< Cumulative prob of remaining alive at beginnin of a given diam class
	vector <vector <double> >            cumTp_exp; ///< This is the **expected** version of cumTp, used for calculating profits
	vector <vector <double> >              vHa_exp; ///< This is the **expected** version of vHa, used for calculating profits
    vector <vector <double> >         cumAlive_exp; ///< This is the **expected** version of cumAlive, used for calculating profits

private:	
	map<string, double>            values;  ///< Map of values for each layer
	mutable map<string, double>::const_iterator vIter; //< Iterator for the map of values
	double                             ID;
	int                               pxX;
	int                               pxY;
	string                     pxComments;
	double                   cachedDouble;  ///< Cachable double used in some optimized algorithms
    vector<double>                 spMods;  ///< The sampled spatial modifiers (by forest type)

};

#endif
