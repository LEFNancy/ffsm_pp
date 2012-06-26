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
	ModelData* MD = MTHREAD->MD;
	int firstYear = MD->getIntSetting("initialYear");
	int secondYear = firstYear+1;
	vector <int> regIds2 = MD->getRegionIds(2);
	vector <string> priProducts = MD->getStringVectorSetting("priProducts");
	vector <string> secProducts = MD->getStringVectorSetting("secProducts");

	for(uint i=0;i<regIds2.size();i++){
		for(uint sp=0;sp<secProducts.size();sp++){
			double value = 0;
			for (uint pp=0;pp<priProducts.size();pp++){
				value += MD->getProdData("pl",regIds2[i],priProducts[pp],secondYear)*
						 MD->getProdData("a",regIds2[i],priProducts[pp],secondYear,secProducts[sp]);
			}
			value += MD->getProdData("m",regIds2[i],secProducts[sp],secondYear);
			cout << "Value in runInitPeriod: " << value << endl;
			MD->setProdData(value,"pl",regIds2[i],secProducts[sp],secondYear);
		}
	}

};

void
ModelCore::runSimulationYear(int year_h){

};
