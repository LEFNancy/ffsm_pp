#include <cmath>
#include <algorithm>

#include "IpIpoptApplication.hpp"
#include "IpSolveStatistics.hpp"

#include "ModelCoreSpatial.h"
#include "ModelData.h"
#include "ThreadManager.h"
#include "Opt.h"
#include "Scheduler.h"
#include "Gis.h"


ModelCoreSpatial::ModelCoreSpatial(ThreadManager *MTHREAD_h){
	MTHREAD = MTHREAD_h;
}

ModelCoreSpatial::~ModelCoreSpatial(){

}

void
ModelCoreSpatial::runInitPeriod(){
	/**
	Some importan notes:
	V (volumes) -> at the end of the year
	In (inventary) -> at the beginning of the year
	Volumes are in Mm^3, Areas in the model in Ha (10000 m^2), in the layers in m^2
	*/
	cacheSettings();             // cashe things like first year, second year, dClasses...
	initializePixelValues();     // compute px volumes vol for 2005
	initMarketModule();          // inside it uses first year, second year
	MTHREAD->SCD->advanceYear(); // 2005->2006
	resetPixelValues();          // swap volumes->lagged_volumes and reset the other pixel vectors
	cachePixelExogenousData();   // compute pixel tp, meta and mort
	computeInventary();          // in=f(vol_t-1)
	computeCumulativeData();     // compute cumTp_exp, vHa_exp
	runBiologicalModule(true);   //
	runManagementModule();
}

void
ModelCoreSpatial::runSimulationYear(){
	resetPixelValues();          // swap volumes->lagged_volumes and reset the other pixel vectors
	cachePixelExogenousData();   // compute pixel tp, meta and mort
	computeInventary();          // in=f(vol_t-1)
	runMarketModule();
	computeCumulativeData();     // compute cumTp_exp, vHa_exp
	cachePixelExogenousData();
	runBiologicalModule(false);

	runManagementModule();
	updateMapAreas();
}

void
ModelCoreSpatial::initMarketModule(){
	msgOut(MSG_INFO, "Starting resource module (init stage)");

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
	} // end of r1 region

}

