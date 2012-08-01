#include <cmath>

#include "IpIpoptApplication.hpp"
#include "IpSolveStatistics.hpp"

#include "ModelCore.h"
#include "ModelData.h"
#include "ThreadManager.h"
#include "Opt.h"
#include "Scheduler.h"


ModelCore::ModelCore(ThreadManager* MTHREAD_h){
	MTHREAD = MTHREAD_h;
}

ModelCore::~ModelCore(){

}


void
ModelCore::runInitPeriod(){
	msgOut(MSG_INFO, "Starting init simulation..");
	int currentYear = MTHREAD->SCD->getYear();
	ModelData* MD = MTHREAD->MD;
	int firstYear = MD->getIntSetting("initialYear");
	int secondYear = firstYear+1;
	int thirdYear  = firstYear+2;
	int WL2 = MD->getIntSetting("worldCodeLev2");
	vector <int> regIds2 = MD->getRegionIds(2);
	vector <string> priProducts = MD->getStringVectorSetting("priProducts");
	vector <string> secProducts = MD->getStringVectorSetting("secProducts");
	vector <string> allProducts = priProducts;
	allProducts.insert( allProducts.end(), secProducts.begin(), secProducts.end() );
	vector <string> dClasses = MD->getStringVectorSetting("dClasses");
	vector <string> pDClasses; // production diameter classes: exclude the fist diameter class below 15 cm
	pDClasses.insert(pDClasses.end(), dClasses.begin()+1, dClasses.end() );
	vector <string> fTypes= MD->getForTypeIds();
	vector <vector <int> > l2r = MD->getRegionIds();

	// starting the GAMS code

	for(uint i=0;i<regIds2.size();i++){
		int r2 = regIds2[i];

		//RPAR('pl',i,p_tr,t-1)   =  sum(p_pr, a(p_pr,p_tr)*RPAR('pl',i,p_pr,t-1))+m(i,p_tr);
		for(uint sp=0;sp<secProducts.size();sp++){
			double value = 0;
			for (uint pp=0;pp<priProducts.size();pp++){
				value += gpd("pl",r2,priProducts[pp],secondYear)*
						 gpd("a",r2,priProducts[pp],secondYear,secProducts[sp]);
			}
			value += gpd("m",r2,secProducts[sp],secondYear);
			spd(value,"pl",r2,secProducts[sp],secondYear);
		}
		// RPAR('dl',i,p_pr,t-1)   =  sum(p_tr, a(p_pr,p_tr)*RPAR('sl',i,p_tr,t-1));
		for (uint pp=0;pp<priProducts.size();pp++){
			double value=0;
			for(uint sp=0;sp<secProducts.size();sp++){
				value += gpd("sl",r2,secProducts[sp],secondYear)*
						 gpd("a",r2,priProducts[pp],secondYear, secProducts[sp]);
			}
			spd(value,"dl",r2,priProducts[pp],secondYear,true);
		}
		// RPAR('st',i,prd,t-1)    =  RPAR('sl',i,prd,t-1)+RPAR('sa',i,prd,t-1);
		// RPAR('dt',i,prd,t-1)    =  RPAR('dl',i,prd,t-1)+RPAR('da',i,prd,t-1);
		for (uint ap=0;ap<allProducts.size();ap++){
			double stvalue =   gpd("sl",r2,allProducts[ap],secondYear)
							 + gpd("sa",r2,allProducts[ap],secondYear);
			double dtvalue =   gpd("dl",r2,allProducts[ap],secondYear)
							 + gpd("da",r2,allProducts[ap],secondYear);
			spd(stvalue,"st",r2,allProducts[ap],secondYear,true);
			spd(dtvalue,"dt",r2,allProducts[ap],secondYear,true);
		}

		// q1(i,p_tr)  =  1/(1+((RPAR('dl',i,p_tr,t-1)/RPAR('da',i,p_tr,t-1))**(1/psi(i,p_tr)))*(RPAR('pl',i,p_tr,t-1)/PT(p_tr,t-1)));
		// p1(i,p_tr)              =  1-q1(i,p_tr);
		// RPAR('dc',i,p_tr,t-1)   =  (q1(i,p_tr)*RPAR('da',i,p_tr,t-1)**((psi(i,p_tr)-1)/psi(i,p_tr))+ p1(i,p_tr)*RPAR('dl',i,p_tr,t-1)**((psi(i,p_tr)-1)/psi(i,p_tr)))**(psi(i,p_tr)/(psi(i,p_tr)-1));
		// RPAR('pc',i,p_tr,t-1)   =  (RPAR('da',i,p_tr,t-1)/RPAR('dc',i,p_tr,t-1))*PT(p_tr,t-1)+(RPAR('dl',i,p_tr,t-1)/RPAR('dc',i,p_tr,t-1))*RPAR('pl',i,p_tr,t-1);
		// RPAR('pc',i,p_pr,t-1)   =  (RPAR('sa',i,p_pr,t-1)/RPAR('sc',i,p_pr,t-1))*PT(p_pr,t-1)+(RPAR('sl',i,p_pr,t-1)/RPAR('sc',i,p_pr,t-1))*RPAR('pl',i,p_pr,t-1);
		// RPAR('pw',i,p_tr,t-1)   =  (RPAR('dl',i,p_tr,t-1)*RPAR('pl',i,p_tr,t-1)+RPAR('da',i,p_tr,t-1)*PT(p_tr,t-1))/RPAR('dt',i,p_tr,t-1) ; //changed 20120419
		// K(i,p_tr,t-1)           =  k1(i,p_tr)*RPAR('sl',i,p_tr,t-1);
		for(uint sp=0;sp<secProducts.size();sp++){
			double psi = gpd("psi",r2,secProducts[sp],secondYear);
			double dl  = gpd("dl",r2,secProducts[sp],secondYear);
			double da  = gpd("da",r2,secProducts[sp],secondYear);
			double pl  = gpd("pl",r2,secProducts[sp],secondYear);
			double sl  = gpd("sl",r2,secProducts[sp],secondYear);
			double k1  = gpd("k1",r2,secProducts[sp],secondYear);
			double pWo = gpd("pl",WL2,secProducts[sp],secondYear); // World price (local price for region 99999)


			double q1  = 1/ ( 1+pow(dl/da,1/psi)*(pl/pWo) );
			double p1  = 1-q1;
			double dc  = pow(
							q1*pow(da,(psi-1)/psi) + p1*pow(dl,(psi-1)/psi)
						 ,
							psi/(psi-1)
						 );
			double pc = (da/dc)*pWo
					   +(dl/dc)*pl;
			double pw = (dl*pl+da*pWo)/(dl+da);
			double k = k1*sl;

			spd(q1,"q1",r2,secProducts[sp],firstYear,true);
			spd(p1,"p1",r2,secProducts[sp],firstYear,true);
			spd(dc,"dc",r2,secProducts[sp],secondYear,true);
			spd(pc,"pc",r2,secProducts[sp],secondYear,true);
			spd(pw,"pw",r2,secProducts[sp],secondYear,true);
			spd(k,"k",r2,secProducts[sp],secondYear,true);
		}

		// t1(i,p_pr)              =  1/(1+((RPAR('sl',i,p_pr,t-1)/RPAR('sa',i,p_pr,t-1))**(1/eta(i,p_pr)))*(RPAR('pl',i,p_pr,t-1)/PT(p_pr,t-1)));
		// r1(i,p_pr)              =  1-t1(i,p_pr);
		// RPAR('sc',i,p_pr,t-1)   =  (t1(i,p_pr)*RPAR('sa',i,p_pr,t-1)**((eta(i,p_pr)-1)/eta(i,p_pr))+ r1(i,p_pr)*RPAR('sl',i,p_pr,t-1)**((eta(i,p_pr)-1)/eta(i,p_pr)))**(eta(i,p_pr)/(eta(i,p_pr)-1))
		// RPAR('pc',i,p_pr,t-1)   =  (RPAR('sa',i,p_pr,t-1)/RPAR('sc',i,p_pr,t-1))*PT(p_pr,t-1)+(RPAR('sl',i,p_pr,t-1)/RPAR('sc',i,p_pr,t-1))*RPAR('pl',i,p_pr,t-1);
		// RPAR('pw',i,p_pr,t-1)   =  (RPAR('sl',i,p_pr,t-1)*RPAR('pl',i,p_pr,t-1)+RPAR('sa',i,p_pr,t-1)*PT(p_pr,t-1))/RPAR('st',i,p_pr,t-1) ; //changed 20120419
		for(uint pp=0;pp<priProducts.size();pp++){

			double sl  = gpd("sl",r2,priProducts[pp],secondYear);
			double sa  = gpd("sa",r2,priProducts[pp],secondYear);
			double eta = gpd("eta",r2,priProducts[pp],secondYear);
			double pl  = gpd("pl",r2,priProducts[pp],secondYear);
			double pWo = gpd("pl",WL2,priProducts[pp],secondYear); // World price (local price for region 99999)


			double t1 = 1/ ( 1+(pow(sl/sa,1/eta))*(pl/pWo) );
			double r1 = 1-t1;
			double sc = pow(
						   t1*pow(sa,(eta-1)/eta) + r1*pow(sl,(eta-1)/eta)
						,
						   eta/(eta-1)
						);
			double pc = (sa/sc)*pWo+(sl/sc)*pl;
			double pw = (sl*pl+sa*pWo)/(sl+sa);

			spd(t1,"t1",r2,priProducts[pp],firstYear,true);
			spd(r1,"r1",r2,priProducts[pp],firstYear,true);
			spd(sc,"sc",r2,priProducts[pp],secondYear,true);
			spd(pc,"pc",r2,priProducts[pp],secondYear,true);
			spd(pw,"pw",r2,priProducts[pp],secondYear,true);
		}

		// up to here tested with gams output on 20120628, that's fine !!

		// //////////////////// BIOLOGICAL MODULE ////////////////////////////////////

		// //////////////////// Inventary on t-2 and stock available on t-1 //////////
		// In(i,p_pr,t-1)             =     sum((u,lambda,essence),prov(u,essence,lambda,p_pr)*V(u,i,lambda,essence,t-2));

		for(uint pp=0;pp<priProducts.size();pp++){
			double in = 0;
			for(uint ft=0;ft<fTypes.size();ft++){
				for(uint dc=0;dc<pDClasses.size();dc++){
					double vol = gfd("vol",r2,fTypes[ft],pDClasses[dc],firstYear);
					in += app(priProducts[pp],fTypes[ft],pDClasses[dc])*vol;
				}
			}
			spd(in,"in",r2,priProducts[pp],secondYear,true);
		}
		for(uint j=0;j<fTypes.size();j++){
			string ft = fTypes[j];
				double freeArea = 0;
				for (uint u=0; u<pDClasses.size(); u++){
					string dc = pDClasses[u];
					//  // ////////// harvestry rate of t-1 //////////////////////////////////////////
					// hr(u,i,essence,lambda,t-1) = sum(p_pr, prov(u,essence,lambda,p_pr)*RPAR('st',i,p_pr,t-1)/In(i,p_pr,t-1));
					//hV(u,i,essence,lambda,t-1) = hr(u,i,essence,lambda,t-1) * V(u,i,lambda,essence,t-2) ;
					// ////// volumes after harvesting on t-1 ////////////////////////////////////
					// loop(u$(ord(u)=1),
					// first diameter class, no harvesting and fixed regenaration..
					// V(u,i,lambda,essence,t-1)=(1-1/(tp(u,i,lambda,essence))-mort(u,i,lambda,essence) )*V(u,i,lambda,essence,t-2)
					//                           +Vregen(i,lambda,essence,t-1);
					// );
					// loop(u$(ord(u)>1),
					//    // generic case..
					//    V(u,i,lambda,essence,t-1)=((1-1/(tp(u,i,lambda,essence))
					//                                 -mort(u,i,lambda,essence) - hr(u,i,essence,lambda,t-1))*V(u,i,lambda,essence,t-2)
					//                                 +(1/(tp(u-1,i,lambda,essence)))*beta(u,i,lambda,essence)*V(u-1,i,lambda,essence,t-2));
					//
					// ///// MANAGEMENT MODULE ///////////////////////////////////////////////////
					// calculating the free area after t-1 harvesting - our first year we calculare regenaration area!
					// freeArea(i,essence, lambda)   = sum(u,hv2fa(i,essence,lambda,u)*hr(u,i,essence,lambda,t-1)*V(u,i,lambda,essence,t-2)*100);
					// regArea(i,essence,lambda,t-1) = freeArea(i,essence, lambda);
					// );
					double hr = 0;
					double pastYearVol = gfd("vol",r2,ft,dc,firstYear);
					double vol;
					double tp          = gfd("tp",r2,ft,dc,secondYear);
					double mort        = gfd("mortCoef",r2,ft,dc,secondYear);
					double vReg        = gfd("vReg",r2,ft,"",secondYear);
					double beta        = gfd("betaCoef",r2,ft,dc,secondYear);
					double hv2fa       = gfd("hv2fa",r2,ft,dc,secondYear);

					for(uint pp=0;pp<priProducts.size();pp++){
						double st = gpd("st",r2,priProducts[pp],secondYear);
						double in = gpd("in",r2,priProducts[pp],secondYear);
						hr += app(priProducts[pp],ft,dc)*st/ in;
					}
					double hV = hr*pastYearVol;
					if(u==0){
						vol = (1-1/tp-mort)*pastYearVol+vReg;
						double debug = vol;
					} else {
						// time of passage and volume of smaller diameter class
						double tp_1 = gfd("tp",r2,ft,pDClasses[u-1],secondYear);
						double pastYearVol_1 = gfd("vol",r2,ft,pDClasses[u-1],firstYear);
						vol = (1-1/tp-mort-hr)*pastYearVol+(1/tp_1)*beta*pastYearVol_1;
						double debug = vol;
					}

					freeArea += hv2fa*hr*pastYearVol*100;
					sfd(hr,"hr",r2,ft,dc,secondYear,true);
					sfd(hV,"hV",r2,ft,dc,secondYear,true);
					sfd(vol,"vol",r2,ft,dc,secondYear);
				}
				sfd(freeArea,"regArea",r2,ft,"",secondYear,true);

		}

	} // end for each region in level 2

// initializing the exports to zero quantities
	for(uint r1=0;r1<l2r.size();r1++){
		for(uint r2=0;r2<l2r[r1].size();r2++){
			for(uint p=0;p<allProducts.size();p++){
			 for(uint r2To=0;r2To<l2r[r1].size();r2To++){
					spd(0,"rt",l2r[r1][r2],allProducts[p],secondYear,true,i2s(l2r[r1][r2To]));  // regional trade, it was exp in gams
				}
			}
		}
	}

};

