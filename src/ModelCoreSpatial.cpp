/***************************************************************************
 *   Copyright (C) 2015 by Laboratoire d'Economie Forestière               *
 *   http://ffsm-project.org                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version, given the compliance with the     *
 *   exceptions listed in the file COPYING that is distribued together     *
 *   with this file.                                                       *
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
#include "Carbon.h"


ModelCoreSpatial::ModelCoreSpatial(ThreadManager *MTHREAD_h){
  MTHREAD = MTHREAD_h;
}

ModelCoreSpatial::~ModelCoreSpatial(){

}

void
ModelCoreSpatial::runInitPeriod(){

  cacheSettings();             ///< cashe things like first year, second year, dClasses...
  initializePixelVolumes();    ///< compute px volumes vol for 2005 (including exogenous loaded volumes)
  initMarketModule();          ///< inside it uses first year, second year
  MTHREAD->DO->print();
  MTHREAD->SCD->advanceYear(); ///< 2005->2006
  int thisYear = MTHREAD->SCD->getYear(); // for debugging
  resetPixelValues();          ///< swap volumes->lagged_volumes and reset the other pixel vectors
  cachePixelExogenousData();   ///< compute pixel tp, meta and mort
  computeInventary();          ///< in=f(vol_t-1)
  computeCumulativeData();     ///< compute cumTp_exp, vHa_exp, vHa
  initializePixelArea();       ///< compute px->area for each ft and dc (including exogenous loaded areas)
  runBiologicalModule();       //
  runManagementModule();
  updateMapAreas();            ///< update the forArea_{ft} layer on each pixel as old value-hArea+regArea
  sumRegionalForData();        ///< only for printing stats as forest data is never used at regional level
  initialiseCarbonModule();
  MTHREAD->DO->print();
}

void
ModelCoreSpatial::runSimulationYear(){
  int thisYear = MTHREAD->SCD->getYear(); // for debugging
  resetPixelValues();          // swap volumes->lagged_volumes and reset the other pixel vectors
  cachePixelExogenousData();   // compute pixel tp, meta and mort
  computeInventary();          // in=f(vol_t-1)
  runMarketModule();           // RUN THE MARKET OPTIMISATION HERE
  computeCumulativeData();     // compute cumTp_exp, vHa_exp
  cachePixelExogenousData();
  runBiologicalModule();
  runManagementModule();
  updateMapAreas();
  sumRegionalForData();      // only for printing stats as forest data is never used at regional level
  registerCarbonEvents();
  MTHREAD->DO->print();
}

void
ModelCoreSpatial::initMarketModule(){
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
      double gamma = gpd("gamma",r2,priProducts[pp]); // elast supply to stock
      double sigma = gpd("sigma",r2,priProducts[pp]); // elast supply to price
      double sigmaCorr = sigma;
      double pc_1  = gpd("pc",r2,priProducts[pp],previousYear);
      double sc_1  = gpd("sc",r2,priProducts[pp],previousYear);
      double in    = gpd("in",r2,priProducts[pp]);
      double in_1  = gpd("in",r2,priProducts[pp],previousYear);
      double supCorr = 1.0; // Coefficient to reduce supply function when inventory is small


      // //When inventary for a resource is almost null and further decreasing supply depends less from the price and more from the resource
      // No longer needed, but it could be used again if we face a problem where in go to zero due to too much harvesting/growth
      // //cout << "gamma orig: " << gamma << endl;
      // if (in<=0.1 && in <= in_1) { // 0.3
      //   gamma = gamma * 1.8; // 1.3: 0.65;
      //   sigmaCorr = sigma*0.2; // 0.4
      //   //supCorr = 0.7;
      //   //cout << "gamma mod: " << gamma << endl;
      // }  else if(in<=1.0 && in <= in_1){
      //   gamma = gamma * 1.8; // 1.24: 0.62;
      //   sigmaCorr = sigma*0.2; // 0.4
      //   //supCorr = 0.8;
      //   //cout << "gamma mod: " << gamma << endl;
      // }


       //if(in<=5.0){
       //  supCorr = 0.8;
       //}


      //double bb = (sigmaCorr/(sigmaCorr+1.0))*pc_1*pow(sc_1,-1.0/sigmaCorr)*pow(in_1/in,gamma/sigmaCorr);
      //double ff = sc_1*pow(pc_1,-sigmaCorr)*pow(in/in_1,gamma); //chi
      double bb = (sigmaCorr/(sigmaCorr+1.0))*pc_1*pow(sc_1,-1.0/sigmaCorr)*pow(in_1/in,gamma/sigmaCorr)*pow(1.0/supCorr,1.0/sigmaCorr);
      double ff = sc_1*pow(pc_1,-sigmaCorr)*pow(in/in_1,gamma)*supCorr; //chi
      //double supCorr2 = pow(1.0/supCorr,1.0/sigmaCorr);

      spd(bb,"bb",r2,priProducts[pp],DATA_NOW,true);
      spd(ff,"ff",r2,priProducts[pp],DATA_NOW,true);
      spd(sigmaCorr,"sigmaCorr",r2,priProducts[pp],DATA_NOW,true);
      //spd(supCorr,"supCorr",r2,priProducts[pp],DATA_NOW,true);
      //spd(supCorr2,"supCorr2",r2,priProducts[pp],DATA_NOW,true);

    }

  } // end for each region in level 2 (and updating variables)



  // *** OPTIMISATION....

  // Create an instance of the IpoptApplication
  //Opt *OPTa = new Opt(MTHREAD);
  //SmartPtr<TNLP> OPTa = new Opt(MTHREAD);
  SmartPtr<IpoptApplication> application = new IpoptApplication();
  string linearSolver = MTHREAD->MD->getStringSetting("linearSolver");
  application->Options()->SetStringValue("linear_solver", linearSolver); // default in ipopt is ma27
  //application->Options()->SetStringValue("hessian_approximation", "limited-memory"); // quasi-newton approximation of the hessian
  //application->Options()->SetIntegerValue("mumps_mem_percent", 100);
  application->Options()->SetNumericValue("obj_scaling_factor", -1); // maximisation
  application->Options()->SetNumericValue("max_cpu_time", 1800); // max 1/2 hour to find the optimus for one single year
  application->Options()->SetStringValue("check_derivatives_for_naninf", "yes");

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
    //Index iter_count = application->Statistics()->IterationCount(); // syserror if model doesn't solve
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
  } // end of each region
}


/**
 * @brief ModelCoreSpatial::runBiologicalModule
 *
 *   Changes in Area:
 *   dc     area_l  area    diff
 *   0      --------->      +regArea -areaFirstProdClass (areaMovingUp_00)
 *   15     --------->      +areaFirstPrClass -hArea_15 -areaMovingUp_15
 *   25     --------->      +areaMovingUp15 - hArea_25 - areaMovingUp_25
 *   35     --------->      +areaMovingUp25 - hArea_35 - areaMovingUp_35
 *   ...
 *   95     --------->      +areaMovingUp85 - hArea_95 - areaMovingUp_95
 *   105    --------->      +areaMovingUp95 - hArea_105
 *
 *   note: regArea is computed in the management module, not here. Further, regArea is already the net one of forest area changes
 */