void
ModelCoreSpatial::runMarketModule(){
	msgOut(MSG_INFO, "Starting market module");
	int thisYear     =  MTHREAD->SCD->getYear();
	int previousYear = MTHREAD->SCD->getYear()-1;
	// *** PRE-OPTIMISATION YEARLY OPERATIONS..
	for(uint i=0;i<regIds2.size();i++){
		int r2 = regIds2[i];
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



	// *** OPTIMISATION....

	// Create an instance of the IpoptApplication
	//Opt *OPTa = new Opt(MTHREAD);
	//SmartPtr<TNLP> OPTa = new Opt(MTHREAD);
	SmartPtr<IpoptApplication> application = new IpoptApplication();
	//application->Options()->SetStringValue("linear_solver", "mumps"); // default in ipopt is ma27
	//application->Options()->SetStringValue("hessian_approximation", "limited-memory"); // quasi-newton approximation of the hessian
	//application->Options()->SetIntegerValue("mumps_mem_percent", 100);
	application->Options()->SetNumericValue("obj_scaling_factor", -1); // maximisation

	// Initialize the IpoptApplication and process the options
	ApplicationReturnStatus status;
	status = application->Initialize();
	if (status != Solve_Succeeded) {
		printf("\n\n*** Error during initialization!\n");
		return;
	}

		msgOut(MSG_INFO,"Running optimisation problem for this year (it may take a few minutes for large models)..");
	status = application->OptimizeTNLP(MTHREAD->OPT);


	// *** POST OPTIMISATION....

	// post-equilibrium variables->parameters assignments..
	// RPAR(type,i,prd,t)   =  RVAR.l(type,i,prd);
	// EX(i,j,prd,t)        =  EXP.l(i,j,prd);
	// ObjT(t)              =  Obj.l ;
	// ==> in Opt::finalize_solution()

	// Retrieve some statistics about the solve
	if (status == Solve_Succeeded) {
				Index iter_count = application->Statistics()->IterationCount();
				Number final_obj = application->Statistics()->FinalObjective();
		printf("\n*** The problem solved in %d iterations!\n", iter_count);
		printf("\n*** The final value of the objective function is %e.\n", final_obj);
		msgOut(MSG_INFO, "The problem solved successfully in "+i2s(iter_count)+" iterations.");
		int icount = iter_count;
		double obj = final_obj;
		MTHREAD->DO->printOptLog(true, icount, obj);
	} else {
				//Number final_obj = application->Statistics()->FinalObjective();
		cout << "***ERROR: MODEL DIDN'T SOLVE FOR THIS YEAR" << endl;
				msgOut(MSG_ERROR, "Model DIDN'T SOLVE for this year");
				// IMPORTANT! Don't place the next two lines above the msgOut() function or it will crash in windows if the user press the stop button
				Index iter_count = application->Statistics()->IterationCount();
				Number final_obj = application->Statistics()->FinalObjective();
				int icount = iter_count;
				double obj = final_obj;
		MTHREAD->DO->printOptLog(false, icount, obj);
	}

	for(uint r2= 0; r2<regIds2.size();r2++){  // you can use r2<=regIds2.size() to try an out-of range memory error that is not detected other than by valgrind (with a message "Invalid read of size 4 in ModelCore::runSimulationYear() in src/ModelCore.cpp:351")
		int regId = regIds2[r2];

		//  // total supply and total demand..
		//  RPAR('st',i,prd,t)   =  RPAR('sl',i,prd,t)+RPAR('sa',i,prd,t);
		//  RPAR('dt',i,prd,t)   =  RPAR('dl',i,prd,t)+RPAR('da',i,prd,t);
		//  // weighted prices.. //changed 20120419
		//  RPAR('pw',i,p_tr,t)   =  (RPAR('dl',i,p_tr,t)*RPAR('pl',i,p_tr,t)+RPAR('da',i,p_tr,t)*PT(p_tr,t))/RPAR('dt',i,p_tr,t) ; //changed 20120419
		//  RPAR('pw',i,p_pr,t)   =  (RPAR('sl',i,p_pr,t)*RPAR('pl',i,p_pr,t)+RPAR('sa',i,p_pr,t)*PT(p_pr,t))/RPAR('st',i,p_pr,t) ; //changed 20120419
		for (uint p=0;p<allProducts.size();p++){
			double st = gpd("sl",regId,allProducts[p])+gpd("sa",regId,allProducts[p]);
			double dt = gpd("dl",regId,allProducts[p])+gpd("da",regId,allProducts[p]);
			spd(st,"st",regId,allProducts[p]);
			spd(dt,"dt",regId,allProducts[p]);
		}
		for (uint p=0;p<secProducts.size();p++){
			double dl = gpd("dl",regId,secProducts[p]);
			double pl = gpd("pl",regId,secProducts[p]);
			double da = gpd("da",regId,secProducts[p]); // bug corrected 20120913
			double pworld = gpd("pl", WL2,secProducts[p]);
			double dt = gpd("dt",regId,secProducts[p]);
			double pw = (dl*pl+da*pworld)/dt;
			spd(pw,"pw",regId,secProducts[p]);
		}
		for (uint p=0;p<priProducts.size();p++){
			double sl = gpd("sl",regId,priProducts[p]);
			double pl = gpd("pl",regId,priProducts[p]);
			double sa = gpd("sa",regId,priProducts[p]);  // bug corrected 20120913
			double pworld = gpd("pl", WL2,priProducts[p]);
			double st = gpd("st",regId,priProducts[p]);
			double pw = (sl*pl+sa*pworld)/st;
			spd(pw,"pw",regId,priProducts[p]);
		}
	} // end of each region
}



void
ModelCoreSpatial::runBiologicalModule(const bool &init){

	msgOut(MSG_INFO, "Starting resource module");
	int thisYear = MTHREAD->SCD->getYear();

	for(uint i=0;i<regIds2.size();i++){
		int r2 = regIds2[i];
		int regId = r2;
		ModelRegion* REG = MTHREAD->MD->getRegion(r2);
		Gis* GIS = MTHREAD->GIS;
		regPx = REG->getMyPixels();
		for (uint p=0;p<regPx.size();p++){
			Pixel* px = regPx[p];
			px->expectedReturns.empty();
			for(uint j=0;j<fTypes.size();j++){
				string ft = fTypes[j];
				vector <double>            hV_byDiam;
				vector < vector <double> > hV_byDiamAndPrd;
				vector <double> hArea_byDc;

				// calculating the regeneration..
				// if we are in a year where the time of passage has not yet been reached
				// for the specific i,e,l then we use the exogenous Vregen, otherwise we
				// calculate it
				//if ( not scen("fxVreg") ,
				//	loop( (i,essence,lambda),
				//		if( ord(t)>=(tp_u1(i,essence,lambda)+2),
				//			Vregen(i,lambda,essence,t)=regArea(i,essence,lambda,t-tp_u1(i,essence,lambda))*volHa_u1(i,essence,lambda)/1000000   ;
				//		);
				//	);
				//);
				int tp_u0 = px->tp.at(j).at(0); // time of passage to reach the first diameter class
				if(regType != "fixed" && (thisYear-secondYear) >= tp_u0 ) { // TODO to be checked
					double pastRegArea = px->getPastRegArea(j,thisYear-tp_u0);
					//double entryVolHa = gfd("entryVolHa",regId,ft,"");
					double vHa = px->vHa.at(j).at(1);
					px->vReg.push_back(pastRegArea*vHa/1000000); // TODO: check the 1000000. Should be ok, as area in ha vol in Mm^3
				} else {
					double regionArea = REG->getValue("forArea_"+ft,OP_SUM);
					double pxArea     = px->getDoubleValue("forArea_"+ft);
					px->vReg.push_back(gfd("entryVolHa",r2,ft,"")*pxArea/regionArea);
				}

				//double harvestedArea = 0;


				for (uint u=0; u<dClasses.size(); u++){
					string dc = dClasses[u];
					double hr =0;
					double pastYearVol = u ? gfd("vol",r2,ft,dc,thisYear-1): 0;
					double pastYearVol_px = px->vol_l.at(j).at(u);
					vector <double> hV_byPrd;

					// harvesting rate & volumes...
					// hr is by region.. no reasons in one pixel the RATE of harvesting will be different than in an other pixel
					//hr(u,i,essence,lambda,t) =    sum(p_pr, prov(u,essence,lambda,p_pr)*RPAR('st',i,p_pr,t)/In(i,p_pr,t));
					//hV(u,i,essence,lambda,t) = hr(u,i,essence,lambda,t) * V(u,i,lambda,essence,t-1);
					//hV_byPrd(u,i,essence,lambda,p_pr,t) =  prov(u,essence,lambda,p_pr)*(RPAR('st',i,p_pr,t)/In(i,p_pr,t))*V(u,i,lambda,essence,t-1);
					for(uint pp=0;pp<priProducts.size();pp++){
						double st = gpd("st",r2,priProducts[pp]);
						double in = gpd("in",r2,priProducts[pp]);
						double hr_pr = app(priProducts[pp],ft,dc)*st/ in;
						hr += hr_pr;
						hV_byPrd.push_back( hr_pr*pastYearVol_px);
					}
					double hV = hr*pastYearVol_px;
					hV_byDiam.push_back(hV);
					hV_byDiamAndPrd.push_back(hV_byPrd);

					// post harvesting remained volumes computation..
					// loop(u$(ord(u)=1),
					//	first diameter class, no harvesting and fixed regenaration..
					//	V(u,i,lambda,essence,t)=(1-1/(tp(u,i,lambda,essence))-mort(u,i,lambda,essence) )*V(u,i,lambda,essence,t-1)
					//	                       +Vregen(i,lambda,essence,t);
					// );
					// loop(u$(ord(u)>1),
					//	generic case..
					//	V(u,i,lambda,essence,t)=((1-1/(tp(u,i,lambda,essence))
					//	                       -mort(u,i,lambda,essence) - hr(u,i,essence,lambda,t))*V(u,i,lambda,essence,t-1)
					//	                       +(1/(tp(u-1,i,lambda,essence)))*beta(u,i,lambda,essence)*V(u-1,i,lambda,essence,t-1));
					double vol;
					double tp          = px->tp.at(j).at(u); //gfd("tp",regId,ft,dc);
					double mort        = px->mort.at(j).at(u); //gfd("mortCoef",regId,ft,dc);
					double vReg        = px->vReg.at(j); //gfd("vReg",regId,ft,""); // Taking it from the memory database as we could be in a fixed vReg scenario and not having calculated it from above!
					double beta        = px->beta.at(j).at(u); //gfd("betaCoef",regId,ft,dc);
					//double hv2fa       = gfd("hv2fa",regId,ft,dc);
					double vHa         = px->vHa.at(j).at(u); //gfd("vHa",regId,ft,dc);
					double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);
					if(u==0){
						vol = 0.0;
					}else if(u==1){
						vol = (1-1/tp-mort)*pastYearVol+vReg;
						double debug = vol;
					} else {
						// time of passage and volume of smaller diameter class
						double tp_1 = px->tp.at(j).at(u-1); //gfd("tp",regId,ft,dClasses[u-1]);
						double pastYearVol_1 = px->vol_l.at(j).at(u-1); //gfd("vol",regId,ft,dClasses[u-1],thisYear-1);
						vol = (1-1/tp-mort-hr)*pastYearVol+(1/tp_1)*beta*pastYearVol_1;
						double debug = vol;
					}
					hArea_byDc.push_back(finalHarvestFlag*1000000*hr*pastYearVol_px/vHa); // volumes are in Mm^3, area in ha, vHa in m^3/ha

					//double debug = hv2fa*hr*pastYearVol*100;
					//cout << "regId|ft|dc| debug | freeArea: " << r2 << "|"<<ft<<"|"<<dc<<"| "<< debug << " | " << freeArea_byU << endl;

					//sfd(hr,"hr",regId,ft,dc);
					//sfd(hV,"hV",regId,ft,dc);
					//sfd(vol,"vol",regId,ft,dc);

					//sfd(freeArea_byU,"harvestedArea",regId,ft,dc,DATA_NOW,true);
				} // end foreach diameter classes
				px->hVol.push_back(hV_byDiam);
				px->hVol_byPrd.push_back(hV_byDiamAndPrd);
				px->hArea.push_back(hArea_byDc);
			} // end of each forest type
		} // end of each pixel
	} // end of each region

}



