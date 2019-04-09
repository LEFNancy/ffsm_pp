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
#include <tr1/array>


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
  cacheSettings();                ///< cashe things like first year, second year, dClasses...
  cacheDynamicSettings();      // cache settings that may change every year
  initializePixelVolumes();       ///< compute px volumes vol for 2005 (including exogenous loaded volumes)
  assignSpMultiplierPropToVols(); // assign the spatial multiplier (used in the time of return) based no more on a Normal distribution but on the volumes present in the pixel: more volume, more the pixel is fit for the ft
  initMarketModule();             ///< inside it uses first year, second year
  initialiseDeathTimber();
  MTHREAD->DO->print(true);
  MTHREAD->SCD->advanceYear();    ///< 2005->2006
  int thisYear = MTHREAD->SCD->getYear(); // for debugging
  resetPixelValues();             ///< swap volumes->lagged_volumes and reset the other pixel vectors
  cachePixelExogenousData();      ///< compute pixel tp, meta and mort
  computeInventary();             ///< in=f(vol_t-1)
  //printDebugInitRegionalValues();
  computeCumulativeData();        ///< compute cumTp_exp, vHa_exp, vHa
  initializePixelArea();          ///< compute px->area for each ft and dc (including exogenous loaded areas)
  runBiologicalModule();
  runManagementModule();
  updateMapAreas();               ///< update the forArea_{ft} layer on each pixel as old value-hArea+regArea
  updateOtherMapData();           ///< update (if the layer exists) other gis-based data, as volumes and expected returns, taking them from the data in the px object
  sumRegionalForData();           ///< only for printing stats as forest data is never used at regional level
  initialiseCarbonModule();
  //MTHREAD->DO->printDebugPixelValues(); // uncomment to enable pixel-level debugging // moved to output->print

  MTHREAD->DO->print();
}

void
ModelCoreSpatial::runSimulationYear(){
  int thisYear = MTHREAD->SCD->getYear(); // for debugging
  cacheDynamicSettings();      // cache settings that may change every year
  resetPixelValues();          // swap volumes->lagged_volumes and reset the other pixel vectors
  cachePixelExogenousData();   // compute pixel tp, meta and mort
  computeInventary();          // in=f(vol_t-1)
  runMarketModule();           // RUN THE MARKET OPTIMISATION HERE
  computeCumulativeData();     // compute cumTp_exp, vHa_exp
  cachePixelExogenousData();
  runBiologicalModule();
  runManagementModule();
  //MTHREAD->DO->printDebugPixelValues(); // uncomment to enable pixel-level debugging // moved to output->print
  updateMapAreas();
  updateOtherMapData();        // update (if the layer exists) other gis-based data, as volumes and expected returns, taking them from the data in the px object
  sumRegionalForData();        // only for printing stats as forest data is never used at regional level
  registerCarbonEvents();
  computeEconomicBalances();   // policy costs and welfare analysis
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
      value += (gpd("m",r2,secProducts[sp],secondYear)* gpd("pol_trSub",r2,secProducts[sp],secondYear));
      spd(value,"pl",r2,secProducts[sp],secondYear,true);
    }
    // RPAR('dl',i,p_pr,t-1)   =  sum(p_tr, a(p_pr,p_tr)*RPAR('sl',i,p_tr,t-1));
    for (uint pp=0;pp<priProducts.size();pp++){
      double value=0;
      for(uint sp=0;sp<secProducts.size();sp++){
        value += gpd("sl",r2,secProducts[sp],secondYear)*
        gpd("a",r2,priProducts[pp],secondYear, secProducts[sp]);
      }
      spd(value,"dl",r2,priProducts[pp],secondYear,true);
      double stvalue =   gpd("sl",r2,priProducts[pp],secondYear)
                       + gpd("sa",r2,priProducts[pp],secondYear);
      spd(stvalue,"st",r2,priProducts[pp],secondYear,true);
      double supply_fromForestShare = gpd("supply_fromForestShare",r2,priProducts[pp],secondYear);
      spd(stvalue*supply_fromForestShare,"st_fromHarv",r2,priProducts[pp],secondYear,true); // needed for the biological module in year starty+1
    }
    // RPAR('st',i,prd,t-1)    =  RPAR('sl',i,prd,t-1)+RPAR('sa',i,prd,t-1);
    // RPAR('dt',i,prd,t-1)    =  RPAR('dl',i,prd,t-1)+RPAR('da',i,prd,t-1);
    for (uint ap=0;ap<allProducts.size();ap++){
      //double debug = gpd("dl",r2,allProducts[ap],secondYear);
      double dtvalue =   gpd("dl",r2,allProducts[ap],secondYear)
                       + gpd("da",r2,allProducts[ap],secondYear);

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
      double sa  = gpd("sa",r2,secProducts[sp],secondYear);
      double sl  = gpd("sl",r2,secProducts[sp],secondYear);
      double k1  = gpd("k1",r2,secProducts[sp],secondYear);
      double pWo = gpd("pl",WL2,secProducts[sp],secondYear); // World price (local price for region 99999)

      double stvalue =   sl+sa;
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

      spd(stvalue,"st",r2,secProducts[sp],secondYear,true);
      spd(q1,"q1",r2,secProducts[sp],firstYear,true);
      //spd(p1,"p1",r2,secProducts[sp],firstYear,true);
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
     //spd(r1,"r1",r2,priProducts[pp],firstYear,true);
      spd(sc,"sc",r2,priProducts[pp],secondYear,true); 
      spd(pc,"pc",r2,priProducts[pp],secondYear,true);
      spd(pw,"pw",r2,priProducts[pp],secondYear,true);
    }

    // up to here tested with gams output on 20120628, that's fine !!
  } // end for each region in level 2


  // initializing the exports to zero quantities
  // initializing of the transport cost for the same region to one and distance to zero
  for(uint r1=0;r1<l2r.size();r1++){
    for(uint r2=0;r2<l2r[r1].size();r2++){
      for(uint p=0;p<allProducts.size();p++){
        for(uint r2To=0;r2To<l2r[r1].size();r2To++){
          spd(0,"rt",l2r[r1][r2],allProducts[p],secondYear,true,i2s(l2r[r1][r2To]));  // regional trade, it was exp in gams
          if(l2r[r1][r2] == l2r[r1][r2To]){
            spd(1,"ct",l2r[r1][r2],allProducts[p],firstYear,true,i2s(l2r[r1][r2To])); // as long this value is higher than zero, rt within the same region is not choosen by the solver, so the value doesn't really matters. If it is zero, the solver still works and results are the same, but reported rt within the region are crazy high (100000)
          }
        }
      } // end each product

      for(uint r2To=0;r2To<l2r[r1].size();r2To++){
        if(l2r[r1][r2] == l2r[r1][r2To]){
          spd(0,"dist",l2r[r1][r2],"",firstYear,true,i2s(l2r[r1][r2To])); // setting distance zero in code, so no need to put it in the data
        }
      }
    } // end of r2 regions
  } // end of r1 region
}

