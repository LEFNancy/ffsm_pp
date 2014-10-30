#include <cmath>
#include <algorithm>

#include "IpIpoptApplication.hpp"
#include "IpSolveStatistics.hpp"

#include "ModelCore.h"
#include "ModelCoreSpatial.h"
#include "ModelData.h"
#include "ThreadManager.h"
#include "Opt.h"
#include "Scheduler.h"
#include "Gis.h"


ModelCore::ModelCore(ThreadManager* MTHREAD_h){
  MTHREAD = MTHREAD_h;
}

ModelCore::~ModelCore(){

}


/**
IMPORTANT NOTE: Volumes in Mm^3, Areas in the model in Ha (10000 m^2), in the layers in m^2
*/
void
ModelCore::runInitPeriod(){
  /**
  Some importan notes:
  V (volumes) -> at the end of the year
  In (inventary) -> at the beginning of the year
  Volumes are in Mm^3, Areas in the model in Ha (10000 m^2), in the layers in m^2
  */
  cacheSettings();             // cashe things like first year, second year, dClasses...
  initMarketModule();          // inside it uses first year, second year
  MTHREAD->DO->print();
  MTHREAD->SCD->advanceYear(); // 2005->2006
  computeInventary();          // in=f(vol_t-1)
  computeCumulativeData();     // compute cumTp_exp, vHa_exp, vHa
  runBiologicalModule();       //
  runManagementModule();
  updateMapAreas();            // update the forArea_{ft} layer on each pixel as old value-hArea+regArea
  MTHREAD->DO->print();
}

void
ModelCore::runSimulationYear(){
  int thisYear = MTHREAD->SCD->getYear();
  computeInventary();          // in=f(vol_t-1)
  runMarketModule();
  computeCumulativeData();     // compute cumTp_exp, vHa_exp
  runBiologicalModule();

    /*double sl = gpd("sl",11041,'softWRoundW');
    double pl = gpd("pl",11041,'softWRoundW');
    double sa = gpd("sa",11041,'softWRoundW');
    double pworld = gpd("pl", WL2,'softWRoundW');
    double st = gpd("st",11041,'softWRoundW');
    double pw = (sl*pl+sa*pworld)/st;
    cout << thisYear <<
    */

  runManagementModule();
  updateMapAreas();
  MTHREAD->DO->print();
}