void
ModelCoreSpatial::runManagementModule(){
	msgOut(MSG_INFO, "Starting management module");


	// Post optimisation management mobule..
	for(uint i=0;i<regIds2.size();i++){
		int r2 = regIds2[i];
		int regId = r2;
		ModelRegion* REG = MTHREAD->MD->getRegion(r2);
		Gis* GIS = MTHREAD->GIS;
		regPx = REG->getMyPixels();
		for (uint p=0;p<regPx.size();p++){
			Pixel* px = regPx[p];
			px->expectedReturns.empty();

			// CALCULATING the expected returns for each forest type...
			for(uint j=0;j<fTypes.size();j++){
				string ft = fTypes[j];

		//if(regType != "fixed" && regType != "fromHrLevel"){
		// 20120910, Antonello: changed.. calculating the expected returns also for fixed and fromHrLevel regeneration (then not used but gives indication)
			// calculating the expected returns..
			//	loop ( (u,i,essence,lambda,p_pr),
			//		if (sum(u2, hV(u2,i,essence,lambda,t))= 0,
			//			expRetPondCoef(u,i,essence,lambda,p_pr) = 0;
			//		else
			//			expRetPondCoef(u,i,essence,lambda,p_pr) = hV_byPrd(u,i,essence,lambda,p_pr,t)/ sum(u2, hV(u2,i,essence,lambda,t));
			//		);
			//	);
			//	expReturns(i,essence,lambda) = sum( (u,p_pr),
			//	          RPAR("pl",i,p_pr,t)*hv2fa(i,essence,lambda,u)*(1/df_byFT(u,i,lambda,essence))*        // df_byFT(u,i,lambda,essence)
			//	          expRetPondCoef(u,i,essence,lambda,p_pr)
			//	          );
			double hV_byFT = 0.; // gfd("hV",regId,ft,DIAM_PROD); // it must include only final harvested products in order to act as weightering agent
			double expReturns = 0;

			for (uint u=0; u<dClasses.size(); u++){
				string dc = dClasses[u];
				double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);
				hV_byFT += finalHarvestFlag * px->hVol.at(j).at(u);
			}

			if(hV_byFT==0.){ // nothing has been harvested in this pixel for this specific forest type. Let's calculate the combination product/diameter class with the highest expected return
				for (uint u=0; u<dClasses.size(); u++){
					string dc = dClasses[u];
					double vHa         = px->vHa_exp.at(j).at(u);
					double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);
					double cumTp_u = px->cumTp_exp.at(j).at(u);
					for (uint pp=0;pp<priProducts.size();pp++){
						double pw    = gpd("pw",regId,priProducts[pp]);
						double pl    = gpd("pl",regId,priProducts[pp]);
						double pwor    = gpd("pl",99999,priProducts[pp]);
						double raw_amount = finalHarvestFlag*pw*vHa;
						double anualised_amount =  MD->calculateAnnualisedEquivalent(raw_amount,cumTp_u);
						if (anualised_amount>expReturns) {expReturns=anualised_amount;}
					}
				}
			} else { // some harvested from this pixel we weight each products/diameter class combination expected returns with the actual harvested
				for (uint u=0; u<dClasses.size(); u++){
					//vector <double> expReturnsPondCoef_byPrd;
					string dc = dClasses[u];
					double vHa         = px->vHa_exp.at(j).at(u);
					double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);
					double cumTp_u = px->cumTp_exp.at(j).at(u);
					for (uint pp=0;pp<priProducts.size();pp++){
						double pw    = gpd("pw",regId,priProducts[pp]);
						double pl    = gpd("pl",regId,priProducts[pp]);
						double pwor    = gpd("pl",99999,priProducts[pp]);
						double hVol_byUPp = px->hVol_byPrd.at(j).at(u).at(pp); // harvested volumes for this product, diameter class on this region and forest type
						//double raw_amount_old = pw*hv2fa*    hVol_byUPp/hV_byFT; // old and wrong. it was in €/m^4
						double raw_amount = finalHarvestFlag*pw*vHa* hVol_byUPp/hV_byFT; // now in €/ha if there is also thining volumes in hV_byFT, I underestimate expected returns !! T.O.D.O change it !! DONE, hV_byFT now excludes thinning volumes
						/// see @ModelData::calculateAnnualisedEquivalent
						double anualised_amount =  MD->calculateAnnualisedEquivalent(raw_amount,cumTp_u); //comTp is on diamClasses, u here is on pDiamClasses
						//cout << "reg|ft|dc|prd|raw amount|ann.amount|tp|hV|hVTot|pw|pl|pW|vHa|fHFlag;";
						//cout << regId <<";"<< ft <<";"<< dc <<";" << priProducts[pp] <<";" << raw_amount <<";"<< anualised_amount<<";";
						//cout << cumTp_u <<";"<< hVol_byUPp << ";" << hV_byFT << ";" << pw << ";" << pl << ";" << pwor << ";" << vHa << ";" << finalHarvestFlag << endl;
						expReturns += anualised_amount;
					}
					//expReturnsPondCoef.push_back(expReturnsPondCoef_byPrd);
				}
			} // end of hV_byFT!=0
			//double debug = expReturns;
			//sfd(expReturns,"expReturns",regId, ft,"",DATA_NOW,true);
			px->expectedReturns.push_back(expReturns);
			//cachedExpectedReturnsByFt.push_back(expReturns);
		//}

	} // end foreach forest type


	// calculating freeArea at the end of the year and choosing the new regeneration area..
	//freeArea(i,essence,lambda) = sum(u, hv2fa(i,essence,lambda,u)*hr(u,i,essence,lambda,t)*V(u,i,lambda,essence,t-1)*100);
	//if(scen("endVreg") ,
	//	regArea(i,essence,lambda,t) = freeArea(i,essence, lambda);   // here we could introduce in/out area from other land usages
	//else
	//	loop (i,
	//		loop( (essence,lambda),
	//			if ( expReturns(i,essence,lambda) = smax( (essence2,lambda2),expReturns(i,essence2,lambda2) ),
	//				regArea (i,essence,lambda,t) =  sum( (essence2, lambda2), freeArea(i,essence2, lambda2) ) * mr;
	//			);
	//		);
	//		regArea(i,essence,lambda,t) = freeArea(i,essence, lambda)*(1-mr);   // here we could introduce in/out area from other land usages
	//	);
	double totalHarvestedArea = vSum(px->hArea); //gfd("harvestedArea",regId,FT_ALL,DIAM_ALL);
	double maxExpReturns = *( max_element( px->expectedReturns.begin(), px->expectedReturns.end() ) );
	bool foundMaxExpReturns = false;

	vector <double> thisYearRegAreas;
	for(uint j=0;j<fTypes.size();j++){
		string ft = fTypes[j];
		double harvestedAreaForThisFT = vSum(px->hArea.at(j)); // gfd("harvestedArea",regId,ft,DIAM_ALL);
		if(regType == "fixed" || regType == "fromHrLevel"){
			// regeneration area is the harvested area..
			double harvestedArea = harvestedAreaForThisFT;
			thisYearRegAreas.push_back(harvestedArea);
			//sfd(harvestedArea,"regArea",regId,ft,"");
		} else {
			// regeneration area is a mix between harvested area and the harvested area of te most profitting forest type..
			double regArea = 0.;
			if (!foundMaxExpReturns && px->expectedReturns[j] == maxExpReturns){
				// I use the foundMaxExpReturns for the unlucky event that two forest types have the same expected return to avoid overcounting of the area
				regArea += totalHarvestedArea*mr;
				foundMaxExpReturns = true;
			}
			regArea +=  harvestedAreaForThisFT*(1-mr);
			//sfd(regArea,"regArea",regId,ft,"");
			thisYearRegAreas.push_back(regArea);
		}
	} // end for each forest type
	//double totalRegArea = gfd("regArea",regId,FT_ALL,DIAM_ALL);
			px->regArea.insert(pair <int, vector<double> > (MTHREAD->SCD->getYear(), thisYearRegAreas));
		} // end of each pixel
	} // end of each region
}