void
ModelCoreSpatial::runMarketModule(){
  msgOut(MSG_INFO, "Starting market module");
  static double cumOverHarvesting = 0.0;
  int thisYear     =  MTHREAD->SCD->getYear();
  int previousYear = MTHREAD->SCD->getYear()-1;

  // Added for carbon poject -------------------------------
  double intRate = MTHREAD->MD->getDoubleSetting("ir");

  // ----------------------------------------------------------------

  // *** PRE-OPTIMISATION YEARLY OPERATIONS..
  for(uint i=0;i<regIds2.size();i++){
    int r2 = regIds2[i];
    for(uint sp=0;sp<secProducts.size();sp++){
      double g1      = gpd("g1",r2,secProducts[sp],previousYear);
      double sigma   = gpd("sigma",r2,secProducts[sp]);
      double pc_1    = gpd("pc",r2,secProducts[sp],previousYear);
      double dc_1    = gpd("dc",r2,secProducts[sp],previousYear);
      double k_1     = gpd("k",r2,secProducts[sp],previousYear);
      double pol_mktDirInt_d   = gpd("pol_mktDirInt_d",r2,secProducts[sp]);
      double pol_mktDirInt_d_1 = gpd("pol_mktDirInt_d",r2,secProducts[sp],previousYear);
      double q1                = gpd("q1",r2,secProducts[sp]);
      double pol_mktStr_d      = gpd("pol_mktStr_d",r2,secProducts[sp]);

      double q1_polCorr        = min(1.0, (q1-0.5)*pol_mktStr_d+0.5); // The pol_mktStr_s coefficient makes t1 (the "b" weight in the CES function) converge toward 0.5 as pol_mktStr_s goes to 0. As pol_mktStr_s can be above 1, an upper cap of unity is imposed to t1



      double k  = (1+g1)*k_1;
      double aa = (sigma/(sigma+1))*pc_1*pow(dc_1,-1/sigma) * pol_mktDirInt_d_1/pol_mktDirInt_d ;
      double gg = dc_1*pow(pc_1*pol_mktDirInt_d_1,-sigma); //alpha

      spd(k, "k" ,r2,secProducts[sp]);
      spd(aa,"aa",r2,secProducts[sp],DATA_NOW,true);
      spd(gg,"gg",r2,secProducts[sp],DATA_NOW,true);
      spd(q1_polCorr,"q1_polCorr",r2,secProducts[sp],DATA_NOW,true);

    }

    // BB(i,p_pr)   =   (sigma(p_pr)/(sigma(p_pr)+1))*RPAR('pc',i,p_pr,t-1)*(RPAR('sc',i,p_pr,t-1)**(-1/sigma(p_pr)))*(In(i,p_pr,t-1)/In(i,p_pr,t))**(gamma(p_pr)/sigma(p_pr));
    // FF(i,p_pr)   =   RPAR('sc',i,p_pr,t-1)*((RPAR('pc',i,p_pr,t-1))**(-sigma(p_pr)))*(In(i,p_pr,t)/In(i,p_pr,t-1))**(gamma(p_pr)); //chi
    for(uint pp=0;pp<priProducts.size();pp++){
      double gamma_incr        = gpd("gamma_incr",r2,priProducts[pp]); // elast supply to increasing stocks
      double gamma_decr        = gpd("gamma_decr",r2,priProducts[pp]); // elast supply to decreasing stocks
      double sigma             = gpd("sigma",r2,priProducts[pp]); // elast supply to price
      double pc_1              = gpd("pc",r2,priProducts[pp],previousYear);
      double sc_1              = gpd("sc",r2,priProducts[pp],previousYear);
      double in                = gpd("in",r2,priProducts[pp])+gpd("in_deathTimber",r2,priProducts[pp]);
      double in_1              = gpd("in",r2,priProducts[pp],previousYear)+gpd("in_deathTimber",r2,priProducts[pp],previousYear);
      double pol_mktDirInt_s   = gpd("pol_mktDirInt_s",r2,priProducts[pp]);
      double pol_mktDirInt_s_1 = gpd("pol_mktDirInt_s",r2,priProducts[pp],previousYear);
      double t1                = gpd("t1",r2,priProducts[pp]);
      double pol_mktStr_s      = gpd("pol_mktStr_s",r2,priProducts[pp]);


      // Added for carbon project ------------------------------------------
      double carbonPrice_1     = gpd("carbonPrice",r2,"",previousYear); //using dummy region until Anto solves issue // Antonello: solved but also generalised
      double omega             = gpd("co2content_products", r2, priProducts[pp])/1000; //kg to tons
      double Pct_1             = carbonPrice_1*intRate;

      // -------------------------------------------------------------------


      double t1_polCorr        = min(1.0, (t1-0.5)*pol_mktStr_s+0.5); // The pol_mktStr_s coefficient makes t1 (the "b" weight in the CES function) converge toward 0.5 as pol_mktStr_s goes to 0. As pol_mktStr_s can be above 1, an upper cap of unity is imposed to t1


      double gamma = in>in_1 ? gamma_incr: gamma_decr; // If inventories resources are decreasing we use a given elasticity, if they are increasing we use an other one

      double in_ratio =  in/in_1;

      double bb = (sigma/(sigma+1.0))*pc_1*pow(sc_1,-1.0/sigma)*pow(1/in_ratio,gamma/sigma)*pow(1.0,1.0/sigma) * pol_mktDirInt_s_1/pol_mktDirInt_s ;

      /* Old FF (without carbon)
      double ff = sc_1*pow(pc_1*pol_mktDirInt_s_1,-sigma)*pow(in_ratio,gamma); //chi
      */

      // Added for carbon project
      double ff = sc_1*pow((pc_1 - omega * Pct_1)*pol_mktDirInt_s_1,-sigma)*pow(in_ratio,gamma);
      // -----------------------------------------------

      spd(bb,"bb",r2,priProducts[pp],DATA_NOW,true);
      spd(ff,"ff",r2,priProducts[pp],DATA_NOW,true);
      spd(sigma,"sigma",r2,priProducts[pp],DATA_NOW,true);
      spd(t1_polCorr,"t1_polCorr",r2,priProducts[pp],DATA_NOW,true);

    }

  } // end for each region in level 2 (and updating variables)



  // *** OPTIMISATION....

  // Create an instance of the IpoptApplication
  //Opt *OPTa = new Opt(MTHREAD);
  //SmartPtr<TNLP> OPTa = new Opt(MTHREAD);
  SmartPtr<IpoptApplication> application = new IpoptApplication();
  string linearSolver = MTHREAD->MD->getStringSetting("linearSolver",DATA_NOW);
  application->Options()->SetStringValue("linear_solver", linearSolver); // default in ipopt is ma27
  //application->Options()->SetStringValue("hessian_approximation", "limited-memory"); // quasi-newton approximation of the hessian
  //application->Options()->SetIntegerValue("mumps_mem_percent", 100);
  application->Options()->SetNumericValue("obj_scaling_factor", -1); // maximisation
  application->Options()->SetNumericValue("max_cpu_time", 1800); // max 1/2 hour to find the optimus for one single year
  application->Options()->SetStringValue("check_derivatives_for_naninf", "yes");
  //application->Options()->SetStringValue("halt_on_ampl_error", "yes"); // not a valid option
  //application->Options()->SetStringValue("print_options_documentation", "yes");  // print the ipopt options (hundreds of pages!)
  // Relaxing tollerance options... worster effect :-(
  //application->Options()->SetNumericValue("tol", 1e-07);
  //application->Options()->SetNumericValue("constr_viol_tol",0.001);
  //application->Options()->SetNumericValue("compl_inf_tol",0.001);
  //application->Options()->SetNumericValue("acceptable_tol", 1e-05);
  //application->Options()->SetNumericValue("acceptable_dual_inf_tol", 1e+9);
  //application->Options()->SetNumericValue("acceptable_constr_viol_tol", 0.1);
  //application->Options()->SetNumericValue("acceptable_compl_inf_tol", 0.1);


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
    printf("\n*** The problem solved year %d in %d iterations!\n", thisYear, iter_count);
    printf("\n*** The final value of the objective function is %e.\n", final_obj);
    msgOut(MSG_INFO, "The problem solved successfully year "+i2s(thisYear)+" in "+i2s(iter_count)+" iterations.");
    int icount = iter_count;
    double obj = final_obj;
    MTHREAD->DO->printOptLog(true, icount, obj);
  } else {
    //Number final_obj = application->Statistics()->FinalObjective();
    int thisYear = MTHREAD->SCD->getYear();
    cout << "***ERROR: MODEL DIDN'T SOLVE FOR THIS YEAR ("<<thisYear<<")"<< endl;
    msgOut(MSG_CRITICAL_ERROR, "Model DIDN'T SOLVE for this year ("+i2s(thisYear)+")");
    // IMPORTANT! Don't place the next two lines above the msgOut() function or it will crash in windows if the user press the stop button
    //Index iter_count = application->Statistics()->IterationCount(); // syserror if model doesn't solve
    //Number final_obj = application->Statistics()->FinalObjective();
    int icount = 0;
    double obj = 0;
    MTHREAD->DO->printOptLog(false, icount, obj);
  }

  for(uint r2= 0; r2<regIds2.size();r2++){  // you can use r2<=regIds2.size() to try an out-of range memory error that is not detected other than by valgrind (with a message "Invalid read of size 4 in ModelCore::runSimulationYear() in src/ModelCore.cpp:351")
    int regId = regIds2[r2];
    ModelRegion* REG = MTHREAD->MD->getRegion(regId);

    // *** st adjustments:***
    // st_or[pp]             -> original st from the mkt module
    // st_fromFor[pp]     -> cover the volumes from both alive trees and death ones. This should be bounded by inResByAnyCombination, that already includes death resources
    // st_fromHarv[pp] -> from harvesting. This should be equal to hV and IGN data
    // st
    // st_or = st
    // st_fromFor = st_or * a
    // st_fromFor > in:
    //   (overharvesting)
    //   overHarv = st_fromFor - in
    //   st_fromFor = in
    //   st = st_fromFor/a
    // st_fromHarv = distribute(st_fromFor)

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
      spd(st,"st_or",regId,allProducts[p],DATA_NOW,true); // original total supply, not corrected by resetting it to min(st, inv).
      spd(dt,"dt",regId,allProducts[p]);
    }
    for (uint p=0;p<secProducts.size();p++){
      double dl = gpd("dl",regId,secProducts[p]);
      double pl = gpd("pl",regId,secProducts[p]);
      double da = gpd("da",regId,secProducts[p]); // bug corrected 20120913
      double pworld = gpd("pl", WL2,secProducts[p]);
      double dt = gpd("dt",regId,secProducts[p]);
      double pw = dt?(dl*pl+da*pworld)/dt:0.0;
      spd(pw,"pw",regId,secProducts[p]);
    }
    for (uint p=0;p<priProducts.size();p++){
      double sl = gpd("sl",regId,priProducts[p]);
      double pl = gpd("pl",regId,priProducts[p]);
      double sa = gpd("sa",regId,priProducts[p]);  // bug corrected 20120913
      double pworld = gpd("pl", WL2,priProducts[p]);
      double st = gpd("st",regId,priProducts[p]);
      double st_fromFor = st * gpd("supply_fromForestShare",regId,priProducts[p]);
      double pw = st?(sl*pl+sa*pworld)/st:0.0;
      spd(pw,"pw",regId,priProducts[p]);
      spd(st_fromFor,"st_fromFor",regId,priProducts[p],DATA_NOW,true);
    }

    // Correcting st if this is over the in

    // Create a vector with all possible combinations of primary products
    vector<vector<int>> priPrCombs = MTHREAD->MD->createCombinationsVector(priProducts.size());
    int nPriPrCombs = priPrCombs.size();

    for (uint i=0;i<priPrCombs.size();i++){
      double stMkMod = 0.0;
      double sumIn = REG->inResByAnyCombination[i];
     // double sumIn2 = 0.0;
      for (uint p=0;p<priPrCombs[i].size();p++){
        stMkMod += gpd("st_fromFor",regId,priProducts[priPrCombs[i][p]]);
        //sumIn2 += gpd("in",regId,priProducts[priPrCombs[i][p]]);
      }

      //if(sumIn<=0.00001){
     //   for (uint p=0;p<priPrCombs[i].size();p++){
     //     spd(0.0,"st",regId,priProducts[priPrCombs[i][p]]);
     //   }
     // } else {
      if(stMkMod>sumIn){ // if we harvested more than available
        string pProductsInvolved = "";
        for (uint p=0;p<priPrCombs[i].size();p++){
          pProductsInvolved += (priProducts[priPrCombs[i][p]]+"; ");
        }
        double inV_over_hV_ratio = stMkMod ? sumIn/stMkMod : 0.0;
        cumOverHarvesting += (stMkMod-sumIn);
        msgOut(MSG_DEBUG, "Overharvesting has happened. Year: "+i2s(thisYear)+ "Region: "+i2s(regId)+"Involved products:  "+pProductsInvolved+". sumIn: "+d2s(sumIn)+" stMkMod:" +d2s(stMkMod) + " cumOverHarvesting: "+d2s(cumOverHarvesting));
        for (uint p=0;p<priPrCombs[i].size();p++){
          double st_fromFor_orig = gpd("st_fromFor",regId,priProducts[priPrCombs[i][p]]);
          spd(st_fromFor_orig*inV_over_hV_ratio,"st_fromFor",regId,priProducts[priPrCombs[i][p]]);
        }
      }

      //}


    }

    // here we create st_fromHarv as st_fromFor - st_from_deathbiomass
    vector <double> total_st(priProducts.size(),0.);
    vector <double> in_deathTimber(priProducts.size(),0.);
    vector <double> in_aliveForest (priProducts.size(),0.);
    for (uint i=0;i<priProducts.size();i++){
      total_st[i] = gpd("st_fromFor",regId,priProducts[i]);
      in_deathTimber[i] = gpd("in_deathTimber",regId,priProducts[i]);
      in_aliveForest[i] = gpd("in",regId,priProducts[i]);
    }

    vector <double> st_fromHarv= allocateHarvesting(total_st, regId);

    for (uint i=0;i<priProducts.size();i++){
      spd(st_fromHarv[i],"st_fromHarv",regId,priProducts[i],DATA_NOW,true);
    }

  } // end of each region
  if (cumOverHarvesting>0.0){
    msgOut(MSG_DEBUG, "Overharvesting is present. Year: "+i2s(thisYear)+" cumOverHarvesting: "+d2s(cumOverHarvesting));
  }
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
  bool useDeathTimber = MD->getBoolSetting("useDeathTimber",DATA_NOW);

  for(uint i=0;i<regIds2.size();i++){
    int r2 = regIds2[i];
    int regId = r2;
    ModelRegion* REG = MTHREAD->MD->getRegion(r2);
    //Gis* GIS = MTHREAD->GIS;
    regPx = REG->getMyPixels();
    double shareMortalityUsableTimber = 0.0;

    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];

      double pxId = px->getID();
      //if (pxId == 3550.0){
      //    cout << "got the pixel" << endl;
      //}
      //px->expectedReturns.clear();
      for(uint j=0;j<fTypes.size();j++){
        string ft = fTypes[j];
        //double pxArea_debug     = px->getDoubleValue("forArea_"+ft, true);
        vector <double>            hV_byDiam;
        vector <double>            productivity_byDc; // mc/ha/y
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

        if(useDeathTimber){
            shareMortalityUsableTimber = gfd("shareMortalityUsableTimber",r2,ft,"");
        } else {
           shareMortalityUsableTimber = 0.0;
        }

        for (uint u=0; u<dClasses.size(); u++){
          string dc  = dClasses[u];
          double hr  = 0;
          double age = getAvgAgeByDc(px, j, u);

          //double pastYearVol_reg = u ? gfd("vol",r2,ft,dc,thisYear-1): 0;
          double pastYearVol = px->vol_l.at(j).at(u);
          vector <double> hV_byPrd;
          vector <double> hr_byPrd;

          // harvesting rate & volumes...
          // hr is by region.. no reasons in one pixel the RATE of harvesting will be different than in an other pixel
          //hr(u,i,essence,lambda,t) =    sum(p_pr, prov(u,essence,lambda,p_pr)*RPAR('st',i,p_pr,t)/In(i,p_pr,t));
          //hV(u,i,essence,lambda,t) = hr(u,i,essence,lambda,t) * V(u,i,lambda,essence,t-1);
          //hV_byPrd(u,i,essence,lambda,p_pr,t) =  prov(u,essence,lambda,p_pr)*(RPAR('st',i,p_pr,t)/In(i,p_pr,t))*V(u,i,lambda,essence,t-1);
          for(uint pp=0;pp<priProducts.size();pp++){
            double st = gpd("st_fromHarv",r2,priProducts[pp]);
            double in = gpd("in",r2,priProducts[pp]);
            double hr_pr = in ? app(priProducts[pp],ft,dc)*st/in : 0.0;
            hr_byPrd.push_back( hr_pr);
            hr += hr_pr;
          }

          // adjusting for overharvesting..
          // 20160204: inserted to account that we let supply to be marginally higher than in in the mamarket module, to let the solver solving
          double origHr = hr;
          hr = min(1.0,hr);
          for(uint pp=0;pp<priProducts.size();pp++){
            double hr_pr = origHr ? hr_byPrd[pp] * min(1.0,1.0/origHr) : 0.0;
            hV_byPrd.push_back( hr_pr*pastYearVol*px->avalCoef.at(j));
          }

          double hV = hr*pastYearVol*px->avalCoef.at(j);


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

          if(useDeathTimber){
            iisskey key(thisYear,r2,ft,dc);

            MD->deathTimberInventory_incrOrAdd(key,vMort*shareMortalityUsableTimber);
          }

          if(u==0){
            vol = 0.0;
          }else if(u==1){
            vol = max(0.0,(1-1/tp-mort))*pastYearVol+vReg; //Antonello, "bug" fixed 20160203: In case of very strong mortality this quantity (that doesn't include harvesting) could be negative!
            //double debug = vol;
            #ifdef QT_DEBUG
            if ((1-1/tp-mort)<0.0){
              msgOut(MSG_DEBUG,"The sum of leaving trres and mortality would have lead to nevative volume if we didn't put a max. 1/tp: "+d2s(1/tp)+",  mort: "+d2s(mort)+", total coeff: "+d2s((1-1/tp-mort))+" ");
            }
            #endif
          } else {
            // time of passage and volume of smaller diameter class
            double inc = (u==dClasses.size()-1)?0:1./tp; // we exclude the possibility for trees in the last diameter class to move to an upper class
            double tp_1 = px->tp.at(j).at(u-1); //gfd("tp",regId,ft,dClasses[u-1]);
            double pastYearVol_1 = px->vol_l.at(j).at(u-1); //gfd("vol",regId,ft,dClasses[u-1],thisYear-1);
            //vol = max(0.0,(1-inc-mort-hr)*pastYearVol+(1/tp_1)*beta*pastYearVol_1);
            vol = max(0.0,(1-inc-mort)*pastYearVol-hV+(1/tp_1)*beta*pastYearVol_1); // I can't use any more hr as it is the harvesting rate over the available volumes, not the whole ones
            #ifdef QT_DEBUG
            if ((1-inc-mort)*pastYearVol-hV+(1/tp_1)*beta*pastYearVol_1 < 0){
              double realVolumes = (1-inc-mort)*pastYearVol-hV+(1/tp_1)*beta*pastYearVol_1;
              msgOut(MSG_DEBUG,"Negative real volumes ("+d2s(realVolumes)+"), possibly because of little bit larger bounds in the market module to avoid zeros. Volumes in the resource module set back to zero, so it should be ok.");
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
              double finalHarvestedRate = pastYearVol?finalHarvestedVolumes/pastYearVol:0.0; // Here we want the harvested rate over the whole volumes, not just the available ones, so we don't need to multiply to px->avalCoef.at(j)
              #ifdef QT_DEBUG
              if (finalHarvestedRate > 1.0){
                msgOut(MSG_CRITICAL_ERROR,"Negative final harvested rate.");
              }
              #endif
              hArea_byDc.push_back(finalHarvestedRate*pastArea); // volumes are in Mm^3, area in ha, vHa in m^3/ha
            }
            px->area.at(j).at(u) = max(0.0, px->area_l.at(j).at(u) - areasMovingUp.at(u) + areasMovingUp.at(u-1) - hArea_byDc.at(u));
            #ifdef QT_DEBUG
            if ((px->area_l.at(j).at(u) - areasMovingUp.at(u) + areasMovingUp.at(u-1) - hArea_byDc.at(u))< 0.0){
              msgOut(MSG_DEBUG,"If not for a max, we would have had a negative area ("+d2s(px->area_l.at(j).at(u) - areasMovingUp.at(u) + areasMovingUp.at(u-1) - hArea_byDc.at(u))+" ha).");
            }
            #endif
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
            msgOut(MSG_CRITICAL_ERROR, "Negative values in runBiologicalModel");
          }
          #endif

          //double debug = hv2fa*hr*pastYearVol*100;
          //cout << "regId|ft|dc| debug | freeArea: " << r2 << "|"<<ft<<"|"<<dc<<"| "<< debug << " | " << freeArea_byU << endl;

          //sfd(hr,"hr",regId,ft,dc);
          //sfd(hV,"hV",regId,ft,dc);
          //sfd(vol,"vol",regId,ft,dc);

          //sfd(freeArea_byU,"harvestedArea",regId,ft,dc,DATA_NOW,true);
          double productivity = hArea_byDc.at(u) ? (1000000.0 *  hV_byDiam.at(u))/(hArea_byDc.at(u)*age) : 0.0;
          productivity_byDc.push_back(productivity);
        } // end foreach diameter classes
        px->hVol.push_back(hV_byDiam);
        px->hVol_byPrd.push_back(hV_byDiamAndPrd);
        px->hArea.push_back(hArea_byDc);
        px->vol.push_back(newVol_byDiam);
        px->vMort.push_back(vMort_byDc);
        px->hProductivity.push_back(productivity_byDc);


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
      sumSt += gpd("st_fromHarv",r2,priProducts[pp]);
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
      msgOut(MSG_DEBUG, "St and harvested volumes diverge in region "+REG->getRegSName()+". St: "+d2s(sumSt)+" hV: "+d2s(sumHv));
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
  double ir = MTHREAD->MD->getDoubleSetting("ir");

  // Post optimisation management module..
  for(uint i=0;i<regIds2.size();i++){
    int r2 = regIds2[i];
    int regId = r2;
    ModelRegion* REG = MTHREAD->MD->getRegion(r2);
    regPx = REG->getMyPixels();

    string flagHartman = MTHREAD->MD->getStringSetting("flagHartman",DATA_NOW,r2);
    //double exogenousReference = MTHREAD->MD->getBoolSetting("exogenousReference"); // new setting to distinguish between exogenous and endogenous carbon reference levels




    // Caching futureCarbonPrices..
    vector<double> futureCarbonPrices;
    if(flagHartman!="NONE"){
        for (int y=0; y<1000; y++) {
            futureCarbonPrices.push_back(gpd("carbonPrice",r2,"",thisYear + y));
        }
    }

    vector<double> polBal_fiSub (fTypes.size(),0.0); // Expenses for forest investment subsides by region and destination ft €

    // Dealing with area change..
    double fArea_reg     = REG->getArea();
    double fArea_diff    = 0.0;
    double fArea_reldiff = 0.0;
    if(forestAreaChangeMethod=="relative"){
      fArea_reldiff = gfd("forestChangeAreaIncrementsRel",r2,"","",DATA_NOW);
      fArea_diff    = fArea_reg * fArea_reldiff;
    } else if (forestAreaChangeMethod=="absolute"){
      fArea_diff    = gfd("forestChangeAreaIncrementsHa",r2,"","",DATA_NOW);
      fArea_reldiff = fArea_diff / fArea_reg;
    }

    double regHArea = 0.0; // for the warning

    // Management rate
    // 20170123: if mr is defined at regional level in the forData table, good, otherwise we look at settings table.
    // If it isn't there as well, we rise a critical error.
    // 20170302: removed the (false positive) error message raised if we put mr in setting table
    // 20180626: mr is now, like all the other settings, powered with a regional dimension, so this looking in the
    // forest data is no longer needed, but we keep it as some old projects have mr in the forData sheet.
    double mr;
    int origErrorLevel = MD->getErrorLevel();
    MTHREAD->MD->setTempBool(true); // this should not be needed as it is set positive in getForData() map soubroutine..
    MD->setErrorLevel(MSG_NO_MSG);
    mr = MD->getForData("mr",regId,"","");
    if(!MTHREAD->MD->getTempBool()) { // mr not found in the forest data
      MD->setErrorLevel(MSG_CRITICAL_ERROR);
      mr = MD->getDoubleSetting("mr",DATA_NOW,regId);
    }
    //try{
    //  mr = MD->getForData("mr",regId,"","");
    //} catch (...){
    //  mr = MD->getDoubleSetting("mr");
    //}
    MD->setErrorLevel(origErrorLevel);

    // Caching carbon reference levels by ftypes
    vector<vector<double>> futureCarbonRef;
    vector<vector<double>> futureExtraBiomass_ratio;

    if(flagHartman!="NONE"){
        for(uint j=0;j<fTypes.size();j++){
            string ft = fTypes[j];
            vector<double> futureCarbonRef_ft;
             vector<double>futureExtraBiomass_ratio_ft;
            for (int y=0; y<1000; y++) {
                double carbonRef = gfd("carbonRef",r2,ft,"", thisYear + y);
                futureCarbonRef_ft.push_back(carbonRef);
                double extraBiomass_ratio = gfd("extraBiomass_ratio",regId,ft,"",DATA_NOW);
                futureExtraBiomass_ratio_ft.push_back(extraBiomass_ratio);
            }
            futureCarbonRef.push_back(futureCarbonRef_ft);
            futureExtraBiomass_ratio.push_back(futureExtraBiomass_ratio_ft);
        }
    }


    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];
      px->expectedReturns.clear();
      px->expectedReturnsNotCorrByRa.clear(); // BUG discovered 20160825
      //px->expectedReturnsCarbon.clear(); TODO
      px->optDc.clear();
      px->optFtChosen.clear();
      px->optDcChosen.clear();
      resetMapValues(hAreaByFTypeGroup,0.0);
      double totalHarvestedAreaOrig = vSum(px->hArea); // still need to remove the forest decrease areas..
      double totalHarvestedArea     = 0.0;             // account for (eventual) forest decreases area)
      vector<double> thisYearRegAreas(fTypes.size(),0.0); // initialize a vector of fTypes.size() zeros.
      vector<double> expectedReturns(fTypes.size(),0.0);  // uncorrected expected returns (without considering transaction costs). These are in form of eai
      vector<double> expectedReturnsCarbon(fTypes.size(),0.0);  // expected return of the carbon only
      vector<int> rotation_dc(fTypes.size(),0.0); //vector to store optimal dc for each ftype (to be reused when flagHartman == "BAU")
      vector< vector<double> > deltaAreas(fTypes.size()+1, vector<double>(fTypes.size()+1,0));  // matrix of regeneration areas ft_from/ft_to



      double fArea_px = vSum(px->area);
      double fArea_diff_px = fArea_px * fArea_diff/ fArea_reg;

      double fArea_incr = max(0.0,fArea_diff_px);
      double fArea_incr_rel = totalHarvestedAreaOrig?fArea_incr/totalHarvestedAreaOrig:0.0;
      double fArea_decr = - min(0.0,fArea_diff_px);
      double fArea_decr_rel = totalHarvestedAreaOrig?min(1.0,fArea_decr/totalHarvestedAreaOrig):0.0; // we can "remove" at maximum what has been harvested
      regHArea += totalHarvestedAreaOrig;
      totalHarvestedArea = totalHarvestedAreaOrig *(1-fArea_decr_rel);


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

       // Added for carbon project
        double co2content_inventory = gfd("co2content_inventory",r2,ft,"",DATA_NOW)/1000; // kilos to tons
       // End of added


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
            double anualised_amount =  MD->calculateAnnualisedEquivalent(raw_amount,cumTp_u, ir);

            // If carbon payments are included and the reference is an exogenous fixed value (e.g. year of reference), we compute revenues from carbon after retrieving reference from input files
            // Added for carbon project-------------------------------------------------------------------------
            if (flagHartman =="EXO") {
                double raw_amount_carbon = 0;
                for (int y=0; y<cumTp_u; y++) {
                    double carbonPrice_y = futureCarbonPrices[y];
                    double carbonRef = futureCarbonRef[j][y];
                    double expectedVHa_y = getVHaByYear(px, j, y, futureExtraBiomass_ratio[j][y]);
                    double carbonPayment_y = carbonPrice_y*ir*(co2content_inventory*expectedVHa_y - carbonRef);
                    raw_amount_carbon += carbonPayment_y*pow(1+ir,cumTp_u - y);
                }
                //Now we add the final payment corresponding to carbon sequestered in long-lived products
                double pickling_pp = gpd("pickling",regId,priProducts[pp]);
                double CarbonPrice_cumTp = futureCarbonPrices[floor(cumTp_u)];
                raw_amount_carbon += CarbonPrice_cumTp * pickling_pp * finalHarvestFlag * vHa * app(priProducts[pp],ft,dc);
                // And we finally annualise everything
                double anualised_amount_carbon = MD->calculateAnnualisedEquivalent(raw_amount_carbon,cumTp_u, ir);
                anualised_amount += anualised_amount_carbon;
                //TODO no this is still within a for each product dc class px->expectedReturnsCarbon.push_back(anualised_amount_carbon);
            }
            // End of added-----------------------------------------------------------------------------------


            if (anualised_amount>expReturns) {
              expReturns=anualised_amount;
              optDc = u;// this is the index of the optimal dc, not the dc itself
            }
          }
        }

        //results are pushed back to pixel only if they are final
        if (flagHartman=="NONE" || flagHartman=="EXO") {
            px->expectedReturnsNotCorrByRa.push_back(expReturns);
        }

        //calculation of expected returns with risk aversion
        if(MD->getBoolSetting("heterogeneousRiskAversion",DATA_NOW)){
          double ra = px->getDoubleValue("ra");
          double cumMort = 1-px->cumAlive_exp.at(j).at(optDc);
          //cout << px->getID() << "\t" << ft << "\t\t" << "optDc" << optDc << "\t" << cumMort << endl;
          double origExpReturns = expReturns;
          expReturns = origExpReturns * (1.0 - ra*cumMort);
        }
        px->expectedReturns.push_back(expReturns);
        px->optDc.push_back(optDc);

        // results are pushed back to pixel only if they are final
        if (flagHartman=="NONE" || flagHartman=="EXO") {
            px->expectedReturns.push_back(expReturns);
        }

        expectedReturns.at(j) = expReturns;
        rotation_dc.at(j) = optDc; // here we store the index of optimal dc for the forest type considered

      } // end foreach forest type


      // Looping again by forest types, here by ft in input (those harvested)
      // Added for second carbon project-------------------------------------------------------------------------------------------------------------------------------------
      // If carbon payments are included and the reference is BAU (ie, endogenous, we need a new loop-------------------------------------------------------------------
      // In this case, the reference is what happens without policy, i.e. the result of the previous loop when (flagHartman =="REFERENCE") is FALSE
      // So we just redo another loop with carbon payments using the previous result as reference

      if (flagHartman =="ENDO") { // if carbon payments take place and reference is endogenous (is BAU)
          // STEP 1 - retrieve optimal regeneration choice without carbon payments
          int ft_opt_index = getMaxPos(expectedReturns); // get ft with max expected returns
          string ft_opt = fTypes[ft_opt_index]; // get its name
          int dc_opt_index = rotation_dc[ft_opt_index]; // get dc for forest type with max expected return
          string dc_opt = dClasses[dc_opt_index]; // get its name
          double cumTP_opt = px->cumTp_exp.at(ft_opt_index).at(dc_opt_index);
          double co2content_inventory_opt = gfd("co2content_inventory",r2,ft_opt,"",DATA_NOW)/1000; // kilos to tons
          // STEP 2 - calculate expected carbon contents on the optimal choice without carbon payments, to use as reference
          vector <double> FutureCarbonContent_opt;
          for (int y=0; y<cumTP_opt; y++) {
              double expectedVHa_opt_y = getVHaByYear(px, ft_opt_index, y, futureExtraBiomass_ratio[ft_opt_index][y]);
              FutureCarbonContent_opt.push_back(co2content_inventory_opt*expectedVHa_opt_y);
          }
          int cumTP_opt_floor = FutureCarbonContent_opt.size(); // need to get an integer value to compare rotation lengths

          // STEP 3 - new loop to find optimal regeneration choice WITH carbon payments, using the choice WITHOUT carbon payments as a reference

          for(uint j=0;j<fTypes.size();j++){
              string ft = fTypes[j];
              double co2content_inventory = gfd("co2content_inventory",r2,ft,"",DATA_NOW)/1000; // kilos to tons

              double expReturns_hartman = 0.;
              int optDc_hartman = 0; // "optimal diameter class", the one on which the expected returns are computed
              for (uint u=0; u<dClasses.size(); u++){
                  string dc = dClasses[u];
                  double vHa         = px->vHa_exp.at(j).at(u);
                  double finalHarvestFlag = gfd("finalHarvestFlag",regId,ft,dc);
                  double cumTp_u = px->cumTp_exp.at(j).at(u);
                  // Here we calculate expected revenues from timber
                  for (uint pp=0;pp<priProducts.size();pp++){
                      double pl            = gpd("pl",regId,priProducts[pp]); // note that this is the OBSERVED price. If we call it at current year+cumTp_u we would have the expected price. But we would first have to compute it, as pw is weigthed price world-local and we don't have local price for the future. DONE 20141202 ;-)
                      double worldCurPrice = gpd("pl",WL2,priProducts[pp]);
                      double worldFutPrice = gpd("pl",WL2,priProducts[pp],thisYear+cumTp_u);
                      double sl            = gpd("sl",regId,priProducts[pp]);
                      double sa            = gpd("sa",regId,priProducts[pp]);
                      double pw_exp        = computeExpectedPrice(pl, worldCurPrice, worldFutPrice, sl, sa, px->expTypePrices); //20141030: added the expected price!
                      double raw_amount = finalHarvestFlag*pw_exp*vHa*app(priProducts[pp],ft,dc); // B.U.G. 20121126, it was missing app(pp,ft,dc) !!
                      double anualised_amount =  MD->calculateAnnualisedEquivalent(raw_amount,cumTp_u, ir);

                      // Now we calculate expected revenues from carbon
                      double raw_amount_carbon = 0;
                      for (int y=0; y<cumTp_u; y++) { // calculate yearly payment for each year of the rotation
                          double carbonPrice_y = futureCarbonPrices[y];
                          double expectedVHa_y = getVHaByYear(px, j, y, futureExtraBiomass_ratio[j][y]);
                          double carbonPayment_y;
                          if (y==0) { // we use euclidian division, so an exception must be made when denominator is equal to 0
                              carbonPayment_y = carbonPrice_y*ir*(co2content_inventory*expectedVHa_y - FutureCarbonContent_opt[y]);
                          } else {
                              int factor = y/cumTP_opt_floor; // euclidian division of the two lengths we compare
                              if (cumTP_opt_floor%y >0) { // if the year is NOT a multiple of optimal rotation time, we need to take the year-th value of vector modulus how many rotations we have passed
                                  carbonPayment_y = carbonPrice_y*ir*(co2content_inventory*expectedVHa_y - FutureCarbonContent_opt[y - factor*cumTP_opt_floor]);
                              } else { // if the year is EXACTLY a multiple of optimal rotation time, we need to take the last value of vector, not the first one
                                  carbonPayment_y = carbonPrice_y*ir*(co2content_inventory*expectedVHa_y - FutureCarbonContent_opt[cumTP_opt_floor]);
                              }
                          }

                          raw_amount_carbon += carbonPayment_y*pow(1+ir,cumTp_u - y); // all payments put at end of rotation
                      }
                      //Now we add the final payment corresponding to carbon sequestered in long-lived products
                      double pickling_pp = gpd("pickling",regId,priProducts[pp]);
                      double CarbonPrice_cumTp = futureCarbonPrices[floor(cumTp_u)];
                      raw_amount_carbon += CarbonPrice_cumTp * pickling_pp * finalHarvestFlag * vHa * app(priProducts[pp],ft,dc);
                      // And we annualise carbon revenues and add them to annualised timber revenues
                      double anualised_amount_carbon = MD->calculateAnnualisedEquivalent(raw_amount_carbon,cumTp_u, ir);
                      anualised_amount += anualised_amount_carbon;
                      //px->expectedReturnsCarbon = anualised_amount_carbon; TODO

                      // If the new expected revenues is higher than previously, we store the maximum value and corresponding dc.
                      if (anualised_amount>expReturns_hartman) {
                          expReturns_hartman=anualised_amount;
                          optDc_hartman = u;
                      }
                  } // end foreach primary product
              } // end foreach DC

              // Here we send results to pixel because they are final
              px->expectedReturnsNotCorrByRa.push_back(expReturns_hartman);
              if(MD->getBoolSetting("heterogeneousRiskAversion",DATA_NOW)){
                  double ra = px->getDoubleValue("ra");
                  double cumMort = 1-px->cumAlive_exp.at(j).at(optDc_hartman);
                  //cout << px->getID() << "\t" << ft << "\t\t" << "optDc" << optDc << "\t" << cumMort << endl;
                  double origExpReturns_hartman = expReturns_hartman;
                  expReturns_hartman = origExpReturns_hartman * (1.0 - ra*cumMort);
              }              
              px->expectedReturns.push_back(expReturns_hartman);

              // Now we replace results from the previous loop with the new ones
              expectedReturns.at(j) = expReturns_hartman;
              rotation_dc.at(j) = optDc_hartman;


          } // end foreach forest type

      } // end IF Hartman reference is BAU (no policy)----------------------------------------------------------------

      // End of added second carbon project---------------------------------------------------------------------------

      for(uint j=0;j<fTypes.size();j++){
        string ft = fTypes[j];
        forType* thisFt = MTHREAD->MD->getForType(ft);

        double harvestedAreaForThisFT = vSum(px->hArea.at(j))*(1-fArea_decr_rel); // gfd("harvestedArea",regId,ft,DIAM_ALL);
        deltaAreas[j][fTypes.size()] += vSum(px->hArea.at(j))*(fArea_decr_rel);
        vector<double> corrExpectedReturns(fTypes.size(),0.0); //  corrected expected returns (considering transaction costs). These are in form of  NPV
        vector<double> polBal_fiSub_unitvalue(fTypes.size(),0.0); // subside (tax) per ha per destination ft €/ha

        // C - Computing the corrected expected returns including transaction costs

        for(uint j2=0;j2<fTypes.size();j2++){
          string ft2 = fTypes[j2];
          double invTransCost = (gfd("invTransCost",regId,ft,ft2,DATA_NOW) *  gfd("pol_fiSub",regId,ft,ft2,DATA_NOW) ) ;
          polBal_fiSub_unitvalue[j2] = gfd("invTransCost",regId,ft,ft2,DATA_NOW) * (gfd("pol_fiSub",regId,ft,ft2,DATA_NOW) - 1) ;
          corrExpectedReturns[j2] = (expectedReturns[j2]/ir)-invTransCost; // changed 20150718: npv = eai/ir + tr. cost // HUGE BUG 20151202: transaction costs should be REDUCED, not added to the npv...
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

        int optFtChosen = getMaxPos(corrExpectedReturns);
        px->optFtChosen.push_back(optFtChosen);
        px->optDcChosen.push_back(px->optDc.at(optFtChosen));

        thisYearRegAreas[getMaxPos(corrExpectedReturns)] += harvestedAreaForThisFT*mr;
        thisYearRegAreas[getMaxPos(expectedReturns)] += fArea_incr*mr/((double)fTypes.size()); // mr quota of new forest area assigned to highest expected returns ft (not considering transaction costs). Done for each forest types

        deltaAreas[j][getMaxPos(corrExpectedReturns)] += harvestedAreaForThisFT*mr;
        deltaAreas[fTypes.size()][getMaxPos(expectedReturns)] += fArea_incr*mr/((double)fTypes.size());

        polBal_fiSub[getMaxPos(corrExpectedReturns)] += harvestedAreaForThisFT*mr*polBal_fiSub_unitvalue[getMaxPos(corrExpectedReturns)];

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
            // Here is the problem with building a transaction matrix ft_from/ft_to: we don't explicitly assign from/to for unmanaged areas !
            for(uint j2=0;j2<fTypes.size();j2++){
              deltaAreas[j2][j] += vSum(px->hArea.at(j2))*(1-fArea_decr_rel)*(1-mr)*freq*hRatio;
            }
            deltaAreas[fTypes.size()][j] += fArea_incr*(1-mr)*freq*hRatio;



          } else { // prob presence not used..

            // Accounting for mortality arising from pathogens. Assigning the area to siblings according to area..
            // TODO: chek that mortRatePath doesn't involve adding forest area to  deltaAreas[j][""]

            double mortRatePath = px->getPathMortality(ft, "0");
            if(mortRatePath > 0){

              string parentFt = MTHREAD->MD->getForTypeParentId(ft);
              vector <string> siblings = MTHREAD->MD->getForTypeChilds(parentFt);
              vector <double> siblingAreas;
              for(uint j2=0;j2<siblings.size();j2++){
                if(siblings[j2]==ft){
                  siblingAreas.push_back(0.0);
                } else {
                  //string debug_sibling_ft = siblings[j2];
                  //int debug_positin = getPos(debug_sibling_ft,fTypes);
                  double thisSiblingArea = vSum(px->area.at(getPos(siblings[j2],fTypes)));
                  siblingAreas.push_back(thisSiblingArea);
                }
              }
              double areaAllSiblings = vSum(siblingAreas);
              thisYearRegAreas[j] += harvestedAreaForThisFT*(1-mr)*(1-mortRatePath);
              deltaAreas[j][j] += harvestedAreaForThisFT*(1-mr)*(1-mortRatePath);

              if(areaAllSiblings>0.0){ // area of siblings is >0: we attribute the area from the pathogen induced mortality to the siblings proportionally to area..
                for(uint j2=0;j2<siblings.size();j2++){
//                  int debug1 =  getPos(siblings[j2],fTypes);
//                  double debug2= harvestedAreaForThisFT;
//                  double debug3 = 1.0-mr;
//                  double debug4 = mortRatePath;
//                  double debug5 = siblingAreas[j2];
//                  double debug6 = areaAllSiblings;
//                  double debug7 = harvestedAreaForThisFT*(1.0-mr)*(mortRatePath)*(siblingAreas[j2]/areaAllSiblings);
                  thisYearRegAreas[getPos(siblings[j2],fTypes)] += harvestedAreaForThisFT*(1.0-mr)*(mortRatePath)*(siblingAreas[j2]/areaAllSiblings);
                  deltaAreas[j][getPos(siblings[j2],fTypes)] += harvestedAreaForThisFT*(1.0-mr)*(mortRatePath)*(siblingAreas[j2]/areaAllSiblings);
                }
              } else if (siblings.size()>1) { // area of all siblings is 0, we just give them the mortality area in equal parts..
                for(uint j2=0;j2<siblings.size();j2++){
                  if (siblings[j2] != ft){
                    thisYearRegAreas[getPos(siblings[j2],fTypes)] += harvestedAreaForThisFT*(1.0-mr)*(mortRatePath)* 1.0 / (( (float) siblings.size())-1.0);
                    deltaAreas[j][getPos(siblings[j2],fTypes)] += harvestedAreaForThisFT*(1.0-mr)*(mortRatePath)* 1.0 / (( (float) siblings.size())-1.0);
                  }
                }
              }
            } else { // mortRatePath == 0
               thisYearRegAreas[j] += harvestedAreaForThisFT*(1.0-mr);
               deltaAreas[j][j] += harvestedAreaForThisFT*(1.0-mr);
            }

            // Allocating non-managed quota of new forest area to ft proportionally to the current area share by ft
            double newAreaThisFt = vSum(px->area) ? fArea_incr*(1-mr)*vSum(px->area.at(j))/vSum(px->area): 0.0;
            thisYearRegAreas[j] +=  newAreaThisFt;
            deltaAreas[fTypes.size()][j] +=  newAreaThisFt;

            if(! (thisYearRegAreas[j] >= 0.0) ){
              msgOut(MSG_ERROR,"thisYearRegAreas[j] is not non negative (j: "+i2s(j)+", thisYearRegAreas[j]: "+i2s( thisYearRegAreas[j])+").");
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
      if (! (totalRegArea==0.0 && totalHarvestedArea==0.0)){
        double ratio = totalRegArea / totalHarvestedArea ;
        if(rescaleFrequencies && (ratio < 0.99999999999 || ratio > 1.00000000001) && forestAreaChangeMethod == "fixed") {
          msgOut(MSG_CRITICAL_ERROR, "Sum of regeneration areas not equal to sum of harvested area in runManagementModel()!");
        }
      }
      #endif

      px->regArea.insert(pair <int, vector<double> > (MTHREAD->SCD->getYear(), thisYearRegAreas));
      px->deltaArea = deltaAreas;

    } // end of each pixel
    if (-fArea_diff > regHArea){
      msgOut(MSG_WARNING,"In region "+ i2s(regId) + " the exogenous area decrement ("+ d2s(-fArea_diff) +" ha) is higger than the harvesting ("+ d2s(regHArea) +" ha). Ratio forced to 1.");
    }

    for(uint j=0;j<fTypes.size();j++){
      double debug = polBal_fiSub[j]/1000000;
      sfd((polBal_fiSub[j]/1000000.0),"polBal_fiSub",regId,fTypes[j],"",DATA_NOW,true); //M€

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
}

void
ModelCoreSpatial::cacheDynamicSettings(){
  // Settings needs explicitly DATA_NOW to have a temporal dymension..
  msgOut(MSG_INFO, "Cashing model settings that may change every year..");
  regType = MTHREAD->MD->getStringSetting("regType",DATA_NOW); // how the regeneration should be computed (exogenous, from hr, from allocation choises)
  natRegAllocation = MTHREAD->MD->getStringSetting("natRegAllocation",DATA_NOW); // how to allocate natural regeneration
  rescaleFrequencies = MD->getBoolSetting("rescaleFrequencies",DATA_NOW);
  oldVol2AreaMethod = MD->getBoolSetting("oldVol2AreaMethod",DATA_NOW);
  //mr = MD->getDoubleSetting("mr");
  forestAreaChangeMethod =  MTHREAD->MD->getStringSetting("forestAreaChangeMethod",DATA_NOW);
  ir = MD->getDoubleSetting("ir",DATA_NOW);
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
      //int debugPx = rpx[j]->getID();
      //int debug2 = debugPx;
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

/**
 * @brief ModelCoreSpatial::assignSpMultiplierPropToVols assigns the spatial multiplier (used in the time of return) based no more on a Normal distribution but on the volumes present in the pixel: more volume, more the pixel is fit for the ft
 *
 * This function apply to the pixel a multiplier of time of passage that is inversely proportional to the volumes of that forest type present in the pixel.
 * The idea is that in the spots where we observe more of a given forest type are probabily the most suited ones to it.
 *
 * The overall multipliers **of time of passage** (that is, the one returned by Pixel::getMultiplier("tp_multiplier") ) will then be the product of this multiplier that account for spatial heterogeneity and of an eventual exogenous
 * multiplier that accounts for different scenarios among the spatio-temporal dimensions.
 *
 * Given that (forest type index omitted):
 * - \f$V_{p}\f$ = volume of a given ft in each pixel (p)
 * - \f$\bar{g}\f$ and \f$\sigma_{g}\f$ = regional average and standard deviation of the growth rate
 * - \f$m_{p}\f$ = multiplier of time of passage
 *
 * This multiplier is computed as:
 * - \f$ v_{p}    = max(V) - V_{p}~~ \f$                   A diff from the max volume is computed in each pixel
 * - \f$ vr_{p}   = v_{p} * \bar{g}/\bar{v}~~ \f$          The volume diff is rescaled to match the regional growth rate
 * - \f$ vrd_{p}  = vr_{p} - \bar{vr}~~ \f$                Deviation of the rescaled volumes are computed
 * - \f$ vrdr_{p} = vrd_{p} * \sigma_{g}/\sigma_{vr}~~ \f$ The deviations are then rescaled to match the standard deviations of the regional growth rate
 * - \f$ m_{p}    = (vrdr_{p} + \bar{vr}) / \bar{g}~~ \f$  The multiplier is computed from the ratio of the average rescaled volumes plus rescaled deviation over the average growth rate.
 *
 * And it has the following properties:
 * - \f$\bar{m} = 1\f$
 * - \f$\sigma_{m} = cv_{g}\f$
 * - \f$m_{p} = V_{p}*\alpha+\beta\f$
 * - \f$m_{\bar{V}} = 1\f$
 *
 * For spreadsheet "proof" see the file computation_of_growth_multipliers_from_know_avg_sd_and_proportional_to_share_of_area_in_each_pixel.ods
 */
void
ModelCoreSpatial::assignSpMultiplierPropToVols(){

  if(!MTHREAD->MD->getBoolSetting("useSpatialVarPropToVol")){return;}
  for(uint r=0;r<regIds2.size();r++){
    int rId = regIds2[r];
    ModelRegion* reg = MD->getRegion(regIds2[r]);
    vector <Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regIds2[r]);
    for(uint f=0;f<fTypes.size();f++){
      string ft = fTypes[f];
      double agr = gfd("agr",regIds2[r],ft,"");
      double sStDev = gfd("sStDev",regIds2[r],ft,"");
      vector<double> vols;
      vector<double> diffVols;
      vector<double> diffVols_rescaled;
      double diffVols_rescaled_deviation;
      double diffVols_rescaled_deviation_rescaled;
      double final_value;
      double multiplier;
      vector<double> multipliers; // for tests

      double vol_max, rescale_ratio_avg, rescale_ratio_sd;
      double diffVols_avg, diffVols_rescaled_avg;
      double diffVols_rescaled_sd;

      for (uint p=0;p<rpx.size();p++){
        Pixel* px = rpx[p];
        vols.push_back(vSum(px->vol[f]));
      } // end for each pixel
      vol_max=getMax(vols);

      for(uint p=0;p<vols.size();p++){
        diffVols.push_back(vol_max-vols[p]);
      }

      diffVols_avg = getAvg(diffVols);
      rescale_ratio_avg = (diffVols_avg != 0.0) ? agr/diffVols_avg : 1.0;
      for(uint p=0;p<diffVols.size();p++){
        diffVols_rescaled.push_back(diffVols[p]*rescale_ratio_avg);
      }
      diffVols_rescaled_avg = getAvg(diffVols_rescaled);
      diffVols_rescaled_sd  = getSd(diffVols_rescaled,false);

      rescale_ratio_sd = (diffVols_rescaled_sd != 0.0) ? sStDev/diffVols_rescaled_sd : 1.0;
      for(uint p=0;p<diffVols_rescaled.size();p++){
        diffVols_rescaled_deviation          = diffVols_rescaled[p]    - diffVols_rescaled_avg;
        diffVols_rescaled_deviation_rescaled = diffVols_rescaled_deviation * rescale_ratio_sd;
        final_value                      = diffVols_rescaled_avg   + diffVols_rescaled_deviation_rescaled;
        multiplier = (agr != 0.0) ? min(1.6, max(0.4,final_value/agr)) : 1.0; //20151130: added bounds for extreme cases. Same bonds as in Gis::applySpatialStochasticValues()
       // multiplier = 1.0;

        Pixel* px = rpx[p];
        px->setSpModifier(multiplier,f);
        multipliers.push_back(multiplier);
      }

      #ifdef QT_DEBUG
      // Check relaxed as we introduced bounds that may change slighly the avg and sd...
      double avgMultipliers = getAvg(multipliers);
      double sdMultipliers  = getSd(multipliers,false);
      if ( avgMultipliers < 0.9 || avgMultipliers > 1.1){
        msgOut(MSG_CRITICAL_ERROR, "The average of multipliers of ft "+ ft +" for the region " + i2s(rId) + " is not 1!");
      }
      if ( ( sdMultipliers -  (sStDev/agr) ) < -0.5 ||  ( sdMultipliers -  (sStDev/agr) ) > 0.5 ){
        double cv = sStDev/agr;
        msgOut(MSG_CRITICAL_ERROR, "The sd of multipliers of ft "+ ft +" for the region " + i2s(rId) + " is not equal to the spatial cv for the region!");
      }
      #endif
    } // end for each ft
  } // end for each region
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

void
ModelCoreSpatial::initialiseDeathTimber(){
  int currentYear = MTHREAD->SCD->getYear();
  for(int y=currentYear;y>currentYear-30;y--){
    for(uint i=0;i<regIds2.size();i++){
      for(uint j=0;j<fTypes.size();j++){
        for (uint u=0;u<dClasses.size();u++){
          iisskey key(y,regIds2[i],fTypes[j],dClasses[u]);
          MD->deathTimberInventory_incrOrAdd(key,0.0);
        }
      }
    }
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
  /// \todo: also update area_l
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
        double pathMort_now, pathMort_t0;

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
          pathMort_now = px->getPathMortality(ft,dc,DATA_NOW);
          pathMort_t0 = px->getPathMortality(ft,dc,firstYear);
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
              mort = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1],thisYear)*mortCoef_multiplier_now+pathMort_now,tp); // mortality of the previous diameter class
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
                mort_exp = 1-pow(1-gfd("mortCoef",regId,ft,dClasses[u-1], firstYear)*mortCoef_multiplier_t0+pathMort_t0,tp_exp); // mortality rate of previous diameter class
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
                mort_noExp = 1-pow(1-min(1.0,gfd("mortCoef",regId,ft,dClasses[u-1], DATA_NOW)*mortCoef_multiplier_now+pathMort_now), tp_noExp); // mortCoef is a yearly value. Mort coeff between class is 1-(1-mortCoeff)^tp
                double mortCoef_multiplier_dynamic = px->getMultiplier("mortCoef_multiplier",ft,thisYear+ceil(cumTp_exp_temp[u-1]));
                double pathMort_dynamic = px->getPathMortality(ft,dc,thisYear+ceil(cumTp_exp_temp[u-1]));
                mort_fullExp = 1-pow(1-min(1.0,gfd("mortCoef",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_exp_temp[u-1]))*mortCoef_multiplier_dynamic+pathMort_dynamic),tp_fullExp); // mortality of the previous diameter class
                //double debug1 = gfd("mortCoef",regId,ft,dClasses[u-1],thisYear+ceil(cumTp_exp_temp[u-1]));
                //double debug2 = debug1*mortCoef_multiplier_dynamic+pathMort_dynamic;
                //double debug3 = min(1.0,debug2);
                //double debug4 = 1.0-debug3;
                //double debug5 = pow(debug4,tp_fullExp);
                //double debug6 = 1.0-debug5;


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
      //double debug = 0.0;
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
      px->deltaArea.clear();
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
  bool applyAvalCoef = MTHREAD->MD->getBoolSetting("applyAvalCoef");

  for(uint r2= 0; r2<regIds2.size();r2++){
    int regId = regIds2[r2];
    regPx = MTHREAD->MD->getRegion(regId)->getMyPixels();
    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];
      px->tp.clear();
      px->beta.clear();
      px->mort.clear();
      px->avalCoef.clear();

      for(uint j=0;j<fTypes.size();j++){
        string ft = fTypes[j];
        vector <double> tp_byu;
        vector <double> beta_byu;
        vector <double> mort_byu;

        double tp_multiplier_now       = px->getMultiplier("tp_multiplier",ft,DATA_NOW);
        double mortCoef_multiplier_now = px->getMultiplier("mortCoef_multiplier",ft,DATA_NOW);
        double betaCoef_multiplier_now = px->getMultiplier("betaCoef_multiplier",ft,DATA_NOW);
        double avalCoef_ft             = applyAvalCoef?px->getSTData("avalCoef",ft,DATA_NOW):1.0;

        if(avalCoef_ft == MTHREAD->MD->getDoubleSetting("noValue")){
          msgOut(MSG_CRITICAL_ERROR,"applyAvalCoef has benn asked for, but I can't find the avalCoef in the data!");
        }

        for (uint u=0; u<dClasses.size(); u++){
          string dc = dClasses[u];
          double pathMortality           = px->getPathMortality(ft,dc,DATA_NOW);
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
          mort_real = min(u?gfd("mortCoef",regId,ft,dClasses[u],DATA_NOW)*mortCoef_multiplier_now+pathMortality :0,1.0);  //Antonello, bug fixed 20160203: In any case, natural plus pathogen mortality can not be larger than 1!
          tp_byu.push_back(tp);
          beta_byu.push_back(beta_real);
          mort_byu.push_back(mort_real);
        } // end of each dc
        px->tp.push_back(tp_byu);
        px->beta.push_back(beta_byu);
        px->mort.push_back(mort_byu);
        px->avalCoef.push_back(avalCoef_ft);
      } // end of each ft
    } // end of each pixel
  } // end of each region
}