void
ModelCoreSpatial::runBiologicalModule(){

  msgOut(MSG_INFO, "Starting resource module..");
  int thisYear = MTHREAD->SCD->getYear();

  for(uint i=0;i<regIds2.size();i++){
    int r2 = regIds2[i];
    int regId = r2;
    ModelRegion* REG = MTHREAD->MD->getRegion(r2);
    //Gis* GIS = MTHREAD->GIS;
    regPx = REG->getMyPixels();
    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];

      double pxId = px->getID();
      //if (pxId == 3550.0){
      //    cout << "got the pixel" << endl;
      //}
      //px->expectedReturns.clear();
      for(uint j=0;j<fTypes.size();j++){
        string ft = fTypes[j];
        double pxArea_debug     = px->getDoubleValue("forArea_"+ft, true);
        vector <double>            hV_byDiam;
        vector < vector <double> > hV_byDiamAndPrd;
        vector <double> hArea_byDc;
        vector <double> newVol_byDiam;
        vector <double> vMort_byDc;
        vector <double> areasMovingUp(dClasses.size(), 0.0);
        double areaFirstProdClass;


        // A - COMPUTING THE REGENERATION..
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
        int tp_u0 = px->tp.at(j).at(0); // time of passage to reach the first production diameter class // bug 20140318, added ceil. 20140318 removed it.. model did go crazy with it
        if(thisYear == secondYear){
            px->initialDc0Area.push_back(px->area_l.at(j).at(0));
        }
        if(regType != "fixed" && (thisYear-secondYear) >= tp_u0 ) { // T.O.D.O to be checked -> 20121109 OK
          double pastRegArea = px->getPastRegArea(j,thisYear-tp_u0);
          double availableArea = px->area_l.at(j).at(0);
          //double entryVolHa = gfd("entryVolHa",regId,ft,"");
          double vHa = px->vHa.at(j).at(1);
          //attenction that at times could take the wrong pastRegArea if tp change too suddenly as in some "strange" scenarios
          if (oldVol2AreaMethod){
            areaFirstProdClass = pastRegArea;
          } else {
            areaFirstProdClass = min(availableArea, pastRegArea); // this is just a start and will need to include the last year area
          }
          px->vReg.push_back(areaFirstProdClass*vHa/1000000.0); // TO.DO: check the 1000000. Should be ok, as area in ha vol in Mm^3
          //if (pxId == 3550.0 && j==3){
          //    cout << "got the pixel" << endl;
          //}
          #ifdef QT_DEBUG
          if (areaFirstProdClass < 0.0){
             //msgOut(MSG_CRITICAL_ERROR,"Negative regeneration volumes in endogenous regeneration");
          }
          if ( (availableArea-pastRegArea) < -0.00000001     ){
             // in a very rare cases tp change first in a direction and then in the other, so that the wrong past regeneration area
             // is picken up.
             //msgOut(MSG_CRITICAL_ERROR,"Upgrading from dc0 more area than the available one in endogenous regeneration");
          }
          #endif
        } else {
          double regionArea = REG->getValue("forArea_"+ft,OP_SUM);
          double pxArea     = px->getDoubleValue("forArea_"+ft, true); // 20121109 bug solved (add get zero for not data)
          double regRegVolumes = gfd("vReg",r2,ft,"");
          double newVReg = regionArea ? regRegVolumes*pxArea/regionArea : 0.0;
          px->vReg.push_back(newVReg); // 20121108 BUG !!! solved // as now we have the area we could also use here entryVolHa
          // only a share of the exogenous area goes up, the regeneration one doesn't yet reach tp0:
          // areaFirstProdClass = (1.0 / px->tp.at(j).at(0) ) * px->area_l.at(j).at(0);
          areaFirstProdClass = (1.0 / ((double) tp_u0) ) * px->initialDc0Area.at(j);
          // in the exogenous period we are exogenously upgrading u0->u1 some areas but, as we do not have the regeneration
          // are corresponding to that we have also to manually add it to u0
          //px->area_l.at(j).at(0) += areaFirstProdClass;
          //areaFirstProdClass = entryVolHa ? newVReg*1000000 /entryVolHa:0.0;
          //if (pxId == 3550.0 && j==3){
          //    cout << "got the pixel" << endl;
          //}

          #ifdef QT_DEBUG
          if (areaFirstProdClass<0.0){
            // msgOut(MSG_CRITICAL_ERROR,"Negative regeneration volumes in exogenous regeneration");
          }
          if (areaFirstProdClass > px->area_l.at(j).at(0)){
             //msgOut(MSG_CRITICAL_ERROR,"Moving up area higher than available area in exogenous regeneration !");
          }
          #endif
          // vReg and entryVolHa are NOT the same thing. vReg is the yearly regeneration volumes
          // for the whole region. We can use them when we don't know the harvested area
          // entryVolHa can lead to vReg calculation only when we know the regeneration area. So in the
          // first years we use vReg and subsequently the endogenous one.
        }

        //double harvestedArea = 0;



        for (uint u=0; u<dClasses.size(); u++){
          string dc = dClasses[u];
          double hr =0;
          //double pastYearVol_reg = u ? gfd("vol",r2,ft,dc,thisYear-1): 0;
          double pastYearVol = px->vol_l.at(j).at(u);
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
            hV_byPrd.push_back( hr_pr*pastYearVol*px->avalCoef);
          }
          double hV = hr*pastYearVol*px->avalCoef;
          hV_byDiam.push_back(hV);
          hV_byDiamAndPrd.push_back(hV_byPrd);

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
          double tp          = px->tp.at(j).at(u); //gfd("tp",regId,ft,dc);
          double mort        = px->mort.at(j).at(u); //gfd("mortCoef",regId,ft,dc);
          double vReg        = px->vReg.at(j); //gfd("vReg",regId,ft,""); // Taking it from the memory database as we could be in a fixed vReg scenario and not having calculated it from above!
          double beta        = px->beta.at(j).at(u); //gfd("betaCoef",regId,ft,dc);
          //double hv2fa       = gfd("hv2fa",regId,ft,dc);
          double vHa         = px->vHa.at(j).at(u); //gfd("vHa",regId,ft,dc);
          double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);

          double vMort       = mort*pastYearVol;

          vMort_byDc.push_back(vMort);

          if(u==0){
            vol = 0.0;
          }else if(u==1){
            vol = (1-1/tp-mort)*pastYearVol+vReg;
            double debug = vol;
            #ifdef QT_DEBUG
            if (vol<0.0){
              msgOut(MSG_CRITICAL_ERROR,"Negative volumes!");
            }
            #endif
          } else {
            // time of passage and volume of smaller diameter class
            double inc = (u==dClasses.size()-1)?0:1./tp; // we exclude the possibility for trees in the last diameter class to move to an upper class
            double tp_1 = px->tp.at(j).at(u-1); //gfd("tp",regId,ft,dClasses[u-1]);
            double pastYearVol_1 = px->vol_l.at(j).at(u-1); //gfd("vol",regId,ft,dClasses[u-1],thisYear-1);
            //vol = max(0.0,(1-inc-mort-hr)*pastYearVol+(1/tp_1)*beta*pastYearVol_1);
            vol = max(0.0,(1-inc-mort)*pastYearVol-hV+(1/tp_1)*beta*pastYearVol_1); // I can't use any more hr as it it the harvesting rate over the available volumes, not the whole ones
            #ifdef QT_DEBUG
            if ((1-inc-mort)*pastYearVol-hV+(1/tp_1)*beta*pastYearVol_1 < -0.00001){
              msgOut(MSG_CRITICAL_ERROR,"Negative volumes!");
            }
            #endif
          }
          if(u != 0){ // this if is required to avoid a 0/0 and na error that then propage also in vSum()
            double inc = (u==dClasses.size()-1)?0:1.0/tp; // we exclude the possibility for trees in the last diameter class to move to an upper class
            double volumesMovingUp = inc*pastYearVol;
            double pastArea = px->area_l.at(j).at(u);
            areasMovingUp.at(u) = inc*pastArea;

            if(oldVol2AreaMethod) {
              hArea_byDc.push_back(finalHarvestFlag*1000000*hV/vHa); // volumes are in Mm^3, area in ha, vHa in m^3/ha
            } else {
              double finalHarvestedVolumes = finalHarvestFlag* hV;
              double finalHarvestedRate = pastYearVol?finalHarvestedVolumes/pastYearVol:0.0; // Here we want the harvested rate over the whole volumes, not just the available ones, so we don't need to multiply to px->avalCoef
              hArea_byDc.push_back(finalHarvestedRate*pastArea); // volumes are in Mm^3, area in ha, vHa in m^3/ha
            }
            px->area.at(j).at(u) = px->area_l.at(j).at(u) - areasMovingUp.at(u) + areasMovingUp.at(u-1) - hArea_byDc.at(u);
          } else {
            areasMovingUp.at(u) = areaFirstProdClass;
            hArea_byDc.push_back(0.);
            px->area.at(j).at(u) = px->area_l.at(j).at(u) - areasMovingUp.at(u) - hArea_byDc.at(u);
            //if (pxId == 3550.0 && j==3){
            //    cout << "got the pixel" << endl;
            //}
          }
          newVol_byDiam.push_back(vol);
          #ifdef QT_DEBUG
          if(px->area.at(j).at(u)< 0.0 || areasMovingUp.at(u) < 0.0 || hArea_byDc.at(u) < 0.0 ){
            // msgOut(MSG_CRITICAL_ERROR, "Negative values in runBiologicalModel");
          }
          #endif

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
        px->vol.push_back(newVol_byDiam);
        px->vMort.push_back(vMort_byDc);

        #ifdef QT_DEBUG
        for (uint u=1; u<dClasses.size(); u++){
          double volMort = vMort_byDc[u];
          double harvVol = hV_byDiam[u];
          double vol_new = newVol_byDiam[u];
          double vol_lagged = px->vol_l.at(j).at(u);
          double gain = vol_new - (vol_lagged-harvVol-volMort);
          if (volMort > vol_lagged){
            msgOut(MSG_CRITICAL_ERROR,"mort vol > lagged volumes ?");
          }
        }
        #endif
      } // end of each forest type
    } // end of each pixel

    #ifdef QT_DEBUG
    // checking that in a region the total hVol is equal to the st for each products. 20150122 Test passed with the new availCoef
    double sumSt = 0.0;
    double sumHv = 0.0;
    for(uint pp=0;pp<priProducts.size();pp++){
      sumSt += gpd("st",r2,priProducts[pp]);
    }
    for (uint p=0;p<regPx.size();p++){
      for(uint j=0;j<fTypes.size();j++){
        for (uint u=0; u<dClasses.size(); u++){
          for(uint pp=0;pp<priProducts.size();pp++){
            // by ft, dc, pp
            sumHv += regPx[p]->hVol_byPrd[j][u][pp];
          }
        }
      }
    }
    if(abs(sumSt-sumHv) > 0.000001){
      msgOut(MSG_CRITICAL_ERROR, "St and harvested volumes diverge in region "+REG->getRegSName());
    }
    #endif
  } // end of each region

}