void
ModelCoreSpatial::cacheSettings(){
	MD = MTHREAD->MD;
	firstYear = MD->getIntSetting("initialYear");
	secondYear = firstYear+1;
	thirdYear  = firstYear+2;
	WL2 = MD->getIntSetting("worldCodeLev2");
	regIds2 = MD->getRegionIds(2);
	priProducts = MD->getStringVectorSetting("priProducts");
	secProducts = MD->getStringVectorSetting("secProducts");
	allProducts = priProducts;
	allProducts.insert( allProducts.end(), secProducts.begin(), secProducts.end() );
	dClasses = MD->getStringVectorSetting("dClasses");
	pDClasses; // production diameter classes: exclude the fist diameter class below 15 cm
	pDClasses.insert(pDClasses.end(), dClasses.begin()+1, dClasses.end() );
	fTypes= MD->getForTypeIds();
	l2r = MD->getRegionIds();
	regType = MTHREAD->MD->getStringSetting("regType"); // how the regeneration should be computed (exogenous, from hr, from allocation choises)
	expType = MD->getDoubleSetting("expType");
	if((expType<0 || expType>1) && expType != -1){
		msgOut(MSG_CRITICAL_ERROR, "expType parameter must be between 1 (expectations) and 0 (adaptative) or -1 (fixed).");
	}
	mr = MD->getDoubleSetting("mr");
}

