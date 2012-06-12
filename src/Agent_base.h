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
#ifndef AGENTBASE_H
#define AGENTBASE_H

// standard headers
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <map>

// Qt headers..
#include <QtCore>
#include <QThread>

// regmas headers..
#include "BaseRegmas.h"
#include "ModelObject.h"
#include "Opt.h"

class RegData;
class Gis; // forward declaration
struct AgentSurvey;
class Manager_base;

/// Base class common to all agents

/**
 This is the base class for each derived agent:

 Agent_base --> agent_nospacial          --> ...

 Agent_base --> agent_spaciallyExplicit  --> ...

 @author Antonello Lobianco
*/

class Agent_base: public BaseRegmas{

public:
	                    Agent_base (ThreadManager* MTHREAD_h, Manager_base* manager_h); ///< Constructor
	                    Agent_base (ThreadManager* MTHREAD_h, int uniqueID_h, Manager_base* manager_h); ///< Constructor with ID already on
	virtual            ~Agent_base();

	/// Associate a new object to the agent.  @param case_h stage (STAGE_INIT or STAGE_NORMAL)
	virtual void        acquireObject(ModelObject acquiredObject_h, int case_h=STAGE_NORMAL);
	/// This function will filter the activity for agent-specific parameters. E.g. subtract transport costs from the gm. A false return value means the activity has to be skipped.
	virtual bool        filterActivity(matrixActivities* ACT)=0;
	virtual void        payInitialYearCosts()=0; ///< Pay the initial costs for the year coming (suckCosts: rental costs and maintainance costs)
	virtual void        pay(double amount_h, bool updateResourceValues=true); ///< Pay a money for whatsoever reason. Subtract liquidity and update the OPT problem.
	virtual void        update()=0;
	virtual void        produce()=0;
	void                answerStats(AgentSurvey &SURVEY);
	virtual int         leaveActivity()=0;
	virtual void        collectProductionEffects()=0;
	void                withdraw(); ///< Withdraw the money needed for living from the liquidity. Made by a fixed part plus a profit-variable one.
	void                debugOptProblem(string debugMessage="", bool useCallCounter=false){OPT.debug(debugMessage,useCallCounter);};

	virtual int         getMouldLocalID(){return mouldLocalID;};
	virtual int         getMouldCoeff(){return mouldCoeff;};
	virtual int         getAgentLocalID(){return agentLocalID;};
	virtual int         getAgentUniqueID(){return agentUniqueID;};
	virtual string      getComments(){return comments;};
	string              getCategory(){return category;};
	double              getResource(string resourceName_h);
	virtual map<string,double> getInitialResources(){return initialResources;};
	virtual double      getInitialResourceByName(string resourceName_h);
	virtual double      getCalculatedResourceByName(string resourceName_h);
	virtual double      getObjectResourceByName(string resourceName_h);
	virtual double      getLiquidity(){return liquidity;};
	/// Return the capital of the agent. @param endOfYear Optional parameter to specify that the capital requested is those that the farm has at the end of the year
	virtual double      getCapital(bool endOfYear=false);
	virtual double      getLandCapital(){return 0;};
	virtual double      getLandShPrice(){return 0;};

	virtual int         getNPlots(int ptype=PLOTS_ALL, int landCode_h = 0){int getRidCompWarning=ptype; getRidCompWarning=landCode_h;return 0;};
	vector <ModelObject*> getOwnedObjects();      ///< Return the owned plots, for financial analysis
	vector <ModelObject*> getAvailableObjects();  ///< Return the available objects, for resource analysis

	virtual int         countMyObjects(){return myObjects.size();};
	double              getZ(){return OPT.getZ();}
	/// netProductionProfit - sunkCostsPaid
	virtual double      getOverallProfit(){return netProductionProfit-sunkCostsPaid;};
	virtual double      getNetProductionProfit(){return netProductionProfit;};
	virtual double      getGrossCoupledPremiums(){return 0;};
	virtual double      getGrossDecoupledPayment(){return 0;};
	virtual double      getNetTotalPremium(){return 0;};
	double              getSunkCostsPaid(){return sunkCostsPaid;};
	double              getLiquidityShPrice(){return liquidityShPrice;};
	double              getLabourShPrice(){return labourShPrice;};

	virtual void        setMouldLocalID(int mouldLocalID_h){mouldLocalID=mouldLocalID_h;};
	void                setCategory(string category_h){category = category_h;};
	virtual void        setMouldCoeff(int mouldCoeff_h){mouldCoeff=mouldCoeff_h;};
	virtual void        setAgentLocalID(int agentLocalID_h){agentLocalID=agentLocalID_h;};
	virtual void        setAgentUniqueID(int agentUniqueID_h){agentUniqueID=agentUniqueID_h;};
	virtual void        setComments(string comments_h){comments=comments_h;};
	virtual void        setInitialResources( map<string,double> initialResources_h){initialResources=initialResources_h;};
	virtual void        setLiquidity(double liquidity_h){liquidity=liquidity_h;};
	virtual void        setNetProductionProfit(double netProductionProfit_h){netProductionProfit = netProductionProfit_h;};
	virtual void        setGrossCoupledPremiums(double grossCoupledPremiums_h){double temp = grossCoupledPremiums_h; temp++;};
	virtual void        setGrossDecoupledPayment(double grossDecoupledPayment_h){double temp = grossDecoupledPayment_h; temp++;};
	virtual void        setNetTotalPremium(double netTotalPremium_h){double temp = netTotalPremium_h; temp++;};
	void                addToSunkCostsPaid(double paid){sunkCostsPaid += paid;};

protected:

	string                              comments;
	string                              category; ///< category of the agent (e.g. "farmer")
	int                             mouldLocalID; ///< only in the "factory" stage
	int                               mouldCoeff; ///< upscaling coefficient
	int                             agentLocalID; ///< only in the implementation
	int                            agentUniqueID; ///< only in the implementation
	vector <ModelObject>               myObjects; ///< objects associated with each agent. Doesn't include spacial objects !! They are stored in the plots !!!
	map<string,double>          initialResources; ///< initialResources	
	/** Available liquidity.
	Liquidity is important at it is never created from scratch.

	During the model run liquidity is increased or decreased according to the cash flow of the agent, but it is never reset. It can (and it is offen) negative in case of the presence of debs.

	A short note: capital is dynamically created with getCapital(), it is not a propriety of the agent.
	*/
	double                             liquidity;
	double                   netProductionProfit; ///< production profit without coupled premiums
	double                         sunkCostsPaid; ///< out-of-MIP paid costs (e.g. rental prices)
	double                      liquidityShPrice; ///< shadow price of own liquidity (if any)
	double                         labourShPrice; ///< shadow price of family labour
	Opt                                      OPT; ///< optimization problem
	Manager_base*                        manager; ///< pointer to the own manager
	QMutex                                 mutex; ///< mutex object to protect multithreads-unsafe calls

};

#endif