void
ModelCore::initMarketModule(){
  msgOut(MSG_INFO, "Starting market module (init stage)..");
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
  } // end of each region


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
ModelCore::runMarketModule(){

  // *** PRE-OPTIMISATION YEARLY OPERATIONS..

  // Updating variables
  // notes:
  // - dispo_sup: not actually entering anywhere, forgiving it for now..
  // - dc0: not needed, it is just the first year demand composite
  int thisYear = MTHREAD->SCD->getYear();
  int previousYear = thisYear-1;

  for(uint i=0;i<regIds2.size();i++){
    int r2 = regIds2[i];

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

  // *** OPTIMISATION....

  // Create an instance of the IpoptApplication
  //Opt *OPTa = new Opt(MTHREAD);
  //SmartPtr<TNLP> OPTa = new Opt(MTHREAD);
  //int initialOptYear = MTHREAD->MD->getIntSetting("initialOptYear");
  SmartPtr<IpoptApplication> application = new IpoptApplication();
  //if(thisYear == initialOptYear){
    //application = new IpoptApplication();
  //} else {
  //  application->Options()->SetStringValue("warm_start_init_point", "yes");
  //}
  string linearSolver = MTHREAD->MD->getStringSetting("linearSolver");
  application->Options()->SetStringValue("linear_solver", linearSolver); // default in ipopt is ma27
  //application->Options()->SetStringValue("hessian_approximation", "limited-memory"); // quasi-newton approximation of the hessian
  //application->Options()->SetIntegerValue("mumps_mem_percent", 100);
  application->Options()->SetNumericValue("obj_scaling_factor", -1); // maximisation
  application->Options()->SetNumericValue("max_cpu_time", 1800); // max 1/2 hour to find the optimus for one single year
  application->Options()->SetStringValue("check_derivatives_for_naninf", "yes"); // detect error but may crash the application.. TO.DO catch this error!
  //application->Options()->SetStringValue("nlp_scaling_method", "equilibration-based"); // much worster
  // Initialize the IpoptApplication and process the options
  ApplicationReturnStatus status;
  status = application->Initialize();
  if (status != Solve_Succeeded) {
    printf("\n\n*** Error during initialization!\n");
    msgOut(MSG_INFO,"Error during initialization! Do you have the solver compiled for the specified linear solver?");
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
        msgOut(MSG_CRITICAL_ERROR, "Model DIDN'T SOLVE for this year");
    // IMPORTANT! Don't place the next two lines above the msgOut() function or it will crash in windows if the user press the stop button
        //Index iter_count = application->Statistics()->IterationCount(); // sys error if model didn't solve
        //Number final_obj = application->Statistics()->FinalObjective();
        int icount = 0;
        double obj = 0;
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
  } // end of foreach region
}

void
ModelCore::runBiologicalModule(){

  msgOut(MSG_INFO, "Starting resource module..");
  hV_byPrd.clear();
  int thisYear = MTHREAD->SCD->getYear();
  int previousYear = thisYear-1;

  for(uint i=0;i<regIds2.size();i++){

    int r2 = regIds2[i];
    int regId = r2;
    // Post optimisation biological mobule..
    vector < vector < vector <double> > > hV_byPrd_regional;
    for(uint j=0;j<fTypes.size();j++){
      string ft = fTypes[j];
      vector < vector <double> > hV_byPrd_ft;

      // calculating the regeneration..
      // if we are in a year where the time of passage has not yet been reached
      // for the specific i,e,l then we use the exogenous Vregen, otherwise we
      // calculate it
      //if ( not scen("fxVreg") ,
      //  loop( (i,essence,lambda),
      //    if( ord(t)>=(tp_u1(i,essence,lambda)+2),
      //      Vregen(i,lambda,essence,t)=regArea(i,essence,lambda,t-tp_u1(i,essence,lambda))*volHa_u1(i,essence,lambda)/1000000   ;
      //    );
      //  );
      //);
            int tp_u0 = gfd("tp",regId,ft,dClasses[0]); // time of passage to reach the first diameter class // bug 20140318, added ceil
      if(regType != "fixed" && (thisYear-secondYear) >= tp_u0 ) { // T.O.D.O to be checked -> 20121109 OK
        double pastRegArea = gfd("regArea",regId,ft,"",thisYear-tp_u0);
        double vHa = gfd("vHa",regId,ft,dClasses[1]);
        //cout << "vHa - entryVolHa: " << vHa << " / " << entryVolHa << endl;
        double vReg = pastRegArea*vHa/1000000; // T.O.D.O: check the 1000000. -> Should be ok, as area in ha vol in Mm^3
        sfd(vReg,"vReg",regId,ft,"");
      }

      for (uint u=0; u<dClasses.size(); u++){
        string dc = dClasses[u];
        double hr = 0;
        double pastYearVol = u?gfd("vol",regId,ft,dc,previousYear):0.;
        double hV_mort = 0.; /// \todo Harvest volumes from death trees
        vector <double> hV_byPrd_dc;

        // harvesting rate & volumes...
        //hr(u,i,essence,lambda,t) =    sum(p_pr, prov(u,essence,lambda,p_pr)*RPAR('st',i,p_pr,t)/In(i,p_pr,t));
        //hV(u,i,essence,lambda,t) = hr(u,i,essence,lambda,t) * V(u,i,lambda,essence,t-1);
        //hV_byPrd(u,i,essence,lambda,p_pr,t) =  prov(u,essence,lambda,p_pr)*(RPAR('st',i,p_pr,t)/In(i,p_pr,t))*V(u,i,lambda,essence,t-1);
                //double debug =0;
        for(uint pp=0;pp<priProducts.size();pp++){
          double st = gpd("st",regId,priProducts[pp]);
          double in = gpd("in",regId,priProducts[pp]);
          double hr_pr = u?app(priProducts[pp],ft,dc)*st/ in:0;
          double hV_byPrd_dc_prd = hr_pr*pastYearVol;
          hr += hr_pr;
          hV_byPrd_dc.push_back( hV_byPrd_dc_prd);
                    //debug += hV_byPrd_dc_prd;
        }
        double hV = hr*pastYearVol;
                //double debug2 = debug-hV;

                // test passed 20131203
                //if(debug2  < -0.000000000001 || debug2  > 0.000000000001){
                //    cout << "Problems!" << endl;
                //}

        // post harvesting remained volumes computation..
        // loop(u$(ord(u)=1),
        //  first diameter class, no harvesting and fixed regenaration..
        //  V(u,i,lambda,essence,t)=(1-1/(tp(u,i,lambda,essence))-mort(u,i,lambda,essence) )*V(u,i,lambda,essence,t-1)
        //                         +Vregen(i,lambda,essence,t);
        // );
        // loop(u$(ord(u)>1),
        //  generic case..
        //  V(u,i,lambda,essence,t)=((1-1/(tp(u,i,lambda,essence))
        //                         -mort(u,i,lambda,essence) - hr(u,i,essence,lambda,t))*V(u,i,lambda,essence,t-1)
        //                         +(1/(tp(u-1,i,lambda,essence)))*beta(u,i,lambda,essence)*V(u-1,i,lambda,essence,t-1));
        double vol;
        double newMortVol;    // new mortality volumes
        double tp          = gfd("tp",regId,ft,dc);
        double mort        = u?gfd("mortCoef",regId,ft,dc):0.;
        double vReg        = gfd("vReg",regId,ft,""); // Taking it from the memory database as we could be in a fixed vReg scenario and not having calculated it from above!
        double beta        = u?gfd("betaCoef",regId,ft,dc):0.;
        //double hv2fa       = gfd("hv2fa",regId,ft,dc);
        double vHa         = gfd("vHa",regId,ft,dc);
        double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);

        if(u==0){
          vol = 0.;
        } else if(u==1){
          vol = (1-1/tp-mort)*pastYearVol+vReg;
          newMortVol = mort*pastYearVol;
          double debug = vol;
        } else {
          double inc = (u==dClasses.size()-1)?0:1./tp; // we exclude the possibility for trees in the last diameter class to move to an upper class
          double tp_1 = gfd("tp",regId,ft,dClasses[u-1]);
          double pastYearVol_1 = gfd("vol",regId,ft,dClasses[u-1],previousYear);
          vol = (1-inc-mort-hr)*pastYearVol+(1/tp_1)*beta*pastYearVol_1;
          newMortVol = mort*pastYearVol;
          double debug = vol;
        }
        double freeArea_byU = u?finalHarvestFlag*1000000*hV/vHa:0; // volumes are in Mm^3, area in ha, vHa in m^3/ha
        //double debug = hv2fa*hr*pastYearVol*100;
        //cout << "regId|ft|dc| debug | freeArea: " << r2 << "|"<<ft<<"|"<<dc<<"| "<< debug << " | " << freeArea_byU << endl;

        sfd(hr,"hr",regId,ft,dc,DATA_NOW,true);
        sfd(hV,"hV",regId,ft,dc,DATA_NOW,true);
        sfd(vol,"vol",regId,ft,dc,DATA_NOW,true); // allowCreate needed for u==0
        sfd(newMortVol,"mortV",regId,ft,dc,DATA_NOW,true);

        sfd(freeArea_byU,"harvestedArea",regId,ft,dc,DATA_NOW, true);
        hV_byPrd_ft.push_back(hV_byPrd_dc);
      } // end foreach diameter classes
      hV_byPrd_regional.push_back(hV_byPrd_ft);
    } // end of each forest type
    hV_byPrd.push_back(hV_byPrd_regional);
  } // end of for each region

}