void
ModelCoreSpatial::initializePixelValues(){
	msgOut(MSG_INFO, "Starting initializing pixel-level values");

	// pxVol = regVol * pxArea/regForArea
	// this function can be done only at the beginning of the model, as it assume that the distribution of volumes by diameter class in the pixels within a certain region is homogeneous, but as the model progress along the time dimension this is no longer true.
	if(!MD->getBoolSetting("usePixelData")) return;
	for(uint i=0;i<regIds2.size();i++){
		ModelRegion* reg = MD->getRegion(regIds2[i]);
		vector <Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regIds2[i]);
		for (uint j=0;j<rpx.size();j++){
			rpx[j]->vol.clear();
			for(uint y=0;y<fTypes.size();y++){
				vector <double> vol_byu;
				double regForArea = reg->getValue("forArea_"+fTypes[y]);
				for (uint z=0;z<dClasses.size();z++){
					double regVol;
					regVol = z ? gfd("vol",regIds2[i],fTypes[y],dClasses[z],firstYear) : 0 ; // if z=0-> regVol= gfd(), otherwise regVol=0;
					double pxArea = rpx[j]->getDoubleValue("forArea_"+fTypes[y]);
					double pxVol = regVol * pxArea/regForArea;
					//rpx[j]->changeValue(pxVol,"vol",fTypes[y],dClasses[z],firstYear);
					vol_byu.push_back(pxVol);
				}
				rpx[j]->vol.push_back(vol_byu);
			}
		}
	}
}