void
ModelCoreSpatial::computeInventary(){ // in=f(vol_t-1)
  msgOut(MSG_INFO, "Starting computing inventary available for this year..");
  int nbounds = pow(2,priProducts.size());
  vector<vector<int>> concernedPriProductsTotal = MTHREAD->MD->createCombinationsVector(priProducts.size());
  int currentYear = MTHREAD->SCD->getYear();

  for(uint i=0;i<regIds2.size();i++){
    int r2 = regIds2[i];
    ModelRegion* REG = MTHREAD->MD->getRegion(r2);
    //Gis* GIS = MTHREAD->GIS;
    regPx = REG->getMyPixels();
    vector <double> in_reg(priProducts.size(),0.);             // should have ceated a vector of size priProducts.size(), all filled with zeros
    vector <double> in_deathTimber_reg(priProducts.size(),0.); // should have ceated a vector of size priProducts.size(), all filled with zeros
    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];
      //int debugPx = px->getID();
      //int debug2 = debugPx;
      //px->in.clear();
      for(uint pp=0;pp<priProducts.size();pp++){
        double in = 0;
        for(uint ft=0;ft<fTypes.size();ft++){
          for(uint dc=0;dc<dClasses.size();dc++){
            in += app(priProducts[pp],fTypes[ft],dClasses[dc])*px->vol_l.at(ft).at(dc)*px->avalCoef.at(ft);
          }
        }
        //px->in.push_back(in);
        in_reg.at(pp) += in;
      } // end of each priProduct
    } // end each pixel


    for(uint pp=0;pp<priProducts.size();pp++){
      vector<string> priProducts_vector;
      priProducts_vector.push_back(priProducts[pp]);

      double in_deathMortality = MD->getAvailableDeathTimber(priProducts_vector,r2,currentYear-1);
      in_deathTimber_reg.at(pp) += in_deathMortality;

      // Even if I fixed all the lower bounds to zero in Opt::get_bounds_info still the model
      // doesn't solve with no-forest in a region.
      // Even with 0.0001 doesn't solve !!
      // With 0.001 some scenarios doesn't solve in 2093
      // With 0.003 vRegFixed doesn't solve in 2096
      // Tried with 0.2 but no changes, so put it back on 0.003
      //spd(max(0.001,in_reg.at(pp)),"in",r2,priProducts[pp],DATA_NOW,true);
      spd(in_reg.at(pp),"in",r2,priProducts[pp],DATA_NOW,true);
      spd(in_deathTimber_reg.at(pp),"in_deathTimber",r2,priProducts[pp],DATA_NOW,true);
      #ifdef QT_DEBUG
      if (in_reg.at(pp) < -0.0){
        msgOut(MSG_CRITICAL_ERROR,"Negative inventory");
      }
      #endif
    }

    // ##### Now creating a set of bonds for the optimisation that account of the fact that the same ft,dc can be used for multiple products:

    // 20160928: Solved a big bug: for each combination instead of taking the UNION of the various priProduct inventory sets I was taking the sum
    // Now both the alive and the death timber are made from the union
    // 20150116: As the same (ft,dc) can be used in more than one product knowing -and bounding the supply in the optimisation- each single
    // in(pp) is NOT enought.
    // We need to bound the supply for each possible combination, that is for 2^(number of prim.pr)
    // Here we compute the detailed inventory. TO.DO: Create the pounds in Opt. done
    // 20160209: Rewritten and corrected a bug that was not giving enought inv to multiproduct combinations
    for (uint i=0; i<nbounds; i++){
      vector<int> concernedPriProducts = concernedPriProductsTotal[i];
      vector<string> concernedPriProducts_ids = positionsToContent(priProducts,concernedPriProducts);
      //double debug       = 0.0;
      //for(uint z=0;z<concernedPriProducts.size();z++){
      //  debug      += gpd("in",r2,priProducts[concernedPriProducts[z]]); // to.do: this will need to be rewritten checked!
      //}
      double bound_alive       = MD->getAvailableAliveTimber(concernedPriProducts_ids,r2); // From px->vol_l, as in "in"
      double bound_deathTimber = MD->getAvailableDeathTimber(concernedPriProducts_ids,r2,currentYear-1); // From deathTimberInventory map
      double bound_total       = bound_alive + bound_deathTimber;

      REG->inResByAnyCombination[i]             = bound_total;
      //REG->inResByAnyCombination_deathTimber[i] = bound_deathTimber;
    } // end for each bond
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
ModelCoreSpatial::updateOtherMapData(){

vector<int> l2Regions =  MTHREAD->MD->getRegionIds(2, true);
vector <string> fTypes =  MTHREAD->MD->getForTypeIds();
int nFTypes = fTypes.size();
int nL2Regions = l2Regions.size();
for(int i=0;i<nL2Regions;i++){
  int regId = l2Regions[i];
  vector<Pixel*> rpx = MTHREAD->GIS->getAllPlotsByRegion(regId);
  for(int j=0;j<nFTypes;j++){
    string ft = fTypes[j];
    for(uint z=0;z<rpx.size();z++){
      Pixel* px = rpx[z];
      double vol = vSum(px->vol.at(j));
      double expectedReturns = px->expectedReturns.at(j);
      if(MTHREAD->GIS->layerExist("vol_"+ft)){
        rpx[z]->changeValue("vol_"+ft, vol);
      }
      if(MTHREAD->GIS->layerExist("expectedReturns_"+ft)){
        rpx[z]->changeValue("expectedReturns_"+ft, expectedReturns);
      }
    }
  }
}

// update GUI image..
for(int j=0;j<nFTypes;j++){
  string ft = fTypes[j];
  MTHREAD->GIS->updateImage("vol_"+ft);
  MTHREAD->GIS->updateImage("expectedReturns_"+ft);
}


}


