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

using namespace Ipopt;

Opt::Opt(ThreadManager* MTHREAD_h){
	MTHREAD = MTHREAD_h;
	nVar    = 0;
	nCons   = 0;
}

Opt::~Opt(){

}


bool
Opt::get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
									Index& nnz_h_lag, IndexStyleEnum& index_style){


	if(MTHREAD->SCD->getIteration()==0){
		// does this caching thing only once
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

		// filling the list of variables and their domain
		// if you add variables in the model that enter optimisation you'll have to add them here
		// the map goes automatically in alphabetical order
		// original order: pc,pl,dc,dl,da,sc,sl,sa,exp
		vars.insert(std::pair<string, int>("da", VDOM_SEC_PR));
		vars.insert(std::pair<string, int>("dc", VDOM_SEC_PR));
		vars.insert(std::pair<string, int>("dl", VDOM_ALL_PR));
		vars.insert(std::pair<string, int>("pc", VDOM_ALL_PR));
		vars.insert(std::pair<string, int>("pl", VDOM_ALL_PR));
		vars.insert(std::pair<string, int>("rt", VDOM_R2_ALL_PR)); // regional trade, it was exp in gams
		vars.insert(std::pair<string, int>("sa", VDOM_PRI_PR));
		vars.insert(std::pair<string, int>("sc", VDOM_PRI_PR));
		vars.insert(std::pair<string, int>("sl", VDOM_ALL_PR));


		// calculating the number of variables and the initial positions in the concatenated array..
		nVar = 0;
		map<string, int>::iterator viter;
		for (viter = vars.begin(); viter != vars.end(); ++viter) {
			string debugs = viter->first;
			int debugpos = nVar;
			initPos.insert(pair<string, int>(viter->first, nVar));
			int vdomtype = viter->second;
			if(vdomtype==VDOM_PRI_PR){
				nVar += nL2r*nPriPr;
			} else if (vdomtype==VDOM_SEC_PR) {
				nVar += nL2r*nSecPr;
			} else if (vdomtype==VDOM_ALL_PR) {
				nVar += nL2r*nAllPr;
			} else if (vdomtype==VDOM_R2_ALL_PR) {
				for(uint r1=0;r1<l2r.size();r1++){
					nVar += l2r[r1].size()*l2r[r1].size()*nAllPr; // EXP(i,j,prd)
				}
			} else {
				msgOut(MSG_CRITICAL_ERROR,"Try to calculate the number of variables of a variable of unknow type ("+viter->first+")");
			}
		}

		// caching the initial positions
		for (viter = vars.begin(); viter != vars.end(); ++viter) {
			//string debugs = viter->first;
			int vdomtype = viter->second;
			vector < vector < vector < vector <int> > > > positionsToAdd;


			if(vdomtype==VDOM_PRI_PR){
				for(uint r1=0;r1<l2r.size();r1++){
					vector < vector < vector <int> > > dim1;
					for(uint r2=0;r2<l2r[r1].size();r2++){
						vector < vector <int> > dim2;
						for(uint p=0;p<priPr.size();p++){
							vector <int> dim3;
							dim3.push_back(gix_uncached(viter->first,r1,r2,p));
							dim2.push_back(dim3);
						}
						dim1.push_back(dim2);
					}
					positionsToAdd.push_back(dim1);
				}
			} else if (vdomtype==VDOM_SEC_PR) {
				for(uint r1=0;r1<l2r.size();r1++){
					vector < vector < vector <int> > > dim1;
					for(uint r2=0;r2<l2r[r1].size();r2++){
						vector < vector <int> > dim2;
						for(uint p=0;p<secPr.size();p++){
							vector <int> dim3;
							dim3.push_back(gix_uncached(viter->first,r1,r2,p));
							dim2.push_back(dim3);
						}
						dim1.push_back(dim2);
					}
					positionsToAdd.push_back(dim1);
				}
			} else if (vdomtype==VDOM_ALL_PR) {
				for(uint r1=0;r1<l2r.size();r1++){
					vector < vector < vector <int> > > dim1;
					for(uint r2=0;r2<l2r[r1].size();r2++){
						vector < vector <int> > dim2;
						for(uint p=0;p<allPr.size();p++){
							vector <int> dim3;
							dim3.push_back(gix_uncached(viter->first,r1,r2,p));
							dim2.push_back(dim3);
						}
						dim1.push_back(dim2);
					}
					positionsToAdd.push_back(dim1);
				}
			} else if (vdomtype==VDOM_R2_ALL_PR) {
				for(uint r1=0;r1<l2r.size();r1++){
					vector < vector < vector <int> > > dim1;
					for(uint r2=0;r2<l2r[r1].size();r2++){
						vector < vector <int> > dim2;
						for(uint p=0;p<allPr.size();p++){
							vector <int> dim3;
							for(uint r2To=0;r2To<l2r[r1].size();r2To++){
								dim3.push_back(gix_uncached(viter->first,r1,r2,p,r2To));
							}
							dim2.push_back(dim3);
						}
						dim1.push_back(dim2);
					}
					positionsToAdd.push_back(dim1);
				}

			} else {
				msgOut(MSG_CRITICAL_ERROR,"Try to calculate the number of variables of a variable of unknow type ("+viter->first+")");
			}
			positions.insert(pair<string, vector < vector < vector < vector <int> > > > >(viter->first,  positionsToAdd));
		}





// some debug code

/*int debug;
debug = gip("pc"); cout << "debug: "<<debug<<endl;
debug = gix("pc",0,0,0); cout << "debug: "<<debug<<endl;
debug = gix("pc",0,0,2); cout << "debug: "<<debug<<endl;
debug = gix("pc",0,2,2); cout << "debug: "<<debug<<endl;
debug = gix("pc",0,2,2); cout << "debug: "<<debug<<endl;
debug = gip("pl"); cout << "debug: "<<debug<<endl;
debug = gix("pl",0,2,2); cout << "debug: "<<debug<<endl;
debug = gip("exp"); cout << "debug: "<<debug<<endl;
debug = gix("exp",0,2,2,2); cout << "debug: "<<debug<<endl;
exit(0);
*/

		// calculating the number of constrains..
		nCons = 0; // leave 0
		nEqualityConstrains = 0;
		nLowerEqualZeroConstrains = 0;
		nGreaterEqualZeroConstrains = 0;
		nCons += nL2r*nSecPr; // mkeq2
		nCons += nL2r*nPriPr; // mkeq3
		nCons += nL2r*nPriPr; // mkeq4
		nCons += nL2r*nPriPr; // mkeq5
		nCons += nL2r*nSecPr; // mkeq6
		nCons += nL2r*nSecPr; // mkeq7
		nCons += nL2r*nPriPr; // mkeq8
		nCons += nL2r*nSecPr; // mkeq13
		nCons += nL2r*nPriPr; // mkeq14
		nCons += nL2r*nSecPr; // mkeq23
		nCons += nL2r*nPriPr; // mkeq24
		nEqualityConstrains = nCons;
		nCons += nL2r*nSecPr; // mkeq17
		nCons += nL2r*nPriPr; // mkeq18
		nCons += 1;           // mkeq26
		nLowerEqualZeroConstrains = nCons - nEqualityConstrains;
		nCons += nL2r*nSecPr; // mkeq25
		nGreaterEqualZeroConstrains = nCons - nEqualityConstrains - nLowerEqualZeroConstrains;

		msgOut(MSG_INFO,"The model will work with "+i2s(nVar)+" variables and "+i2s(nCons)+" constrains.");


	} // finish initialisation things to be done only the first year

	previousYear = MTHREAD->SCD->getYear()-1; // this has to be done EVERY years !!

	n = nVar; // 300; // nVar;

	m = nCons; // 70; // nCons;

	std::cout<<"PRE MEMORY ALLOCATION"<<std::endl;
	// in this example the jacobian is dense. Hence, it contains n*m nonzeros
	nnz_jac_g = n*m;

	// the hessian is also dense and has n*n total nonzeros, but we
	// only need the lower left corner (since it is symmetric)
	nnz_h_lag = n*(n-1)/2+n;
	std::cout<<"PRE GENERATE TAPES ALLOCATION"<<std::endl;
	generate_tapes(n, m);
	std::cout<<"POST MEMORY ALLOCATION"<<std::endl;
	// use the C style indexing (0-based)
	index_style = C_STYLE;

	return true;
}

