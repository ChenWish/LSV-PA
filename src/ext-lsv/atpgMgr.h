#ifndef ATPGMGR_H
#define ATPGMGR_H

#include "atpgPKG/atpg.h"
#include "atpgPKG/circuit.h"
#include "atpgPKG/fault.h"
#include "base/abc/abc.h"

// using namespace std;
using namespace CoreNs; 

class AtpgMgr{
public:
	inline AtpgMgr(){pCir = 0; pSimulator = 0; atpg = 0; pPatPro = 0; numOfANDandINV = 0;};
    inline AtpgMgr(Abc_Ntk_t* pNtk);
    inline ~AtpgMgr();
	// inline void reset(Abc_Ntk_t* pNtk);
	inline void init(Abc_Ntk_t* pNtk);
    inline void myselftest(Abc_Ntk_t* pNtk);
    int runSingleSAF(int gateID, CoreNs::Fault::FAULT_TYPE faultType); // run single stuck at fault
	CoreNs::Circuit* pCir;
private:
	CoreNs::Simulator* pSimulator;
	CoreNs::Atpg* atpg;
	CoreNs::PatternProcessor *pPatPro;
	int numOfANDandINV; // number of aig node and the complemented edge(which will be replaced by INV and positive edge later)
	inline void calculateTotalGates(Abc_Ntk_t* pNtk);

};
AtpgMgr::AtpgMgr(Abc_Ntk_t* pNtk){
	pCir = new Circuit;
	// pCir->buildCircuit(pNtk, 1);
	pSimulator = new Simulator(pCir);
	atpg = new Atpg(pCir, pSimulator);
}

AtpgMgr::~AtpgMgr(){
	delete atpg;
	delete pSimulator;
	delete pCir;
	delete pPatPro;
}
void AtpgMgr::init(Abc_Ntk_t* pNtk){
	if(pCir != 0){
		delete pCir;
		pCir = 0;
	}
	if(pSimulator != 0){
		delete pSimulator;
		pSimulator = 0;
	}
	if(atpg != 0){
		delete atpg;
		atpg = 0;
	}
	if(pPatPro != 0){
		delete pPatPro;
		pPatPro = 0;
	}
	calculateTotalGates(pNtk);
	pCir = new Circuit;
	pCir->buildCircuit(pNtk, 1, numOfANDandINV);
	pPatPro = new PatternProcessor;
	// pSimulator = new Simulator(pCir);
	atpg = new Atpg(pCir, pSimulator);
}


void AtpgMgr::myselftest(Abc_Ntk_t* pNtk){
    Abc_Obj_t *pPo;
    int i;
    Abc_NtkForEachPo(pNtk, pPo, i){
        std::cout << pPo->fCompl0 << std::endl;
    }
    return;
}

void AtpgMgr::calculateTotalGates(Abc_Ntk_t* pNtk){
	Abc_Obj_t* pObj;
	int i;
	numOfANDandINV = Abc_NtkNodeNum(pNtk);
	Abc_NtkForEachObj(pNtk, pObj, i){
		if(Abc_ObjFaninC0(pObj) == 1)
			++numOfANDandINV;
		if(Abc_ObjFaninC1(pObj) == 1)
			++numOfANDandINV;
	}
}


#endif
// ATPGMGR_H

