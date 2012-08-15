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

#include <stdlib.h>

#include <cassert>

#include <map>

#include "Opt.h"

#include "ModelData.h"
#include "Pixel.h"
#include "ThreadManager.h"
#include "Scheduler.h"
#include "ModelRegion.h"
#include "Opt.h"

#define CONSTRAIN_START_LOOP(pVector,cn) \
	for (uint r1=0;r1<l2r.size();r1++){ \
		for (uint r2=0;r2<l2r[r1].size();r2++){ \
			for (uint p=0;p<(pVector).size();p++){ \
				int psec = p+nPriPr; \
				cix = gix((cn), r1, r2, p);
#define CONSTRAIN_END_LOOP \
	}}}


using namespace Ipopt;

typedef std::map<std::string, int >  DomainMap;
typedef std::pair<std::string, int > DomainPair;

// ****** MODEL IMPLEMENTATION START HERE ********************************

void
Opt::declareVariables(){
		// filling the list of variables and their domain
		// if you add variables in the model that enter optimisation you'll have to add them here
		// the map goes automatically in alphabetical order
		// original order: pc,pl,dc,dl,da,sc,sl,sa,exp
		vars.insert(DomainPair("da", DOM_SEC_PR));
		vars.insert(DomainPair("dc", DOM_SEC_PR));
		vars.insert(DomainPair("dl", DOM_ALL_PR));
		vars.insert(DomainPair("pc", DOM_ALL_PR));
		vars.insert(DomainPair("pl", DOM_ALL_PR));
		vars.insert(DomainPair("rt", DOM_R2_ALL_PR)); // regional trade, it was exp in gams
		vars.insert(DomainPair("sa", DOM_PRI_PR));
		vars.insert(DomainPair("sc", DOM_PRI_PR));
		vars.insert(DomainPair("sl", DOM_ALL_PR));
}
/**
Declare the constrains and their properties. For the domain type @see BaseClass
*/
void
Opt::declareConstrains (){
	// domain of constrains variables
	// for domain
	constrain mkeq2;
	mkeq2.name="mkeq2";
	mkeq2.comment="[h1] Conservation of matters of transformed products";
	mkeq2.domain=DOM_SEC_PR;
	mkeq2.direction = CONSTR_EQ;
	mkeq2.cvars.insert(DomainPair("dl",CDOM_SCALAR));
	mkeq2.cvars.insert(DomainPair("sl",CDOM_SCALAR));
	mkeq2.cvars.insert(DomainPair("rt",CDOM_R2FROMTO));
	//mkeq2.evaluate = Opt::mkteq2f;

	constrain mkeq3;
	mkeq3.name="mkeq3";
	mkeq3.comment="[h2] Conservation of matters of raw products";
	mkeq3.domain=DOM_PRI_PR;
	mkeq3.direction = CONSTR_EQ;
	mkeq3.cvars.insert(DomainPair("dl",CDOM_SCALAR));
	mkeq3.cvars.insert(DomainPair("sl",CDOM_PRI_PR));
	mkeq3.cvars.insert(DomainPair("rt",CDOM_R2FROMTO));
	//mkeq3.evaluate = Opt::mkteq3f;

	constrain mkeq4;
	mkeq4.name="mkeq4";
	mkeq4.comment="[eq 13] Leontief transformation function";
	mkeq4.domain=DOM_PRI_PR;
	mkeq4.direction = CONSTR_EQ;
	mkeq4.cvars.insert(DomainPair("dl",CDOM_SCALAR));
	mkeq4.cvars.insert(DomainPair("sl",CDOM_SEC_PR));

	constrain mkeq5;
	mkeq5.name="mkeq5";
	mkeq5.comment="[eq 21] Raw product supply function";
	mkeq5.domain=DOM_PRI_PR;
	mkeq5.direction = CONSTR_EQ;
	mkeq5.cvars.insert(DomainPair("sc",CDOM_SCALAR));
	mkeq5.cvars.insert(DomainPair("pc",CDOM_SCALAR));

	constrain mkeq6;
	mkeq6.name="mkeq6";
	mkeq6.comment="[eq 20] Trasformed products demand function";
	mkeq6.domain=DOM_SEC_PR;
	mkeq6.direction = CONSTR_EQ;
	mkeq6.cvars.insert(DomainPair("dc",CDOM_SCALAR));
	mkeq6.cvars.insert(DomainPair("pc",CDOM_SCALAR));

	constrain mkeq7;
	mkeq7.name="mkeq7";
	mkeq7.comment="[h7 and h3] Transformed products import function";
	mkeq7.domain=DOM_SEC_PR;
	mkeq7.direction = CONSTR_EQ;
	mkeq7.cvars.insert(DomainPair("da",CDOM_SCALAR));
	mkeq7.cvars.insert(DomainPair("dl",CDOM_SCALAR));
	mkeq7.cvars.insert(DomainPair("pl",CDOM_SCALAR));

	constrain mkeq8;
	mkeq8.name="mkeq8";
	mkeq8.comment="[h8 and h4] Raw products export function";
	mkeq8.domain=DOM_PRI_PR;
	mkeq8.direction = CONSTR_EQ;
	mkeq8.cvars.insert(DomainPair("sa",CDOM_SCALAR));
	mkeq8.cvars.insert(DomainPair("sl",CDOM_SCALAR));
	mkeq8.cvars.insert(DomainPair("pl",CDOM_SCALAR));

	constrain mkeq13;
	mkeq13.name="mkeq13";
	mkeq13.comment="[h9] Calculation of the composite price of transformed products (PPC_Dp)";
	mkeq13.domain=DOM_SEC_PR;
	mkeq13.direction = CONSTR_EQ;
	mkeq13.cvars.insert(DomainPair("pc",CDOM_SCALAR));
	mkeq13.cvars.insert(DomainPair("dc",CDOM_SCALAR));
	mkeq13.cvars.insert(DomainPair("dl",CDOM_SCALAR));
	mkeq13.cvars.insert(DomainPair("pl",CDOM_SCALAR));
	mkeq13.cvars.insert(DomainPair("da",CDOM_SCALAR));

	constrain mkeq14;
	mkeq14.name="mkeq14";
	mkeq14.comment="[h10] Calculation of the composite price of raw products (PPC_Sw)";
	mkeq14.domain=DOM_PRI_PR;
	mkeq14.direction = CONSTR_EQ;
	mkeq14.cvars.insert(DomainPair("pc",CDOM_SCALAR));
	mkeq14.cvars.insert(DomainPair("sc",CDOM_SCALAR));
	mkeq14.cvars.insert(DomainPair("sl",CDOM_SCALAR));
	mkeq14.cvars.insert(DomainPair("pl",CDOM_SCALAR));
	mkeq14.cvars.insert(DomainPair("sa",CDOM_SCALAR));

	constrain mkeq17;
	mkeq17.name="mkeq17";
	mkeq17.comment="[h16] Constrain of the transformaton supply (lower than the regional maximal production capacity)";
	mkeq17.domain=DOM_SEC_PR;
	mkeq17.direction = CONSTR_LE0;
	mkeq17.cvars.insert(DomainPair("sl",CDOM_SCALAR));


	constrain mkeq18;
	mkeq18.name="mkeq18";
	mkeq18.comment="Constrain on raw material supply (lower than gross regional biological availability)";
	mkeq18.domain=DOM_PRI_PR;
	mkeq18.direction = CONSTR_LE0;
	mkeq18.cvars.insert(DomainPair("sa",CDOM_SCALAR));
	mkeq18.cvars.insert(DomainPair("sl",CDOM_SCALAR));

	constrain mkeq23;
	mkeq23.name="mkeq23";
	mkeq23.comment="[h3] Composit demand eq. (Dp)";
	mkeq23.domain=DOM_SEC_PR;
	mkeq23.direction = CONSTR_EQ;
	mkeq23.cvars.insert(DomainPair("dc",CDOM_SCALAR));
	mkeq23.cvars.insert(DomainPair("da",CDOM_SCALAR));
	mkeq23.cvars.insert(DomainPair("dl",CDOM_SCALAR));

	constrain mkeq24;
	mkeq24.name="mkeq24";
	mkeq24.comment="[h4] Composite supply eq. (Sw)";
	mkeq24.domain=DOM_PRI_PR;
	mkeq24.direction = CONSTR_EQ;
	mkeq24.cvars.insert(DomainPair("sc",CDOM_SCALAR));
	mkeq24.cvars.insert(DomainPair("sa",CDOM_SCALAR));
	mkeq24.cvars.insert(DomainPair("sl",CDOM_SCALAR));

	constrain mkeq25;
	mkeq25.name="mkeq25";
	mkeq25.comment="Verification of the null trasformers supply (price of raw product + trasf product > trasf product)";
	mkeq25.domain=DOM_SEC_PR;
	mkeq25.direction = CONSTR_GE0;
	mkeq25.cvars.insert(DomainPair("pl",CDOM_PRI_PR_AND_SCALAR));

	constrain mkeq26;
	mkeq26.name="mkeq26";
	mkeq26.comment="[eq ] Verification of the null transport agents supply";
	mkeq26.domain=DOM_R2_ALL_PR;
	mkeq26.direction = CONSTR_LE0;
	mkeq26.cvars.insert(DomainPair("pl",CDOM_SCALAR_DOUBLE));


	cons.push_back(mkeq2);
	cons.push_back(mkeq6);
	cons.push_back(mkeq7);
	cons.push_back(mkeq13);
	cons.push_back(mkeq23);
	cons.push_back(mkeq3);
	cons.push_back(mkeq4);
	cons.push_back(mkeq5);
	cons.push_back(mkeq8);
	cons.push_back(mkeq14);
	cons.push_back(mkeq24);
	cons.push_back(mkeq17);
	cons.push_back(mkeq18);
	cons.push_back(mkeq26);
	cons.push_back(mkeq25);
;



}
/**
Define the objective function
*/
template<class T> bool
Opt::eval_obj(Index n, const T *x, T& obj_value){

	double aa, bb, dc0, sigma, a_pr, ct, m;
	obj_value = 0.;

	for (uint r1=0;r1<l2r.size();r1++){
		for (uint r2=0;r2<l2r[r1].size();r2++){
		//  // consumer's surplus..
		//  sum ((i,p_tr),
		//    AA(i,p_tr)*(RVAR('dc',i,p_tr)**((sigma(p_tr)+1)/sigma(p_tr)))
		//    - AA(i,p_tr)*((0.5*dc0(i,p_tr))**((sigma(p_tr)+1)/sigma(p_tr)))
		//    - RVAR('pc',i,p_tr)*RVAR('dc',i,p_tr)
		//  )
			for (uint p=0;p<secPr.size();p++){
				aa = gpd("aa",l2r[r1][r2],secPr[p]);
				sigma = gpd("sigma",l2r[r1][r2],secPr[p]);
				dc0 = gpd("dc",l2r[r1][r2],secPr[p],secondYear);
				obj_value += aa*pow(x[gix("dc",r1,r2,p)],(sigma+1)/sigma)-aa*pow(0.5*dc0,(sigma+1)/sigma)-x[gix("pc",r1,r2,p+nPriPr)]*x[gix("dc",r1,r2,p)];
			}
			//  // producers surplus..
			//  + sum((i,p_pr),
			//    RVAR('pc',i,p_pr)*RVAR('sc',i,p_pr)
			//    - BB(i,p_pr)*(RVAR('sc',i,p_pr)**((sigma(p_pr)+1)/sigma(p_pr)))
			//  )
			for (uint p=0;p<priPr.size();p++){
				bb = gpd("bb",l2r[r1][r2],priPr[p]);
				sigma = gpd("sigma",l2r[r1][r2],priPr[p]);
				obj_value += x[gix("pc",r1,r2,p)]*x[gix("sc",r1,r2,p)] - bb*pow(x[gix("sc",r1,r2,p)],((sigma+1)/sigma));
			}
			//  // trasformations between primary products
			//  + sum ((i,p_pr,p_pr2),
			//  +RVAR('pc',i,p_pr2)*pres(p_pr,p_pr2)*RVAR('sc',i,p_pr)
			//  -BB(i,p_pr2)*(pres(p_pr,p_pr2)*RVAR('sc',i,p_pr))**((sigma(p_pr2)+1)/sigma(p_pr2))
			//   )

			for (uint p1=0;p1<priPr.size();p1++){
				for (uint p2=0;p2<priPr.size();p2++){
					a_pr = gpd("a_pr",l2r[r1][r2],priPr[p1],DATA_NOW,priPr[p2]);
					bb = gpd("bb",l2r[r1][r2],priPr[p2]);
					sigma = gpd("sigma",l2r[r1][r2],priPr[p2]);
					obj_value += x[gix("pc",r1,r2,p2)]*a_pr*x[gix("sc",r1,r2,p1)]-bb*pow(a_pr*x[gix("sc",r1,r2,p1)],(sigma+1)/sigma);
				}
			}
			//  // surplus of transport agents..
			//  + sum((i,j,prd), (RVAR('pl',j,prd)-RVAR('pl',i,prd)-CT(i,j,prd))*EXP(i,j,prd))
			for (uint p=0;p<allPr.size();p++){
				for (uint r2To=0;r2To<l2r[r1].size();r2To++){
					ct = gpd("ct",l2r[r1][r2],allPr[p],DATA_NOW,i2s(l2r[r1][r2To]));
					obj_value += (x[gix("pl",r1,r2To,p)]-x[gix("pl",r1,r2,p)]-ct)*x[gix("rt",r1,r2,p,r2To)];
				}
			}

			//  // transformers surplus..
			//  + sum((i,p_tr), (RVAR('pl',i,p_tr)-m(i,p_tr))*(RVAR('sl',i,p_tr))) // attenction it's local. if we include w imports or p expports this have to change
			for (uint p=0;p<secPr.size();p++){
				m = gpd("m",l2r[r1][r2],secPr[p]);
				obj_value += (x[gix("pl",r1,r2,p+nPriPr)]-m)*x[gix("sl",r1,r2,p+nPriPr)];
			}
			//  - sum((i,p_pr), RVAR('pl',i,p_pr)*RVAR('dl',i,p_pr))               // to total and an other equation total=local+abroad should be added
			for (uint p=0;p<priPr.size();p++){
				obj_value -= x[gix("pl",r1,r2,p)]*x[gix("dl",r1,r2,p)];
			}
		} // end of each lev2 regions

	} //end of each r1 regions

	//obj_value = -obj_value; // we want maximisation, ipopt minimize! (donei n the options - scaling obj function)

 //exit(0);
	return true;
	// checked 20120802 this function is ok with gams, both in input and in output of the preoptimisation stage

}