bool
Opt::get_bounds_info(Index n, Number* x_l, Number* x_u, Index m, Number* g_l, Number* g_u){
	// all variables have upper or equal than zero bound
	for (Index i=0; i<n; i++) {
		x_l[i] =    0.;
		x_u[i] =  1e20;
	}

	// Set the bounds for the constraints
	for (Index i=0; i<m; i++) {
		if(i<nEqualityConstrains){
			g_l[i] = 0.;
			g_u[i] = 0.;
		} else if(i<nLowerEqualZeroConstrains){
			g_l[i] = -1e20;
			g_u[i] = 0.;
		} else {
			g_l[i] = 0.;
			g_u[i] = 1e20;
		}
	}

	return true;
}

bool
Opt::get_starting_point(Index n, bool init_x, Number* x, bool init_z, Number* z_L, Number* z_U,
												Index m, bool init_lambda, Number* lambda){

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
		if(vdomtype==VDOM_PRI_PR){
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<priPr.size();p++){
						x[gix(viter->first,r1,r2,p)]= gpd(viter->first,l2r[r1][r2],priPr[p],previousYear);
					}
				}
			}
		} else if (vdomtype==VDOM_SEC_PR) {
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<secPr.size();p++){
						x[gix(viter->first,r1,r2,p)]= gpd(viter->first,l2r[r1][r2],secPr[p],previousYear);
					}
				}
			}
		} else if (vdomtype==VDOM_ALL_PR) {
			for(uint r1=0;r1<l2r.size();r1++){
				for(uint r2=0;r2<l2r[r1].size();r2++){
					for(uint p=0;p<allPr.size();p++){
						x[gix(viter->first,r1,r2,p)]= gpd(viter->first,l2r[r1][r2],allPr[p],previousYear);
					}
				}
			}
		} else if (vdomtype==VDOM_R2_ALL_PR) {
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

template<class T> bool
Opt::eval_obj(Index n, const T *x, T& obj_value){

	double aa, bb, dc0, sigma, pc, a_pr, ct, m;
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
				dc0 = gpd("dc",l2r[r1][r2],secPr[p],firstYear);
				obj_value += aa*pow(x[gix("dc",r1,r2,p)],(sigma+1)/sigma)-aa*pow(0.5*dc0,(sigma+1)/sigma)-x[gix("pc",r1,r2,p)]*x[gix("dc",r1,r2,p)];
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
					obj_value += x[gix("pc",r1,r2,p2)]*a_pr*x[gix("sc",r1,r2,p1)]-bb*a_pr*pow(x[gix("sc",r1,r2,p1)],(sigma+1)/sigma);
				}
			}
			//  // surplus of transport agents..
			//  + sum((i,j,prd), (RVAR('pl',j,prd)-RVAR('pl',i,prd)-CT(i,j,prd))*EXP(i,j,prd))
			for (uint r2To=0;r2To<l2r[r1].size();r2To++){
				for (uint p=0;p<allPr.size();p++){
					ct = gpd("ct",l2r[r1][r2],allPr[p],DATA_NOW,i2s(l2r[r1][r2To]));
					obj_value += (x[gix("pl",r1,r2To,p)]-x[gix("pl",r1,r2,p)]-ct)*x[gix("rt",r1,r2,p,r2To)];
				}
			}
			//  // transformers surplus..
			//  + sum((i,p_tr), (RVAR('pl',i,p_tr)-m(i,p_tr))*(RVAR('sl',i,p_tr))) // attenction it's local. if we include w imports or p expports this have to change
			for (uint p=0;p<secPr.size();p++){
				m = gpd("m",l2r[r1][r2],secPr[p]);
				obj_value += (x[gix("pl",r1,r2,p)]-m)*x[gix("sl",r1,r2,p)];
			}
			//  - sum((i,p_pr), RVAR('pl',i,p_pr)*RVAR('dl',i,p_pr))               // to total and an other equation total=local+abroad should be added
			for (uint p=0;p<priPr.size();p++){
				obj_value -= x[gix("pl",r1,r2,p)]*x[gix("dl",r1,r2,p)];
			}
		} // end of each lev2 regions

	} //end of each r1 regions

	return true;
}

