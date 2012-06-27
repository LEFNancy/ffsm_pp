#include <cmath>

#include "ModelCore.h"
#include "ModelData.h"
#include "ThreadManager.h"

ModelCore::ModelCore(ThreadManager* MTHREAD_h){
	MTHREAD = MTHREAD_h;
}

ModelCore::~ModelCore(){

}


void
ModelCore::runInitPeriod(){
	msgOut(MSG_INFO, "Starting init simulation..");
	ModelData* MD = MTHREAD->MD;
	int firstYear = MD->getIntSetting("initialYear");
	int secondYear = firstYear+1;
	int WL2 = MD->getIntSetting("worldCodeLev2");
	vector <int> regIds2 = MD->getRegionIds(2);
	vector <string> priProducts = MD->getStringVectorSetting("priProducts");
	vector <string> secProducts = MD->getStringVectorSetting("secProducts");
	vector <string> allProducts = priProducts;
	allProducts.insert( allProducts.end(), secProducts.begin(), secProducts.end() );




	for(uint i=0;i<regIds2.size();i++){
		//RPAR('pl',i,p_tr,t-1)   =  sum(p_pr, a(p_pr,p_tr)*RPAR('pl',i,p_pr,t-1))+m(i,p_tr);
		for(uint sp=0;sp<secProducts.size();sp++){
			double value = 0;
			for (uint pp=0;pp<priProducts.size();pp++){
				value += MD->getProdData("pl",regIds2[i],priProducts[pp],secondYear)*
						 MD->getProdData("a",regIds2[i],priProducts[pp],secondYear,secProducts[sp]);
			}
			value += MD->getProdData("m",regIds2[i],secProducts[sp],secondYear);
			MD->setProdData(value,"pl",regIds2[i],secProducts[sp],secondYear);
		}
		// RPAR('dl',i,p_pr,t-1)   =  sum(p_tr, a(p_pr,p_tr)*RPAR('sl',i,p_tr,t-1));
		for (uint pp=0;pp<priProducts.size();pp++){
			double value=0;
			for(uint sp=0;sp<secProducts.size();sp++){
				value += MD->getProdData("sl",regIds2[i],secProducts[sp],secondYear)*
						 MD->getProdData("a",regIds2[i],priProducts[pp],secondYear, secProducts[sp]);
			}
			MD->setProdData(value,"dl",regIds2[i],priProducts[pp],secondYear,true);
		}
		// RPAR('st',i,prd,t-1)    =  RPAR('sl',i,prd,t-1)+RPAR('sa',i,prd,t-1);
		// RPAR('dt',i,prd,t-1)    =  RPAR('dl',i,prd,t-1)+RPAR('da',i,prd,t-1);
		for (uint ap=0;ap<allProducts.size();ap++){
			double stvalue =   MD->getProdData("sl",regIds2[i],allProducts[ap],secondYear)
							 + MD->getProdData("sa",regIds2[i],allProducts[ap],secondYear);
			double dtvalue =   MD->getProdData("dl",regIds2[i],allProducts[ap],secondYear)
							 + MD->getProdData("da",regIds2[i],allProducts[ap],secondYear);
			MD->setProdData(stvalue,"st",regIds2[i],allProducts[ap],secondYear,true);
			MD->setProdData(dtvalue,"dt",regIds2[i],allProducts[ap],secondYear,true);
		}
		 //q1(i,p_tr)  =  1/(1+((RPAR('dl',i,p_tr,t-1)/RPAR('da',i,p_tr,t-1))**(1/psi(i,p_tr)))*(RPAR('pl',i,p_tr,t-1)/PT(p_tr,t-1)));
		for(uint sp=0;sp<secProducts.size();sp++){
			double q1 = 1/(
							1+ pow(
									MD->getProdData("dl",regIds2[i],secProducts[sp],secondYear)
								   /MD->getProdData("da",regIds2[i],secProducts[sp],secondYear)
								,
								   1/MD->getProdData("psi",regIds2[i],secProducts[sp],secondYear)
								)
							*(
								MD->getProdData("pl",regIds2[i],secProducts[sp],secondYear)
							   /MD->getProdData("pw",WL2,secProducts[sp],secondYear)
							)
						);
			MD->setProdData(q1,"q1",regIds2[i],secProducts[sp],firstYear,true);
		}


	} // end for each region






};

void
ModelCore::runSimulationYear(int year_h){

};
