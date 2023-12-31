#include "atpgMgr.h"
#include "lsvUtils.h"
// #include "base/abc/abcAig.c"
// #include "atpgPKG/atpg.h"

using namespace CoreNs;

extern "C"{
    int Lsv_AigReplace( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, Abc_Obj_t* pFanout, int fUpdateLevel, int ispNewCompl, int forcedFanouttoPositive);
    // Abc_Obj_t * Abc_AigAndCreate( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
    // Abc_Obj_t * Abc_AigAndCreateFrom( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * pAnd );
    // void        Abc_AigAndDelete( Abc_Aig_t * pMan, Abc_Obj_t * pThis );
    // void        Abc_AigResize( Abc_Aig_t * pMan );
}

// the gateID is the ABC gate ID
// if target gate != -1 --> gateId is the fanout gate of the target gate(i.e., the wire that happens stuck at fault is the wire between them)
int LSV::AtpgMgr::runSingleSAF(Abc_Ntk_t* pNtk, int gateID, CoreNs::Fault::Type faultType, int targetGateId, bool getAtpgVal){
    // std::cout << "=======================" << std::endl;
    // for(auto i : atpg->pCircuit_->unorderedIdtoOrderId)
    //     std::cout << i.first << ' ' << i.second << std::endl;
    // std::cout << "=======================" << std::endl;
    // int internalGateId = atpg->pCircuit_->unorderedIdtoOrderId.at(gateID - 1);
    int internalGateId = pCir->unorderedIdtoOrderId.at(gateID - 1);
    int internalTargetGateId = -1;
    if(targetGateId != -1)
        internalTargetGateId = pCir->unorderedIdtoOrderId.at(targetGateId - 1);
    int res = -1;
    if(getAtpgVal == 0)
        res = atpgMgr->runSingleSAF(pCir, internalGateId, faultType, internalTargetGateId); // res = 0 -> pattern found

    if(getAtpgVal == 1){
        std::vector<int> &maVal = faultType == Fault::SA0 ? ma0Val : ma1Val;
        if(faultType == Fault::SA0){
            sa0GateId = gateID;
        }
        else{
            sa1GateId = gateID;
        }
        maVal.resize(Abc_NtkObjNum(pNtk), -1);
        res = atpgMgr->runSingleSAF(pCir, internalGateId, faultType, internalTargetGateId, pNtk, &maVal); // res = 0 -> pattern found
        for(auto i : maVal){
            cout << i << ' ';
        }
        cout << endl;

        // res = atpg->runSingleSAF(pPatPro, internalGateId, faultType, pNtk, &maVal); // res = 0 -> pattern found
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
// #include "base/abc/abcAig.c"
// extern (*Abc_AigAndDelete_ptr)();
int LSV::AtpgMgr::simpleNodeSub(Abc_Ntk_t* pNtk){

    // for(auto p:vTFO)
    //     cout << p->Id << endl;
    // extern Abc_Obj_t * Abc_AigAndCreateFrom( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * pAnd );
    // extern void        Abc_AigAndDelete( Abc_Aig_t * pMan, Abc_Obj_t * pThis );

    if(sa0GateId != sa1GateId){
        std::cout << "sa0GateId != sa1GateId !!!" << std::endl;
        return 0;
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
    bool AigNodeHasPositiveFanoutEdge = Lsv_AigNodeHasPositiveFanoutEdge(pObj);
    bool isP1empty = possibleSubNodes1.empty(), isP2empty = possibleSubNodes2.empty();
    // if((isP1empty && isP2empty)||(isP1empty && !AigNodeHasComplFanoutEdge )|| (isP2empty && AigNodeHasComplFanoutEdge)){
    // if((AigNodeHasComplFanoutEdge && isP2empty) || (AigNodeHasPositiveFanoutEdge && isP1empty) || Abc_ObjType(Abc_ObjFanout0(pObj)) == ABC_OBJ_PO){
    if(isP2empty && isP1empty){
        std::cout << pObj->Id << " cannot be replaced" << std::endl;
        return 0;
    }
    int possibleSubNode = -1;
    int mycompl = 0;
    if(isP1empty){
        possibleSubNode = possibleSubNodes2.front();
        mycompl = 1;
    }
    else if(isP2empty){
        possibleSubNode = possibleSubNodes1.front();
        mycompl = 0;
    }
    else{
        possibleSubNode = possibleSubNodes1.front() > possibleSubNodes2.front() ? possibleSubNodes2.front() : possibleSubNodes1.front();
        mycompl = possibleSubNodes1.front() > possibleSubNodes2.front() ? 1 : 0;
    }
    cout << pObj->Id << " is replaced by "<<possibleSubNode << ".(simple node Sub)\n" ;
    Abc_ObjForEachFanout(pObj, pFanout, i){
        bool isFanoutCompl = isFanoutC(pObj, pFanout);
        int finalCompl = (mycompl ^ isFanoutCompl);
        Lsv_AigReplace( (Abc_Aig_t *)pNtk->pManFunc, pObj, Abc_NtkObj(pNtk, possibleSubNode), pFanout, 1, finalCompl, 0);
        // if(isFanoutC(pObj, pFanout))    // if the fanout edge is complemented, use the node in possibleSubNode2
        // {
        // index of the other fanin

        // int iFanin = Vec_IntFind( &pFanout->vFanins, pObj->Id );
        // assert(iFanin == 1 || iFanin == 0);
        // iFanin = iFanin == 1 ? 0 : 1;
        // Abc_AigAndDelete( (Abc_Aig_t *)pNtk->pManFunc, pFanout );
        // // remove the fanins of the old fanout
        // Abc_ObjRemoveFanins( pFanout );
        // // recreate the old fanout with new fanins and add it to the table
        // Abc_AigAndCreateFrom( (Abc_Aig_t *)pNtk->pManFunc,  Abc_ObjRegular(Abc_NtkObj(pNtk, possibleSubNode)),  Abc_ObjRegular(Abc_NtkObj(pNtk, iFanin)), pFanout );
        // Abc_AigAndCreateFrom( (Abc_Aig_t *)pNtk->pManFunc, pFanin1, pFanin2, pFanout );

        // Abc_ObjPatchFanin(pFanout, Abc_ObjRegular(pObj), Abc_ObjRegular(Abc_NtkObj(pNtk, possibleSubNode)));
        // Abc_AigAndDelete( (Abc_Aig_t *)pNtk->pManFunc, pFanout);
        // remove the old fanout node from the structural hashing table
        // recreate the old fanout with new fanins and add it to the table
        // Abc_AigAndCreateFrom( (Abc_Aig_t *)pNtk->pManFunc,  Abc_ObjRegular(Abc_NtkObj(pNtk, possibleSubNode)),  Abc_ObjRegular(Abc_NtkObj(pNtk, iFanin)), pFanout );
        
        // }
        // else
        //     Abc_ObjPatchFanin(pFanout, Abc_ObjRegular(pObj), Abc_ObjRegular(Abc_NtkObj(pNtk, possibleSubNodes1.front())));
        //     int iFanin = Vec_IntFind( &pFanout->vFanins, pObj->Id );
        //     assert(iFanin == 1 || iFanin == 0);
        //     iFanin = iFanin == 1 ? 0 : 1;
        //     // remove the old fanout node from the structural hashing table
        //     Abc_AigAndDelete( (Abc_Aig_t *)pNtk->pManFunc, pFanout);
        //     // recreate the old fanout with new fanins and add it to the table
        //     Abc_AigAndCreateFrom( (Abc_Aig_t *)pNtk->pManFunc,  Abc_ObjRegular(Abc_NtkObj(pNtk, possibleSubNodes1.front())),  Abc_ObjRegular(Abc_NtkObj(pNtk, iFanin)), pFanout );
        //     // Abc_ObjPatchFanin(pFanout, pObj, Abc_NtkObj(pNtk, possibleSubNodes1.front()));
    }
    // Abc_AigDeleteNode((Abc_Aig_t *)pNtk->pManFunc, pObj);
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    Abc_NtkCheck(pNtk);
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
// if the target wire is complemented --> stuck at 0(so after the signal pass to the fanout gate, it become sa1)
// target wire should be the target gate's fanout node's fanin wire
int LSV::AtpgMgr::collectSMA(Abc_Ntk_t* pNtk, unordered_map<int, bool>& SMAs,int targetWireId, CoreNs::Fault::Type faultType){
    if(faultType == Fault::SA0)
        assert(targetWireId == sa0GateId);
    else
        assert(targetWireId == sa1GateId);
    std::vector<int> &maVal = faultType == Fault::SA0 ? ma0Val : ma1Val;
    SMAs.clear();
    Abc_Obj_t* pNode;
    int i;
    Abc_NtkForEachPi(pNtk, pNode, i){
        if(maVal[pNode->Id] == 0 || maVal[pNode->Id] == 1)
            assert(SMAs.insert(pair<int, bool>(pNode->Id, maVal[pNode->Id])).second);
    }
}
int LSV::AtpgMgr::collectCandidateDestination(Abc_Ntk_t* pNtk, unordered_map<int, bool>& candidateDestinations,int targetWireId, CoreNs::Fault::Type faultType){
    if(faultType == Fault::SA0)
        assert(targetWireId = sa0GateId);
    else
        assert(targetWireId = sa1GateId);
    std::vector<int> &maVal = faultType == Fault::SA0 ? ma0Val : ma1Val;
    Abc_Obj_t* pNode;
    int i;
    Abc_NtkForEachNode(pNtk, pNode, i){
        if(maVal[pNode->Id] == 0 || maVal[pNode->Id] == 1)
            assert(candidateDestinations.insert(pair<int, bool>(pNode->Id, maVal[pNode->Id])).second);
    }
}
// if return 1 --> the two error cancel each other
int LSV::AtpgMgr::DestinationNodeCheck(Abc_Ntk_t* pNtk, unordered_map<int, bool>* SMAs,int targetWireId, CoreNs::Fault::Type faultTypeT ,int candidateDestination, CoreNs::Fault::Type faultTypeD){
    stupid(pNtk, targetWireId, faultTypeT, candidateDestination, faultTypeD, SMAs);
}

int LSV::AtpgMgr::addRectificationNetwork_v2(Abc_Ntk_t* pNtk, Abc_Obj_t* pDstNode, CoreNs::Fault::Type ndFaultType, Abc_Obj_t* pPossibleSMANode){
    Abc_Obj_t* pRec;
    if(ndFaultType == Fault::SA0){
        pRec = Abc_AigOr((Abc_Aig_t *)pNtk->pManFunc, pDstNode, pPossibleSMANode);   // 應該不用考慮fanout是不是complement因為是加在branch之前？
        assert(pRec != 0);
    }
    else{
        pRec = Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc, pDstNode, Abc_ObjNot(pPossibleSMANode));   // 應該不用考慮fanout是不是complement因為是加在branch之前？
        assert(pRec != 0);
    }
    if(!Abc_AigReplace((Abc_Aig_t *)pNtk->pManFunc, pDstNode, pRec, 1, 1, Abc_ObjRegular(pRec)->Id))
        return 0;
    Abc_AigCheck((Abc_Aig_t *)pNtk->pManFunc);
    Abc_NtkReassignIds( pNtk );
    Abc_NtkStrash(pNtk, 0, 0, 0);
    return 1;
    
}


int LSV::AtpgMgr::addRectificationNetwork(Abc_Ntk_t* pNtk, Abc_Obj_t* pDstNode, CoreNs::Fault::Type ndFaultType,unordered_map<int, bool>& SMAs){
    // vector<Abc_Obj_t*> AND_SMA({Abc_ObjNotCond(Abc_NtkObj(pNtk, SMAs.begin()->first), SMAs.begin()->second)});
    // for(auto i = std::next(SMAs.begin()), ni = SMAs.end(); i != ni; ++i){
    //     Abc_Obj_t* pAnd = Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc, Abc_ObjNotCond(Abc_NtkObj(pNtk, i->first), i->second), AND_SMA.back());
    //     assert(pAnd != 0);
    //     AND_SMA.push_back(pAnd);
    // }

    Abc_Obj_t* pRec, *pANDSMAs = getANDSMAs(pNtk, SMAs);
    if(ndFaultType == Fault::SA0){
        // pRec = Abc_AigOr((Abc_Aig_t *)pNtk->pManFunc, pDstNode, AND_SMA.back());   // 應該不用考慮fanout是不是complement因為是加在branch之前？
        pRec = Abc_AigOr((Abc_Aig_t *)pNtk->pManFunc, pDstNode, pANDSMAs);   // 應該不用考慮fanout是不是complement因為是加在branch之前？
        assert(pRec != 0);
    }
    else{
        // pRec = Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc, pDstNode, Abc_ObjNot(AND_SMA.back()));   // 應該不用考慮fanout是不是complement因為是加在branch之前？
        if(Abc_AigAndLookup((Abc_Aig_t *)pNtk->pManFunc, pDstNode, Abc_ObjNot(pANDSMAs)) != NULL)
            return 0;
        pRec = Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc, pDstNode, Abc_ObjNot(pANDSMAs));   // 應該不用考慮fanout是不是complement因為是加在branch之前？
        assert(pRec != 0);
    }
    // int i;
    // Abc_Obj_t* pFanout;
    // Abc_ObjForEachFanout(pDstNode, pFanout, i){
    //     int j;
    //     Abc_Obj_t* pFanin;
    //     Abc_ObjForEachFanin(pFanout, pFanin, j)
    //         if(Abc_ObjRegular(pFanin)->Id == Abc_ObjRegular(pRec)->Id)
    //             return 0;
    // }
    if(!Abc_AigReplace((Abc_Aig_t *)pNtk->pManFunc, pDstNode, pRec, 1, 1, Abc_ObjRegular(pRec)->Id))
        return 0;
    Abc_AigCheck((Abc_Aig_t *)pNtk->pManFunc);
    Abc_NtkReassignIds( pNtk );
    Abc_NtkStrash(pNtk, 0, 0, 0);
    return 1;
    
}
int LSV::AtpgMgr::sourceNodeIdentification(Abc_Ntk_t* pNtk, int validDestinationNodeId, vector<Abc_Obj_t*>& possibleSourceNodes1, vector<Abc_Obj_t*>& possibleSourceNodes2){

    if(sa0GateId != sa1GateId || validDestinationNodeId != sa0GateId){
        std::cout << "sa0GateId != sa1GateId !!!" << std::endl;
        return 0;
    }
    std::unordered_set<Abc_Obj_t*> vTFO;
    collectTFO(vTFO, Abc_NtkObj(pNtk, sa1GateId));
    possibleSourceNodes1.clear();
    possibleSourceNodes2.clear();
    // std::vector<int> possibleSubNodes1;  // == 1 when nt sa0, == 0 when nt sa1
    // std::vector<int> possibleSubNodes2;  // == 0 when nt sa0, == 1 when nt sa1
    Abc_Obj_t* pObj;
    int i;
    Abc_NtkForEachNode(pNtk, pObj, i){
        if(pObj == Abc_NtkObj(pNtk, sa0GateId))
            continue;
        if(ma1Val[pObj->Id] != -1 && ma0Val[pObj->Id] != -1 && !vTFO.count(pObj)){
            if(ma1Val[pObj->Id] == 0 && ma0Val[pObj->Id] == 1)
                possibleSourceNodes1.push_back(pObj);
            else if(ma1Val[pObj->Id] == 1 && ma0Val[pObj->Id] == 0)
                possibleSourceNodes2.push_back(pObj);
        }
    }
    pObj = Abc_NtkObj(pNtk, sa0GateId);
    Abc_Obj_t* pFanout;
    bool AigNodeHasComplFanoutEdge = Abc_AigNodeHasComplFanoutEdge(pObj);
    bool AigNodeHasPositiveFanoutEdge = Lsv_AigNodeHasPositiveFanoutEdge(pObj);
    bool isP1empty = possibleSourceNodes1.empty(), isP2empty = possibleSourceNodes1.empty();
    if(isP2empty && isP1empty){
        std::cout << validDestinationNodeId << " doesn't have valid source node" << std::endl;
        return 0;
    }
    return 1;
    
}

Abc_Obj_t* LSV::AtpgMgr::getANDSMAs(Abc_Ntk_t* pNtk, unordered_map<int, bool>& SMAs){
    vector<Abc_Obj_t*> AND_SMA({Abc_ObjNotCond(Abc_NtkObj(pNtk, SMAs.begin()->first), SMAs.begin()->second)});
    for(auto i = std::next(SMAs.begin()), ni = SMAs.end(); i != ni; ++i){
        Abc_Obj_t* pAnd = Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc, Abc_ObjNotCond(Abc_NtkObj(pNtk, i->first), i->second), AND_SMA.back());
        assert(pAnd != 0);
        AND_SMA.push_back(pAnd);
    }
    if(AND_SMA.empty())
        return NULL;
    return AND_SMA.back();
}