void
ModelCoreSpatial::runManagementModule(){
  msgOut(MSG_INFO, "Starting management module..");
  vector<string> allFTypes = MTHREAD->MD->getForTypeIds(true);
  map<string,double> hAreaByFTypeGroup = vectorToMap(allFTypes,0.0);
  int thisYear = MTHREAD->SCD->getYear();

  // Post optimisation management mobule..
  for(uint i=0;i<regIds2.size();i++){
    int r2 = regIds2[i];
    int regId = r2;
    ModelRegion* REG = MTHREAD->MD->getRegion(r2);
    regPx = REG->getMyPixels();

    // Dealing with area change..
    double fArea_reg     = REG->getArea();
    double fArea_diff    = 0.0;
    double fArea_reldiff = 0.0;
    if(forestAreaChangeMethod=="relative"){
      fArea_reldiff = gfd("forestChangeAreaIncrementsRel",r2,"","",DATA_NOW);
      fArea_diff    = fArea_reg * fArea_reldiff;
    } else if (forestAreaChangeMethod=="absolute"){
      fArea_diff    = gfd("forestChangeAreaIncrementsHa",r2,"","",DATA_NOW);
      //fArea_reldiff = fArea_diff / fArea_reg;
    }
    double regHArea = 0.0; // for the warning




    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];
      px->expectedReturns.clear();
      resetMapValues(hAreaByFTypeGroup,0.0);
      double totalHarvestedArea = vSum(px->hArea); // still need to remove the forest decrease areas..
      vector<double> thisYearRegAreas(fTypes.size(),0.0); // initialize a vector of fTypes.size() zeros.
      vector<double> expectedReturns(fTypes.size(),0.0);  // uncorrected expected returns (without considering transaction costs). These are in form of eai

      double fArea_px = vSum(px->area);
      double fArea_diff_px = fArea_px * fArea_diff/ fArea_reg;
      double fArea_incr = max(0.0,fArea_diff_px);
      double fArea_decr = - min(0.0,fArea_diff_px);
      double fArea_decr_rel = min(1.0,fArea_decr/totalHarvestedArea);
      regHArea += totalHarvestedArea;
      totalHarvestedArea = totalHarvestedArea *(1-fArea_decr_rel);


      // A - Computing the harvestingArea by parent ft group (for the allocation according to the prob of presence):
      for(uint j=0;j<fTypes.size();j++){
        string ft = fTypes[j];
        string parentFt = MTHREAD->MD->getForTypeParentId(ft);
        double hAreaThisFt=vSum(px->hArea.at(j))*(1-fArea_decr_rel);
        incrMapValue(hAreaByFTypeGroup,parentFt,hAreaThisFt); // increment the parent ft of the harvested area, neeed for assigning the frequences (prob. of presence)
      }

      // B - Computing the uncorrected expected returns (without considering transaction costs)
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
      for(uint j=0;j<fTypes.size();j++){
        string ft = fTypes[j];
        double expReturns = 0.;
        int optDc = 0; // "optimal diameter class", the one on which the expected returns are computed
        for (uint u=0; u<dClasses.size(); u++){
          string dc = dClasses[u];
          double vHa         = px->vHa_exp.at(j).at(u);
          double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);
          double cumTp_u = px->cumTp_exp.at(j).at(u);
          for (uint pp=0;pp<priProducts.size();pp++){
            double pl            = gpd("pl",regId,priProducts[pp]); // note that this is the OBSERVED price. If we call it at current year+cumTp_u we would have the expected price. But we would first have to compute it, as pw is weigthed price world-local and we don't have local price for the future. DONE 20141202 ;-)
            double worldCurPrice = gpd("pl",WL2,priProducts[pp]);
            double worldFutPrice = gpd("pl",WL2,priProducts[pp],thisYear+cumTp_u);
            double sl            = gpd("sl",regId,priProducts[pp]);
            double sa            = gpd("sa",regId,priProducts[pp]);
            double pw_exp        = computeExpectedPrice(pl, worldCurPrice, worldFutPrice, sl, sa, px->expTypePrices); //20141030: added the expected price!
            double raw_amount = finalHarvestFlag*pw_exp*vHa*app(priProducts[pp],ft,dc); // B.U.G. 20121126, it was missing app(pp,ft,dc) !!
            double anualised_amount =  MD->calculateAnnualisedEquivalent(raw_amount,cumTp_u);
            if (anualised_amount>expReturns) {
              expReturns=anualised_amount;
              optDc = u;
            }
          }
        }
        px->expectedReturnsNotCorrByRa.push_back(expReturns);
        if(MD->getBoolSetting("heterogeneousRiskAversion")){
          double ra = px->getDoubleValue("ra");
          double cumMort = 1-px->cumAlive_exp.at(j).at(optDc);
          //cout << px->getID() << "\t" << ft << "\t\t" << "optDc" << optDc << "\t" << cumMort << endl;
          double origExpReturns = expReturns;
          expReturns = origExpReturns * (1.0 - ra*cumMort);
        }
        px->expectedReturns.push_back(expReturns);
        expectedReturns.at(j) = expReturns;
      } // end foreach forest type

      for(uint j=0;j<fTypes.size();j++){
        string ft = fTypes[j];

        double harvestedAreaForThisFT = vSum(px->hArea.at(j))*(1-fArea_decr_rel); // gfd("harvestedArea",regId,ft,DIAM_ALL);
        vector<double> corrExpectedReturns(fTypes.size(),0.0); //  corrected expected returns (considering transaction costs). These are in form of  NPV

        // C - Computing the corrected expected returns including transaction costs
        for(uint j2=0;j2<fTypes.size();j2++){
          string ft2 = fTypes[j2];
          //double invTransConstCoeff = gfd("invTransConstCoeff",regId,ft,ft2,DATA_NOW);
          //corrExpectedReturns[j2] = expectedReturns[j2]*invTransConstCoeff;
          double invTransConst = gfd("invTransConst",regId,ft,ft2,DATA_NOW);
          corrExpectedReturns[j2] = (expectedReturns[j2]/ir)+invTransConst; // changed 20150718: npv = eai/ir + tr. cost
        }

        //int highestReturnFtIndex = getMaxPos(corrExpectedReturns);

        // D - Assigning the Managed area
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
        //if (j==highestReturnFtIndex){
        //  thisYearRegAreas[j] += totalHarvestedArea*mr;
        //}
        // If I Implement this I'll have a minimal diff in total area.. why ?????


        thisYearRegAreas[getMaxPos(corrExpectedReturns)] += harvestedAreaForThisFT*mr;
        thisYearRegAreas[getMaxPos(expectedReturns)] += fArea_incr*mr/((double)fTypes.size()); // mr quota of new forest area assigned to highest expected returns ft (not considering transaction costs). Done for each forest types


        // E - Assigning unmanaged area
        //for(uint j2=0;j2<fTypes.size();j2++){
          if(natRegAllocation=="pp"){ // according to prob presence
            //string ft2 = fTypes[j2];
            string parentFt = MTHREAD->MD->getForTypeParentId(ft);
            double freq = rescaleFrequencies ? gfd("freq_norm",regId,parentFt,""):gfd("freq",regId,parentFt,""); // "probability of presence" for unmanaged forest, added 20140318
            double hAreaThisFtGroup = findMap(hAreaByFTypeGroup,parentFt);
            double hRatio = 1.0;
            if(hAreaThisFtGroup>0){
                //double harvestedAreaForThisFT2 = vSum(px->hArea.at(j2));
                hRatio = harvestedAreaForThisFT/hAreaThisFtGroup;
            } else {
                int nFtChilds = MTHREAD->MD->getNForTypesChilds(parentFt);
                hRatio = 1.0/nFtChilds;
            }
            thisYearRegAreas[j] +=  totalHarvestedArea*(1-mr)*freq*hRatio;
            thisYearRegAreas[j] +=  fArea_incr*(1-mr)*freq*hRatio; // non-managed quota of new forest area assigning proportionally on pp at sp group level
            //thisYearRegAreas[j2] +=  harvestedAreaForThisFT*(1-mr)*freq*hRatio;
          } else { // prob presence not used..
            thisYearRegAreas[j] += harvestedAreaForThisFT*(1-mr);
            // Allocating non-managed quota of new forest area to ft proportionally to the current area share by ft
            double newAreaThisFt = vSum(px->area) ? fArea_incr*(1-mr)*vSum(px->area.at(j))/vSum(px->area): 0.0;
            thisYearRegAreas[j] +=  newAreaThisFt;
            if(! (thisYearRegAreas[j] >= 0.0) ){
              msgOut(MSG_ERROR,"thisYearRegAreas[j] is not non negative.");
            }
            //thisYearRegAreas[j2] += harvestedAreaForThisFT*(1-mr);
          }
        //}
      } // end for each forest type

      // adding regeneration area to the fist (00) diameter class
      for(uint j=0;j<fTypes.size();j++){
        px->area.at(j).at(0) += thisYearRegAreas.at(j);
      }




      #ifdef QT_DEBUG
      double totalRegArea = vSum(thisYearRegAreas);
      double ratio = totalRegArea / totalHarvestedArea ;
      if(rescaleFrequencies && (ratio < 0.99999999999 || ratio > 1.00000000001) ) {
       // msgOut(MSG_CRITICAL_ERROR, "Sum of regeneration areas not equal to sum of harvested area in runManagementModel()!");
      }
      #endif
      px->regArea.insert(pair <int, vector<double> > (MTHREAD->SCD->getYear(), thisYearRegAreas));
    } // end of each pixel
    if (-fArea_diff > regHArea){
      msgOut(MSG_WARNING,"In region "+ i2s(regId) + " the exogenous area decrement ("+ d2s(-fArea_diff) +" ha) is higger than the harvesting ("+ d2s(regHArea) +" ha). Ratio forced to 1.");
    }

  } // end of each region
}