/** Template function to implement (define) the previously declared constains.
To the initial macro loop it must be passed the product vector over where to loop (priPr, secPr or allPr) and the order of the constrain has it has been added to the const vector.
It could be possible to change this in a map and uses name, but then we would loose control on the constrains order, and we saw that it matters for finding the equilibrium.

*/
template<class T> bool
Opt::eval_constraints(Index n, const T *x, Index m, T* g){

	double a_pr, a, sigma, ff, gg, q1, p1v, t1, r1v, psi, eta, pworld, ct, k, dispor, mv;
	Index cix = 0;
	Index debug = 0;

	// mkteq2(i,p_tr)..  RVAR('dl',i,p_tr)+sum(j,EXP(i,j,p_tr))  =e=  RVAR('sl',i,p_tr)+ sum(b,EXP(b,i,p_tr)); // h1
	CONSTRAIN_START_LOOP(secPr, 0) // attenction! you have to give the same order number as you inserted in the cons vector
		g[cix] = x[gix("dl",r1,r2,psec)]-x[gix("sl",r1,r2,psec)];
		for (uint r2To=0;r2To<l2r[r1].size();r2To++){
			g[cix] += x[gix("rt",r1,r2,psec,r2To)]-x[gix("rt",r1,r2To,psec,r2)];
		}
	CONSTRAIN_END_LOOP

	// mkteq6(i,p_tr)..  RVAR('dc',i,p_tr)  =e=  GG(i,p_tr)*(RVAR('pc',i,p_tr)**sigma(p_tr)); // eq. 20
	CONSTRAIN_START_LOOP(secPr,1)
		gg = gpd("gg",l2r[r1][r2],secPr[p]);
		sigma = gpd("sigma",l2r[r1][r2],secPr[p]);
		g[cix] = x[gix("dc",r1,r2,p)]-gg*pow(x[gix("pc",r1,r2,psec)],sigma);
	CONSTRAIN_END_LOOP

	// mkteq7(i,p_tr)..  RVAR('da',i,p_tr)/RVAR('dl',i,p_tr)  =e=  ((q1(i,p_tr)*RVAR('pl',i,p_tr))/(p1(i,p_tr)*PT_t(p_tr)))**psi(i,p_tr); // h7 and h3 ?
	CONSTRAIN_START_LOOP(secPr,2)
		q1 = gpd("q1",l2r[r1][r2],secPr[p]);
		p1v = 1-q1;
		psi = gpd("psi",l2r[r1][r2],secPr[p]);
		pworld = gpd("pl", worldCodeLev2,secPr[p]);
		g[cix] = x[gix("da",r1,r2,p)]/x[gix("dl",r1,r2,psec)] - pow((q1*x[gix("pl",r1,r2,psec)])/(p1v*pworld),psi);
	CONSTRAIN_END_LOOP

	// mkteq13(i,p_tr).. RVAR('pc',i,p_tr)*RVAR('dc',i,p_tr)  =e=  RVAR('dl',i,p_tr)*RVAR('pl',i,p_tr)+RVAR('da',i,p_tr)*PT_t(p_tr);  // h9
	CONSTRAIN_START_LOOP(secPr,3)
		pworld = gpd("pl", worldCodeLev2,secPr[p]);
		g[cix] = x[gix("pc",r1,r2,psec)]*x[gix("dc",r1,r2,p)]-x[gix("dl",r1,r2,psec)]*x[gix("pl",r1,r2,psec)]-x[gix("da",r1,r2,p)]*pworld;
	CONSTRAIN_END_LOOP

	// mkteq23(i,p_tr).. RVAR('dc',i,p_tr)  =e=  (q1(i,p_tr)*(RVAR('da',i,p_tr)**((psi(i,p_tr)-1)/psi(i,p_tr)))+ p1(i,p_tr)*(RVAR('dl',i,p_tr)**((psi(i,p_tr)-1)/psi(i,p_tr))))**(psi(i,p_tr)/(psi(i,p_tr)-1)); // h3
	CONSTRAIN_START_LOOP(secPr,4)
		q1 = gpd("q1",l2r[r1][r2],secPr[p]);
		psi = gpd("psi",l2r[r1][r2],secPr[p]);
		p1v = 1-q1;
		g[cix] = x[gix("dc",r1,r2,p)] -
			pow(
						q1 * pow(x[gix("da",r1,r2,p)],(psi-1)/psi)
					+ p1v * pow(x[gix("dl",r1,r2,psec)],(psi-1)/psi),
					psi/(psi-1)
			);
	CONSTRAIN_END_LOOP

	// mkteq3(i,p_pr)..  RVAR('dl',i,p_pr)+sum(j,EXP(i,j,p_pr))   =e=  RVAR('sl',i,p_pr)+ sum(b,EXP(b,i,p_pr))+sum(p_pr2, pres(p_pr2,p_pr)* RVAR('sl',i,p_pr2)); // h2
	CONSTRAIN_START_LOOP(priPr,5)
		g[cix] = x[gix("dl",r1,r2,p)]-x[gix("sl",r1,r2,p)];
		for (uint r2To=0;r2To<l2r[r1].size();r2To++){
			g[cix] += x[gix("rt",r1,r2,p,r2To)]-x[gix("rt",r1,r2To,p,r2)];
		}
		for (uint p2=0;p2<priPr.size();p2++){
			a_pr = gpd("a_pr",l2r[r1][r2],priPr[p2],DATA_NOW,priPr[p]);
			g[cix] -= a_pr*x[gix("sl",r1,r2,p2)];
		}
	CONSTRAIN_END_LOOP

	//mkteq4(i,p_pr)..  RVAR('dl',i,p_pr)  =e=  sum(p_tr, a(p_pr,p_tr)*(RVAR('sl',i,p_tr))); // eq. 13
	CONSTRAIN_START_LOOP(priPr,6)
		g[cix] = x[gix("dl",r1,r2,p)];
		for (uint p2=0;p2<secPr.size();p2++){
			a = gpd("a",l2r[r1][r2],priPr[p],DATA_NOW,secPr[p2]);
			g[cix] -= a*x[gix("sl",r1,r2,p2+nPriPr)];
		}
	CONSTRAIN_END_LOOP

	// mkteq5(i,p_pr)..  RVAR('sc',i,p_pr)  =e=  FF(i,p_pr)*(RVAR('pc',i,p_pr)**sigma(p_pr)); // eq. 21
	CONSTRAIN_START_LOOP(priPr,7)
		ff = gpd("ff",l2r[r1][r2],priPr[p]);
		sigma = gpd("sigma",l2r[r1][r2],priPr[p]);
		g[cix] = x[gix("sc",r1,r2,p)]-ff*pow(x[gix("pc",r1,r2,p)],sigma);
	CONSTRAIN_END_LOOP


	// mkteq8(i,p_pr)..  RVAR('sa',i,p_pr)/RVAR('sl',i,p_pr)  =e=  ((t1(i,p_pr)*RVAR('pl',i,p_pr))/(r1(i,p_pr)*PT_t(p_pr)))**eta(i,p_pr); // h8 and h4 ?
	CONSTRAIN_START_LOOP(priPr,8)
		t1 = gpd("t1",l2r[r1][r2],priPr[p]);
		r1v = 1-t1;
		eta = gpd("eta",l2r[r1][r2],priPr[p]);
		pworld = gpd("pl", worldCodeLev2,priPr[p]);
		g[cix] = x[gix("sa",r1,r2,p)]/x[gix("sl",r1,r2,p)] - pow((t1*x[gix("pl",r1,r2,p)])/(r1v*pworld),eta);
	CONSTRAIN_END_LOOP

	// mkteq14(i,p_pr).. RVAR('pc',i,p_pr)*RVAR('sc',i,p_pr)  =e=  RVAR('sl',i,p_pr)*RVAR('pl',i,p_pr)+RVAR('sa',i,p_pr)*PT_t(p_pr);  // h10
	CONSTRAIN_START_LOOP(priPr,9)
		pworld = gpd("pl", worldCodeLev2,priPr[p]);
		g[cix] = x[gix("pc",r1,r2,p)]*x[gix("sc",r1,r2,p)]-x[gix("sl",r1,r2,p)]*x[gix("pl",r1,r2,p)]-x[gix("sa",r1,r2,p)]*pworld;
	CONSTRAIN_END_LOOP

	//mkteq24(i,p_pr).. RVAR('sc',i,p_pr)  =e=  (t1(i,p_pr)*(RVAR('sa',i,p_pr)**((eta(i,p_pr)-1)/eta(i,p_pr)))+ r1(i,p_pr)*(RVAR('sl',i,p_pr)**((eta(i,p_pr)-1)/eta(i,p_pr))))**(eta(i,p_pr)/(eta(i,p_pr)-1)); // h4
	CONSTRAIN_START_LOOP(priPr,10)
		t1 = gpd("t1",l2r[r1][r2],priPr[p]);
		r1v = 1-t1;
		eta = gpd("eta",l2r[r1][r2],priPr[p]);
		g[cix] = x[gix("sc",r1,r2,p)] -
					pow(
						 t1 * pow(x[gix("sa",r1,r2,p)],(eta-1)/eta)
					+ r1v * pow(x[gix("sl",r1,r2,p)],(eta-1)/eta),
					eta/(eta-1)
				);
	CONSTRAIN_END_LOOP

	// mkteq17(i,p_tr).. RVAR('sl',i,p_tr)                    =l=  Kt(i,p_tr);     // h16 in the presentation paper
	CONSTRAIN_START_LOOP(secPr,11)
		k = gpd("k",l2r[r1][r2],secPr[p]);
		g[cix] = x[gix("sl",r1,r2,p+nPriPr)]-k;
	CONSTRAIN_END_LOOP

	// mkteq18(i,p_pr).. RVAR('sa',i,p_pr)+RVAR('sl',i,p_pr)  =l=  dispor(i,p_pr); // total supply lower than the available stock
	CONSTRAIN_START_LOOP(priPr,12)
		dispor = gpd("dispor",l2r[r1][r2],priPr[p]);
		g[cix] = x[gix("sa",r1,r2,p)]+x[gix("sl",r1,r2,p)]-dispor;
	CONSTRAIN_END_LOOP

	// mkeq26(i,prd,j).. RVAR('pl',j,prd)-RVAR('pl',i,prd)-CT(i,j,prd) =l= 0;
	CONSTRAIN_START_LOOP(allPr,13)
		for (uint r2To=0;r2To<l2r[r1].size();r2To++){
			cix = gix(13, r1, r2, p,r2To); // attenction we must redefine it, as we are now in a r2to loop
			ct = gpd("ct",l2r[r1][r2],allPr[p],DATA_NOW,i2s(l2r[r1][r2To]));
			g[cix] = (x[gix("pl",r1,r2To,p)]-x[gix("pl",r1,r2,p)]-ct);
		}
	CONSTRAIN_END_LOOP

	// mkteq25(i,p_tr).. sum(p_pr, a(p_pr,p_tr)*RVAR('pl',i,p_pr))+m(i,p_tr)  =g=  (RVAR('pl',i,p_tr));        // price of raw products + transf cost > trasf product
	CONSTRAIN_START_LOOP(secPr,14)
		mv = gpd("m",l2r[r1][r2],secPr[p]);
		g[cix] = mv - x[gix("pl",r1,r2,p+nPriPr)];
		for (uint p2=0;p2<priPr.size();p2++){
			a = gpd("a",l2r[r1][r2],priPr[p2],DATA_NOW,secPr[p]);
			g[cix] += a * x[gix("pl",r1,r2,p2)];
		}
	CONSTRAIN_END_LOOP

	return true;
}