void
ModelCoreSpatial::sumRegionalForData(){

  msgOut(MSG_INFO, "Summing data pixels->region..");
  //vector <string> outForVariables  = MTHREAD->MD->getStringVectorSetting("outForVariables");
  int currentYear = MTHREAD->SCD->getYear();

  //map<vector<string>,double>  hVol_byPrd; // by regid, ft, dc, pp
  map<tr1::array<string, 4>,double> hVol_byPrd; // by regid, ft, dc, pp

  // OLD CODE TO
  for(uint r2= 0; r2<regIds2.size();r2++){
    int regId = regIds2[r2];
    regPx = MTHREAD->MD->getRegion(regId)->getMyPixels(); // by regId, ft, dc, pp

    // vector < vector <vector <double> > >hVol_byPrd; // by ft, dc, pp

    for(uint j=0;j<fTypes.size();j++){
      string ft = fTypes[j];

      double regArea = 0.;
      double sumAreaByFt = 0.;
      double pxForAreaByFt = 0.;
      double vReg = 0.;
      double sumSumHProductivity = 0.;
      double sumHArea = 0.;

      for (uint u=0; u<dClasses.size(); u++){
        string dc = dClasses[u];
        double vol =0.;
        double hV = 0.;
        double hArea = 0.;
        double vMort = 0.;
        double sumHProductivity = 0.; // productivity * area
        for (uint pi=0;pi<regPx.size();pi++){
          Pixel* px = regPx[pi];
          vol += px->vol.at(j).at(u);
          hV += px->hVol.at(j).at(u);
          hArea += px->hArea.at(j).at(u);
          vMort += px->vMort.at(j).at(u);
          sumHProductivity += (px->hProductivity.at(j).at(u) * px->hArea.at(j).at(u));
          if(MD->getBoolSetting("outDetailedHv",DATA_NOW)){
            for(uint pp=0;pp<priProducts.size();pp++){
              string pr =  priProducts[pp];
              // vector <string> hVKey = {i2s(regId), ft, dc, pr};
              tr1::array<string, 4> hVKey = {i2s(regId), ft, dc, pr};
              //double debug = px->hVol_byPrd.at(j).at(u).at(pp);
              //cout << px->getID() << '\t' << j << '\t' << u << '\t' << pp << '\t' << debug << endl;
              incrOrAddMapValue(hVol_byPrd, hVKey, px->hVol_byPrd.at(j).at(u).at(pp));
            }
          }
        }
        if(u){
          sfd(vol,"vol",regId,ft,dc,DATA_NOW);
          sfd(hV,"hV",regId,ft,dc,DATA_NOW,true);
          double hProductivityByDc = hArea ?  sumHProductivity/hArea : 0.;
          sumSumHProductivity += (hProductivityByDc*hArea);
          sumHArea += hArea;
          sfd(hProductivityByDc,"hProductivityByDc",regId,ft,dc,DATA_NOW,true);
          sfd(hArea,"harvestedArea",regId,ft,dc,DATA_NOW, true);
          sfd(vMort,"vMort",regId,ft,dc,DATA_NOW,true);
          double vol_1 = gfd("vol",regId,ft,dc,currentYear-1);
          if(vol_1){
            sfd(hV/vol_1,"hr",regId,ft,dc,DATA_NOW, true);
          } else {
            sfd(0.,"hr",regId,ft,dc,DATA_NOW, true);
          }

        }
      } // end foreach dc
      double hProductivity = sumHArea? sumSumHProductivity / sumHArea : 0.;
      sfd(hProductivity,"hProductivity",regId,ft,"",DATA_NOW,true);

      for (uint p=0;p<regPx.size();p++){
        Pixel* px = regPx[p];
        vReg += px->vReg.at(j);
        regArea += findMap(px->regArea,currentYear).at(j);
        pxForAreaByFt = (px->getDoubleValue("forArea_"+ft,true)/10000);

        sumAreaByFt += pxForAreaByFt;
        //double debug1 = sumAreaByFt;
        if(! (sumAreaByFt >= 0.0) ){
          msgOut(MSG_CRITICAL_ERROR,"sumAreaByFt is not non negative.");
        }
      }
      sfd(vReg,"vReg",regId,ft,"",DATA_NOW, true);
      sfd(regArea,"regArea",regId,ft,"",DATA_NOW, true);
      sfd(sumAreaByFt,"forArea",regId,ft,"",DATA_NOW, true);
    } // end of for each ft

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
    } // end of each pixel


  } // end each region
  //cout << "done" << endl;
  MTHREAD->DO->printDetailedHV(hVol_byPrd);


  // Taking care of expected returns here..
  // (Changed 25/08/2016 afternoon: expRet{ft,r} are now sum{px}{expRet{ft,px}*fArea_{px}}/fArea{r} and no longer sum{px}{expRet{ft,px}*fArea_{px,ft}}/fArea{r,ft} )
  // Also now we report the expReturns by group and by forest, each of which is made only with the best ones within their group

  vector<string>  parentFtypes = MTHREAD->MD->getForTypeParents();

  for(uint r2= 0; r2<regIds2.size();r2++){
    int regId = regIds2[r2];
    regPx = MTHREAD->MD->getRegion(regId)->getMyPixels();
    double totRegionForArea = 0.;
    double totSumExpRet  = 0.;
    vector <double> totSumExpRet_byFTParent(parentFtypes.size(),0.0);
    vector <double> totSumExpRet_byFTypes(fTypes.size(),0.0);

    // First computing the sumExpectedReturns..
    for (uint p=0;p<regPx.size();p++){
      Pixel* px = regPx[p];
      //int debug_pxid = px->getID();
      double pxForArea = vSum(px->area);
      totRegionForArea += pxForArea;
      double bestPxExpectedRet = getMax(px->expectedReturnsNotCorrByRa);
      for(uint i=0;i<parentFtypes.size();i++){
        vector <string> childIds = MTHREAD->MD->getForTypeChilds(parentFtypes[i]);
        vector <int> childPos = MTHREAD->MD->getForTypeChilds_pos(parentFtypes[i]);
        vector<double> pxExpReturnsByChilds(childPos.size(),0.0);
        for(uint j=0;j<childPos.size();j++){
          double pxExpReturn_singleFt = px->expectedReturns.at(childPos[j]);
          // Manual fix to not have the expected returns of ash within the general "broadL" expected returns.
          // To do: remove it after we work on the ash project.. I don't like manual fixes !!!
          pxExpReturnsByChilds.at(j) = (childIds.at(j) == "ash") ? 0.0 : pxExpReturn_singleFt;
          //pxExpReturnsByChilds.at(j) = pxExpReturn_singleFt;
          totSumExpRet_byFTypes.at(childPos[j]) += pxExpReturn_singleFt*pxForArea;
        } // end of each ft
        totSumExpRet_byFTParent[i] += getMax(pxExpReturnsByChilds)*pxForArea;
      } // end for each partentFt
      totSumExpRet += bestPxExpectedRet * pxForArea;
    } // end for each px

    // ..and now computing the expReturns and storing them
    for(uint i=0;i<parentFtypes.size();i++){
      vector <int> childPos = MTHREAD->MD->getForTypeChilds_pos(parentFtypes[i]);
      for(uint j=0;j<childPos.size();j++){
        //double debug1 = totSumExpRet_byFTypes.at(childPos[j])/totRegionForArea;
        sfd(totSumExpRet_byFTypes.at(childPos[j]),"sumExpReturns",regId,fTypes.at(childPos[j]),"",DATA_NOW, true);
        sfd(totSumExpRet_byFTypes.at(childPos[j])/totRegionForArea,"expReturns",regId,fTypes.at(childPos[j]),"",DATA_NOW, true);
      } // end of each ft
      //double debug2 = totSumExpRet_byFTParent.at(i)/totRegionForArea;
      sfd(totSumExpRet_byFTParent.at(i),"sumExpReturns",regId,parentFtypes[i],"",DATA_NOW, true);
      sfd(totSumExpRet_byFTParent.at(i)/totRegionForArea,"expReturns",regId,parentFtypes[i],"",DATA_NOW, true);

    } // end for each partentFt
    //double debug3 = totSumExpRet/totRegionForArea;
    sfd(totSumExpRet,"sumExpReturns",regId,"","",DATA_NOW, true);
    sfd(totSumExpRet/totRegionForArea,"expReturns",regId,"","",DATA_NOW, true);

  } // end for each region

  // Computing pathogens share of forest invasion
  if(MD->getBoolSetting("usePathogenModule")){
      for(uint r2= 0; r2<regIds2.size();r2++){
        int regId = regIds2[r2];
        regPx = MTHREAD->MD->getRegion(regId)->getMyPixels();
        double totalForArea = 0.0;
        double invadedArea  = 0.0;
        for (uint p=0;p<regPx.size();p++){
          Pixel* px = regPx[p];
          int invaded = 0.0;
          for(uint j=0;j<fTypes.size();j++){
            for (uint u=0; u<dClasses.size(); u++){
              if(px->getPathMortality(fTypes[j],dClasses[u]) > 0){
                invaded = 1.0;
              }
            }
          }
          totalForArea += vSum(px->area);
          invadedArea  += vSum(px->area)*invaded;
        }
        sfd(invadedArea/totalForArea,"totalShareInvadedArea",regId,"","",DATA_NOW, true);
      }
  } // end we are using path model




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
 * It uses volumes from gis data to "move" volumes from one forest type to the other (when called with what="vol"). Then it moves areas
 * proportionally and, as dc0 volumes are not defined but area it is, compute, again proportionally, area in destination forest times for dc=0
 * It acts on the pix->vol, pix->area and pix->area_l vectors. It also create/update the px->values layer map for the area, but it doesn't cash the
 * results in forDataMap.
 *
 * It is called first with parameter what="vol" in initializePixelVolumes() and then with what="area" in initializePixelAreas().
 * As we need the original volumes in the area allocation, original_vols is set as a static variable.
 *
 */