void
ModelCoreSpatial::cacheSettings(){
  msgOut(MSG_INFO, "Cashing initial model settings..");
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
  natRegAllocation = MTHREAD->MD->getStringSetting("natRegAllocation"); // how to allocate natural regeneration
  rescaleFrequencies = MD->getBoolSetting("rescaleFrequencies");
  oldVol2AreaMethod = MD->getBoolSetting("oldVol2AreaMethod");
  mr = MD->getDoubleSetting("mr");
  forestAreaChangeMethod =  MTHREAD->MD->getStringSetting("forestAreaChangeMethod");
  ir = MD->getDoubleSetting("ir");


}

void
ModelCoreSpatial::initializePixelVolumes(){
  msgOut(MSG_INFO, "Starting initializing pixel-level values");

  // pxVol = regVol * pxArea/regForArea
  // this function can be done only at the beginning of the model, as it assume that the distribution of volumes by diameter class in the pixels within a certain region is homogeneous, but as the model progress along the time dimension this is no longer true.
  if(!MD->getBoolSetting("usePixelData")) return;
  for(uint i=0;i<regIds2.size();i++){
    ModelRegion* reg = MD->getRegion(regIds2[i]);
    vector <Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regIds2[i]);
    for (uint j=0;j<rpx.size();j++){
      int debugPx = rpx[j]->getID();
      int debug2 = debugPx;
      rpx[j]->vol.clear(); // not actually necessary
      for(uint y=0;y<fTypes.size();y++){
        vector <double> vol_byu;
        double regForArea = reg->getValue("forArea_"+fTypes[y]);
        for (uint z=0;z<dClasses.size();z++){
          double regVol;
          regVol = z ? gfd("vol",regIds2[i],fTypes[y],dClasses[z],firstYear) : 0 ; // if z=0-> regVol= gfd(), otherwise regVol=0;
          double pxArea = rpx[j]->getDoubleValue("forArea_"+fTypes[y], true); // bug solved 20121109. get zero for not data
          if (pxArea<0.0){
            msgOut(MSG_CRITICAL_ERROR,"Error in initializePixelVolumes, negative pxArea!");
          }
          double pxVol = regForArea ? regVol * pxArea/regForArea: 0; // if we introduce new forest types without initial area we must avoid a 0/0 division
          //rpx[j]->changeValue(pxVol,"vol",fTypes[y],dClasses[z],firstYear);
          vol_byu.push_back(pxVol);
        }
        rpx[j]->vol.push_back(vol_byu);
      }
    }
  }
  loadExogenousForestLayers("vol");
}

void
ModelCoreSpatial::initialiseCarbonModule(){

  ///< call initialiseDeathBiomassStocks(), initialiseProductsStocks() and initialiseEmissionCounters()
  MTHREAD->CBAL->initialiseEmissionCounters();

  for(uint i=0;i<regIds2.size();i++){
    vector<double> deathBiomass;
    for(uint j=0;j<fTypes.size();j++){
      double deathBiomass_ft = gfd("vMort",regIds2[i],fTypes[j],DIAM_ALL,DATA_NOW);
      deathBiomass.push_back(deathBiomass_ft);
    }
    MTHREAD->CBAL->initialiseDeathBiomassStocks(deathBiomass, regIds2[i]);
    vector<double>qProducts;
    for(int p=0;p<priProducts.size();p++){
      // for the primary products we consider only the exports as the domestic consumption is entirelly transformed in secondary products
      double int_exports = gpd("sa",regIds2[i],priProducts[p],DATA_NOW);
      qProducts.push_back(int_exports);
    }
    for(int p=0;p<secProducts.size();p++){
      // for the tranformed product we skip those that are imported, hence derived from other forest systems
      double consumption = gpd("dl",regIds2[i],secProducts[p],DATA_NOW); // dl = sl + net regional imports
      qProducts.push_back(consumption);
    }
    MTHREAD->CBAL->initialiseProductsStocks(qProducts, regIds2[i]);

  }
}

/**
 * @brief ModelCoreSpatial::initializePixelArea
 *
 * This function compute the initial area by ft and dc. It requires vHa computed in computeCumulativeData, this is why it is
 * separated form the other initialisedPixelValues().
 * As the sum of area computed using vHa may differ from the one memorised in forArea_* layer, all values are scaled to match
 * it before being memorised.
 * Also assign area = area_l
 */