//  ******** NOTHING TO DO BELOW THIS LINE ********************************

Opt::Opt(ThreadManager* MTHREAD_h){
	MTHREAD = MTHREAD_h;
	nVar    = 0;
	nCons   = 0;
	debugRunOnce = false;
	initOpt = true;
}

Opt::~Opt(){

}


bool
Opt::get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
									Index& nnz_h_lag, IndexStyleEnum& index_style){


	if(initOpt){
		// does this initialisation code only once
		priPr = MTHREAD->MD->getStringVectorSetting("priProducts");
		secPr = MTHREAD->MD->getStringVectorSetting("secProducts");
		allPr = priPr;
		allPr.insert( allPr.end(), secPr.begin(), secPr.end() );
		nPriPr = priPr.size();
		nSecPr = secPr.size();
		nAllPr = allPr.size();
		std::vector<int> l1regIds =  MTHREAD->MD->getRegionIds(1, true);
		nL2r =  MTHREAD->MD->getRegionIds(2, true).size();
		firstYear = MTHREAD->MD->getIntSetting("initialYear");
		secondYear = firstYear+1;
		worldCodeLev2 = MTHREAD->MD->getIntSetting("worldCodeLev2");

		for(uint i=0;i<l1regIds.size();i++){
			std::vector<int> l2ChildrenIds;
			ModelRegion* l1Region = MTHREAD->MD->getRegion(l1regIds[i]);
			std::vector<ModelRegion*> l2Childrens = l1Region->getChildren(true);
			for(uint j=0;j<l2Childrens.size();j++){
				l2ChildrenIds.push_back(l2Childrens[j]->getRegId());
			}
			if(l2ChildrenIds.size()){
				l2r.push_back(l2ChildrenIds);
			}
		}

		// put the variables and their domain in the vars map
		declareVariables();

		// declaring the contrains...
		declareConstrains();

		// calculate number of variables and constrains..
		calculateNumberVariablesConstrains();

		// cache initial positions (variables and constrains)..
		cacheInitialPosition();

		// cache initial positions (variables and constrains)..
		cachePositions();

		calculateSparsityPattern();

		//tempDebug();

		//debugPrintParameters();
		initOpt=false;
	} // finish initialisation things to be done only the first year

	previousYear = MTHREAD->SCD->getYear()-1; // this has to be done EVERY years !!

	n = nVar; // 300; // nVar;

	m = nCons; // 70; // nCons;

	// in this example the jacobian is dense. Hence, it contains n*m nonzeros
	//nnz_jac_g = n*m;
	nnz_jac_g = nzjelements.size();

	// the hessian is also dense and has n*n total nonzeros, but we
	// only need the lower left corner (since it is symmetric)
	nnz_h_lag = n*(n-1)/2+n;
	generate_tapes(n, m);
	// use the C style indexing (0-based)
	index_style = C_STYLE;

	return true;
}