template<class T> bool
Opt::eval_constraints(Index n, const T *x, Index m, T* g){

	double a_pr, a, sigma, ff, gg, q1, p1v, t1, r1v, psi, eta, pworld, ct, k, dispor, mv;
	Index cix = 0;

	// *** equality contrains.. ***
	for (uint r1=0;r1<l2r.size();r1++){
		for (uint r2=0;r2<l2r[r1].size();r2++){

			for (uint p=0;p<secPr.size();p++){
				// mkteq2(i,p_tr)..  RVAR('dl',i,p_tr)+sum(j,EXP(i,j,p_tr))  =e=  RVAR('sl',i,p_tr)+ sum(b,EXP(b,i,p_tr)); // h1
				g[cix] = x[gix("dl",r1,r2,p)]-x[gix("sl",r1,r2,p)];
				for (uint r2To=0;r2To<l2r[r1].size();r2To++){
					g[cix] += x[gix("rt",r1,r2,p,r2To)]-x[gix("rt",r1,r2To,p,r2)];
				}
				cix++;
				// mkteq6(i,p_tr)..  RVAR('dc',i,p_tr)  =e=  GG(i,p_tr)*(RVAR('pc',i,p_tr)**sigma(p_tr)); // eq. 20
				gg = gpd("gg",l2r[r1][r2],secPr[p]);
				sigma = gpd("sigma",l2r[r1][r2],secPr[p]);
				g[cix] = x[gix("dc",r1,r2,p)]-gg*pow(x[gix("pc",r1,r2,p)],sigma);
				cix++;
				// mkteq7(i,p_tr)..  RVAR('da',i,p_tr)/RVAR('dl',i,p_tr)  =e=  ((q1(i,p_tr)*RVAR('pl',i,p_tr))/(p1(i,p_tr)*PT_t(p_tr)))**psi(i,p_tr); // h7 and h3 ?
				q1 = gpd("q1",l2r[r1][r2],secPr[p]);
				p1v = 1-q1;
				psi = gpd("psi",l2r[r1][r2],secPr[p]);
				pworld = gpd("pl", worldCodeLev2,secPr[p]);
				g[cix] = x[gix("da",r1,r2,p)]/x[gix("dl",r1,r2,p)] - pow((q1*x[gix("pl",r1,r2,p)])/(p1v*pworld),psi);
				cix++;
				// mkteq13(i,p_tr).. RVAR('pc',i,p_tr)*RVAR('dc',i,p_tr)  =e=  RVAR('dl',i,p_tr)*RVAR('pl',i,p_tr)+RVAR('da',i,p_tr)*PT_t(p_tr);  // h9
				pworld = gpd("pl", worldCodeLev2,secPr[p]);
				g[cix] = x[gix("pc",r1,r2,p)]*x[gix("dc",r1,r2,p)]-x[gix("dl",r1,r2,p)]*x[gix("pl",r1,r2,p)]-x[gix("da",r1,r2,p)]*pworld;
				cix++;
				 // mkteq23(i,p_tr).. RVAR('dc',i,p_tr)  =e=  (q1(i,p_tr)*(RVAR('da',i,p_tr)**((psi(i,p_tr)-1)/psi(i,p_tr)))+ p1(i,p_tr)*(RVAR('dl',i,p_tr)**((psi(i,p_tr)-1)/psi(i,p_tr))))**(psi(i,p_tr)/(psi(i,p_tr)-1)); // h3
				q1 = gpd("q1",l2r[r1][r2],secPr[p]);
				psi = gpd("psi",l2r[r1][r2],secPr[p]);
				p1v = 1-q1;
				g[cix] = x[gix("dc",r1,r2,p)] -
						pow(
								 q1 * pow(x[gix("da",r1,r2,p)],(psi-1)/psi)
							+ p1v * pow(x[gix("dl",r1,r2,p)],(psi-1)/psi),
							psi/(psi-1)
						);
				cix++;

			} // end secondary products constrains

			for (uint p=0;p<priPr.size();p++){
				// mkteq3(i,p_pr)..  RVAR('dl',i,p_pr)+sum(j,EXP(i,j,p_pr))   =e=  RVAR('sl',i,p_pr)+ sum(b,EXP(b,i,p_pr))+sum(p_pr2, pres(p_pr2,p_pr)* RVAR('sl',i,p_pr2)); // h2
				g[cix] = x[gix("dl",r1,r2,p)]-x[gix("sl",r1,r2,p)];
				for (uint r2To=0;r2To<l2r[r1].size();r2To++){
					g[cix] += x[gix("rt",r1,r2,p,r2To)]-x[gix("rt",r1,r2To,p,r2)];
				}
				for (uint p2=0;p2<priPr.size();p2++){
					a_pr = gpd("a_pr",l2r[r1][r2],priPr[p2],DATA_NOW,priPr[p]);
					g[cix] -= a_pr*x[gix("sl",r1,r2,p2)];
				}
				cix++;
				//mkteq4(i,p_pr)..  RVAR('dl',i,p_pr)  =e=  sum(p_tr, a(p_pr,p_tr)*(RVAR('sl',i,p_tr))); // eq. 13
				g[cix] = x[gix("dl",r1,r2,p)];
				for (uint p2=0;p2<secPr.size();p2++){
					a = gpd("a",l2r[r1][r2],priPr[p],DATA_NOW,secPr[p2]);
					g[cix] -= a*x[gix("sl",r1,r2,p2)];
				}
				cix++;
				// mkteq5(i,p_pr)..  RVAR('sc',i,p_pr)  =e=  FF(i,p_pr)*(RVAR('pc',i,p_pr)**sigma(p_pr)); // eq. 21
				ff = gpd("ff",l2r[r1][r2],priPr[p]);
				sigma = gpd("sigma",l2r[r1][r2],priPr[p]);
				g[cix] = x[gix("sc",r1,r2,p)]-ff*pow(x[gix("pc",r1,r2,p)],sigma);
				cix++;

				 // mkteq8(i,p_pr)..  RVAR('sa',i,p_pr)/RVAR('sl',i,p_pr)  =e=  ((t1(i,p_pr)*RVAR('pl',i,p_pr))/(r1(i,p_pr)*PT_t(p_pr)))**eta(i,p_pr); // h8 and h4 ?
				t1 = gpd("t1",l2r[r1][r2],priPr[p]);
				r1v = 1-t1;
				eta = gpd("eta",l2r[r1][r2],priPr[p]);
				pworld = gpd("pl", worldCodeLev2,priPr[p]);
				g[cix] = x[gix("sa",r1,r2,p)]/x[gix("sl",r1,r2,p)] - pow((t1*x[gix("pl",r1,r2,p)])/(r1v*pworld),eta);
				cix++;
				// mkteq14(i,p_pr).. RVAR('pc',i,p_pr)*RVAR('sc',i,p_pr)  =e=  RVAR('sl',i,p_pr)*RVAR('pl',i,p_pr)+RVAR('sa',i,p_pr)*PT_t(p_pr);  // h10
				pworld = gpd("pl", worldCodeLev2,secPr[p]);
				g[cix] = x[gix("pc",r1,r2,p)]*x[gix("sc",r1,r2,p)]-x[gix("sl",r1,r2,p)]*x[gix("pl",r1,r2,p)]-x[gix("sa",r1,r2,p)]*pworld;
				cix++;
				// mkteq24(i,p_pr).. RVAR('sc',i,p_pr)  =e=  (t1(i,p_pr)*(RVAR('sa',i,p_pr)**((eta(i,p_pr)-1)/eta(i,p_pr)))+ r1(i,p_pr)*(RVAR('sl',i,p_pr)**((eta(i,p_pr)-1)/eta(i,p_pr))))**(eta(i,p_pr)/(eta(i,p_pr)-1)); // h4
				t1 = gpd("t1",l2r[r1][r2],priPr[p]);
				r1v = 1-t1;
				eta = gpd("eta",l2r[r1][r2],priPr[p]);
				g[cix] = x[gix("sc",r1,r2,p)] -
						pow(
								 q1 * pow(x[gix("sa",r1,r2,p)],(eta-1)/eta)
							+ r1v * pow(x[gix("sl",r1,r2,p)],(eta-1)/eta),
							psi/(psi-1)
						);
				cix++;

			} // end primary products constrains

		} // end level2 regions
	} // end lev1 regions


	// *** lower than zero contrains.. ***
	for (uint r1=0;r1<l2r.size();r1++){
		for (uint r2=0;r2<l2r[r1].size();r2++){

			for (uint p=0;p<secPr.size();p++){
				// mkteq17(i,p_tr).. RVAR('sl',i,p_tr)                    =l=  Kt(i,p_tr);     // h16 in the presentation paper
				k = gpd("k",l2r[r1][r2],secPr[p]);
				g[cix] = x[gix("sl",r1,r2,p)]-k;
				cix++;
			} // end secondary products

			for (uint p=0;p<priPr.size();p++){
			// mkteq18(i,p_pr).. RVAR('sa',i,p_pr)+RVAR('sl',i,p_pr)  =l=  dispor(i,p_pr); // total supply lower than the available stock
				dispor = gpd("dispor",l2r[r1][r2],priPr[p]);
				g[cix] = x[gix("sa",r1,r2,p)]+x[gix("sl",r1,r2,p)]-dispor;
				cix++;
			} // end primary products
		} //end level2 regions
	} // end lev1 regions

	//mkteq26        .. sum((i,j,prd), (RVAR('pl',j,prd)-RVAR('pl',i,prd)-CT(i,j,prd))*EXP(i,j,prd)) =l=  0;  // 0				margin on transport
	g[cix] = 0.;
	for (uint r1=0;r1<l2r.size();r1++){
		for (uint r2=0;r2<l2r[r1].size();r2++){
			for (uint r2To=0;r2To<l2r[r1].size();r2To++){
				for (uint p=0;p<allPr.size();p++){
					ct = gpd("ct",l2r[r1][r2],allPr[p],DATA_NOW,i2s(l2r[r1][r2To]));
					g[cix] += (x[gix("pl",r1,r2To,p)]-x[gix("pl",r1,r2,p)]-ct)*x[gix("rt",r1,r2,p,r2To)];
				}
			}
		}
	}
	cix++;



	// *** higher than zero contrains.. ***
	for (uint r1=0;r1<l2r.size();r1++){
		for (uint r2=0;r2<l2r[r1].size();r2++){

			for (uint p=0;p<secPr.size();p++){
				// mkteq25(i,p_tr).. sum(p_pr, a(p_pr,p_tr)*RVAR('pl',i,p_pr))+m(i,p_tr)  =g=  (RVAR('pl',i,p_tr));        // price of raw products + transf cost > trasf product
				mv = gpd("m",l2r[r1][r2],secPr[p]);
				g[cix] = mv - x[gix("pl",r1,r2,p)];
				for (uint p2=0;p2<priPr.size();p2++){
					a = gpd("a",l2r[r1][r2],priPr[p2],DATA_NOW,secPr[p]);
					g[cix] += a * x[gix("pl",r1,r2,p2)];
				}
				cix++ ;
			} // end secondary products


		} //end level2 regions
	} // end lev1 regions


	return true;
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
Opt::eval_jac_g(Index n, const Number* x, bool new_x,Index m, Index nele_jac,
								Index* iRow, Index *jCol, Number* values){
	if (values == NULL) {
		// return the structure of the jacobian,
		// assuming that the Jacobian is dense

		Index idx = 0;
		for(Index i=0; i<m; i++)
			for(Index j=0; j<n; j++)
		{
			iRow[idx] = i;
			jCol[idx++] = j;
		}
 }
	else {
		// return the values of the jacobian of the constraints

		jacobian(tag_g,m,n,x,Jac);

		Index idx = 0;
		for(Index i=0; i<m; i++)
			for(Index j=0; j<n; j++)
			values[idx++] = Jac[i][j];

	}

	return true;
}

bool
Opt::eval_h(Index n, const Number* x, bool new_x, Number obj_factor, Index m, const Number* lambda,
						bool new_lambda, Index nele_hess, Index* iRow, Index* jCol, Number* values){
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
}

void
Opt::finalize_solution(SolverReturn status,
											 Index n, const Number* x, const Number* z_L, const Number* z_U,
											 Index m, const Number* g, const Number* lambda,
											 Number obj_value, const IpoptData* ip_data, IpoptCalculatedQuantities* ip_cq){

	printf("\n\nObjective value\n");
	printf("f(x*) = %e\n", obj_value);

// Memory deallocation for ADOL-C variables

	delete[] x_lam;

	for(Index i=0;i<m;i++)
		delete[] Jac[i];
	delete[] Jac;

	for(Index i=0;i<n+m+1;i++)
		delete[] Hess[i];
	delete[] Hess;
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

int
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

int
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

int
Opt::gix_uncached(const string &varName, int r1Ix, int r2Ix, int prIx, int r2IxTo){

	// attenction, for computational reason we are not checking the call is within vectors limits!!!

	int vdtype = gdt(varName);
	int othCountriesRegions = 0;
	for (uint i=0;i<r1Ix;i++){
		othCountriesRegions += l2r[i].size();
	}

	if(vdtype == VDOM_PRI_PR){
		return gip(varName)+(othCountriesRegions+r2Ix)*nPriPr+prIx;
	} else if (vdtype == VDOM_SEC_PR){
		return gip(varName)+(othCountriesRegions+r2Ix)*nSecPr+prIx;
	} else if (vdtype == VDOM_ALL_PR){
		return gip(varName)+(othCountriesRegions+r2Ix)*nAllPr+prIx;
	} else if (vdtype == VDOM_R2_ALL_PR){
		int othCountrReg = 0;
		for (uint i=0;i<r1Ix;i++){
			othCountrReg +=l2r[i].size()*l2r[i].size();
		}
		// initial position + (other countries region pairs + same country other regions from pair + regions to)* number of all products+product
		return gip(varName)+(othCountrReg+r2Ix*l2r[r1Ix].size()+r2IxTo)*nAllPr+prIx;

	} else {
		msgOut(MSG_CRITICAL_ERROR,"Try to calculate the number of variables of a variable of unknow type ("+varName+")");
		return 0;
	}

}

int
Opt::gix(const string &varName, int r1Ix, int r2Ix, int prIx, int r2IxTo){
	// attenction, for computational reasons we are not checking the call is within vectors limits!!!
	map <string, vector < vector < vector < vector <int> > > > >::const_iterator p;
	p=positions.find(varName);
	if(p != positions.end()) {
		return p->second[r1Ix][r2Ix][prIx][r2IxTo];
	}
	else {
		msgOut(MSG_CRITICAL_ERROR, "Asking the position of a variable ("+varName+") that doesn't exist!");
		return 0;
	}
}


