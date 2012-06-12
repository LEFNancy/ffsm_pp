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
#ifndef STDOPT_H
#define STDOPT_H



//regmas headers
#include "BaseRegmas.h"

struct matrixActivities;
class Agent_base;
class Pixel;
class RegActivities;
class ModelObject;

/// Class containing the optimization problem (the matrix and its methods) for each agent

/**
Agents can decide trought comportamental functions or trought a optimization routine. In the latter case, this is their personal matrix..

<p>The matrix is built first setting the resources and then the activities.

<br>RESOURCES: First the input fixed resources are added. Then each plot owned by the farmer is appended in the resources list (rows).
<br>ACTIVITIES: First are added the non-spacial explicit activities. Then for each spacially-explicit activity it is looped over all the available plots to search for those: a)allowing such activity; b)that have the required object (if needed);
<br>Following activities, for each non-spacial investment a "buy" activity for each of then is appended. Finally, similar with spacial activities, for each spatial investment it is looped over each plot to search plots available for that specific investment (plots: a) with compatible soil type; b) without already such investment object)

@author Antonello Lobianco
*/
class Opt: public BaseRegmas{
public:
	                    Opt(ThreadManager* MTHREAD_h, Agent_base* AGENT_h); ///< Constructor
	                   ~Opt();
	void                test();  ///< standard test from glpk manual. Used to see if everything is installed correctly.
	void                update(); ///< this is a "full" update, the matrix is completelly re-wroted
	void                updateResourceValues();  ///< update the resource values (availabilities) according to individual agent assets
	void                debug(string debugMessage="", bool useCallCounter=false);  ///< print for debugging reasons the current version of the matrix
	void                solve();  ///< call glpk to solve the matrix and store the result whitin the OPT object
	void                saveToCache();  ///< save the current level of z and optimal quantities to a temp cache
	void                restoreFromCache(); ///< restore from the temp cache z and the optimal quantities
	void                addPixel(const Pixel* px_h); ///< add a pixel. The OPT matrix is updated without the need of a full rewriting
	void                removeLastAddedPixel(); ///< remove the latest added pixel. The OPT matrix doesn't require a complete recalculation
    vector <matrixActivities*>  getActs();
	double              getZ(){return z;};
	double              getStat(){return stat;}; ///< return the output or error code from the latest solve() call
	double              getActivityObjValue(string name_h); ///< Return the MIP obj value. @param name_h *FULL* name of the activity (e.g. dWheat_on_plot_1234)

private:
	void                updateActivities();      ///< update the activities according to individual agent assets
	void                updateResourceNames();   ///< update the resource names according to individual agent assets
	void                solveGlpk(); ///< solve the problem using glpk library


	vector <matrixActivities>               ACTS; ///< vector of matrixActivities
	Agent_base*                            AGENT; ///< pointer to the owner of the object istances
	vector <string>                resourceNames; ///< name of the resources
	vector <double>               resourceValues; ///< value of the resources (availabilities)
	int                                     stat; ///< last output/error value
	int                               cachedStat;
	double                                     z; ///< last value reached by the target function
	double                               cachedZ; ///< cashed valued of last value reached by the target function
	int                     actsAddedOnLastPixel; ///< number of activities added when the last pixel was added



};

/// Struct containing MIP-level data about the activities. It is initialized from the activity moulds, but it is thinked to works whitin the MIP matrix.
/**
matrixActivities are initialized from the activity moulds, but are then each one "filtered" by each agents thatcan change them according to some personal data. E.g. for spatial activities, the distance costs from the hosting plot and the farmstead are subtracted from the gross margin.

@author Antonello Lobianco
*/
struct matrixActivities{
	bool                isSpatial(){if(plotHost){return true;} else{return false;}}; ///< false->normal activity, true->spatial activity
	string                                  name; ///< name of the mould activity or object
	string                              fullName; ///< name of the mould activity or object plus the plot ID

	bool                                isInvest; ///< 0->continuous/activity, 1->integer/investment
	double                              objValue; ///< objective value
	vector <double>                       values; ///< matrix values
	double                                     q; ///< MIP result quantities
	double                               cachedQ; ///< cached valued of output quantities

	const Pixel*                        plotHost; ///< plot where the activity is hosted (if it is a spatial activity)
	RegActivities*                      actMould; ///< pointer to the original activity mould
	ModelObject*                        objMould; ///< pointer to the original object mould
};



#endif