bool
Opt::get_bounds_info(Index n, Number* x_l, Number* x_u, Index m, Number* g_l, Number* g_u){


	// all variables have upper or equal than zero bound
	for (Index i=0; i<n; i++) {
	// removed 20120812, as not really needed.. doesn't actually change either obj or efforts to reach it
//		if(
//				 ( n>=gip("dl")&& n<gip("dl")+nAllPr)
//			|| ( n>=gip("sa")&& n<gip("sa")+nPriPr)
//			|| ( n>=gip("sl")&& n<gip("sl")+nAllPr)
//			|| ( n>=gip("da")&& n<gip("da")+nSecPr)
//			|| ( n>=gip("pl")&& n<gip("sa")+nAllPr)
//		) {  // I could make a function to return the number of positions by a certain variable name, looking up the the map that define its domain
//			x_l[i] = 0.00000001;
//			x_u[i] =       1e20;
//		} else {
			x_l[i] =         0.;
			x_u[i] =       1e20;
//		}
	}

	// Set the bounds for the constraints
	for (Index i=0; i<m; i++) {
		int direction = getConstrainDirectionByIndex(i);
		//cout << "i: " << i << "direction: " << direction << endl;
		switch (direction){
			case CONSTR_EQ:
				g_l[i] = 0.;
				g_u[i] = 0.;
				break;
			case CONSTR_LE0:
				g_l[i] = -2e19;
				g_u[i] = 0.;
				break;
			case CONSTR_GE0:
				g_l[i] = 0.;
				g_u[i] = 2e19;
				break;
		}
	}
	return true;
}