void
ModelCoreSpatial::computeCumulativeData(){

	msgOut(MSG_INFO, "Starting computing some cumulative values");
	int thisYear     =  MTHREAD->SCD->getYear();

	for(uint r2= 0; r2<regIds2.size();r2++){
		int regId = regIds2[r2];
			regPx = MTHREAD->MD->getRegion(regId)->getMyPixels();

		for (uint p=0;p<regPx.size();p++){
			Pixel* px = regPx[p];
			px->cumTp_exp.clear();
			px->vHa_exp.clear();
			px->vHa.clear();

			for(uint j=0;j<fTypes.size();j++){
				string ft = fTypes[j];

				double tp_multiplier_now       = px->getMultiplier("tp_multiplier",ft,DATA_NOW);
				double tp_multiplier_t0        = px->getMultiplier("tp_multiplier",ft,firstYear);
				double mortCoef_multiplier_now = px->getMultiplier("mortCoef_multiplier",ft,DATA_NOW);
				double mortCoef_multiplier_t0  = px->getMultiplier("mortCoef_multiplier",ft,firstYear);
				double betaCoef_multiplier_now = px->getMultiplier("betaCoef_multiplier",ft,DATA_NOW);
				double betaCoef_multiplier_t0  = px->getMultiplier("betaCoef_multiplier",ft,firstYear);

				// calculating the cumulative time of passage and the (cumulativelly generated) vHa for each diameter class (depending on forest owners diam growth expectations)
				//loop(u$(ord(u)=1),
				//   cumTp(u,i,lambda,essence) = tp_u1(i,essence,lambda);
				//);
				//loop(u$(ord(u)>1),
				//   cumTp(u,i,lambda,essence) = cumTp(u-1,i,lambda,essence)+tp(u-1,i,lambda,essence);
				//);
				////ceil(x) DNLP returns the smallest integer number greater than or equal to x
				//loop( (u,i,lambda,essence),
				//  cumTp(u,i,lambda,essence) =   ceil(cumTp(u,i,lambda,essence));
				//);
				vector <double> cumTp_exp_temp; //cumulative time of passage to REACH a diameter class (tp is to LEAVE to the next one)
				vector <double> vHa_exp_temp; // volume at hectar by each diameter class [m^3/ha]
				vector <double> vHa_temp;
				MD->setErrorLevel(MSG_NO_MSG); // as otherwise on 2007 otherwise sfd() will complain that is filling multiple years (2006 and 2007)
				for (uint u=0; u<dClasses.size(); u++){
					string dc = dClasses[u];
					double cumTp_u, cumTp_u_noExp, cumTp_u_fullExp;
					double vHa_u, vHa_u_noExp, vHa_u_fullExp, vHa_u_real, beta, beta_noExp, beta_fullExp, mort, mort_noExp, mort_fullExp;

					// only cumTp is depending for the expectations, as it is what it is used by owner to calculate return of investments.
					// the tp, beta and mort coefficients instead are the "real" ones as predicted by scientist for that specific time

					if(u==0) {
						cumTp_u = 0.;
						vHa_u   = 0.;
						//sfd(cumTp_u,"cumTp",regId,ft,dc,DATA_NOW,true);
						//px->changeValue(cumTp_u,"cumTp",ft,dc,thisYear);

						//sfd(vHa_u,"vHa",regId,ft,dc,DATA_NOW,true);
						cumTp_exp_temp.push_back(cumTp_u);
						vHa_exp_temp.push_back(vHa_u);
						vHa_temp.push_back(vHa_u);
					} else {
						/**
						param expType Specify how the forest owners (those that make the investments) behave will be the time of passage in the future in order to calculate the cumulative time of passage in turn used to discount future revenues.
						Will forest owners behave adaptively believing the time of passage between diameter classes will be like the observed one at time they make decision (0) or they will have full expectations believing forecasts (1) or something in the middle ?
						For compatibility with the GAMS code, a -1 value means using initial simulation tp values (fixed cumTp)."
						*/
						if (expType == -1){
							cumTp_u = cumTp_exp_temp[u-1]+gfd("tp",regId,ft,dClasses[u-1],firstYear)*tp_multiplier_t0;
							if(u==1) {
								vHa_u = gfd("entryVolHa",regId,ft,"",firstYear);
								vHa_u_real = vHa_u;
							} else {
								mort = gfd("mortCoef",regId,ft,dClasses[u-1], firstYear)*mortCoef_multiplier_t0; // mortality rate of previous diameter class
								beta = gfd("betaCoef",regId,ft,dc, firstYear)*betaCoef_multiplier_t0;
								vHa_u = vHa_exp_temp[u-1]*beta*(1-mort);

								// bloody hell, in order to calculate the real vHa, indipendently from expectations, I need to calculate a lot of parameters tacking them at the real times.
								double tp_multiplier_dynamic = px->getMultiplier("tp_multiplier",ft,thisYear+ceil(cumTp_exp_temp[u-1]));
								double mortCoef_multiplier_dynamic = px->getMultiplier("mortCoef_multiplier",ft,thisYear+ceil(cumTp_exp_temp[u-1]));
								double betaCoef_multiplier_dynamic = px->getMultiplier("betaCoef_multiplier",ft,thisYear+ceil(cumTp_u));
								cumTp_u_fullExp = cumTp_exp_temp[u-1]+gfd("tp",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_exp_temp[u-1]))*tp_multiplier_dynamic ;
								mort_fullExp = gfd("mortCoef",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_exp_temp[u-1]))*mortCoef_multiplier_dynamic; // mortality of the previous diameter class
								beta_fullExp = gfd("betaCoef",regId,ft,dc, thisYear+ceil(cumTp_u))*betaCoef_multiplier_dynamic;
								vHa_u_real = vHa_temp[u-1]*beta_fullExp*(1-mort_fullExp);

							}
							//sfd(cumTp_u,"cumTp",regId,ft,dc,DATA_NOW,true);
							//px->changeValue(cumTp_u,"cumTp",ft,dc,thisYear);

							//sfd(vHa_u,"vHa",regId,ft,dc,DATA_NOW,true);
							cumTp_exp_temp.push_back(cumTp_u);
							vHa_exp_temp.push_back(vHa_u_real);
							vHa_temp.push_back(vHa_u_real);
						} else {
							cumTp_u_noExp = cumTp_exp_temp[u-1]+gfd("tp",regId,ft,dClasses[u-1])*tp_multiplier_now;
							double tp_multiplier_dynamic = px->getMultiplier("tp_multiplier",ft,thisYear+ceil(cumTp_exp_temp[u-1]));
							cumTp_u_fullExp = cumTp_exp_temp[u-1]+gfd("tp",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_exp_temp[u-1]))*tp_multiplier_dynamic ; // it adds to the time of passage to reach the previous diameter class the time of passage that there should be to reach this diameter class in the year where the previous diameter class will be reached
							cumTp_u = cumTp_u_fullExp*expType+cumTp_u_noExp*(1-expType);
							if(u==1) {
								vHa_u_noExp   = gfd("entryVolHa",regId,ft,"",DATA_NOW);
								vHa_u_fullExp = gfd("entryVolHa",regId,ft,"",thisYear+ceil(cumTp_u));
								vHa_u = vHa_u_fullExp*expType+vHa_u_noExp*(1-expType);
								vHa_u_real = vHa_u_fullExp;
							} else {
								mort_noExp = gfd("mortCoef",regId,ft,dClasses[u-1], DATA_NOW)*mortCoef_multiplier_now;
								double mortCoef_multiplier_dynamic = px->getMultiplier("mortCoef_multiplier",ft,thisYear+ceil(cumTp_exp_temp[u-1]));
								mort_fullExp = gfd("mortCoef",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_exp_temp[u-1]))*mortCoef_multiplier_dynamic; // mortality of the previous diameter class
								beta_noExp   = gfd("betaCoef",regId,ft,dc, DATA_NOW)*betaCoef_multiplier_now;
								double betaCoef_multiplier_dynamic = px->getMultiplier("betaCoef_multiplier",ft,thisYear+ceil(cumTp_u));
								beta_fullExp = gfd("betaCoef",regId,ft,dc, thisYear+ceil(cumTp_u))*betaCoef_multiplier_dynamic;
								mort = mort_fullExp*expType+mort_noExp*(1-expType);
								beta = beta_fullExp*expType+beta_noExp*(1-expType);
								vHa_u = vHa_exp_temp[u-1]*beta*(1-mort);
								vHa_u_real = vHa_temp[u-1]*beta_fullExp*(1-mort_fullExp);
							}
							//sfd(cumTp_u,"cumTp",regId,ft,dc,DATA_NOW,true);
							//px->changeValue(cumTp_u,"cumTp",ft,dc,thisYear);
							//sfd(vHa_u,  "vHa",regId,ft,dc,DATA_NOW,true);
							cumTp_exp_temp.push_back(cumTp_u);
							vHa_exp_temp.push_back(vHa_u);
							vHa_temp.push_back(vHa_u_real);
						}
					}
					// debug stuff on vHa
					//double vHa_new = gfd("vHa",regId,ft,dc,DATA_NOW);
					//double hv2fa_old = gfd("hv2fa",regId,ft,dc,DATA_NOW);
					//cout << "Reg|Ft|dc|vHa (new)|1/hv2fa (old):   " << regId << " | " << ft;
					//cout << " | " << dc << " | " << vHa_new << " | " << 1/hv2fa_old  << endl;

				} // end of each diam class
				px->cumTp_exp.push_back(cumTp_exp_temp);
				px->vHa_exp.push_back(vHa_exp_temp);
				px->vHa.push_back(vHa_temp);

			} // end of each ft
		} // end of each pixel
	} // end of each region
	MD->setErrorLevel(MSG_ERROR);
}