void
ModelCore::runManagementModule(){

  msgOut(MSG_INFO, "Starting management module..");
    //int thisYear = MTHREAD->SCD->getYear();
    //int previousYear = thisYear-1;
    MTHREAD->DO->expReturnsDebug.clear();
    int outputLevel = MTHREAD->MD->getIntSetting("outputLevel");
    bool weightedAverageExpectedReturns = MTHREAD->MD->getBoolSetting("weightedAverageExpectedReturns");

    //vector <vector < vector <vector <vector <double> > > > > expReturnsDebug; ///< l2_region, for type, d.c., pr prod, variable name
    //cout << "year/dc/pp/eai/cumTp/vHa/pw" << endl;

    int thisYear = MTHREAD->SCD->getYear();

  for(uint i=0;i<regIds2.size();i++){
        vector < vector <vector <vector <double> > > >  expReturnsDebug_region;

    int r2 = regIds2[i];
    int regId = r2;
    vector <double> cachedExpectedReturnsByFt;

    // PART 1: COMPUTING THE EXPECTED RETURNS FOR EACH FOREST TYPE

    for(uint j=0;j<fTypes.size();j++){
      string ft = fTypes[j];
            vector <vector <vector <double> > >  expReturnsDebug_ft;
      // Post optimisation management mobule..

      //if(regType != "fixed" && regType != "fromHrLevel"){
      // 20120910, Antonello: changed.. calculating the expected returns also for fixed and fromHrLevel regeneration (then not used but gives indication)
      // calculating the expected returns..
      //  loop ( (u,i,essence,lambda,p_pr),
      //    if (sum(u2, hV(u2,i,essence,lambda,t))= 0,
      //      expRetPondCoef(u,i,essence,lambda,p_pr) = 0;
      //    else
      //      expRetPondCoef(u,i,essence,lambda,p_pr) = hV_byPrd(u,i,essence,lambda,p_pr,t)/ sum(u2, hV(u2,i,essence,lambda,t));
      //    );
      //  );
      //  expReturns(i,essence,lambda) = sum( (u,p_pr),
      //            RPAR("pl",i,p_pr,t)*hv2fa(i,essence,lambda,u)*(1/df_byFT(u,i,lambda,essence))*        // df_byFT(u,i,lambda,essence)
      //            expRetPondCoef(u,i,essence,lambda,p_pr)
      //            );
      double hV_byFT = 0.; // gfd("hV",regId,ft,DIAM_PROD); // it must include only final harvested products in order to act as weightering agent
      double expReturns = 0;


      for (uint u=0; u<dClasses.size(); u++){
        string dc = dClasses[u];
        double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);
                double hV = gfd("hV",regId,ft,dc);
                hV_byFT += finalHarvestFlag * hV;
      }

            if(hV_byFT==0. || !weightedAverageExpectedReturns){ // nothing has been harvested in this pixel for this specific forest type. Let's calculate the combination product/diameter class with the highest expected return
        for (uint u=0; u<dClasses.size(); u++){
                    vector <vector <double> >  expReturnsDebug_dc;
          string dc = dClasses[u];
          double vHa              = gfd("vHa_exp",regId,ft,dc);
          double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);
          double cumTp_u          = gfd("cumTp_exp",regId,ft,dc);
          for (uint pp=0;pp<priProducts.size();pp++){
                        vector <double>  expReturnsDebug_pp;
            double pw    = gpd("pw",regId,priProducts[pp]);
            double raw_amount = finalHarvestFlag*pw*vHa*app(priProducts[pp],ft,dc); // B.U.G. 20121126, it was missing app(pp,ft,dc) !!
            double anualised_amount =  MD->calculateAnnualisedEquivalent(raw_amount,cumTp_u);
                        if (anualised_amount>expReturns) {
                            expReturns=anualised_amount;
                           // if (ft == "con_highF" && regId == 11041){
                           //     cout << thisYear << "/" << dc << "/" << priProducts[pp] << "/" << anualised_amount << "/" << cumTp_u << "/" << vHa << "/" << pw << endl;
                           // }
                        }
                        if(outputLevel >= OUTVL_ALL){
                          expReturnsDebug_pp.push_back(0.0);
                          expReturnsDebug_pp.push_back(hV_byFT);
                          expReturnsDebug_pp.push_back(finalHarvestFlag);
                          expReturnsDebug_pp.push_back(0.0);
                          expReturnsDebug_pp.push_back(pw);
                          expReturnsDebug_pp.push_back(cumTp_u);
                          expReturnsDebug_pp.push_back(vHa);
                          expReturnsDebug_pp.push_back(anualised_amount);
                          expReturnsDebug_pp.push_back(0);
                        }
                        expReturnsDebug_dc.push_back(expReturnsDebug_pp);
                    } // end each pp
                    expReturnsDebug_ft.push_back(expReturnsDebug_dc);
                } // end dc
      } else {
        for (uint u=0; u<dClasses.size(); u++){
                    vector <vector <double> >  expReturnsDebug_dc;
          string dc  = dClasses[u];
          double vHa = gfd("vHa_exp",regId,ft,dc);
          double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);
          double cumTp_u = gfd("cumTp_exp",regId,ft,dc);

          for (uint pp=0;pp<priProducts.size();pp++){
                        vector <double>  expReturnsDebug_pp;
            double pw    = gpd("pw",regId,priProducts[pp]);
            double pl    = gpd("pl",regId,priProducts[pp]);
            double pwor    = gpd("pl",99999,priProducts[pp]);

            double hVol_byUPp = hV_byPrd.at(i).at(j).at(u).at(pp); // harvested volumes for this product, diameter class on this region and forest type

            //double raw_amount_old = pw*hv2fa*    hVol_byUPp/hV_byFT; // old and wrong. it was in €/m^4
                        double raw_amount = finalHarvestFlag*pw*vHa* hVol_byUPp/hV_byFT; // now in €/ha if there is also thining volumes in hV_byFT, I underestimate expected returns !! TO.DO change it !! DONE, DONE...
            /**
            see @ModelData::calculateAnnualisedEquivalent
            */
            double anualised_amount =  MD->calculateAnnualisedEquivalent(raw_amount,cumTp_u); //comTp is on diamClasses, u here is on pDiamClasses
            //cout << "reg|ft|dc|prd|raw amount|ann.amount|tp|hV|hVTot|pw|pl|pW|vHa|fHFlag;";
            //cout << regId <<";"<< ft <<";"<< dc <<";" << priProducts[pp] <<";" << raw_amount <<";"<< anualised_amount<<";";
            //cout << cumTp_u <<";"<< hVol_byUPp << ";" << hV_byFT << ";" << pw << ";" << pl << ";" << pwor << ";" << vHa << ";" << finalHarvestFlag << endl;
            expReturns += anualised_amount;

                        if(outputLevel >= OUTVL_ALL){
                          expReturnsDebug_pp.push_back(hVol_byUPp);
                          expReturnsDebug_pp.push_back(hV_byFT);
                          expReturnsDebug_pp.push_back(finalHarvestFlag);
                          expReturnsDebug_pp.push_back(finalHarvestFlag*hVol_byUPp/hV_byFT);
                          expReturnsDebug_pp.push_back(pw);
                          expReturnsDebug_pp.push_back(cumTp_u);
                          expReturnsDebug_pp.push_back(vHa);
                          expReturnsDebug_pp.push_back(MD->calculateAnnualisedEquivalent(finalHarvestFlag*pw*vHa,cumTp_u));
                          expReturnsDebug_pp.push_back(1);
                        }
                        expReturnsDebug_dc.push_back(expReturnsDebug_pp);
                    } // end for each priProducts

                    expReturnsDebug_ft.push_back(expReturnsDebug_dc);
          //expReturnsPondCoef.push_back(expReturnsPondCoef_byPrd);
                } // end for each dc
            } // ending "it has been harvested" condition
      double debug = expReturns;
      sfd(expReturns,"expReturns",regId, ft,"",DATA_NOW,true);
      cachedExpectedReturnsByFt.push_back(expReturns);
            expReturnsDebug_region.push_back(expReturnsDebug_ft);
        } // end foreach forest
        MTHREAD->DO->expReturnsDebug.push_back(expReturnsDebug_region);


    // PART 2: ALLOCATING THE HARVESTED AREA TO REGENERATION AREA BASED ON EXPECTED RETURNS

    // calculating freeArea at the end of the year and choosing the new regeneration area..
    //freeArea(i,essence,lambda) = sum(u, hv2fa(i,essence,lambda,u)*hr(u,i,essence,lambda,t)*V(u,i,lambda,essence,t-1)*100);
    //if(scen("endVreg") ,
    //  regArea(i,essence,lambda,t) = freeArea(i,essence, lambda);   // here we could introduce in/out area from other land usages
    //else
    //  loop (i,
    //    loop( (essence,lambda),
    //      if ( expReturns(i,essence,lambda) = smax( (essence2,lambda2),expReturns(i,essence2,lambda2) ),
    //        regArea (i,essence,lambda,t) =  sum( (essence2, lambda2), freeArea(i,essence2, lambda2) ) * mr;
    //      );
    //    );
    //    regArea(i,essence,lambda,t) = freeArea(i,essence, lambda)*(1-mr);   // here we could introduce in/out area from other land usages
    //  );
    double totalHarvestedArea = gfd("harvestedArea",regId,FT_ALL,DIAM_ALL);

    double maxExpReturns = *( max_element( cachedExpectedReturnsByFt.begin(), cachedExpectedReturnsByFt.end() ) );
    bool foundMaxExpReturns = false;
    for(uint j=0;j<fTypes.size();j++){
      string ft = fTypes[j];
      double harvestedAreaForThisFT = gfd("harvestedArea",regId,ft,DIAM_ALL);
      if(regType == "fixed" || regType == "fromHrLevel"){
        // regeneration area is the harvested area..
        double harvestedArea = harvestedAreaForThisFT;
        sfd(harvestedArea,"regArea",regId,ft,"",DATA_NOW,true);
      } else {
        // regeneration area is a mix between harvested area and the harvested area of te most profitting forest type..
        double regArea = 0;
        if (!foundMaxExpReturns && cachedExpectedReturnsByFt[j] == maxExpReturns){
                    // I use the foundMaxExpReturns for the unlikely event that two forest types have the same expected return to avoid overcounting of the area
          regArea += totalHarvestedArea*mr;
          foundMaxExpReturns = true;
        }
                double freq = rescaleFrequencies ? gfd("freq_norm",regId,ft,""):gfd("freq",regId,ft,""); // "probability of presence" for unmanaged forest, added 20140318
                regArea +=  harvestedAreaForThisFT*(1-mr)*freq;
        sfd(regArea,"regArea",regId,ft,"",DATA_NOW,true);
      }
    }// end of foreach forest type
    double totalRegArea = gfd("regArea",regId,FT_ALL,DIAM_ALL);
    } // end of each r2
    //vector <vector < vector <vector <vector <double> > > > > expReturnsDebug = MTHREAD->DO->expReturnsDebug;
    //cout << "bla" << endl;

}