bool
Opt::get_starting_point(Index n, bool init_x, Number* x, bool init_z, Number* z_L, Number* z_U,
												Index m, bool init_lambda, Number* lambda){

// function checked on 20120724 on a subset of 3 regions and 4 products. All variables initial values are correctly those outputed by gams in 2006.

	msgOut(MSG_DEBUG,"Giving optimising variables previous years value as starting point");
	// Here, we assume we only have starting values for x, if you code
	// your own NLP, you can provide starting values for the others if
	// you wish.
	assert(init_x == true);
	assert(init_z == false);
	assert(init_lambda == false);

	map<string, int>::iterator viter;

	// fixing the starting points for each variable at the level of the previous years
	for (viter = vars.begin(); viter != vars.end(); ++viter) {
		//string debugs = viter->first;
		int vdomtype = viter->second;
		if(vdomtype==DOM_PRI_PR){
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<priPr.size();p++){
						x[gix(viter->first,r1,r2,p)]= gpd(viter->first,l2r[r1][r2],priPr[p],previousYear);
					}
				}
			}
		} else if (vdomtype==DOM_SEC_PR) {
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<secPr.size();p++){
						x[gix(viter->first,r1,r2,p)]= gpd(viter->first,l2r[r1][r2],secPr[p],previousYear);
					}
				}
			}
		} else if (vdomtype==DOM_ALL_PR) {
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<allPr.size();p++){
						x[gix(viter->first,r1,r2,p)]= gpd(viter->first,l2r[r1][r2],allPr[p],previousYear);
					}
				}
			}
		} else if (vdomtype==DOM_R2_ALL_PR) {
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<allPr.size();p++){
						for(uint r2To=0;r2To<l2r[r1].size();r2To++){
							x[gix(viter->first,r1,r2,p,r2To)]= gpd(viter->first,l2r[r1][r2],allPr[p],previousYear,i2s(l2r[r1][r2To]));
						}
					}
				}
			}
		} else {
			msgOut(MSG_CRITICAL_ERROR,"Try to setting the initial value of a variable of unknow type ("+viter->first+")");
		}
	}

	msgOut(MSG_DEBUG,"Finisced initial value assignments");

	return true;
}







void
Opt::finalize_solution(SolverReturn status,
											 Index n, const Number* x, const Number* z_L, const Number* z_U,
											 Index m, const Number* g, const Number* lambda,
											 Number obj_value, const IpoptData* ip_data, IpoptCalculatedQuantities* ip_cq){

	printf("\n\nObjective value\n");
	printf("f(x*) = %e\n", obj_value);

	// --> here is where to code the assignment of optimal values to to spd()

		map<string, int>::iterator viter;

	// fixing the starting points for each variable at the level of the previous years
	for (viter = vars.begin(); viter != vars.end(); ++viter) {
		//string debugs = viter->first;
		int vdomtype = viter->second;
		if(vdomtype==DOM_PRI_PR){
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<priPr.size();p++){
						spd(x[gix(viter->first,r1,r2,p)],viter->first,l2r[r1][r2],priPr[p]);
					}
				}
			}
		} else if (vdomtype==DOM_SEC_PR) {
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<secPr.size();p++){
						spd(x[gix(viter->first,r1,r2,p)],viter->first,l2r[r1][r2],secPr[p]);

					}
				}
			}
		} else if (vdomtype==DOM_ALL_PR) {
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<allPr.size();p++){
						spd(x[gix(viter->first,r1,r2,p)],viter->first,l2r[r1][r2],allPr[p]);
					}
				}
			}
		} else if (vdomtype==DOM_R2_ALL_PR) {
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<allPr.size();p++){
						for(uint r2To=0;r2To<l2r[r1].size();r2To++){
							spd(x[gix(viter->first,r1,r2,p,r2To)],viter->first,l2r[r1][r2],allPr[p],DATA_NOW,false,i2s(l2r[r1][r2To]));
						}
					}
				}
			}
		} else {
			msgOut(MSG_CRITICAL_ERROR,"Try to setting the solved value of a variable of unknow type ("+viter->first+")");
		}
	}



// Memory deallocation for ADOL-C variables

	delete[] x_lam;

	for(Index i=0;i<m;i++)
		delete[] Jac[i];
	delete[] Jac;

	for(Index i=0;i<n+m+1;i++)
		delete[] Hess[i];
	delete[] Hess;
}

//*************************************************************************
//
//
//         Nothing has to be changed below this point !!
//
//
//*************************************************************************


bool
Opt::eval_f(Index n, const Number* x, bool new_x, Number& obj_value){
	eval_obj(n,x,obj_value);

	return true;
}

bool
Opt::eval_grad_f(Index n, const Number* x, bool new_x, Number* grad_f){

	gradient(tag_f,n,x,grad_f);

	return true;
}

bool
Opt::eval_g(Index n, const Number* x, bool new_x, Index m, Number* g){

	eval_constraints(n,x,m,g);

	return true;
}

bool
Opt::eval_jac_g(Index n, const Number *x, bool new_x,Index m, Index nele_jac,
								Index* iRow, Index *jCol, Number* values){
	if (values == NULL) {
		// return the structure of the jacobian,
		// assuming that the Jacobian is dense


		for(Index idx=0;idx<nzjelements.size();idx++){
			iRow[idx] = nzjelements[idx][0];
			jCol[idx] = nzjelements[idx][1];
		}

		/*
		Index idx = 0;
		for(Index i=0; i<m; i++)
			for(Index j=0; j<n; j++)
		{
			iRow[idx] = i;
			jCol[idx++] = j;
		}
			*/
 }
	else {
		// return the values of the jacobian of the constraints

		jacobian(tag_g,m,n,x,Jac);

/*
		vector < vector<double> > fakeJ;
		for(Index r=0; r<nCons;r++){
			vector<double> fakeJRow;
			for (Index c=0; c<nVar; c++){
				fakeJRow.push_back(Jac[r][c]);
			}
			fakeJ.push_back(fakeJRow);
		}


for(Index r=0; r<nCons; r++){
	for (Index c=0; c<nVar; c++){
		double val = Jac[r][c];
		if (val != 0){
			bool found = false;
			//looping in the nzjelements to be sure we included it
			for(uint i=0;i<nzjelements.size();i++){
				if(nzjelements[i][0]==r && nzjelements[i][1]==c){
					found=true;
					break;
				}
			}
			if(!found){
				cout << "***** HERE WE ARE !!!! r/c/value: "<< r << " / " << c << " / " << Jac[r][c] << endl;
			}
		}
	}
}
*/

		for(Index idx=0;idx<nzjelements.size();idx++){
			values[idx] = Jac[nzjelements[idx][0]][nzjelements[idx][1]];
		}

		/*
		Index idx = 0;
		for(Index i=0; i<m; i++)
			for(Index j=0; j<n; j++){
				values[idx++] = Jac[i][j];
			}
		*/

	}

	return true;
}