void
ModelCoreSpatial::initializePixelArea(){
  msgOut(MSG_INFO, "Starting initializing pixel-level area");
  if(!MD->getBoolSetting("usePixelData")) return;
  for(uint i=0;i<regIds2.size();i++){
    ModelRegion* reg = MD->getRegion(regIds2[i]);
    vector <Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regIds2[i]);
    for (uint p=0;p<rpx.size();p++){
      Pixel* px = rpx[p];
      double pxid= px->getID();
      for(uint j=0;j<fTypes.size();j++){
        string ft = fTypes[j];
        vector <double> tempAreas;
        vector <double> areasByFt;
        double pxArea = px->getDoubleValue("forArea_"+ft,true)/10000.0; //ha
        for (uint u=0;u<dClasses.size();u++){
          if(u==0){
            double regionArea = reg->getValue("forArea_"+ft,OP_SUM)/10000.0; //ha
            double regRegVolumes = gfd("vReg",regIds2[i],ft,""); // regional regeneration volumes.. ugly name !!
            double newVReg = regionArea ? regRegVolumes*pxArea/regionArea : 0.0;
            double tp_u0 = px->tp.at(j).at(0); // time of passage to reach the first production diameter class
            double entryVolHa = gfd("entryVolHa",regIds2[i],ft,"");
            double tempArea = (newVReg*1000000.0/entryVolHa)*tp_u0;
            tempAreas.push_back(tempArea);
          } else {
            string dc = dClasses[u];
            double dcVol = px->vol_l.at(j).at(u)*1000000.0; // m^3
            double dcVHa = px->vHa.at(j).at(u); // m^3/ha
            #ifdef QT_DEBUG
            if(dcVol < 0.0 || dcVHa < 0.0){
                msgOut(MSG_CRITICAL_ERROR, "Negative volumes or density in initializePixelArea");
            }
            #endif
            double tempArea = dcVHa?dcVol/dcVHa:0;
            tempAreas.push_back(tempArea);
          }

        } // end dc
        double sumTempArea = vSum(tempAreas);
       // double sharedc0 = 5.0/90.0; // an arbitrary share of total area allocated to first diameter class
        //tempAreas.at(0) = sumTempArea * sharedc0;
        //sumTempArea = vSum(tempAreas);
        double normCoef = sumTempArea?pxArea/ sumTempArea:0;
        //cout << i << '\t' << pxid << '\t' << ft << '\t' << normCoef << endl;
        #ifdef QT_DEBUG
        if(normCoef < 0.0){
            msgOut(MSG_CRITICAL_ERROR, "Negative normCoef in initializePixelArea");
        }
        #endif
        for (uint u=0;u<dClasses.size();u++){
          areasByFt.push_back(tempAreas.at(u)*normCoef); //manca la costruzione originale del vettore
        }
        #ifdef QT_DEBUG
        if (pxArea != 0.0){
            double ratio = vSum(areasByFt)/ pxArea;  // vSum(areasByFt) should be equal to pxArea
            if(ratio < 0.99999999999 || ratio > 1.00000000001) {
              msgOut(MSG_CRITICAL_ERROR, "pxArea is not equal to vSum(areasByFt) in initializePixelArea");
            }
        }
        #endif
        px->area_l.push_back(areasByFt);
        /// \todo here I have finally also area_ft_dc_px and I can implement the new one I am in 2006
      } // end ft
      px->area = px->area_l;  //Assigning initial value of area to the area of the old year
    } // end px
  } // end region
  loadExogenousForestLayers("area");
}

void
ModelCoreSpatial::computeCumulativeData(){

  msgOut(MSG_INFO, "Starting computing some cumulative values..");
  int thisYear     =  MTHREAD->SCD->getYear();

//    double sumCumTP=0;
//    double sumVHa = 0;
//    double count = 0;
//    double avg_sumCumTp;
//    double avg_sumVHa;

  for(uint r2= 0; r2<regIds2.size();r2++){
    int regId = regIds2[r2];
    regPx = MTHREAD->MD->getRegion(regId)->getMyPixels();

    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];
      px->cumTp.clear();
      px->cumTp_exp.clear();
      px->vHa_exp.clear();
      px->vHa.clear();
      px->cumAlive.clear();
      px->cumAlive_exp.clear();
      double expType = px->expType;

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
        vector <double> cumTp_temp;     // cumulative time of passage to REACH a diameter class (tp is to LEAVE to the next one)
        vector <double> vHa_temp;       // volume at hectar by each diameter class [m^3/ha]
        vector <double> cumAlive_temp;     // cumulated alive rate to reach a given diameter class
        vector <double> cumTp_exp_temp; // expected version of cumTp_temp
        vector <double> vHa_exp_temp;   // expected version of vHa_temp
        vector <double> cumAlive_exp_temp; // "expected" version of cumMort

        MD->setErrorLevel(MSG_NO_MSG); // as otherwise on 2007 otherwise sfd() will complain that is filling multiple years (2006 and 2007)
        for (uint u=0; u<dClasses.size(); u++){
          string dc = dClasses[u];
          double cumTp_u, cumTp_u_exp, cumTp_u_noExp, cumTp_u_fullExp;
          double tp, tp_exp, tp_noExp, tp_fullExp;
          double vHa_u, vHa_u_exp, vHa_u_noExp, vHa_u_fullExp, beta, beta_exp, beta_noExp, beta_fullExp, mort, mort_exp, mort_noExp, mort_fullExp;
          double cumAlive_u, cumAlive_exp_u;

          // only cumTp is depending for the expectations, as it is what it is used by owner to calculate return of investments.
          // the tp, beta and mort coefficients instead are the "real" ones as predicted by scientist for that specific time

          if(u==0) {
            // first diameter class.. expected  and real values are the same (0)
            cumTp_u = 0.;
            vHa_u   = 0.;
            cumAlive_u = 1.;
            cumTp_temp.push_back(cumTp_u);
            vHa_temp.push_back(vHa_u);
            cumTp_exp_temp.push_back(cumTp_u);
            vHa_exp_temp.push_back(vHa_u);
            cumAlive_temp.push_back(cumAlive_u);
            cumAlive_exp_temp.push_back(cumAlive_u);
          } else {
            // other diameter classes.. first dealing with real values and then with expected ones..
            // real values..
            // real values..
            tp = gfd("tp",regId,ft,dClasses[u-1],thisYear)*tp_multiplier_now;
            cumTp_u = cumTp_temp[u-1] + tp;
            if (u==1){
              /**
              Note on the effect of mortality modifiers on the entryVolHa.
              Unfortunatly for how it is defined the mortality multiplier (the ratio with the new mortality rate over the old one) we can't
              compute a entryVolHa based on it. It is NOT infact just like: vHa_adjusted = vHa_orig / mort_multiplier.
              The effect of mortality on the vHa of the first diameter class is unknow, and so we can't compute the effect of a relative
              increase.
              */
              vHa_u = gfd("entryVolHa",regId,ft,"",thisYear);
              mort = 0.; // not info about mortality first diameter class ("00")
            } else {
              mort = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1],thisYear)*mortCoef_multiplier_now,tp); // mortality of the previous diameter class
              beta = gfd("betaCoef",regId,ft,dc, thisYear)*betaCoef_multiplier_now;
              vHa_u = vHa_temp[u-1]*beta*(1-mort);
            }
            cumAlive_u = max(0.,cumAlive_temp[u-1]*(1-mort));
            cumAlive_temp.push_back(cumAlive_u);
            cumTp_temp.push_back(cumTp_u);
            vHa_temp.push_back(vHa_u);
            // expected values..
            /**
            param expType Specify how the forest owners (those that make the investments) behave will be the time of passage in the future in order to calculate the cumulative time of passage in turn used to discount future revenues.
            Will forest owners behave adaptively believing the time of passage between diameter classes will be like the observed one at time they make decision (0) or they will have full expectations believing forecasts (1) or something in the middle ?
            For compatibility with the GAMS code, a -1 value means using initial simulation tp values (fixed cumTp)."
            */
            if (expType == -1){
              tp_exp = gfd("tp",regId,ft,dClasses[u-1],firstYear)*tp_multiplier_t0;
              //tp = px->tp.at(u); no. not possible, tp stored at pixel level is the current year one
              cumTp_u_exp = cumTp_exp_temp[u-1]+tp_exp;
              cumTp_exp_temp.push_back(cumTp_u_exp);
              if(u==1) {
                vHa_u_exp = gfd("entryVolHa",regId,ft,"",firstYear);
                mort_exp = 0.; // not info about mortality first diameter class ("00")
              } else {
                mort_exp = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1], firstYear)*mortCoef_multiplier_t0,tp_exp); // mortality rate of previous diameter class
                beta_exp = gfd("betaCoef",regId,ft,dc, firstYear)*betaCoef_multiplier_t0;
                vHa_u_exp = vHa_exp_temp[u-1]*beta_exp*(1-mort_exp);
              }
            } else {
              double tp_multiplier_dynamic = px->getMultiplier("tp_multiplier",ft,thisYear+ceil(cumTp_exp_temp[u-1]));
              tp_noExp = gfd("tp",regId,ft,dClasses[u-1])*tp_multiplier_now;
              cumTp_u_noExp = cumTp_exp_temp[u-1]+tp_noExp;
              tp_fullExp = gfd("tp",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_exp_temp[u-1]))*tp_multiplier_dynamic ; // time of passage that there should be to reach this diameter class in the year where the previous diameter class will be reached
              cumTp_u_fullExp = cumTp_exp_temp[u-1]+tp_fullExp ; // it adds to the time of passage to reach the previous diameter class the time of passage that there should be to reach this diameter class in the year where the previous diameter class will be reached
              cumTp_u_exp = cumTp_u_fullExp*expType+cumTp_u_noExp*(1-expType); // 20121108: it's math the same as cumTp_exp_temp[u-1] + tp
              cumTp_exp_temp.push_back(cumTp_u_exp);
              if(u==1) {
                vHa_u_noExp   = gfd("entryVolHa",regId,ft,"",DATA_NOW);
                vHa_u_fullExp = gfd("entryVolHa",regId,ft,"",thisYear+ceil(cumTp_u));
                vHa_u_exp = vHa_u_fullExp*expType+vHa_u_noExp*(1-expType);
                mort_exp = 0.; // not info about mortality first diameter class ("00")
              } else {
                mort_noExp = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1], DATA_NOW)*mortCoef_multiplier_now, tp_noExp); // mortCoef is a yearly value. Mort coeff between class is 1-(1-mortCoeff)^tp
                double mortCoef_multiplier_dynamic = px->getMultiplier("mortCoef_multiplier",ft,thisYear+ceil(cumTp_exp_temp[u-1]));
                mort_fullExp = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_exp_temp[u-1]))*mortCoef_multiplier_dynamic,tp_fullExp); // mortality of the previous diameter class
                beta_noExp   = gfd("betaCoef",regId,ft,dc, DATA_NOW)*betaCoef_multiplier_now;
                double betaCoef_multiplier_dynamic = px->getMultiplier("betaCoef_multiplier",ft,thisYear+ceil(cumTp_u));
                beta_fullExp = gfd("betaCoef",regId,ft,dc, thisYear+ceil(cumTp_u))*betaCoef_multiplier_dynamic;
                mort_exp = mort_fullExp*expType+mort_noExp*(1-expType);
                beta_exp = beta_fullExp*expType+beta_noExp*(1-expType);
                vHa_u_exp = vHa_exp_temp[u-1]*beta_exp*(1-mort_exp); // BUG !!! mort is yearly value, not between diameter class. SOLVED 20121108
              }
            }
            vHa_exp_temp.push_back(vHa_u_exp);
            cumAlive_exp_u = max(0.,cumAlive_exp_temp[u-1]*(1-mort_exp));
            cumAlive_exp_temp.push_back(cumAlive_exp_u);

            //cout << "********" << endl;
            //cout << "dc;mort;cumAlive;cumAlive_exp "<< endl ;
            //cout << dClasses[u] << ";"<< mort << ";" << cumAlive_u << ";" << cumAlive_exp_u << endl;

          }
          // debug stuff on vHa
          //double vHa_new = gfd("vHa",regId,ft,dc,DATA_NOW);
          //double hv2fa_old = gfd("hv2fa",regId,ft,dc,DATA_NOW);
          //cout << "Reg|Ft|dc|vHa (new)|1/hv2fa (old):   " << regId << " | " << ft;
          //cout << " | " << dc << " | " << vHa_new << " | " << 1/hv2fa_old  << endl;

        } // end of each diam
        //double pixID = px->getID();
        //cout << thisYear << ";"<< regIds2[r2] << ";" << pixID << ";" << ft << ";" << cumTp_exp_temp[3] << ";" << vHa_exp_temp[3] << endl;
        px->cumTp.push_back(cumTp_temp);
        px->vHa.push_back(vHa_temp);
        px->cumAlive.push_back(cumAlive_temp);
        px->cumTp_exp.push_back(cumTp_exp_temp);
        px->vHa_exp.push_back(vHa_exp_temp);
        px->cumAlive_exp.push_back(cumAlive_exp_temp);

        //sumCumTP += cumTp_exp_temp[3];
        //sumVHa += vHa_exp_temp[3];
        //count ++;


      } // end of each ft
    } // end of each pixel
  } // end of each region
  MD->setErrorLevel(MSG_ERROR);
    //avg_sumCumTp = sumCumTP/ count;
    //avg_sumVHa = sumVHa / count;
    //cout << "Avg sumCumTp_35 and sumVha_35: " << avg_sumCumTp << " and " << avg_sumVHa << " (" << count << ")" << endl;
    //exit(0);
}