void
ModelCore::computeInventary(){
  msgOut(MSG_INFO, "Starting computing inventary available for this year..");
  int thisYear = MTHREAD->SCD->getYear();

  // In(i,p_pr,t)   =   sum((u,lambda,essence),prov(u,essence,lambda,p_pr)*V(u,i,lambda,essence,t-1));
  for(uint i=0;i<regIds2.size();i++){
    int r2 = regIds2[i];
    for(uint pp=0;pp<priProducts.size();pp++){
      double in = 0;
      for(uint ft=0;ft<fTypes.size();ft++){
        for(uint dc=0;dc<dClasses.size();dc++){
          double vol = dc?gfd("vol",r2,fTypes[ft],dClasses[dc],thisYear-1):0.;
          in += app(priProducts[pp],fTypes[ft],dClasses[dc])*vol;
        }
      }
      spd(in,"in",r2,priProducts[pp],thisYear,true);

    }
  } // end of for each region
}

void
ModelCore::cacheSettings(){
  msgOut(MSG_INFO, "Cashing initial model settings..");
  int currentYear = MTHREAD->SCD->getYear();

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
  rescaleFrequencies = MD->getBoolSetting("rescaleFrequencies");
  if((expType<0 || expType>1) && expType != -1){
    msgOut(MSG_CRITICAL_ERROR, "expType parameter must be between 1 (expectations) and 0 (adaptative) or -1 (fixed).");
  }
  mr = MD->getDoubleSetting("mr");
}