bool
Opt::eval_h(Index n, const Number* x, bool new_x, Number obj_factor, Index m, const Number* lambda,
						bool new_lambda, Index nele_hess, Index* iRow, Index* jCol, Number* values){

/*
	if (values == NULL) {
		// return the structure. This is a symmetric matrix, fill the lower left
		// triangle only.

		// the hessian for this problem is actually dense
		Index idx=0;
		for (Index row = 0; row < n; row++) {
			for (Index col = 0; col <= row; col++) {
				iRow[idx] = row;
				jCol[idx] = col;
				idx++;
			}
		}

		assert(idx == nele_hess);
	}
	else {
		// return the values. This is a symmetric matrix, fill the lower left
		// triangle only

		for(Index i = 0; i<n ; i++)
			x_lam[i] = x[i];
		for(Index i = 0; i<m ; i++)
			x_lam[n+i] = lambda[i];
		x_lam[n+m] = obj_factor;

		hessian(tag_L,n+m+1,x_lam,Hess);

		Index idx = 0;

		for(Index i = 0; i<n ; i++)
			{
		for(Index j = 0; j<=i ; j++)
			{
				values[idx++] = Hess[i][j];
			}
			}
	}

	return true;
*/
	return false;
}


//***************    ADOL-C part ***********************************

void
Opt::generate_tapes(Index n, Index m){
	Number *xp    = new double[n];
	Number *lamp  = new double[m];
	Number *zl    = new double[m];
	Number *zu    = new double[m];

	adouble *xa   = new adouble[n];
	adouble *g    = new adouble[m];
	adouble *lam  = new adouble[m];
	adouble sig;
	adouble obj_value;

	double dummy;

	//TODO: remove the next
	Jac = new double*[m];
	for(Index i=0;i<m;i++)
		Jac[i] = new double[n];

	x_lam   = new double[n+m+1];

	Hess = new double*[n+m+1];
	for(Index i=0;i<n+m+1;i++)
		Hess[i] = new double[i+1];

	get_starting_point(n, 1, xp, 0, zl, zu, m, 0, lamp);

	trace_on(tag_f);

		for(Index i=0;i<n;i++)
			xa[i] <<= xp[i];

		eval_obj(n,xa,obj_value);

		obj_value >>= dummy;

	trace_off();

	trace_on(tag_g);

		for(Index i=0;i<n;i++)
			xa[i] <<= xp[i];

		eval_constraints(n,xa,m,g);


		for(Index i=0;i<m;i++)
			g[i] >>= dummy;

	trace_off();

	 trace_on(tag_L);

		for(Index i=0;i<n;i++)
			xa[i] <<= xp[i];
		for(Index i=0;i<m;i++)
			lam[i] <<= 1.0;
		sig <<= 1.0;

		eval_obj(n,xa,obj_value);

		obj_value *= sig;
		eval_constraints(n,xa,m,g);

		for(Index i=0;i<m;i++)
			obj_value += g[i]*lam[i];

		obj_value >>= dummy;

	trace_off();

	delete[] xa;
	delete[] xp;
	delete[] g;
	delete[] lam;
	delete[] lamp;
	delete[] zu;
	delete[] zl;

}


// ***************    PYTHIA OPT specific part ***********************************

const int
Opt::gip(const string &varName){ // get initial position
	map<string, int>::const_iterator p;
	p=initPos.find(varName);
	if(p != initPos.end()) {
		return p->second;
	}
	else {
		msgOut(MSG_CRITICAL_ERROR, "Asking the initial position in the concatenated array of a variable ("+varName+") that doesn't exist!");
		return 0;
	}
};

const int
Opt::gip(const int &cn){ // get initial position
	return cInitPos.at(cn);
};

const int
Opt::gdt(const string &varName){ // get domain type
	map<string, int>::const_iterator p;
	p=vars.find(varName);
	if(p != initPos.end()) {
		return p->second;
	}
	else {
		msgOut(MSG_CRITICAL_ERROR, "Asking the domain type of a variable ("+varName+") that doesn't exist!");
		return 0;
	}
}

const int
Opt::gdt(const int &cn){ // get domain type
	return cons.at(cn).domain;
}

template<class T> const int
Opt::gix_uncached(const T &v_or_c, int r1Ix, int r2Ix, int prIx, int r2IxTo){

	// attenction, for computational reason we are not checking the call is within vectors limits!!!

	int dType = gdt(v_or_c);
	int othCountriesRegions = 0;
	int othCountriesRegions_r2case = 0;
	for (uint i=0;i<r1Ix;i++){
		othCountriesRegions += l2r[i].size();
	}
	for (uint i=0;i<r1Ix;i++){
		othCountriesRegions_r2case +=l2r[i].size()*l2r[i].size();
	}

	switch (dType){
		case DOM_PRI_PR:
			return gip(v_or_c)+(othCountriesRegions+r2Ix)*nPriPr+prIx;
		case DOM_SEC_PR:
			return gip(v_or_c)+(othCountriesRegions+r2Ix)*nSecPr+prIx;;
		case DOM_ALL_PR:
			return gip(v_or_c)+(othCountriesRegions+r2Ix)*nAllPr+prIx;
		case DOM_R2_PRI_PR:
			return gip(v_or_c)+(othCountriesRegions_r2case)*nAllPr+(r2Ix*nPriPr+prIx)*l2r[r1Ix].size()+r2IxTo;
		case DOM_R2_SEC_PR:
			return gip(v_or_c)+(othCountriesRegions_r2case)*nAllPr+(r2Ix*nSecPr+prIx)*l2r[r1Ix].size()+r2IxTo;
		case DOM_R2_ALL_PR:
			return gip(v_or_c)+(othCountriesRegions_r2case)*nAllPr+(r2Ix*nAllPr+prIx)*l2r[r1Ix].size()+r2IxTo; // new 20120814, looping r1,r2,p,r2to
						// initial position + (other countries region pairs + same country other regions from pair + regions to)* number of all products+product
			//return gip(v_or_c)+(othCountriesRegions_r2case+r2Ix*l2r[r1Ix].size()+r2IxTo)*nAllPr+prIx; // looping r1,r2,r2to,p
		case DOM_SCALAR:
			return gip(v_or_c);
	default:
		msgOut(MSG_CRITICAL_ERROR,"Try to calculate the position of a variable (or constrain) of unknow type.");
		return 0;
	}
}