void
ModelCoreSpatial::resetPixelValues(){
  msgOut(MSG_INFO, "Starting resetting pixel level values");
  for(uint r2= 0; r2<regIds2.size();r2++){
    int regId = regIds2[r2];
    regPx = MTHREAD->MD->getRegion(regId)->getMyPixels();
    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];
      px->swap(VAR_VOL);  // vol_l = vol
      px->swap(VAR_AREA); // area_l = area
      // 20121108 BUG! Solved, used empty (just return true if the vector is empty) instead of clear (it actually clears the vector)
      px->vol.clear(); // by ft,dc
      px->area = px->area_l; // ATTENCTION, DIFFERENT FROM THE OTHERS. Here it is not cleared, it is assigned the previous year as default
      /*px->area.clear(); // by ft,dc*/
      px->hArea.clear(); // by ft, dc
      //px->regArea.clear(); // by year, ft NO, this one is a map, it doesn't need to be changed
      px->hVol.clear(); // by ft, dc
      px->hVol_byPrd.clear(); // by ft, dc, pp
      //px->in.clear(); // by pp
      //px->hr.clear(); // by pp
      px->vReg.clear(); // by ft
      px->expectedReturns.clear(); // by ft

      px->beta.clear();
      px->mort.clear();
      px->tp.clear();
      px->cumTp.clear();
      px->vHa.clear();
      px->cumTp_exp.clear();
      px->vHa_exp.clear();
      px->cumAlive.clear();
      px->cumAlive_exp.clear();
      px->vMort.clear();
      //std::fill(rpx[j]->vMort.begin(), rpx[j]->vMort.end(), 0.0);

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
          if (u==0){
            // tp of first diameter class not making it change across the time dimension, otherwise problems in getting the rigth past
            // regenerations. BUT good, px->tp.at(0) is used only to pick up the right regeneration, so the remaining of the model
            // uses the getMultiplier version and cumTp consider the dynamic effects also in the first dc.
            tp  = gfd("tp",regId,ft,dClasses[u],firstYear)*px->getMultiplier("tp_multiplier",ft,firstYear); // tp is defined also in the first diameter class, as it is the years to reach the NEXT diameter class
          } else {
            tp  = gfd("tp",regId,ft,dClasses[u],DATA_NOW)*tp_multiplier_now; // tp is defined also in the first diameter class, as it is the years to reach the NEXT diameter class
          }
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
  msgOut(MSG_INFO, "Starting computing inventary available for this year..");
  for(uint i=0;i<regIds2.size();i++){
    int r2 = regIds2[i];
    ModelRegion* REG = MTHREAD->MD->getRegion(r2);
    //Gis* GIS = MTHREAD->GIS;
    regPx = REG->getMyPixels();
    vector <double> in_reg(priProducts.size(),0.); // should have ceated a vector of size priProducts.size(), all filled with zeros
    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];
      //int debugPx = px->getID();
      //int debug2 = debugPx;
      //px->in.clear();
      for(uint pp=0;pp<priProducts.size();pp++){
        double in = 0;
        for(uint ft=0;ft<fTypes.size();ft++){
          for(uint dc=0;dc<dClasses.size();dc++){
            in += app(priProducts[pp],fTypes[ft],dClasses[dc])*px->vol_l.at(ft).at(dc)*px->avalCoef;
          }
        }
        //px->in.push_back(in);
        in_reg.at(pp) += in;
      } // end of each priProduct
    } // end each pixel
    for(uint pp=0;pp<priProducts.size();pp++){
      // Even if I fixed all the lower bounds to zero in Opt::get_bounds_info still the model
      // doesn't solve with no-forest in a region.
      // Even with 0.0001 doesn't solve !!
      // With 0.001 some scenarios doesn't solve in 2093
      // With 0.003 vRegFixed doesn't solve in 2096
      // Tried with 0.2 but no changes, so put it back on 0.003
      //spd(max(0.001,in_reg.at(pp)),"in",r2,priProducts[pp],DATA_NOW,true);
      spd(in_reg.at(pp),"in",r2,priProducts[pp],DATA_NOW,true);
      #ifdef QT_DEBUG
      if (in_reg.at(pp) < -0.0){
        msgOut(MSG_CRITICAL_ERROR,"Negative inventory");
      }
      #endif
    }

    // 20150116: As the same (ft,dc) can be used in more than one product knowing -and bounding the supply in the optimisation- each single
    // in(pp) is NOT enought.
    // We need to bound the supply for each possible combination, that is for 2^(number of prim.pr)
    // Here we compute the detailed inventory. TODO: Create the pounds in Opt.

    int nbounds = pow(2,priProducts.size());

    for (uint i=0; i<nbounds; i++){
      vector<int> concernedPriProducts;
      double bound = 0.0;
      for(uint j=0;j<priProducts.size();j++){
        uint j2 = pow(2,j);
        if(i & j2){ // bit a bit operator, p217 C++ book
          concernedPriProducts.push_back(j);
        }
      }
      //concernedPriProducts.push_back(i);
      // for each ft and dc if at least one app with concerning product is 1 we add the volumes
      for(uint ft=0;ft<fTypes.size();ft++){
        for(uint dc=0;dc<dClasses.size();dc++){
          bool addIt = false;
          for(uint z=0;z<concernedPriProducts.size();z++){
            if (app(priProducts[concernedPriProducts[z]],fTypes[ft],dClasses[dc])){
              addIt = true;
              break;
            }
          }
          if (addIt){
            for (uint p=0;p<regPx.size();p++){
              Pixel* px = regPx[p];
              bound += px->vol_l.at(ft).at(dc)*px->avalCoef;
            }
          }
        }
      }
      REG->inResByAnyCombination[i] = bound;
    }
  } // end each region
}

