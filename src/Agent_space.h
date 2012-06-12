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
#ifndef AGENTSPACE_H
#define AGENTSPACE_H

// standard headers
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <algorithm>

// regmas headers
#include "Agent_base.h"

/// Base class for spaciallly-located agents

/**
  This is the base class for each derived agent WITH spaccially explicit carachteristics:
  <br>Agent_base --> Agent_nospace  --> ...
  <br>Agent_base --> Agent_space    --> ...
  @author Antonello Lobianco
*/

class Agent_space : public Agent_base{

public:
	                            Agent_space(ThreadManager* MTHREAD_h, Manager_base* manager_h); ///< Constructor
	                            Agent_space(ThreadManager* MTHREAD_h, int uniqueID_h, Manager_base* manager_h);
	                           ~Agent_space();

	/// Return the number of plots to complete the initial dotation. @param ptype type: PLOTS_ALL(d), PLOTS_OWNED, PLOTS_RENTED
	int                         uncompletePlotDotation (int landCode_h, int ptype=PLOTS_ALL);
	/// Return pointer to the plots owned or rented by the agent (eventually by type).
	vector <Pixel *>            getPlots(int ptype=PLOTS_ALL, int landCode_h = 0);
	/// Return number of owned/rented plots by type.
	int                         getNPlots(int ptype=PLOTS_ALL, int landCode_h = 0);
	virtual int                 countMyObjects();
	double                      getDistance(const Pixel* px1); ///< distance in meters from the homePlot
	double                      getLandCapital(){return landCapital;};
	virtual void                update()=0;
	virtual void                payInitialYearCosts();
	void                        freeLand();


	// member data interface...
	virtual map<int,double>     getInitialOwnedPlots(){return initialOwnedPlots;};
	virtual map<int,double>     getInitialRentedPlots(){return initialRentedPlots;};
	int                         getHomeLandUseCode(){return homeLandUseCode;};
	Pixel*                      getHomePlot(){return homePlot;};
	virtual double              getLandShPrice(){return landShPrice;};

	void                        setHomeLandUseCode(int homeLandUseCode_h){homeLandUseCode = homeLandUseCode_h;};
	virtual void                setInitialOwnedPlots(
	                                map<int,double> initialOwnedPlots_h){initialOwnedPlots=initialOwnedPlots_h;};
	virtual void                setInitialRentedPlots(
                                    map<int,double> initialRentedPlots_h){initialRentedPlots=initialRentedPlots_h;};
	virtual void                setHomePlot(Pixel* plot_h){homePlot = plot_h;};
	void                        addPlotToOwnedList(Pixel* px_h){ownedPlots.push_back(px_h);};
	void                        addPlotToRentedList(Pixel* px_h){rentedPlots.push_back(px_h);};
	void                        addPlotToMIP(Pixel* px_h){OPT.addPixel(px_h);};
	void                        setLandCapital(double landCapital_h){landCapital = landCapital_h;};

protected:
	Pixel *                             homePlot;
	map<int,double>            initialOwnedPlots;
	map<int,double>           initialRentedPlots;
	vector <Pixel *>                  ownedPlots;
	vector <Pixel *>                 rentedPlots;
	/// Land use code where the "home" has to be place (urban or a specific agricultural land type)
	int                          homeLandUseCode;
	double                           landCapital; ///< Value of owned land
	double                           landShPrice; ///< Shadow price of the owned land (sum of rental prices)

};

#endif