void
ModelCoreSpatial::loadExogenousForestLayers(const string & what){
  if(!MD->getBoolSetting("useSpExplicitForestTypes")) return;

  int nFTypes = fTypes.size();
  int nDC     = dClasses.size();
  int pxC     = 0;

  for(uint ir=0;ir<regIds2.size();ir++){
    int r2 = regIds2[ir];
    ModelRegion* REG = MTHREAD->MD->getRegion(r2);
    regPx = REG->getMyPixels();
    pxC += regPx.size();
  }

  static vector<vector<vector<double>>> original_vols(pxC, vector<vector<double>>(nFTypes, vector<double>(nDC, 0.0))); // by px counter, ftype, dc

  if(what=="vol"){
    // first, before transfering volumes, saving the original ones..
    for(uint i=0;i<fTypes.size();i++){
      for (uint u=0; u<dClasses.size(); u++){
        int pxC_loc = 0;
        for(uint ir=0;ir<regIds2.size();ir++){
          int r2 = regIds2[ir];
          ModelRegion* REG = MTHREAD->MD->getRegion(r2);
          regPx = REG->getMyPixels();
          for (uint p=0;p<regPx.size();p++){
            Pixel* px = regPx[p];
            original_vols[pxC_loc][i][u] += px->vol[i][u];
            pxC_loc ++;
          }
        }
      }
    }
    for(uint i=0;i<fTypes.size();i++){
      string fti = fTypes[i];
      for(uint o=0;o<fTypes.size();o++){
        string fto = fTypes[o];
        for (uint u=1; u<dClasses.size(); u++){ // first diameter class volumes are computed from the model..
          string layerName =  "spInput#vol#"+fto+"#"+fti+"#"+i2s(u);
          if (MTHREAD->GIS->layerExist(layerName)){
            for(uint ir=0;ir<regIds2.size();ir++){
              int r2 = regIds2[ir];
              ModelRegion* REG = MTHREAD->MD->getRegion(r2);
              regPx = REG->getMyPixels();
              for (uint p=0;p<regPx.size();p++){
                Pixel* px = regPx[p];
                double vol_transfer = min(px->getDoubleValue(layerName,true)/1000000,px->vol[i][u]) ; // Vol in the layer are in m^3, in the model in Mm^3
                px->vol[i][u] -= vol_transfer;
                px->vol[o][u] += vol_transfer;
             }
           }
          }
        }
      }
    }
  }

  if(what=="area"){
    /**
    Allocate area proportionally to volumes (see file test_proportional_computation_of_areas_from_volumes.ods)
    Example:
    FtIn   FtOut Vtrasfer
    con    ash   0.2
    brHf   ash   0.1
    brCopp ash   0.3
    con    oak   0.3
    brHf   oak   0.2
    brCopp oak   0.1

           Vorig Aorig Vnew  Anew
    con    10    30    9.5   28.5  Aorig-Aorig*(Vtrasfer1/Vorig)-Aorig(Vtrasfer2/Vorig)
    brHf   5     20    4.7   18.8
    brCopp 2     20    1.6   16
    ash    0     0     0.6   4     Aorig1*Vtrasfer1/(Vorig1)+Aorig2*Vtrasfer2/(Vorig2)+...
    oak    0     0     0.6   2.7
                 70          70
    */
    // first, before transfering areas, saving the original ones (we already saved the vols in the what="vol" section, that is called before this one)..
    vector<vector<vector<double>>> original_areas(pxC, vector<vector<double>>(nFTypes, vector<double>(nDC, 0.0))); // by px counter, ftype, dc
    for(uint i=0;i<fTypes.size();i++){
      for (uint u=0; u<dClasses.size(); u++){
        int pxC_loc = 0;
        for(uint ir=0;ir<regIds2.size();ir++){
          int r2 = regIds2[ir];
          ModelRegion* REG = MTHREAD->MD->getRegion(r2);
          regPx = REG->getMyPixels();
          for (uint p=0;p<regPx.size();p++){
            Pixel* px = regPx[p];
            original_areas[pxC_loc][i][u] += px->area_l[i][u];
            pxC_loc ++;
          }
        }
      }
    }


    // transferred areas ordered by pxcounter, i and then o ftype. Used to then repart the 0 diameter class..
    vector<vector<vector<double>>> transferred_areas(pxC, vector<vector<double>>(nFTypes, vector<double>(nFTypes, 0.0))); // initialize a 3d vector of nFTypes zeros.

    for(uint i=0;i<fTypes.size();i++){
      string fti = fTypes[i];
      for(uint o=0;o<fTypes.size();o++){
        string fto = fTypes[o];
        for (uint u=1; u<dClasses.size(); u++){ // first diameter class area is comuted proportionally..
          string layerName =  "spInput#vol#"+fto+"#"+fti+"#"+i2s(u);
          if (MTHREAD->GIS->layerExist(layerName)){
            int pxC_loc = 0;
            for(uint ir=0;ir<regIds2.size();ir++){
              int r2 = regIds2[ir];
              ModelRegion* REG = MTHREAD->MD->getRegion(r2);
              regPx = REG->getMyPixels();
              for (uint p=0;p<regPx.size();p++){
                Pixel* px = regPx[p];
                double vol_i_orig    = original_vols[pxC_loc][i][u];
                double vol_transfer  = vol_i_orig?px->getDoubleValue(layerName,true)/1000000:0.0; // Vol in the layer are in m^3, in the model in Mm^3
                double area_i_orig   = original_areas[pxC_loc][i][u];
                double area_transfer = vol_i_orig?area_i_orig*vol_transfer/vol_i_orig:0.0;
                px->area_l[i][u] -=  area_transfer;
                px->area[i][u]    = px->area_l[i][u];
                px->area_l[o][u] += area_transfer;
                px->area[o][u]    = px->area_l[o][u];
                transferred_areas[pxC_loc][i][o] += area_transfer;
                pxC_loc ++;
             }
           }
          }
        }
      }
    }

    // Moving the area in the 0 diameter class, for which no info is normally available..
    double pxC_loc = 0;
    for(uint ir=0;ir<regIds2.size();ir++){
      int r2 = regIds2[ir];
      ModelRegion* REG = MTHREAD->MD->getRegion(r2);
      regPx = REG->getMyPixels();
      for (uint p=0;p<regPx.size();p++){
        Pixel* px = regPx[p];
        for(uint i=0;i<fTypes.size();i++){
          for(uint o=0;o<fTypes.size();o++){
            double area_i_orig = 0.0;
            for (uint u=1; u<dClasses.size(); u++){ // we want to skip the 0 diameter class, this is why we don't simply use vSum()..
              area_i_orig += original_areas[pxC_loc][i][u];
            }
            double area_transfer_u0 = area_i_orig?original_areas[pxC_loc][i][0]*(transferred_areas[pxC_loc][i][o]/area_i_orig):0.0;
            px->area_l[i][0] -= area_transfer_u0 ;
            px->area[i][0] = px->area_l[i][0];
            px->area_l[o][0] += area_transfer_u0 ; // bug corrected 20151130: it was 0 instead of o (output) !!
            px->area[o][0] = px->area_l[0][0];     // bug corrected 20151130: it was 0 instead of o (output) !!
          }
        }
        pxC_loc++;
      }
    }

    // Alligning the area memorised in the px layers to the new areas of the ft..
    for(uint i=0;i<fTypes.size();i++){
      string fti_id = fTypes[i];
      forType* fti  = MTHREAD->MD->getForType(fti_id);
      int ft_memType = fti->memType;
      string ft_layerName = fti->forLayer;
      //if(ft_memType==3){
      //  MTHREAD->GIS->addLayer(ft_layerName,ft_layerName,false,true); //20151130: no needed as we already added it in applyForestReclassification (yes, odd, as memory type 3 layerrs do not have any reclassification rule associated, but if I don't add the layer at that time I got other errors)
      // }
      if(ft_memType==3 ||ft_memType==2){
        for(uint ir=0;ir<regIds2.size();ir++){
          int r2 = regIds2[ir];
          ModelRegion* REG = MTHREAD->MD->getRegion(r2);
          regPx = REG->getMyPixels();
          for (uint p=0;p<regPx.size();p++){
            Pixel* px = regPx[p];
            double area_px = vSum(px->area[i]);
            px->changeValue(ft_layerName,area_px*10000);
          }
        }
      }
    }
  } // end if what is area
}