void
ModelCoreSpatial::resetPixelValues(){
	msgOut(MSG_INFO, "Starting resetting pixel level values");
	for(uint r2= 0; r2<regIds2.size();r2++){
		int regId = regIds2[r2];
		regPx = MTHREAD->MD->getRegion(regId)->getMyPixels();
		for (uint p=0;p<regPx.size();p++){
			Pixel* px = regPx[p];
			px->swap(VAR_VOL);

			px->vol.empty(); // by ft,dc
			px->area.empty(); // by ft,dc
			px->hArea.empty(); // by ft, dc
			//px->regArea.empty(); // by year, ft NO, this one is a map, it doesn't need to be changed
			px->hVol.empty(); // by ft, dc
			px->hVol_byPrd.empty(); // by ft, dc, pp
			px->in.empty(); // by pp
			px->hr.empty(); // by pp
			px->vReg.empty(); // by ft
			px->expectedReturns.empty(); // by ft

			px->beta.empty();
			px->mort.empty();
			px->tp.empty();
			px->cumTp_exp.empty();
			px->vHa_exp.empty();
			px->vHa.empty();
		}
	}
}

void
ModelCoreSpatial::cachePixelExogenousData(){

	msgOut(MSG_INFO, "Starting cashing on pixel spatial-level exogenous data");
	for(uint r2= 0; r2<regIds2.size();r2++){
		int regId = regIds2[r2];
		regPx = MTHREAD->MD->getRegion(regId)->getMyPixels();
		for (uint p=0;p<regPx.size();p++){
			Pixel* px = regPx[p];
			px->tp.clear();
			px->beta.clear();
			px->mort.clear();

			for(uint j=0;j<fTypes.size();j++){
				string ft = fTypes[j];
				vector <double> tp_byu;
				vector <double> beta_byu;
				vector <double> mort_byu;

				double tp_multiplier_now       = px->getMultiplier("tp_multiplier",ft,DATA_NOW);
				double mortCoef_multiplier_now = px->getMultiplier("mortCoef_multiplier",ft,DATA_NOW);
				double betaCoef_multiplier_now = px->getMultiplier("betaCoef_multiplier",ft,DATA_NOW);

				for (uint u=0; u<dClasses.size(); u++){
					string dc = dClasses[u];
					double tp, beta_real, mort_real;
					tp        = gfd("tp",regId,ft,dClasses[u],DATA_NOW)*tp_multiplier_now;
					beta_real = u?gfd("betaCoef",regId,ft,dClasses[u],DATA_NOW)*betaCoef_multiplier_now:0;
					mort_real = u?gfd("mortCoef",regId,ft,dClasses[u],DATA_NOW)*mortCoef_multiplier_now:0;
					tp_byu.push_back(tp);
					beta_byu.push_back(beta_real);
					mort_byu.push_back(mort_real);
				} // end of each tp
				px->tp.push_back(tp_byu);
				px->beta.push_back(beta_byu);
				px->mort.push_back(mort_byu);
			} // end of each ft
		} // end of each pixel
	} // end of each region
}