const int
Opt::gix(const string &varName, int r1Ix, int r2Ix, int prIx, int r2IxTo){
	// attenction, for computational reasons we are not checking the call is within vectors limits!!!
	map <string, vector < vector < vector < vector <int> > > > >::const_iterator p;
	p=vpositions.find(varName);
	if(p != vpositions.end()) {
		return p->second[r1Ix][r2Ix][prIx][r2IxTo];
	}
	else {
		msgOut(MSG_CRITICAL_ERROR, "Asking the position of a variable ("+varName+") that doesn't exist!");
		return 0;
	}
}

const int
Opt::gix(const int &cn, int r1Ix, int r2Ix, int prIx, int r2IxTo){
	return cpositions[cn][r1Ix][r2Ix][prIx][r2IxTo];
}

void
Opt::cacheInitialPosition(){
	int vInitialPosition = 0;
	int cInitialPosition = 0;
	map<string, int>::iterator viter;
	for (viter = vars.begin(); viter != vars.end(); ++viter) {
		initPos.insert(pair<string, int>(viter->first, vInitialPosition));
		vInitialPosition += getDomainElements(viter->second);
	}
	for (uint i=0;i<cons.size();i++){
		cInitPos.push_back(cInitialPosition);
		cInitialPosition += getDomainElements(cons[i].domain);
	}
}

void
Opt::cachePositions(){

	// variables..
	map<string, int>::iterator viter;
	for (viter = vars.begin(); viter != vars.end(); ++viter) {
		vpositions.insert(pair<string, vector < vector < vector < vector <int> > > > >(viter->first,  buildPositionVector(viter->first, viter->second)));
	}
	// constrains..
	for (uint i=0; i<cons.size();i++){
		cpositions.push_back(buildPositionVector(i, cons[i].domain));
	}

}

template<class T> vector < vector < vector < vector <int> > > >
Opt::buildPositionVector(const T &v_or_c, int dType){
	switch (dType){
		case DOM_PRI_PR:
			return buildPositionVector(v_or_c, priPr);
		case DOM_SEC_PR:
			return buildPositionVector(v_or_c, secPr);
		case DOM_ALL_PR:
			return buildPositionVector(v_or_c, allPr);
		case DOM_R2_PRI_PR:
			return buildPositionVector(v_or_c, priPr);
		case DOM_R2_SEC_PR:
			return buildPositionVector(v_or_c, secPr);
		case DOM_R2_ALL_PR:
			return buildPositionVector(v_or_c, allPr);
		case DOM_SCALAR:
			return buildPositionVector(v_or_c, allPr); // it will simply fill the matrix all with the same value (the ip)
	default:
		msgOut(MSG_CRITICAL_ERROR,"Try to build the position of a variable (or contrain) of unknow type.");
	}
}

template<class T> vector < vector < vector < vector <int> > > >
Opt::buildPositionVector(const T &v_or_c, vector<string> &prVector){
	vector < vector < vector < vector <int> > > > positionsToAdd;
	for(uint r1=0;r1<l2r.size();r1++){
		vector < vector < vector <int> > > dim1;
		for(uint r2=0;r2<l2r[r1].size();r2++){
			vector < vector <int> > dim2;
			for(uint p=0;p<prVector.size();p++){
				vector <int> dim3;
					for(uint r2To=0;r2To<l2r[r1].size();r2To++){
						dim3.push_back(gix_uncached(v_or_c,r1,r2,p,r2To));
					}
				dim2.push_back(dim3);
			}
			dim1.push_back(dim2);
		}
		positionsToAdd.push_back(dim1);
	}
	return positionsToAdd;
}


void
Opt::calculateNumberVariablesConstrains(){
		// calculating the number of variables and the initial positions in the concatenated array..
		nVar = 0;
		map<string, int>::iterator viter;
		for (viter = vars.begin(); viter != vars.end(); ++viter) {
				nVar += getDomainElements(viter->second);
		}

		// calculating the number of constrains..
		nCons = 0;
		nEqualityConstrains = 0;
		nLowerEqualZeroConstrains = 0;
		nGreaterEqualZeroConstrains = 0;
		for(uint i=0;i<cons.size();i++){
			nCons += getDomainElements(cons[i].domain);
			if(cons[i].direction == CONSTR_EQ){
				nEqualityConstrains += getDomainElements(cons[i].domain);
				continue;
			} else if (cons[i].direction == CONSTR_LE0) {
				nLowerEqualZeroConstrains += getDomainElements(cons[i].domain);
				continue;
			} else if (cons[i].direction == CONSTR_GE0) {
				nGreaterEqualZeroConstrains += getDomainElements(cons[i].domain);
				continue;
			} else {
				msgOut(MSG_CRITICAL_ERROR, "Asking for a constrain with unknown direction ("+i2s(cons[i].direction)+")");
			}
		}

		msgOut(MSG_INFO,"The model will work with "+i2s(nVar)+" variables and "+i2s(nCons)+" constrains ("+i2s(nEqualityConstrains)+" equalities, "+i2s(nLowerEqualZeroConstrains)+" lower than 0 and "+i2s(nGreaterEqualZeroConstrains)+" greater than 0)");
}

int
Opt::getDomainElements(int domain){
	int elements = 0;
	switch (domain){
		case DOM_PRI_PR:
			return nL2r*nPriPr;
		case DOM_SEC_PR:
			return nL2r*nSecPr;
		case DOM_ALL_PR:
			return nL2r*nAllPr;
		case DOM_R2_PRI_PR:
			for(uint r1=0;r1<l2r.size();r1++){
					elements += l2r[r1].size()*l2r[r1].size()*nPriPr; // EXP(i,j,p_pr)
			}
			return elements;
		case DOM_R2_SEC_PR:
			for(uint r1=0;r1<l2r.size();r1++){
					elements += l2r[r1].size()*l2r[r1].size()*nSecPr; // EXP(i,j,p_tr)
			}
			return elements;
		case DOM_R2_ALL_PR:
			for(uint r1=0;r1<l2r.size();r1++){
					elements += l2r[r1].size()*l2r[r1].size()*nAllPr; // EXP(i,j,prd)
			}
			return elements;
		case DOM_SCALAR:
			return 1;
	default:
		msgOut(MSG_CRITICAL_ERROR, "Asking for an unknown domain type ("+i2s(domain)+")");
	}
}

int
Opt::getConstrainDirectionByIndex(int idx){
	for(uint i=0;i<cons.size();i++){
		if(i!=cons.size()-1){
			if (idx >= gip(i) && idx < gip(i+1)){
				return cons[i].direction;
			}
		} else {
			if (idx >= gip(i) && idx < nCons){
			return cons[i].direction;
			}
		}
	}
	msgOut(MSG_CRITICAL_ERROR, "Asking contrain direction for an out of range contrain index!");
}

constrain*
Opt::getConstrainByIndex(int idx){
	for(uint i=0;i<cons.size();i++){
		if(i!=cons.size()-1){
			if (idx >= gip(i) && idx < gip(i+1)){
				return &cons[i];
			}
		} else {
			if (idx >= gip(i) && idx < nCons){
			return &cons[i];
			}
		}
	}
	msgOut(MSG_CRITICAL_ERROR, "Asking contrain direction for an out of range contrain index!");
}