void
ModelCoreSpatial::printDebugInitRegionalValues(){
  // Print debug stats on inventory and supplies in each region..
  cout << "Printing debug information on initial regional inventories and supplies.." << endl;
  cout << "Reg\tProduct\t\tInv\tSt\tSa\tSl" << endl;
  for(uint r1=0;r1<l2r.size();r1++){
    for(uint r2c=0;r2c<l2r[r1].size();r2c++){
      for(uint p=0;p<priProducts.size();p++){
        int r2 = l2r[r1][r2c];
        double inv = gpd("in",r2,priProducts[p],secondYear);
        double st = gpd("st",r2,priProducts[p],secondYear);
        double sl = gpd("sl",r2,priProducts[p],secondYear);
        double sa = gpd("sa",r2,priProducts[p],secondYear);
        cout << r2 << "\t" << priProducts[p] << "\t\t" << inv << "\t" << st << "\t" << sl << "\t" << sa << endl;
      }
    }
  } // end of r1 region
  exit(0);

}

/**
 * @brief ModelCoreSpatial::allocateHarvesting
 * @param total_st vector of total supply by primary products
 * @return a vector of the remaining supply that goes allocated to alive timber (that is, to harvesting)
 *
 * The algorithm is such that is loops the deathTimberInventory map for each year (newer to older), dc (higher to smaller) and ft.
 * It compute the primary products allocable from that combination and allocate the cell amount to decrease the total_st of that products
 * in a proportional way to what still remain of the allocable products.
 *
 * It is called in the runMarketModule() function.
 *
 */