/**
* Computing some fully exogenous parameters that require complex operations, e.g. cumulative time of passage or volume per hectare.
* This happen at the very beginning of the init period and after each simulated year
*
* It doesn't include tp and mort multipliers, but this could be added as now there is a regional versiopn of them and not just a pixel version.
*/
void
ModelCore::computeCumulativeData(){

    msgOut(MSG_INFO, "Starting computing some cumulative values..");
    int thisYear     =  MTHREAD->SCD->getYear();

    // debug
    //cout << "cumTp and vHa by dc:" << endl;
    //cout << "regId|ft|varName|0|15|25|35|45|55|65|75|85|95|150|" << endl;

    for(uint r2= 0; r2<regIds2.size();r2++){
      int regId = regIds2[r2];
      for(uint j=0;j<fTypes.size();j++){
        string ft = fTypes[j];
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
        /**
        param expType Specify how the forest owners (those that make the investments) behave will be the time of passage in the future in order to calculate the cumulative time of passage in turn used to discount future revenues.
        Will forest owners behave adaptively believing the time of passage between diameter classes will be like the observed one at time they make decision (0) or they will have full expectations believing forecasts (1) or something in the middle ?
For compatibility with the GAMS code, a -1 value means using initial simulation tp values (fixed cumTp)."
        */
                vector <double> cumTp_temp;        // cumulative time of passage to REACH a diameter class (tp is to LEAVE to the next one)
                vector <double> vHa_temp;          // volume at hectar by each diameter class [m^3/ha]
                vector <double> cumAlive_temp;     // cumulated alive rate to reach a given diameter class
                vector <double> cumTp_exp_temp;    // "expected" version of cumTp
                vector <double> vHa_exp_temp;      // "expected" version of vHa
                vector <double> cumAlive_exp_temp; // "expected" version of cumMort

        MD->setErrorLevel(MSG_NO_MSG); // as otherwise on 2007 otherwise sfd() will complain that is filling multiple years (2006 and 2007)
        for (uint u=0; u<dClasses.size(); u++){
          string dc = dClasses[u];
          double cumTp_u, cumTp_u_exp, cumTp_u_noExp, cumTp_u_fullExp;
          double vHa_u, vHa_u_exp, vHa_u_noExp, vHa_u_fullExp, beta, beta_exp, beta_noExp, beta_fullExp, mort, mort_exp, mort_noExp, mort_fullExp;
          double tp_u, tp_exp;
                    double cumAlive_u, cumAlive_exp_u;

          if(u==0) {
            // first diameter class.. expected  and real values are the same (0)
            cumTp_u = 0.;
            vHa_u   = 0.;
                        cumAlive_u = 1.;
            cumTp_temp.push_back(cumTp_u);
            cumTp_exp_temp.push_back(cumTp_u);
            vHa_temp.push_back(vHa_u);
            vHa_exp_temp.push_back(vHa_u);
                        cumAlive_temp.push_back(cumAlive_u);
                        cumAlive_exp_temp.push_back(cumAlive_u);
            sfd(cumTp_u,"cumTp",regId,ft,dc,DATA_NOW,true);
            sfd(cumTp_u,"cumTp_exp",regId,ft,dc,DATA_NOW,true);
            sfd(vHa_u,  "vHa",regId,ft,dc,DATA_NOW,true);
            sfd(vHa_u,  "vHa_exp",regId,ft,dc,DATA_NOW,true);
                        sfd(cumAlive_u,"cumAlive",regId,ft,dc,DATA_NOW,true);
                        sfd(cumAlive_u,"cumAlive_exp",regId,ft,dc,DATA_NOW,true);
          } else {
            // other diameter classes.. first dealing with real values and then with expected ones..
            // real values..
            cumTp_u = cumTp_temp[u-1] + gfd("tp",regId,ft,dClasses[u-1],thisYear); // it adds to the time of passage to reach the previous diameter class the time of passage that there should be to reach this diameter class in the year where the previous diameter class will be reached
            if (u==1){
              vHa_u = gfd("entryVolHa",regId,ft,"",thisYear);
                            mort = 0.; // not info about mortality first diameter class ("00")
            } else {
              mort = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1],thisYear),gfd("tp",regId,ft,dClasses[u-1],thisYear)); // mortality of the previous diameter class
                            beta = gfd("betaCoef",regId,ft,dc, thisYear);
              vHa_u = vHa_temp[u-1]*beta*(1-mort);
            }
                        cumAlive_u = max(0.,cumAlive_temp[u-1]*(1-mort));
                        cumAlive_temp.push_back(cumAlive_u);
            cumTp_temp.push_back(cumTp_u);
            vHa_temp.push_back(vHa_u);
            sfd(cumTp_u,"cumTp",regId,ft,dc,DATA_NOW,true);
            sfd(vHa_u,"vHa",regId,ft,dc,DATA_NOW,true);
                        sfd(cumAlive_u,"cumAlive",regId,ft,dc,DATA_NOW,true);

            // expected values..
            if (expType == -1){
              cumTp_u_exp = cumTp_exp_temp[u-1]+gfd("tp",regId,ft,dClasses[u-1],firstYear); // it adds to the time of passage to reach the previous diameter class the time of passage that there should be to reach this diameter class in the year where the previous diameter class will be reached
              cumTp_exp_temp.push_back(cumTp_u_exp);
              if(u==1) {
                vHa_u_exp = gfd("entryVolHa",regId,ft,"",firstYear);
                                mort_exp = 0.; // not info about mortality first diameter class ("00")
              } else {
                mort_exp  = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1], firstYear),gfd("tp",regId,ft,dClasses[u-1],firstYear)) ; // mortality rate of previous diameter class
                beta_exp  = gfd("betaCoef",regId,ft,dc, firstYear);
                vHa_u_exp = vHa_exp_temp[u-1]*beta_exp*(1-mort_exp);
              }
            } else {
              cumTp_u_noExp   = cumTp_exp_temp[u-1]+gfd("tp",regId,ft,dClasses[u-1]);
              cumTp_u_fullExp = cumTp_exp_temp[u-1]+gfd("tp",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_exp_temp[u-1])); // it adds to the time of passage to reach the previous diameter class the time of passage that there should be to reach this diameter class in the year where the previous diameter class will be reached
              cumTp_u_exp     = cumTp_u_fullExp*expType+cumTp_u_noExp*(1-expType);
              cumTp_exp_temp.push_back(cumTp_u_exp);
              if(u==1) {
                vHa_u_noExp   = gfd("entryVolHa",regId,ft,"",DATA_NOW);
                vHa_u_fullExp = gfd("entryVolHa",regId,ft,"",thisYear+ceil(cumTp_u));
                vHa_u_exp     = vHa_u_fullExp*expType+vHa_u_noExp*(1-expType);
                                mort_exp      = 0. ;
              } else {
                mort_noExp   = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1], DATA_NOW),cumTp_exp_temp[u]-cumTp_exp_temp[u-1]);
                mort_fullExp = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_temp[u-1])),cumTp_exp_temp[u]-cumTp_exp_temp[u-1]); // mortality of the previous diameter class
                beta_noExp   = gfd("betaCoef",regId,ft,dc, DATA_NOW);
                beta_fullExp = gfd("betaCoef",regId,ft,dc, thisYear+ceil(cumTp_u));
                mort_exp     = mort_fullExp*expType+mort_noExp*(1-expType);
                beta_exp     = beta_fullExp*expType+beta_noExp*(1-expType);
                vHa_u_exp    = vHa_exp_temp[u-1]*beta_exp*(1-mort_exp);
              }
            }
            vHa_exp_temp.push_back(vHa_u_exp);
                        cumAlive_exp_u = max(0.,cumAlive_exp_temp[u-1]*(1-mort_exp));
                        cumAlive_exp_temp.push_back(cumAlive_exp_u);
            sfd(cumTp_u_exp,"cumTp_exp",regId,ft,dc,DATA_NOW,true);
            sfd(vHa_u_exp,  "vHa_exp",regId,ft,dc,DATA_NOW,true);
                        sfd(cumAlive_exp_u,"cumAlive_exp",regId,ft,dc,DATA_NOW,true);
                        //sfd(cumMort_u_exp,  "cumMort_exp",regId,ft,dc,DATA_NOW,true);

                        //cout << "********" << endl;
                        //cout << "dc: " << dClasses[u] << endl ;
                        //cout << "mort: " << mort << endl;
                        //cout << "mort_exp: " << mort_exp << endl;
                        //cout << "cumAlive: " << cumAlive_u << endl;
                        //cout << "cumAlive_exp: " << cumAlive_exp_u << endl;


          }

        } // end of each diam class


        //            // debug stuff on vHa
        //            cout << regId << "|" << ft << "|cumTp_temp|";
        //            for (uint u=0; u<dClasses.size(); u++){
        //                cout << cumTp_temp.at(u)<<"|";
        //            }
        //            cout << endl;
        //            cout << regId << "|" << ft << "|cumTp_exp|";
        //            for (uint u=0; u<dClasses.size(); u++){
        //                cout << cumTp_exp_temp.at(u)<<"|";
        //            }
        //            cout << endl;
        //            cout << regId << "|" << ft << "|vHa_temp|";
        //            for (uint u=0; u<dClasses.size(); u++){
        //                cout << vHa_temp.at(u)<<"|";
        //            }
        //            cout << endl;
        //            cout << regId << "|" << ft << "|vHa_exp|";
        //            for (uint u=0; u<dClasses.size(); u++){
        //                cout << vHa_exp_temp.at(u)<<"|";
        //            }
        //            cout << endl;


      } // end of each ft
    } // end of each region
    MD->setErrorLevel(MSG_ERROR);
}



