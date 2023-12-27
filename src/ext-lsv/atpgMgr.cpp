#include "atpgMgr.h"
#include "lsvUtils.h"
// #include "atpgPKG/atpg.h"

using namespace CoreNs;


// the gateID is the ABC gate ID
int AtpgMgr::runSingleSAF(Abc_Ntk_t* pNtk, int gateID, CoreNs::Fault::FAULT_TYPE faultType, bool getAtpgVal){
    // std::cout << "=======================" << std::endl;
    // for(auto i : atpg->pCircuit_->unorderedIdtoOrderId)
    //     std::cout << i.first << ' ' << i.second << std::endl;
    // std::cout << "=======================" << std::endl;
    int internalGateId = atpg->pCircuit_->unorderedIdtoOrderId.at(gateID - 1);
    int res = -1;
    if(getAtpgVal == 0)
        res = atpg->runSingleSAF(pPatPro, internalGateId, faultType); // res = 0 -> pattern found

    if(getAtpgVal == 1){
        std::vector<int> &maVal = faultType == Fault::SA0 ? ma0Val : ma1Val;
        if(faultType == Fault::SA0){
            sa0GateId = gateID;
        }
        else{
            sa1GateId = gateID;
        }
        maVal.resize(Abc_NtkObjNum(pNtk), -1);
        res = atpg->runSingleSAF(pPatPro, internalGateId, faultType, pNtk, &maVal); // res = 0 -> pattern found
        // if(res == 1)
        //     return 1;
        // Abc_Obj_t* pObj;
        // int i;
        // Abc_NtkForEachObj(pNtk, pObj, i){
        //     if(Abc_ObjType(pObj) == ABC_OBJ_CONST1)
        //         continue;
        //     maVal[i] = -1;
        //     unsigned atpgval = static_cast<unsigned>(atpg->pCircuit_->circuitGates_[atpg->pCircuit_->unorderedIdtoOrderId.at(pObj->Id - 1)].prevAtpgValStored_);
        //     if(atpgval == 0U || atpgval == 1U || atpgval == 3U || atpgval == 4U)
        //         maVal[i] = atpgval;
        // }
    }
    return res;

}
// can not be in TFO!!!
// if a node have different value at MAs(nt = 0) & MAs(nt = 1) -> substitute nt with the node
int AtpgMgr::simpleNodeSub(Abc_Ntk_t* pNtk){

    // for(auto p:vTFO)
    //     cout << p->Id << endl;


    if(sa0GateId != sa1GateId){
        std::cout << "sa0GateId != sa1GateId !!!" << std::endl;
        return -1;
    }
    std::unordered_set<Abc_Obj_t*> vTFO;
    collectTFO(vTFO, Abc_NtkObj(pNtk, sa1GateId));
    std::vector<int> possibleSubNodes1;  // == 1 when nt sa0, == 0 when nt sa1
    std::vector<int> possibleSubNodes2;  // == 0 when nt sa0, == 1 when nt sa1
    Abc_Obj_t* pObj;
    int i;
    Abc_NtkForEachNode(pNtk, pObj, i){
        if(pObj == Abc_NtkObj(pNtk, sa0GateId))
            continue;
        if(ma1Val[pObj->Id] != -1 && ma0Val[pObj->Id] != -1 && !vTFO.count(pObj)){
            if(ma1Val[pObj->Id] == 0 && ma0Val[pObj->Id] == 1)
                possibleSubNodes1.push_back(pObj->Id);
            else if(ma1Val[pObj->Id] == 1 && ma0Val[pObj->Id] == 0)
                possibleSubNodes2.push_back(pObj->Id);

        }
    }
    pObj = Abc_NtkObj(pNtk, sa0GateId);
    Abc_Obj_t* pFanout;
    bool AigNodeHasComplFanoutEdge = Abc_AigNodeHasComplFanoutEdge(pObj);
    bool isP1empty = possibleSubNodes1.empty(), isP2empty = possibleSubNodes2.empty();
    if((isP1empty && isP2empty)||(isP1empty && !AigNodeHasComplFanoutEdge )|| (isP2empty && AigNodeHasComplFanoutEdge)){
        std::cout << pObj->Id << " cannot be replaced" << std::endl;
        return 0;
    }
    Abc_ObjForEachFanout(pObj, pFanout, i){
        if(isFanoutC(pObj, pFanout))    // if the fanout edge is complemented, use the node in possibleSubNode2
            Abc_ObjPatchFanin(pFanout, pObj, Abc_NtkObj(pNtk, possibleSubNodes2.front()));
        else
            Abc_ObjPatchFanin(pFanout, pObj, Abc_NtkObj(pNtk, possibleSubNodes1.front()));
    }
    Abc_AigDeleteNode((Abc_Aig_t *)pNtk->pManFunc, pObj);
    return 1;
    // 應該不用挑最小的，因為是topological order再push_back的應該第一個就是最小的
    // int minLevel = 2147483647, minlvlnode = -1;
    // for(auto j : possibleSubNode){
    //     int curLevel = atpg->pCircuit_->circuitGates_[atpg->pCircuit_->unorderedIdtoOrderId.at(j - 1)].numLevel_;
    //     if(minLevel > curLevel){
    //         minLevel = curLevel;
    //         minlvlnode = j;
    //     }
    // }
    // return minlvlnode;
    
} 