vector <double>
ModelCoreSpatial::allocateHarvesting(vector<double> total_st, const int &regId){
   if(!MD->getBoolSetting("useDeathTimber",DATA_NOW)) return total_st;
   //vector <double> st_fromHarv(priProducts.size(),0.);
   //map<iisskey, double > *  deathTimberInventory= MD->getDeathTimberInventory();
   int maxYears = MD->getMaxYearUsableDeathTimber();
   int currentYear = MTHREAD->SCD->getYear();
   for(uint y = currentYear-1; y>currentYear-1-maxYears; y--){
     for (int u = dClasses.size()-1; u>=0; u--){  // I need to specify u as an integer !
       string dc = dClasses.at(u);
       for (uint f=0; f<fTypes.size(); f++){
         string ft = fTypes[f];
         vector<int>allocableProducts =  MD->getAllocableProductIdsFromDeathTimber(regId, ft, dc, y, currentYear-1);
         iisskey key(y,regId,ft,dc);
         double deathTimber = MD->deathTimberInventory_get(key);
         double sum_total_st_allocable = 0;
         // Computing shares/weights or remaining st to allcate
         for(uint ap=0;ap<allocableProducts.size();ap++){
           sum_total_st_allocable += total_st.at(allocableProducts[ap]);
         }
         for(uint ap=0;ap<allocableProducts.size();ap++){
           double allocableShare = sum_total_st_allocable?total_st.at(allocableProducts[ap])/sum_total_st_allocable:0.0;
           double allocated = min(total_st[allocableProducts[ap]],deathTimber*allocableShare);
           MD->deathTimberInventory_incrOrAdd(key,-allocated);
           total_st[allocableProducts[ap]] -= allocated;
         }
       }
     }
   }
   return total_st;
}