void
Opt::unpack(int ix_h, int domain, int initial, int &r1_h, int &r2_h, int&p_h, int&r2to_h, bool fullp){

	ix_h = ix_h-initial;
	double ix=0;
	bool r2flag = false;
	int pIndexToAdd = 0;
	int np=0;
	if(domain==DOM_PRI_PR || domain==DOM_R2_PRI_PR) {
		np = nPriPr;
	} else if (domain==DOM_SEC_PR || domain==DOM_R2_SEC_PR) {
		np = nSecPr;
	} else if (domain==DOM_ALL_PR || domain==DOM_R2_ALL_PR) {
		np = nAllPr;
	} else if (domain=DOM_SCALAR){
		r1_h=0;r2_h=0;p_h=0;r2to_h=0;
		return;
	} else {
		msgOut(MSG_CRITICAL_ERROR,"unknow domain ("+i2s(domain)+") in unpack() function.");
	}
	if(domain==DOM_R2_PRI_PR || domain==DOM_R2_SEC_PR ||domain==DOM_R2_ALL_PR){
		r2flag = true;
	}
	if(fullp && (domain==DOM_ALL_PR || domain==DOM_R2_ALL_PR)){ // not working, lead to crash
		pIndexToAdd = nPriPr;
		//cout << "pindexToAdd: " << pIndexToAdd << endl;
	}

	for (uint r1=0;r1<l2r.size();r1++){
		for (uint r2=0;r2<l2r[r1].size();r2++){
			for (uint p=0;p<np;p++){
				if(!r2flag){
					if(ix==ix_h){
						r1_h=r1;
						r2_h=r2;
						p_h=p+pIndexToAdd;
						r2to_h=0;
						return;
					}
					ix++;
				} else {
					for (uint r2To=0;r2To<l2r[r1].size();r2To++){
						if(ix==ix_h){
							r1_h=r1;
							r2_h=r2;
							p_h=p+pIndexToAdd;
							r2to_h=r2To;
							return;
						}
						ix++;
					}
				}
			}
		}
	}
	msgOut(MSG_CRITICAL_ERROR, "Error in unpack() function. Ix ("+i2s(ix_h)+") can not be unpacked");
}

int
Opt::getConNumber(constrain *con){
	for(uint i=0;i<cons.size();i++){
		if(   cons[i].name       == con->name
			 && cons[i].comment    == con->comment
			 && cons[i].domain     == con->domain
			 && cons[i].direction  == con->direction){
			return i;
			 }
	}
	msgOut(MSG_CRITICAL_ERROR,"Constrain didn't found in list.");
}

void
Opt::calculateSparsityPattern(){
	constrain* CON;
	int r1,r2,p,pfull,pForV,r2to,ipos,cdom,cpos,vdom,vdomInC;
	int raw=0,col1=0,col2=0;
	map<string, int>::iterator viter;


	for(uint c=0;c<nCons;c++){
		raw = c;
		CON = getConstrainByIndex(c);
		cpos = getConNumber(CON);
		ipos = gip(cpos);
		cdom = gdt(cpos);
		// ok cout << "c/name/cpos/initpos/dom: " << c << "/" << CON->name << "/" << cpos << "/" << ipos << "/" << dom << endl;
		unpack(c,cdom,ipos,r1,r2,p,r2to);
		if(cdom==DOM_SEC_PR || cdom==DOM_R2_SEC_PR){
			pfull=nPriPr+p;
		} else {
			pfull=p;
		}

		for (viter = CON->cvars.begin(); viter != CON->cvars.end(); ++viter) {
			string debugname = viter->first;
			vdom = gdt(viter->first);
			vdomInC     = viter->second;
			if(vdom==DOM_ALL_PR || vdom==DOM_R2_ALL_PR){
				pForV=pfull;
			} else {
				pForV=p;
			}
			switch (vdomInC){
				case CDOM_SCALAR:{
					col1 = gix(viter->first,r1,r2,pForV,r2to);
					vector<int> nzjelement1;
					nzjelement1.push_back(raw);
					nzjelement1.push_back(col1);
					nzjelements.push_back(nzjelement1);
				}
				break;
				case CDOM_R2FROMTO:{
					for(int r2c=0;r2c<l2r[r1].size();r2c++){
						col1 = gix(viter->first,r1,r2,pForV,r2c);
						col2 = gix(viter->first,r1,r2c,pForV,r2);
						vector<int> nzjelement1,nzjelement2;
						nzjelement1.push_back(raw);
						nzjelement1.push_back(col1);
						nzjelement2.push_back(raw);
						nzjelement2.push_back(col2);
						nzjelements.push_back(nzjelement1);
						if(col2!=col1) nzjelements.push_back(nzjelement2);
					}
				}
				break;
				case CDOM_PRI_PR:{
					for(int p2=0;p2<nPriPr;p2++){
						col1 = gix(viter->first,r1,r2,p2);
						vector<int> nzjelement1;
						nzjelement1.push_back(raw);
						nzjelement1.push_back(col1);
						nzjelements.push_back(nzjelement1);
					}
				}
				break;
				case CDOM_SEC_PR:{
					int p2c;
					for(int p2=0;p2<nSecPr;p2++){
						if(vdom==DOM_ALL_PR || vdom==DOM_R2_ALL_PR){
							p2c = nPriPr+p2;
						} else {
							p2c = p2;
						}
						col1 = gix(viter->first,r1,r2,p2c);
						vector<int> nzjelement1;
						nzjelement1.push_back(raw);
						nzjelement1.push_back(col1);
						nzjelements.push_back(nzjelement1);
					}
				}
				break;
				case CDOM_ALL_PR:{
					for(int p2=0;p2<nAllPr;p2++){
						col1 = gix(viter->first,r1,r2,p2);
						vector<int> nzjelement1;
						nzjelement1.push_back(raw);
						nzjelement1.push_back(col1);
						nzjelements.push_back(nzjelement1);
					}
				}
				break;
				case CDOM_PRI_PR_AND_SCALAR:{
					for(int p2=0;p2<nPriPr;p2++){
						col1 = gix(viter->first,r1,r2,p2);
						vector<int> nzjelement1;
						nzjelement1.push_back(raw);
						nzjelement1.push_back(col1);
						nzjelements.push_back(nzjelement1);
					}
					col2 = gix(viter->first,r1,r2,pForV);
					vector<int> nzjelement2;
					nzjelement2.push_back(raw);
					nzjelement2.push_back(col2);
					nzjelements.push_back(nzjelement2);
				}
				break;
				case CDOM_SCALAR_DOUBLE:{
					col1 = gix(viter->first,r1,r2,pForV,r2to);
					col2 = gix(viter->first,r1,r2to,pForV,r2);
					vector<int> nzjelement1,nzjelement2;
					nzjelement1.push_back(raw);
					nzjelement1.push_back(col1);
					nzjelement2.push_back(raw);
					nzjelement2.push_back(col2);
					nzjelements.push_back(nzjelement1);
					if(col2!=col1) nzjelements.push_back(nzjelement2);
				}
				break;
				default:{
					msgOut(MSG_CRITICAL_ERROR,"In calCulateSparSePattern() looping trough a variable with an unknow domain type in the constrain ("+i2s(vdomInC)+")");
				}
			}
		}
	}
}

void
Opt::tempDebug(){


	cout << "IDX | ROW | COL" << endl;
	for(uint i=0;i<nzjelements.size();i++){
		cout << i << " | " << nzjelements[i][0] << " | " << nzjelements[i][1] << endl;
	}

	cout << "Dense jacobian: " << nCons * nVar << " elements" << endl;
	//exit(0);


}