void
ModelCoreSpatial::updateMapAreas(){
  msgOut(MSG_INFO, "Updating map areas..");

  if (!oldVol2AreaMethod){
    if(!MD->getBoolSetting("usePixelData")) return;
    for(uint i=0;i<regIds2.size();i++){
      ModelRegion* reg = MD->getRegion(regIds2[i]);
      vector <Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regIds2[i]);
      for (uint p=0;p<rpx.size();p++){
        Pixel* px = rpx[p];
        double pxid= px->getID();
        for(uint j=0;j<fTypes.size();j++){
          string ft = fTypes[j];
          double forArea = vSum(px->area.at(j));
          #ifdef QT_DEBUG
          if(forArea < 0.0 ){
            msgOut(MSG_CRITICAL_ERROR, "Negative forArea in updateMapAreas");
          }
          #endif
          px->changeValue("forArea_"+ft, forArea*10000);
        } // end ft
      } // end px
    } // end region
  } else {
    int currentYear = MTHREAD->SCD->getYear();
    map<int,double> forestArea; // foresta area by each region
    pair<int,double > forestAreaPair;
    vector<int> l2Regions =  MTHREAD->MD->getRegionIds(2, true);
    vector <string> fTypes =  MTHREAD->MD->getForTypeIds();
    int nFTypes = fTypes.size();
    int nL2Regions = l2Regions.size();
    for(int i=0;i<nL2Regions;i++){
      int regId = l2Regions[i];
      vector<Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regId);
      for(int j=0;j<nFTypes;j++){
        string ft = fTypes[j];
        //double regForArea = reg->getValue("forArea_"+ft);
        //double harvestedArea = gfd("harvestedArea",regId,ft,DIAM_ALL);
        //double regArea = gfd("regArea",regId,ft,DIAM_ALL);
        //cout << "Regid/ft/area/harvested/regeneration: " <<regId<<";"<<ft<<";"<<regForArea<<";"<<harvestedArea<<";" <<regArea<<endl;
        //double newAreaNet = regArea-harvestedArea;
        //double newAreaRatio =  newAreaNet / regForArea;
        for(uint z=0;z<rpx.size();z++){
          Pixel* px = rpx[z];
          double oldValue = px->getDoubleValue("forArea_"+ft,true)/10000;
          double hArea = vSum(px->hArea.at(j));              //bug 20140205 areas in the model are in ha, in the layer in m^2
          double regArea = findMap(px->regArea,currentYear).at(j); //bug 20140205 areas in the model are in ha, in the layer in m^2
          //double newValue = oldValue*(1. + newAreaRatio);
          double newValue = oldValue-hArea+regArea;
          double areaNetOfRegeneration = oldValue-hArea;
          #ifdef QT_DEBUG
          if (areaNetOfRegeneration<0.0){
            msgOut(MSG_CRITICAL_ERROR,"areaNetOfRegeneration negative in updateMapAreas");
          }
          if (newValue<0.0){
            msgOut(MSG_CRITICAL_ERROR,"for area negative in updateMapAreas");
          }
          #endif
          rpx[z]->changeValue("forArea_"+ft, newValue*10000);
        }
      }
    }
  }
}

void
ModelCoreSpatial::sumRegionalForData(){

  msgOut(MSG_INFO, "Summing data pixels->region..");
  //vector <string> outForVariables  = MTHREAD->MD->getStringVectorSetting("outForVariables");
  int currentYear = MTHREAD->SCD->getYear();
  for(uint r2= 0; r2<regIds2.size();r2++){
    int regId = regIds2[r2];
    regPx = MTHREAD->MD->getRegion(regId)->getMyPixels();
    for(uint j=0;j<fTypes.size();j++){
      string ft = fTypes[j];

      double regArea = 0.;
      double sumExpReturns = 0.;
      double sumArea = 0.;
      double pxForArea = 0.;
      double vReg = 0.;

      for (uint u=0; u<dClasses.size(); u++){
        string dc = dClasses[u];
        double vol =0.;
        double hV = 0.;
        double hArea = 0.;
        double vMort = 0.;
        for (uint p=0;p<regPx.size();p++){
          Pixel* px = regPx[p];
          vol += px->vol.at(j).at(u);
          hV += px->hVol.at(j).at(u);
          hArea += px->hArea.at(j).at(u);
          vMort += px->vMort.at(j).at(u);
        }
        if(u){
          sfd(vol,"vol",regId,ft,dc,DATA_NOW);
          sfd(hV,"hV",regId,ft,dc,DATA_NOW,true);
          sfd(hArea,"harvestedArea",regId,ft,dc,DATA_NOW, true);
          sfd(vMort,"vMort",regId,ft,dc,DATA_NOW,true);
          double vol_1 = gfd("vol",regId,ft,dc,currentYear-1);
          if(vol_1){
            sfd(hV/vol_1,"hr",regId,ft,dc,DATA_NOW, true);
          } else {
            sfd(0.,"hr",regId,ft,dc,DATA_NOW, true);
          }

        }
      }

      for (uint p=0;p<regPx.size();p++){
        Pixel* px = regPx[p];
        int debug_pxid = px->getID();
        vReg += px->vReg.at(j);
        regArea += findMap(px->regArea,currentYear).at(j);
        pxForArea = (px->getDoubleValue("forArea_"+ft,true)/10000);
        sumExpReturns += (px->expectedReturns.at(j)* pxForArea);
        sumArea += pxForArea;
        double debug1 = sumArea;
        if(! (sumArea >= 0.0) ){
          msgOut(MSG_CRITICAL_ERROR,"sumArea is not non negative.");
        }
      }
      sfd(vReg,"vReg",regId,ft,"",DATA_NOW, true);
      sfd(regArea,"regArea",regId,ft,"",DATA_NOW, true);
      sfd(sumExpReturns/sumArea,"expReturns",regId,ft,"",DATA_NOW, true);
      sfd(sumExpReturns,"sumExpReturns",regId,ft,"",DATA_NOW, true);
      sfd(sumArea,"forArea",regId,ft,"",DATA_NOW, true);
    } // end of for each ft

    double totRegExpReturn = 0.0;
    double totRegForArea = 0.0;

    for (uint p=0;p<regPx.size();p++){
    Pixel* px = regPx[p];
    double totPxForArea = vSum(px->area);

#ifdef QT_DEBUG
    double totPxForArea_debug = 0.0;
    for(uint j=0;j<fTypes.size();j++){
      string ft = fTypes[j];
      totPxForArea_debug += (px->getDoubleValue("forArea_"+ft,true)/10000);
    }

    if ( (totPxForArea - totPxForArea_debug) > 0.0001 || (totPxForArea - totPxForArea_debug) < -0.0001 ){
      cout << "*** ERROR: area discrepance in pixel " << px->getID() << " of " << (totPxForArea - totPxForArea_debug) << " ha!" << endl;
      msgOut(MSG_CRITICAL_ERROR,"Total forest area in pixel do not coincide if token from layer forArea or (pixel) vector area!");
    }
#endif

      double bestExpectedReturn = *max_element(px-> expectedReturnsNotCorrByRa.begin(), px->expectedReturnsNotCorrByRa.end()); /// Attenction, reported expReturns at "forest" level (compared with those at forest type level) do NOT include ra
      double totPxExpReturn = bestExpectedReturn * totPxForArea;
      totRegExpReturn += totPxExpReturn;
      totRegForArea += totPxForArea;
    } // end of each pixel
    sfd(totRegExpReturn/totRegForArea,"expReturns",regId,"","",DATA_NOW, true);
    sfd(totRegExpReturn,"sumExpReturns",regId,"","",DATA_NOW, true);
  }

}
/**
 * This function call registerHarvesting() (accounts for emissions from for. operations), registerDeathBiomass() (registers new stocks of death biomass),
 * registerProducts() (registers new stock of products) and registerTransports() (accounts for emissions from transportation).
 *
 * It pass to registerProducts():
 * - for primary products, the primary products exported out of the country, but not those exported to other regions or used in the region as
 *   these are assumed to be totally transformed to secondary products;
 * - for secondary products, those produced in the region from locally or regionally imported primary product plus those secondary products
 *   imported from other regions, less those exported to other regions. It doesn't include the secondary products imported from abroad the country.
 */