/**
 * This function has two tasks:
 * 1) compute the policy balances (- -> public cost,subside; + -> public revenue, tax) for pol_mktDirInt_s, pol_mktDirInt_d, pol_trSub and pol_tcSub
 *    (pol_fiSub are done directly in the runManagementModule() function)
 * 2) compute the producer/consumer surpluses
 */
void
ModelCoreSpatial::computeEconomicBalances(){
  //int thisYear     =  MTHREAD->SCD->getYear();
  int previousYear = MTHREAD->SCD->getYear()-1;

  for(uint r2=0; r2<regIds2.size();r2++){
    int regId = regIds2[r2];
    ModelRegion* reg = MD->getRegion(regId);

    for(uint p=0;p<priProducts.size();p++){
      double pol_mktDirInt_s   = gpd("pol_mktDirInt_s",regId,priProducts[p]);
      double pol_mktDirInt_s_1 = gpd("pol_mktDirInt_s",regId,priProducts[p],previousYear);
      double sigma             = gpd("sigma",regId,priProducts[p]);
      double in                = gpd("in",regId,priProducts[p]);
      double in_1              = gpd("in",regId,priProducts[p], previousYear);
      double pc                = gpd("pc",regId,priProducts[p]);
      double pc_1              = gpd("pc",regId,priProducts[p], previousYear);
      double sc                = gpd("sc",regId,priProducts[p]);
      double sc_1              = gpd("sc",regId,priProducts[p],previousYear);
      double gamma_incr        = gpd("gamma_incr",regId,priProducts[p]); // elast supply to increasing stocks
      double gamma_decr        = gpd("gamma_decr",regId,priProducts[p]); // elast supply to decreasing stocks

      double gamma              = in>in_1 ? gamma_incr: gamma_decr;
      double polBal_mktDirInt_s = pc * (1-pol_mktDirInt_s) * sc;
      double surplus_prod       =     pc * sc -
                                      pc_1
                                    * pol_mktDirInt_s_1
                                    * pow(pol_mktDirInt_s,-1)
                                    * pow(sc_1,-1/sigma)
                                    * pow(in_1/in,gamma/sigma)
                                    * (sigma/(sigma+1))
                                    * pow(sc,(sigma+1)/sigma);

      spd(polBal_mktDirInt_s,"polBal_mktDirInt_s",regId,priProducts[p],DATA_NOW,true); // M€
      spd(surplus_prod,"surplus_prod",regId,priProducts[p],DATA_NOW,true);
    }
    for(uint p=0;p<secProducts.size();p++){
      double pol_mktDirInt_d   = gpd("pol_mktDirInt_d",regId,secProducts[p]);
      double pol_mktDirInt_d_1 = gpd("pol_mktDirInt_d",regId,secProducts[p],previousYear);
      double sigma             = gpd("sigma",regId,secProducts[p]);
      double pol_trSub         = gpd("pol_trSub",regId,secProducts[p]);
      double pc                = gpd("pc",regId,secProducts[p]);
      double pc_1              = gpd("pc",regId,secProducts[p],previousYear);
      double dc                = gpd("dc",regId,secProducts[p]);
      double dc_1              = gpd("dc",regId,secProducts[p],previousYear);
      double dc0               = gpd("dc",regId,secProducts[p],secondYear);
      double m                 = gpd("m",regId,secProducts[p]);
      double sl                = gpd("sl",regId,secProducts[p]); // sl because dl include also the production from byproduct that has not subsides in the model

      double polBal_mktDirInt_d = pc * (pol_mktDirInt_d - 1) * dc;
      double polBal_trSub       = m * (pol_trSub-1) * sl;
      double trs                = 0.5; // 0.1: we consider 90 percent of the demand integral
      double surplus_cons       =   pc_1
                                  * pol_mktDirInt_d_1
                                  * pow(dc_1,-1/sigma)
                                  * pow(pol_mktDirInt_d,-1)
                                  * (sigma/(sigma+1))
                                  * (pow(dc,(sigma+1)/sigma) - pow(trs*dc0,(sigma+1)/sigma))
                                  - (dc - trs * dc0) * pc ;

      spd(polBal_mktDirInt_d,"polBal_mktDirInt_d",regId,secProducts[p],DATA_NOW,true); // M€
      spd(polBal_trSub,"polBal_trSub",regId,secProducts[p],DATA_NOW,true); // M€
      spd(surplus_cons,"surplus_cons",regId,secProducts[p],DATA_NOW,true); // M€
    }

    for(uint p=0;p<allProducts.size();p++){
      double pol_tcSub = gpd("pol_tcSub",regId,allProducts[p]);
      double polBal_tcSub = 0;
      vector<ModelRegion*> siblings = reg->getSiblings();
      for(uint rTo=0;rTo<siblings.size();rTo++){
        int regId2 = siblings[rTo]->getRegId();
        double ct = gpd("ct",regId,allProducts[p],DATA_NOW,i2s(regId2));
        double rt = gpd("rt",regId,allProducts[p],DATA_NOW,i2s(regId2));
        polBal_tcSub += ct*(pol_tcSub-1) * rt;
      }
      spd(polBal_tcSub,"polBal_tcSub",regId,allProducts[p],DATA_NOW,true); // M€
    }

  }

}

/**
 * Return the average age using cumTp vector
 */
double
ModelCoreSpatial::getAvgAgeByDc(Pixel* px, int ft, int dc){
  if(dc ==  dClasses.size()-1) { return px->cumTp[ft][dc] * 1.2;}
  else {return (px->cumTp[ft][dc]+px->cumTp[ft][dc+1])/2; }
}




// added for carbon project
/**
  * Return the Volume/ha in a forest after a given number of year after planting, for a specific forest type
  */
double
ModelCoreSpatial::getVHaByYear(const Pixel* px, const int& ft, const int& year, const double& extraBiomass_ratio) const {
    double vHa = 0;
    //int regId = px->getMyRegion()->getRegId();
    //double extraBiomass_ratio = gfd("extraBiomass_ratio",regId,fTypes[ft],"",DATA_NOW);

    for (int u = 0; u<dClasses.size(); u++){
        double cumTP_exp = px->cumTp_exp[ft][u];
        if (year>=cumTP_exp){
            vHa = px->vHa_exp[ft][u]*(1+extraBiomass_ratio);
        } else {
            return vHa;
        }
    }
    return vHa;
}
