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
#ifndef MANAGERFARMERS_H
#define MANAGERFARMERS_H

#include <QtCore>
//#include <QThread>
#include <QString>
#include <QRunnable>


#include <Manager_space.h>



class Agent_farmer;

struct bidsEstimatedCoefficients;


/// Derived class to explicitelly model the manager of farmer agents

/**
This is the interface agent_side and it derive from class agentManager. \\
It implement all the APIs with the specific target of the farmer agents. \\
@author Antonello Lobianco
*/
class Manager_farmers : public Manager_space {

public:
	                    Manager_farmers(ThreadManager* MTHREAD_h, string name_h);
	                   ~Manager_farmers();
	virtual void        setAgentMoulds(); ///< "prepare" the farmer moulds
	virtual void        riseMyAgents();   ///< get the real agents out of the mould
	virtual void        locateMyAgents(); ///< locate farmers in the space and allocate them plots.
	virtual void        createBehaviours(); ///< create the decision matrix on each agent
	virtual void        prepare(); ///< update liquidity, renting and investment decisions
	virtual void        landAllocation(); ///< renting decisions
	void                calibrateLandValues(); ///< calculate the renting sh price of owned plots
	double              quotePlot(Pixel* plot); ///< return the renting value of an individual plot. This Function should be move to a more agent-type neutral class, allowing competition from different agent types. 
	virtual void        act(); ///< production
	virtual void        update(); ///< update agents' status variables
	virtual void        planNext(); ///< decision if continue or leave
	virtual void        removeFromModel(Agent_farmer* agent_h); ///< remove agent from list and do some clean-up
	virtual void        endInit(); ///< here we do some nice stuff before the simulation finally start...

	/// Assign a plot to a specific agent. @param assignmentType: plot type: PLOTS_OWNED, PLOTS_RENTED(d)
	virtual void        assignPlot(Pixel* plot_h, Agent_space* agent_h, double rentalCost=0, int assignmentType=PLOTS_RENTED);
	double              getAverageRightsByAct(int mouldCounter=-1);

protected:
	vector < vector<double> >    sumOfRightsOfDeadAgents; // keep record of rights of dead agents, by each mould
	vector < vector<double> >    sumOfHaOfDeadAgents;     // keep record of surface giving rights of dead agents, by each mould


};
/*
class Manager_farmers_threads : public QThread {
	Q_OBJECT

public:
	Manager_farmers_threads();
	void                assignJob(Agent_space* agent_h, const Pixel*plot_h, QMutex* mutex_h);

protected:
	void run();

private:
	volatile Agent_space* agent;
	const Pixel* plot;
	QMutex* mutex;

};
*/
class Manager_farmers_threads : public QRunnable {

public:
	Manager_farmers_threads();
	void                assignJob(Agent_space* agent_h, const Pixel*plot_h);

protected:
	void run();

private:
	volatile Agent_space* agent;
	const Pixel* plot;
	//QMutex* mutex;

};

#endif