/**
This function take for each region the difference for each forest type between the harvested area and the new regeneration one and apply such delta to each pixel of the region proportionally to the area that it already hosts.
*/
void
ModelCore::updateMapAreas(){

  msgOut(MSG_INFO, "Updating map areas..");
  map<int,double> forestArea; // foresta area by each region
  pair<int,double > forestAreaPair;
    int thisYear = MTHREAD->SCD->getYear();
  vector<int> l2Regions =  MTHREAD->MD->getRegionIds(2, true);
  vector <string> fTypes =  MTHREAD->MD->getForTypeIds();
  int nFTypes = fTypes.size();
  int nL2Regions = l2Regions.size();
  for(uint i=0;i<nL2Regions;i++){
    int regId = l2Regions[i];
    vector<Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regId);
    ModelRegion* reg = MTHREAD->MD->getRegion(regId);
    for(uint j=0;j<nFTypes;j++){
      string ft = fTypes[j];
            double oldRegioForArea;
            if(thisYear <= firstYear+1) {
                oldRegioForArea = reg->getValue("forArea_"+ft)/10000;
            } else {
                oldRegioForArea = gfd("forArea",regId,ft,DIAM_ALL,thisYear-1);
            }
            //oldRegioForArea = reg->getValue("forArea_"+ft)/10000;
            //double debug = gfd("forArea",regId,ft,DIAM_ALL,thisYear-1);
            //double debug_diff = oldRegioForArea - debug;
            //cout << thisYear << ";" << regId << ";" << ft << ";" << oldRegioForArea << ";" << debug << ";" << debug_diff << endl;
            double harvestedArea = gfd("harvestedArea",regId,ft,DIAM_ALL); //20140206
            double regArea = gfd("regArea",regId,ft,DIAM_ALL); //20140206
      double newRegioForArea = oldRegioForArea + regArea - harvestedArea;
      sfd(newRegioForArea,"forArea",regId,ft,"",DATA_NOW, true);
      for(uint z=0;z<rpx.size();z++){
        double oldValue = rpx[z]->getDoubleValue("forArea_"+ft,true);
                double ratio = newRegioForArea/oldRegioForArea;
                double newValue = oldValue*ratio;
        rpx[z]->changeValue("forArea_"+ft, newValue);
      }

    }
  }
}