void
ModelCoreSpatial::computeInventary(){ // in=f(vol_t-1)
	msgOut(MSG_INFO, "Starting computing inventary available for this year");
	for(uint i=0;i<regIds2.size();i++){
		int r2 = regIds2[i];
		ModelRegion* REG = MTHREAD->MD->getRegion(r2);
		Gis* GIS = MTHREAD->GIS;
		regPx = REG->getMyPixels();
		vector <double> in_reg(priProducts.size(),0); // should have ceated a vector of size priProducts.size(), all filled with zeros
		for (uint p=0;p<regPx.size();p++){
			Pixel* px = regPx[p];
			px->in.clear();
			for(uint pp=0;pp<priProducts.size();pp++){
				double in = 0;
				for(uint ft=0;ft<fTypes.size();ft++){
					for(uint dc=0;dc<dClasses.size();dc++){
						in += app(priProducts[pp],fTypes[ft],dClasses[dc])*px->vol_l.at(ft).at(dc);
					}
				}
				px->in.push_back(in);
				in_reg.at(pp) += in;
			} // end of each priProduct
		} // end each pixel
		for(uint pp=0;pp<priProducts.size();pp++){
			spd(in_reg.at(pp),"in",r2,priProducts[pp],DATA_NOW,true);
		}
	} // end each region
}

void
ModelCoreSpatial::updateMapAreas(){
	msgOut(MSG_INFO, "Updating map areas..");
}

