#ifndef ATPGMGR_H
#define ATPGMGR_H

// #include "atpgPKG/atpg.h"
// #include "atpgPKG/circuit.h"
// #include "atpgPKG/fault.h"
// #include "atpg/atpg.h"
#include "atpg/atpg_mgr.h"
#include "atpg/gate1.h"
#include "base/main/main.h"
#include "atpg/circuit1.h"
#include "atpg/fault.h"
#include "base/abc/abc.h"
#include "lsvUtils.h"
#include <unordered_set>

// using namespace std;
using namespace CoreNs; 
namespace LSV{

class AtpgMgr{
public:
	inline AtpgMgr();
    inline AtpgMgr(Abc_Ntk_t* pNtk);
    inline ~AtpgMgr();
	// inline void reset(Abc_Ntk_t* pNtk);
	inline void init(Abc_Ntk_t* pNtk);
    inline int myselftest(Abc_Ntk_t* pNtk, int GateId );
	// res == 0 -> PATTERN_FOUND, res == 1 -> UNTESTTABLE
	// run single stuck at fault
    // int runSingleSAF(Abc_Ntk_t* pNtk, int gateID, CoreNs::Fault::FAULT_TYPE faultType, bool getAtpgVal = 0); 
	// if target gate != -1 --> gateId is the fanout gate of the target gate(i.e., the wire that happens stuck at fault is the wire between them)
    int runSingleSAF(Abc_Ntk_t* pNtk, int gateID, CoreNs::Fault::Type faultType, int targetGateId = -1, bool getAtpgVal = 0); 
	// int runSingleSAF(PatternProcessor *pPatternProcessor, int gateID, CoreNs::Fault::FAULT_TYPE faultType,  bool getAtpgVal = 0);
	// if a node have different value at MAs(nt = 0) & MAs(nt = 1) -> substitute nt with the node
	int simpleNodeSub(Abc_Ntk_t* pNtk); // return the sub node GateId
	int NodeTarget(){ return sa0GateId == sa1GateId ? sa0GateId : -1;}
	int collectCandidateDestination(Abc_Ntk_t* pNtk, unordered_map<int, bool>& candidateDestinations, int targetWireId, CoreNs::Fault::Type faultType);
	int collectSMA(Abc_Ntk_t* pNtk, unordered_map<int, bool>& SMAs, int targetWireId, CoreNs::Fault::Type faultType);
	int DestinationNodeCheck(Abc_Ntk_t* pNtk, unordered_map<int, bool>* SMAs,int targetWireId, CoreNs::Fault::Type faultTypeT ,int candidateDestination, CoreNs::Fault::Type faultTypeD);
	int sourceNodeIdentification(Abc_Ntk_t* pNtk, int validDestinationNodeId, vector<Abc_Obj_t*>& possibleSourceNodes1, vector<Abc_Obj_t*>& possibleSourceNodes2);
	int addRectificationNetwork(Abc_Ntk_t* pNtk, Abc_Obj_t* pDstNode, CoreNs::Fault::Type ndFaultType,unordered_map<int, bool>& SMAs);
	int addRectificationNetwork_v2(Abc_Ntk_t* pNtk, Abc_Obj_t* pDstNode, CoreNs::Fault::Type ndFaultType, Abc_Obj_t* pPossibleSMANode);
	Abc_Obj_t* getANDSMAs(Abc_Ntk_t* pNtk, unordered_map<int, bool>& SMAs);
	int rewire(Abc_Frame_t* pAbc, Abc_Ntk_t* pNtk, Abc_Obj_t* pTargetNode, Abc_Obj_t* pTargetNodeFanout, bool* cone = 0);

private:
	// CoreNs::Simulator* pSimulator;
	// CoreNs::Atpg* atpg;
	// CoreNs::PatternProcessor *pPatPro;
	CoreNs::Circuit* pCir;
	CoreNs::AtpgMgr* atpgMgr;
    std::vector<int> ma1Val;
	std::vector<int> ma0Val;	// each entry is the MA of the corresponding gate ID
	int sa0GateId;	// to mark the owner of ma0Val
	int sa1GateId; 	// to mark the owner of ma1Val // abc GateId
	int numOfANDandINV; // number of aig node and the complemented edge(which will be replaced by INV and positive edge later)
	inline void calculateTotalGates(Abc_Ntk_t* pNtk);

};
}

LSV::AtpgMgr::AtpgMgr(){
	atpgMgr = new CoreNs::AtpgMgr;
	pCir = new Circuit;
	// pSimulator = 0; 
	// atpg = 0; 
	// pPatPro = 0; 
	numOfANDandINV = 0; 
	sa0GateId = -2; 
	sa1GateId = -1;
}
LSV::AtpgMgr::AtpgMgr(Abc_Ntk_t* pNtk){
	pCir = new Circuit;
	// pCir->buildCircuit(pNtk, 1);
	// pSimulator = new Simulator(pCir);
	// atpg = new Atpg(pCir, pSimulator);
}

LSV::AtpgMgr::~AtpgMgr(){
	// delete atpg;
	// delete pSimulator;
	delete pCir;
	delete atpgMgr;
	// delete pPatPro;
}

void LSV::AtpgMgr::init(Abc_Ntk_t* pNtk){
	if(pCir != 0){
		delete pCir;
		pCir = 0;
	}
	// if(pSimulator != 0){
	// 	delete pSimulator;
	// 	pSimulator = 0;
	// }
	if(atpgMgr != 0){
		delete atpgMgr;
		atpgMgr = 0;
	}
	// if(pPatPro != 0){
	// 	delete pPatPro;
	// 	pPatPro = 0;
	// }
	ma0Val.clear();
	ma1Val.clear(); 
	sa0GateId = -1;	// to mark the owner of ma0Val
	sa1GateId = -1;
	numOfANDandINV = 0; 
	calculateTotalGates(pNtk);
	pCir = new Circuit;
	pCir->buildCircuit(pNtk, 1, numOfANDandINV);
	atpgMgr = new CoreNs::AtpgMgr;
	// pPatPro = new PatternProcessor;
	// pSimulator = new Simulator(pCir);
	// atpg = new Atpg(pCir, pSimulator);
}




int LSV::AtpgMgr::myselftest(Abc_Ntk_t* pNtk, int GateId){
	calculateTotalGates(pNtk);
	pCir->buildCircuit(pNtk, 1, numOfANDandINV);
	for(int i = 0; i < pCir->ngate_; ++i){
		CoreNs::Gate& thisgate = pCir->gates_[i];
		thisgate.print();
	}
	// return atpgMgr->runSingleSAF(pCir,0, pCir->unorderedIdtoOrderId.at(GateId - 1), Fault::SA0);
    // Abc_Obj_t *pPo;
    // int i;
    // Abc_NtkForEachPo(pNtk, pPo, i){
    //     std::cout << pPo->fCompl0 << std::endl;
    // }
    // return;
}

void LSV::AtpgMgr::calculateTotalGates(Abc_Ntk_t* pNtk){
	Abc_Obj_t* pObj;
	int i;
	numOfANDandINV = Abc_NtkNodeNum(pNtk);
	Abc_NtkForEachObj(pNtk, pObj, i){
		if(Abc_ObjFaninC0(pObj) == 1)
			++numOfANDandINV;
		if( Abc_ObjType(pObj) != ABC_OBJ_PO && Abc_ObjFaninC1(pObj) == 1)
			++numOfANDandINV;
	}
}


#endif
// ATPGMGR_H