void
ModelCore::runSimulationYear(){

	using namespace Ipopt;

	msgOut(MSG_INFO, "Starting model simulation for year "+i2s(MTHREAD->SCD->getYear())+ "...");
	ModelData* MD = MTHREAD->MD;
	int thisYear     =  MTHREAD->SCD->getYear();
	int previousYear = MTHREAD->SCD->getYear()-1;
	int WL2 = MD->getIntSetting("worldCodeLev2");
	vector <int> regIds2 = MD->getRegionIds(2);
	vector <string> priProducts = MD->getStringVectorSetting("priProducts");
	vector <string> secProducts = MD->getStringVectorSetting("secProducts");
	vector <string> allProducts = priProducts;
	allProducts.insert( allProducts.end(), secProducts.begin(), secProducts.end() );
	vector <string> dClasses = MD->getStringVectorSetting("dClasses");
	vector <string> pDClasses; // production diameter classes: exclude the fist diameter class below 15 cm
	pDClasses.insert(pDClasses.end(), dClasses.begin()+1, dClasses.end() );
	vector <string> fTypes= MD->getForTypeIds();

	// Updating variables
	// notes:
	// - dispo_sup: not actually entering anywhere, forgiving it for now..
	// - dc0: not needed, it is just the first year demand composite

	for(uint i=0;i<regIds2.size();i++){
		int r2 = regIds2[i];
		// In(i,p_pr,t)   =   sum((u,lambda,essence),prov(u,essence,lambda,p_pr)*V(u,i,lambda,essence,t-1));
		for(uint pp=0;pp<priProducts.size();pp++){
			double in = 0;
			for(uint ft=0;ft<fTypes.size();ft++){
				for(uint dc=0;dc<pDClasses.size();dc++){
					double vol = gfd("vol",r2,fTypes[ft],pDClasses[dc],previousYear);
					in += app(priProducts[pp],fTypes[ft],pDClasses[dc])*vol;
				}
			}
			spd(in,"in",r2,priProducts[pp]);
		}
		// K(i,p_tr,t)  =  (1+g1(i,p_tr))*K(i,p_tr,t-1);
		// AA(i,p_tr)   =   (sigma(p_tr)/(sigma(p_tr)+1))*RPAR('pc',i,p_tr,t-1)*(RPAR('dc',i,p_tr,t-1)**(-1/sigma(p_tr)));
		// GG(i,p_tr)   =   RPAR('dc',i,p_tr,t-1)*((RPAR('pc',i,p_tr,t-1))**(-sigma(p_tr))); //alpha
		for(uint sp=0;sp<secProducts.size();sp++){
			double g1    = gpd("g1",r2,secProducts[sp],previousYear);
			double sigma = gpd("sigma",r2,secProducts[sp]);
			double pc_1  = gpd("pc",r2,secProducts[sp],previousYear);
			double dc_1  = gpd("dc",r2,secProducts[sp],previousYear);
			double k_1   = gpd("k",r2,secProducts[sp],previousYear);

			double k  = (1+g1)*k_1;
			double aa = (sigma/(sigma+1))*pc_1*pow(dc_1,-1/sigma);
			double gg = dc_1*pow(pc_1,-sigma); //alpha

			spd(k, "k" ,r2,secProducts[sp]);
			spd(aa,"aa",r2,secProducts[sp],DATA_NOW,true);
			spd(gg,"gg",r2,secProducts[sp],DATA_NOW,true);
		}

		// BB(i,p_pr)   =   (sigma(p_pr)/(sigma(p_pr)+1))*RPAR('pc',i,p_pr,t-1)*(RPAR('sc',i,p_pr,t-1)**(-1/sigma(p_pr)))*(In(i,p_pr,t-1)/In(i,p_pr,t))**(gamma(p_pr)/sigma(p_pr));
		// FF(i,p_pr)   =   RPAR('sc',i,p_pr,t-1)*((RPAR('pc',i,p_pr,t-1))**(-sigma(p_pr)))*(In(i,p_pr,t)/In(i,p_pr,t-1))**(gamma(p_pr)); //chi
		for(uint pp=0;pp<priProducts.size();pp++){
			double gamma = gpd("gamma",r2,priProducts[pp]);
			double sigma = gpd("sigma",r2,priProducts[pp]);
			double pc_1  = gpd("pc",r2,priProducts[pp],previousYear);
			double sc_1  = gpd("sc",r2,priProducts[pp],previousYear);
			double in    = gpd("in",r2,priProducts[pp]);
			double in_1  = gpd("in",r2,priProducts[pp],previousYear);

			double bb = (sigma/(sigma+1))*pc_1*pow(sc_1,-1/sigma)*pow(in_1/in,gamma/sigma);
			double ff = sc_1*pow(pc_1,-sigma)*pow(in/in_1,gamma); //chi

			spd(bb,"bb",r2,priProducts[pp],DATA_NOW,true);
			spd(ff,"ff",r2,priProducts[pp],DATA_NOW,true);
		}

	} // end for each region in level 2 (and updating variables)

	// Create an instance of the IpoptApplication
	SmartPtr<IpoptApplication> app = new IpoptApplication();
	//app->Options()->SetStringValue("linear_solver", "mumps"); // default in ipopt is ma27
	//app->Options()->SetStringValue("hessian_approximation", "limited-memory"); // quasi-newton approximation of the hessian
	app->Options()->SetIntegerValue("mumps_mem_percent", 100);
	app->Options()->SetNumericValue("obj_scaling_factor", -1); // maximisation



	// Initialize the IpoptApplication and process the options
	ApplicationReturnStatus status;
	status = app->Initialize();
	if (status != Solve_Succeeded) {
		printf("\n\n*** Error during initialization!\n");
		return;
	}

	status = app->OptimizeTNLP(MTHREAD->OPT);

	if (status == Solve_Succeeded) {
		// Retrieve some statistics about the solve
		Index iter_count = app->Statistics()->IterationCount();
		printf("\n\n*** The problem solved in %d iterations!\n", iter_count);

		Number final_obj = app->Statistics()->FinalObjective();
		printf("\n\n*** The final value of the objective function is %e.\n", final_obj);
	}

	return;

};
