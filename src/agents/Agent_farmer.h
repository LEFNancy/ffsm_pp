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
#ifndef AGENTFARMER_H
#define AGENTFARMER_H

// standard headers
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <map>

// regmas headers...
#include "Agent_space.h"
#include "Manager_farmers.h"
#include "Opt.h"

/// Specific class to model farmer agents behaviour

/**
This is the specific derived class for farmer agents: \\
Agent_base --> Agent_space  --> agent_farmer \\
@author Antonello Lobianco
*/

class Agent_farmer: public Agent_space {

public:
	                    Agent_farmer(ThreadManager* MTHREAD_h, Manager_base* manager_h);                 ///< mould only constructor
	                    Agent_farmer(ThreadManager* MTHREAD_h, int uniqueID_h, Manager_base* manager_h); ///< real agent constructor
	                   ~Agent_farmer();

	virtual bool        filterActivity(matrixActivities* ACT);
	double              getShadowPrice(const Pixel* px_h, bool initSolve=false);
	double              offerRentalPrice(const Pixel* px_h, bool initSolve=false);
	void                update();
	void                produce(){OPT.update(); OPT.solve();}; // OPT update is needed otherwise I got slighly other results..
	virtual int         leaveActivity();
	virtual void        collectProductionEffects();
	double              applyModulation(double grossPremium);
	double              getDecoupledPayments();
	/// return UAA hectars. @param ptype: plot type: PLOTS_ALL(d), PLOTS_OWNED, PLOTS_RENTED
	double              getUAA(int ptype=PLOTS_ALL);
	double              getAgrDistCost(const Pixel* px1);
	friend void         Manager_farmers::setAgentMoulds();
	void                initMIP();
	void                test();

	string              getFadnID(){return fadnID;};
	string              getFarmType(){return farmType;};
	double              getInitialDecPayment(){return initialDecPayment;};
	int                 getAgrRegionNumber(){return agrRegionNumber;};
	/// netProductionProfit + netTotalPremium - sunkCostsPaid
	virtual double      getOverallProfit(){return netProductionProfit+netTotalPremium-sunkCostsPaid;};
	virtual double      getGrossCoupledPremiums(){return grossCoupledPremiums;};
	virtual double      getGrossDecoupledPayment(){return grossDecoupledPayment;};
	virtual double      getNetTotalPremium(){return netTotalPremium;};
	double              getDecRightsByAct(int actMouldCounter, int policyPremiumCounter){return decRights.at(actMouldCounter).at(policyPremiumCounter);};
	double              getDecHaByActMould(int actMouldCounter, int policyPremiumCounter){return decHa.at(actMouldCounter).at(policyPremiumCounter);};
	double              getCachedOffer(){return cachedOffer;};


	void                setFadnID(string fadnID_h){fadnID =fadnID_h ;};
	void                setFarmType(string farmType_h){farmType = farmType_h;};
	void                setInitialDecPayment(double initialDecPayment_h){initialDecPayment = initialDecPayment_h;};
	void                setCachedOffer(double cachedOffer_h){cachedOffer = cachedOffer_h;};
	virtual void        setGrossCoupledPremiums(double grossCoupledPremiums_h){grossCoupledPremiums = grossCoupledPremiums_h;};
	virtual void        setGrossDecoupledPayment(double grossDecoupledPayment_h){grossDecoupledPayment = grossDecoupledPayment_h;};
	virtual void        setNetTotalPremium(double netTotalPremium_h){netTotalPremium = netTotalPremium_h;};
	void                setAgrRegionNumber(int agrRegionNumber_h){agrRegionNumber=agrRegionNumber_h;};

	double                           debugLandUse; ///< debug!!
	volatile double                           cachedOffer;

private:
	string                                fadnID;
	string                              farmType;
	double                     initialDecPayment;
	int                          agrRegionNumber;

	vector< vector<double> >                     decRights; ///< This is already the average by year of the coupled premiums got during the reference period - By act mould - than premium
	vector< vector<int> >                         decYears; ///< Counter of how many years the average "decYears" has been counted for - By act mould - than premium
	vector< vector<double> >                         decHa; ///< Counter of the average hectars generating the rights (reference area) - By act mould - than premium

	/*vector<double>                     decRights; ///< This is already the average by year of the coupled premiums got during the reference period - By act mould
	vector<int>                         decYears; ///< Counter of how many years the average "decYears" has been counted for - By act mould
	vector<double>                         decHa; ///< Counter of the average hectars generating the rights (reference area) - By act mould */

	double                  grossCoupledPremiums; ///< pre-modulation coupled premiums
	double                 grossDecoupledPayment; ///< pre-modulation decoupled premium
	double                       netTotalPremium; ///< post-modulation coupled and decoupled premiums





};

#endif