void
ModelCoreSpatial::registerCarbonEvents(){

  //void                  registerHarvesting(const int & regId, const string & fType, const double & value); ///< register the harvesting of trees -> cumEmittedForOper
  //void                  registerDeathBiomass(const double &value, const int & regId, const string &fType);
  //void                  registerProducts(const double &value, const int & regId, const string &productName);
  //void                  registerTransports(const double &distQ, const int & regId);

  for(uint i=0;i<regIds2.size();i++){
    for(uint j=0;j<fTypes.size();j++){
      double deathBiomass = gfd("vMort",regIds2[i],fTypes[j],DIAM_ALL,DATA_NOW);
      double harvesting = gfd("hV",regIds2[i],fTypes[j],DIAM_ALL,DATA_NOW);
      MTHREAD->CBAL->registerDeathBiomass(deathBiomass, regIds2[i], fTypes[j]);  // register new stock
      MTHREAD->CBAL->registerHarvesting(harvesting, regIds2[i], fTypes[j]);      // account for emissions. Added 201500715: it also moves the extra biomass to the death biomass pool
    }

    for(uint p=0;p<priProducts.size();p++){
      // for the primary products we consider only the exports as the domestic consumption is entirelly transformed in secondary products
      double int_exports = gpd("sa",regIds2[i],priProducts[p],DATA_NOW);
      MTHREAD->CBAL->registerProducts(int_exports, regIds2[i], priProducts[p]);  // register new stock
    }
    for(uint p=0;p<secProducts.size();p++){
      // for the tranformed product we skip those that are imported, hence derived from other forest systems
      // but we consider those coming from other regions
      double consumption = gpd("dl",regIds2[i],secProducts[p],DATA_NOW); // dl = sl + net regional imports
      MTHREAD->CBAL->registerProducts(consumption, regIds2[i], secProducts[p]); // register new stock
    }

  }
  for (uint r1=0;r1<l2r.size();r1++){
    for (uint r2=0;r2<l2r[r1].size();r2++){
      int rfrom= l2r[r1][r2];
      double distQProd = 0.0;
      for (uint r3=0;r3<l2r[r1].size();r3++){
        int rto = l2r[r1][r3];
        double dist = gpd("dist",rfrom,"",DATA_NOW,i2s(rto)); //km
        for(uint p=0;p<allProducts.size();p++){
          distQProd += dist*gpd("rt",rfrom,allProducts[p],DATA_NOW,i2s(rto)); //km*Mm^3
        }
      }
      MTHREAD->CBAL->registerTransports(distQProd, rfrom);
    }
  }
  MTHREAD->CBAL->HWP_eol2energy(); // used to compute the energy substitution from hwp that reach the end of life and doesn't go to landfil. Previously the energy substitution was computed in registerProducts(), that is at the time when the product was produced.

}

/**
 * Compute the expectation weighted price based on the ratio of the international (world) price between the future and now.
 *
 * @param curLocPrice The local current price
 * @param worldCurPrice The world current price
 * @param worldFutPrice The world future price
 * @param sl Supply local
 * @param sa Supply abroad
 * @param expCoef The expectation coefficient for prices for the agent [0,1]
 * @return The expType-averaged local (or weighter) price
 */
double
ModelCoreSpatial::computeExpectedPrice(const double & curLocPrice, const double & worldCurPrice, const double & worldFutPrice, const double & sl, const double & sa, const double & expCoef){
  double fullExpWPrice = (curLocPrice*(worldFutPrice/worldCurPrice)*sl+worldFutPrice*sa)/(sa+sl);
  double curWPrice = (curLocPrice*sl+worldCurPrice*sa)/(sl+sa);
  return curWPrice * (1-expCoef) + fullExpWPrice * expCoef;
}

/**
 * It uses data from tables initSpVols and initSpAreas to "move" either volumes or area from one forest type to the other.
 * It acts on the pix->vol and pix->area vectors, and it doesn't cash the results on the other data containers, that is the
 * forDataMAp and, for areas, the px->values "layer" map.
 *
 * It is called in initializePixelVolumes() and initializePixelAreas().
 *
 */
void
ModelCoreSpatial::loadExogenousForestLayers(const string & what){
  if(!MD->getBoolSetting("useSpExplicitForestTypes")) return;
  LLData table(MTHREAD,"");
  if(what=="vol"){
    table = MD->getTable(MD->getStringSetting("spExplicitForTypesVols"), MSG_CRITICAL_ERROR);
  } else if(what=="area"){
    table = MD->getTable(MD->getStringSetting("spExplicitForTypesAreas"), MSG_CRITICAL_ERROR);
  } else {
    msgOut(MSG_CRITICAL_ERROR, "Unknown what par in "+ ( (string) __func__ ) +".");
  }
  int nheaders = table.nheaders();
  for(int i=0; i< table.nrecords(); i++){
    int pixId = s2i(table.getData(i,"pxId"));
    Pixel* px = MTHREAD->GIS->getPixel(pixId);
    string ftInString = table.getData(i,"ftIn");
    string ftToString = table.getData(i,"ftTo");
    int ftIn = MD->getForTypeCounter(ftInString);
    int ftTo = MD->getForTypeCounter(ftToString);
    vector<string> dc = MD->getDiameterClasses();
    vector<double> vols_by_dc;
    if (what == "vol"){
      for (int u=0;u<dc.size();u++){
        double newvol = s2d(table.getData(i,"value_"+i2s(u)));
        px->vol.at(ftIn).at(u) = max(0.0,px->vol.at(ftIn).at(u)-newvol);
        px->vol.at(ftTo).at(u) = newvol;
      }
    } else {
      for (int u=0;u<dc.size();u++){
        double newarea = s2d(table.getData(i,"value_"+i2s(u)));
        px->area_l.at(ftIn).at(u) = max(0.0,px->area_l.at(ftIn).at(u)-newarea);
        px->area_l.at(ftTo).at(u) = newarea;
      }
      px->area = px->area_l;
    }
  }
}

