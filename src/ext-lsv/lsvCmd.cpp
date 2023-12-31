#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "booleanChain.h"
#include "atpgMgr.h"
#include "sat/glucose2/AbcGlucose2.h"
#include "sat/glucose2/SimpSolver.h"
// #include "atpgPKG/atpg.h"
#include "lsvUtils.h"
#include <iostream>
#include <set>
#include <unordered_map>
#include "lsvCone.h"

#include "sat/cnf/cnf.h"
extern "C"{
  Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
  void Abc_NtkCecFraig( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nSeconds, int fVerbose, int* isEqui);
  int Lsv_AigReplace( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, Abc_Obj_t* pFanout, int fUpdateLevel, int ispNewCompl, int forcedFanouttoPositive);
  Abc_Obj_t * Abc_AigAndCreateFrom( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * pAnd );
  void        Abc_AigAndDelete( Abc_Aig_t * pMan, Abc_Obj_t * pThis );

}

using namespace std;

static int XDC_simp(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandNtk2Chain(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainTimeLimit(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandPrintChain(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainReduce(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainBadRootClear(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainDCClear(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainDCBound(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChain2Ntk(Abc_Frame_t* pAbc, int argc, char** argv);
static int test_Command(Abc_Frame_t* pAbc, int argc, char** argv);
extern int Boolean_Chain_Insertion(Abc_Ntk_t*& retntk ,Abc_Ntk_t* pNtk, Abc_Obj_t* pNode,bool needntk, BooleanChain& bc);
static int test2_Command(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandMyTest(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandConst1Searching(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandSimpleNodeSub(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandRewiring(Abc_Frame_t* pAbc, int argc, char** argv);
// static int test2_Command(Abc_Frame_t* pAbc, int argc, char** argv);

static BooleanChain booleanChain;
static LSV::AtpgMgr atpgmgr;

extern "C"{
    Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
}

void init(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "LSV", "lsv_print_nodes", Lsv_CommandPrintNodes, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_ntk2chain", Lsv_CommandNtk2Chain, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_time_limit", Lsv_CommandChainTimeLimit, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_print_chain", Lsv_CommandPrintChain, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_reduce", Lsv_CommandChainReduce, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_bad_root_clear", Lsv_CommandChainBadRootClear, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_dc_clear", Lsv_CommandChainDCClear, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_dc_bound", Lsv_CommandChainDCBound, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain2ntk", Lsv_CommandChain2Ntk, 1);
  Cmd_CommandAdd(pAbc, "LSV", "test", test_Command, 0);
  Cmd_CommandAdd(pAbc, "LSV", "xtest", XDC_simp, 1);
  // Cmd_CommandAdd(pAbc, "LSV", "test2", test2_Command, 0);
  srand(5487);
  Cmd_CommandAdd(pAbc, "LSV", "z", Lsv_CommandConst1Searching, 1);
  Cmd_CommandAdd(pAbc, "LSV", "s", Lsv_CommandSimpleNodeSub, 1);
  Cmd_CommandAdd(pAbc, "LSV", "x", Lsv_CommandMyTest, 0);
  Cmd_CommandAdd(pAbc, "LSV", "myrw", Lsv_CommandRewiring, 1);
  // Cmd_CommandAdd(pAbc, "LSV", "test2", test2_Command, 0);
}

//  先執行 Lsv_CommandConst1Searching，再執行 Lsv_CommandSimpleNodeSub
void destroy(Abc_Frame_t* pAbc) {}

Abc_FrameInitializer_t frame_initializer = {init, destroy};

struct PackageRegistrationManager {
  PackageRegistrationManager() { Abc_FrameAddInitializer(&frame_initializer); }
} lsvPackageRegistrationManager;

void Lsv_NtkPrintNodes(Abc_Ntk_t* pNtk) {
  Abc_Obj_t* pObj;
  int i;
  Abc_NtkForEachNode(pNtk, pObj, i) {
    printf("Object Id = %d, name = %s\n", Abc_ObjId(pObj), Abc_ObjName(pObj));
    Abc_Obj_t* pFanin;
    int j;
    Abc_ObjForEachFanin(pObj, pFanin, j) {
      printf("  Fanin-%d: Id = %d, name = %s\n", j, Abc_ObjId(pFanin),
             Abc_ObjName(pFanin));
    }
    if (Abc_NtkHasSop(pNtk)) {
      printf("The SOP of this node:\n%s", (char*)pObj->pData);
    }
  }
}

int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF) {
    switch (c) {
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (!pNtk) {
    Abc_Print(-1, "Empty network.\n");
    return 1;
  }
  Lsv_NtkPrintNodes(pNtk);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_print_nodes [-h]\n");
  Abc_Print(-2, "\t        prints the nodes in the network\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}



int Lsv_CommandRewiring(Abc_Frame_t* pAbc, int argc, char** argv) {
  vector<vector<int>> rewiredNodes;
  cout << "rewiring" << endl;
  Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
  if(!Abc_NtkIsStrash(pNtk)){
    printf("this command can only be called after strashed.\n");
    return 0;
  }
  bool flag = 0;
  // atpgmgr.init(pNtk);
  // atpgmgr.runSingleSAF(pNtk, pTargetNode->Id, CoreNs::Fault::SA1, -1, 1);
  // atpgmgr.init(pNtk);
  // atpgmgr.runSingleSAF(pNtk, pTargetNodeFanout0->Id, CoreNs::Fault::SA0, pTargetNode->Id, 1);
  Abc_Obj_t* pTargetNode;
  int j = 0;
  Abc_NtkForEachNode(pNtk, pTargetNode, j){

    // Abc_Obj_t* pTargetNode = Abc_NtkObj(pNtk, 7);
    // Abc_Obj_t* pTargetNodeFanout0 = Abc_ObjFanout0(pTargetNode);
    Abc_Obj_t* pTargetNodeFanout;
    int k = 0;
    Abc_ObjForEachFanout(pTargetNode, pTargetNodeFanout, k){
      atpgmgr.init(pNtk);
      int res = 1;
      res = atpgmgr.runSingleSAF(pNtk, pTargetNodeFanout->Id, CoreNs::Fault::SA1, pTargetNode->Id, 1);
      // if(isFanoutC(pTargetNode, pTargetNodeFanout0))
      // else
        // res =  atpgmgr.runSingleSAF(pNtk, pTargetNodeFanout0->Id, CoreNs::Fault::SA1, pTargetNode->Id, 1);
      if(res == 1)
        continue;
      unordered_map<int, bool> SMAs, CandidateDestinations, ValidDestinations;
      atpgmgr.collectSMA(pNtk, SMAs, pTargetNodeFanout->Id, CoreNs::Fault::SA1);
      atpgmgr.collectCandidateDestination(pNtk, CandidateDestinations, pTargetNodeFanout->Id, CoreNs::Fault::SA1);
      for(auto i : CandidateDestinations){
        if(1 == atpgmgr.DestinationNodeCheck(pNtk, &SMAs, pTargetNodeFanout->Id, CoreNs::Fault::SA1, i.first, i.second == 1 ? Fault::SA0 : Fault::SA1)){
            ValidDestinations[i.first] = i.second;
        }
      }
      CandidateDestinations.clear();
      if(ValidDestinations.empty())
        continue;
      for(auto i : ValidDestinations){
        bool found = 0;
        if(i.first < pTargetNode->Id || i.first < pTargetNodeFanout->Id || i.first == pTargetNodeFanout->Id)
          continue;
        Abc_Ntk_t* pNtkNew = Abc_NtkDup(pNtk);
        std::unordered_set<Abc_Obj_t*> vTFO;
        collectTFO(vTFO, Abc_NtkObj(pNtk,  i.first));
        Abc_Obj_t* pPossibleSMANode;
        int j;
        Abc_NtkForEachNode(pNtkNew, pPossibleSMANode, j){
          if(pPossibleSMANode->Id >= i.first)
            break;
          if(vTFO.count(pPossibleSMANode) == 1 || pPossibleSMANode->Id == pTargetNode->Id)
            continue;

          // add rectificationNetwork
          atpgmgr.addRectificationNetwork_v2(pNtkNew, Abc_NtkObj(pNtkNew, i.first), i.second == 1 ? Fault::SA0 : Fault::SA1, pPossibleSMANode);

          // remove w_t
          Abc_Obj_t* pSrcNode = Abc_NtkObj(pNtkNew, pTargetNode->Id), *pDstNode = Abc_NtkObj(pNtkNew, pTargetNodeFanout->Id), *pSrcNodeSiblingFanin = Abc_ObjFanin1(pDstNode) == pSrcNode ? Abc_ObjFanin0(pDstNode) : Abc_ObjFanin1(pDstNode);
          Abc_AigReplace((Abc_Aig_t*)pNtkNew->pManFunc, pDstNode, pSrcNodeSiblingFanin, 1, 1, -1); // a!b!c = a(!b!c) != a!(b!c)
          Abc_NtkCheck(pNtkNew);
          Abc_NtkReassignIds(pNtkNew);
          int* isEqui = new int;
          *isEqui = 0;
          Abc_NtkCecFraig( pNtk, pNtkNew, 100, 1, isEqui);
          assert(*isEqui == 1 || *isEqui == 0);
          if(*isEqui == 1 && rand() % 2 == 1)
            *isEqui = 0;
          if(*isEqui == 0){
            Abc_NtkDelete(pNtkNew);
            pNtkNew = Abc_NtkDup(pNtk);
          }
          else{
            // Abc_NtkDelete(pNtk);
            Abc_NtkStrash(pNtkNew, 0, 0, 0);
            Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);
            vector<int> tmp({pPossibleSMANode->Id, i.first, pTargetNode->Id, pTargetNodeFanout->Id});
            rewiredNodes.push_back(tmp);
            flag = 1;
            found = 1;
            cout << "we use " << pPossibleSMANode->Id << " and " << i.first << " to replace wire from " << pTargetNode->Id << " to " << pTargetNodeFanout->Id << endl;
            return 1;
            break;
          }

          // Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);
          // return 1;
        }
        if(found)
          break;
      }
    }
    }
    if(flag){
      cout << "we did find some alternative wire" << endl;
      for(auto i : rewiredNodes){ 
          cout << "we use " << i[0] << " and " << i[1] << " to replace wire from " << i[2] << " to " << i[3] << endl;
      }
      return 1;
    }
    else{
      cout << "didn't find any alternative wire" << endl;
      return 0;
    }
      // for(auto i : ValidDestinations){
      //   Abc_Ntk_t* pNtkNew = Abc_NtkDup(pNtk);
      //   if(i.first < pTargetNode->Id || i.first < pTargetNodeFanout->Id){
      //     Abc_NtkDelete(pNtkNew);
      //     continue;
      //   }
      //   // cout << "rect circuit at " <<  i.first << " is added" << endl;

      //   // add rectification network

      //   if(!atpgmgr.addRectificationNetwork(pNtkNew, Abc_NtkObj(pNtkNew, i.first), i.second == 1 ? Fault::SA0 : Fault::SA1, SMAs)){
      //     Abc_NtkDelete(pNtkNew);
      //     continue;
      //   }

        
      //   // remove target wire

      //   // those whose objID < Nd will not change after the ID reassignment
      //   // Abc_Obj_t* pSrcNode = Abc_NtkObj(pNtk, 18), *pDstNode  = Abc_NtkObj(pNtk, 19), *pSrcNodeSiblingFanin = Abc_ObjFanin1(pDstNode) == pSrcNode ? Abc_ObjFanin0(pDstNode) : Abc_ObjFanin1(pDstNode);
      //   Abc_Obj_t* pSrcNode = Abc_NtkObj(pNtkNew, pTargetNode->Id), *pDstNode = Abc_NtkObj(pNtkNew, pTargetNodeFanout->Id), *pSrcNodeSiblingFanin = Abc_ObjFanin1(pDstNode) == pSrcNode ? Abc_ObjFanin0(pDstNode) : Abc_ObjFanin1(pDstNode);
      //   Abc_AigReplace((Abc_Aig_t*)pNtkNew->pManFunc, pDstNode, pSrcNodeSiblingFanin, 1, 1, -1); // a!b!c = a(!b!c) != a!(b!c)
      //   Abc_NtkCheck(pNtkNew);
      //   Abc_NtkReassignIds(pNtkNew);

      //   // source node identification

      //   vector<Abc_Obj_t*> possibleSourceNodes1, possibleSourceNodes2;
      //   Abc_Obj_t* pSMA = atpgmgr.getANDSMAs(pNtkNew, SMAs);
      //   if(pSMA == 0){
      //     Abc_NtkDelete(pNtkNew);
      //     continue;
      //   } 
      //   if(atpgmgr.runSingleSAF(pNtkNew, pSMA->Id, CoreNs::Fault::SA0, -1, 1)){
      //     Abc_NtkDelete(pNtkNew);
      //     continue;
      //   }
      //   if(atpgmgr.runSingleSAF(pNtkNew, pSMA->Id, CoreNs::Fault::SA1, -1, 1)){
      //     Abc_NtkDelete(pNtkNew);
      //     continue;
      //   }
      //   atpgmgr.sourceNodeIdentification(pNtkNew, i.first, possibleSourceNodes1, possibleSourceNodes2);
      //   if(possibleSourceNodes1.empty() && possibleSourceNodes2.empty()){
      //     Abc_NtkDelete(pNtkNew);
      //     continue;
      //   }
      //   cout << "possible source nodes1: " << endl;
      //   for(auto i : possibleSourceNodes1)
      //     cout << i->Id << ' ';
      //   cout << endl;
      //   cout << "possible source nodes2: " << endl;
      //   for(auto i : possibleSourceNodes2)
      //     cout << i->Id << ' ';
      //   cout << endl;
      //   // cout << "we find some possible source nodes" << endl;
      //   Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);
        
      //   return 1;
      // }
    // }
    
  // }
  // return 1;

  // // add a wire
  // Abc_Obj_t* pSrcNode = Abc_NtkObj(pNtk, 10), *pDstNode  = Abc_NtkObj(pNtk, 9), *pDstNodeFanin0 = Abc_ObjFanin1(pDstNode);
  // Abc_Obj_t* pNewNode = Abc_AigAnd((Abc_Aig_t*)pNtk->pManFunc, pSrcNode, Abc_ObjNotCond(pDstNodeFanin0, Abc_ObjFaninC1(pDstNode)));
  // Lsv_AigReplace((Abc_Aig_t*)pNtk->pManFunc, pDstNodeFanin0, pNewNode, pDstNode, 1, 0, 1); // a!b!c = a(!b!c) != a!(b!c)
  // Abc_NtkCheck(pNtk);
  // Abc_NtkReassignIds( pNtk );
  // // Abc_NtkStrash(pNtk, 0, 0, 0);


  /*
  // // delete a wire // 不要刪到po或pi出來的
  // if old node has only one fanout -> replace it with const1
  Abc_Obj_t* pSrcNode = Abc_NtkObj(pNtk, 18), *pDstNode  = Abc_NtkObj(pNtk, 19), *pSrcNodeSiblingFanin = Abc_ObjFanin1(pDstNode) == pSrcNode ? Abc_ObjFanin0(pDstNode) : Abc_ObjFanin1(pDstNode);
  // Lsv_AigReplace((Abc_Aig_t*)pNtk->pManFunc, pSrcNode, Abc_AigConst1(pNtk), pDstNode, 1, isFanoutC(pSrcNode, pDstNode), 0); // a!b!c = a(!b!c) != a!(b!c)


    // // remove the old fanout node from the structural hashing table
    //     Abc_AigAndDelete( pMan, pFanout );
    //     // remove the fanins of the old fanout
    //     Abc_ObjRemoveFanins( pFanout );
    //     // recreate the old fanout with new fanins and add it to the table
    //     Abc_AigAndCreateFrom( pMan, pFanin1, pFanin2, pFanout);


  // Lsv_AigReplace((Abc_Aig_t*)pNtk->pManFunc, pSrcNode, Abc_AigConst1(pNtk), pDstNode, 1, 0, 1); // a!b!c = a(!b!c) != a!(b!c)
  // Lsv_AigReplace((Abc_Aig_t*)pNtk->pManFunc, pSrcNode, pSrcNodeSiblingFanin, pDstNode, 1, isFanoutC(pSrcNodeSiblingFanin, pDstNode), 0); // a!b!c = a(!b!c) != a!(b!c)
  // Abc_AigReplace((Abc_Aig_t*)pNtk->pManFunc, pSrcNode, Abc_ObjNotCond(Abc_AigConst1(pNtk), isFanoutC(pSrcNode, pDstNode)), 1); // a!b!c = a(!b!c) != a!(b!c)
  // Abc_AigReplace((Abc_Aig_t*)pNtk->pManFunc, pSrcNode, pDstNode, 1); // a!b!c = a(!b!c) != a!(b!c)

  // Abc_AigReplace((Abc_Aig_t*)pNtk->pManFunc, pDstNode, pSrcNode, 1, 1, -1); // a!b!c = a(!b!c) != a!(b!c)
  Abc_AigReplace((Abc_Aig_t*)pNtk->pManFunc, pDstNode, pSrcNodeSiblingFanin, 1, 1, -1); // a!b!c = a(!b!c) != a!(b!c)
  // Abc_NtkStrash(pNtk, 0, 0, 0);
  Abc_NtkCheck(pNtk);
  Abc_NtkReassignIds( pNtk );
  return 0;
  */

  // add rectification network
  // unordered_map<int, bool> smas;
  // smas[1] = 1; 
  // smas[2] = 0; 
  // smas[3] = 0; 
  // atpgmgr.addRectificationNetwork(pNtk, Abc_NtkObj(pNtk, 18), Fault::SA0, smas);
  // return 0;

  // int i;
  // Abc_Ntk_t* pNtkNew = Abc_NtkDup(pNtk);
  // atpgmgr.init(pNtk);
  // Abc_NtkForEachNodeReverse(pNtk, pNode, i){
  //   if(atpgmgr.runSingleSAF(pNtk, pNode->Id, CoreNs::Fault::SA0, 1))
  //     continue;
  //   if(atpgmgr.runSingleSAF(pNtk, pNode->Id, CoreNs::Fault::SA1, 1))
  //     continue;
  //   if(atpgmgr.simpleNodeSub(pNtkNew)){
  //     Abc_NtkReassignIds( pNtkNew );
  //     Abc_NtkStrash(pNtkNew, 0, 0, 0);
  //     int* isEqui = new int;
  //     *isEqui = 0;
  //     Abc_NtkCecFraig( pNtk, pNtkNew, 100, 1, isEqui);
  //     assert(*isEqui == 1 || *isEqui == 0);
  //     if(*isEqui == 0){
  //       Abc_NtkDelete(pNtkNew);
  //       pNtkNew = Abc_NtkDup(pNtk);
  //     }
  //     else{
  //       // Abc_NtkDelete(pNtk);
  //       Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);
  //       return 0;
  //     }
  //     delete isEqui;
  //     // findSub = 1;
  //     // atpgmgr.init(pNtk);
  //     // break;
  //   }
  // }

}
int Lsv_CommandMyTest(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    if(!Abc_NtkIsStrash(pNtk)){
      printf("this command can only be called after strashed.\n");
      return 0;
    }
  // atpgmgr.myselftest(pNtk, 10);

  // unordered_set<Abc_Obj_t*> vTFO;
  // collectTFO(vTFO, Abc_NtkObj(pNtk, 7) );
  // for(auto p:vTFO)
  //   cout << p->Id << endl;


  atpgmgr.init(pNtk);
  // cout << Abc_ObjFanout(Abc_NtkObj(pNtk, 1), 4)->Id << ' ' << isFanoutC(Abc_NtkObj(pNtk, 1), Abc_ObjFanout(Abc_NtkObj(pNtk, 1), 4)) << endl;
  // return 0;
    
  // cout << "numPI_ = " << atpgmgr.pCir->numPI_ << endl;                // Number of PIs.
  // cout << "numPPI_ = " << atpgmgr.pCir->numPPI_ << endl;                // Number of PIs.
  // cout << "numPO_ = " << atpgmgr.pCir->numPO_ << endl;                // Number of PIs.
  // cout << "numComb_ = " << atpgmgr.pCir->numComb_ << endl;                // Number of PIs.
  // cout << "numGate_ = " << atpgmgr.pCir->numGate_ << endl;                // Number of PIs.
  // cout << "numNet_ = " << atpgmgr.pCir->numNet_ << endl;                // Number of PIs.
  // cout << "circuitLvl_ = " << atpgmgr.pCir->circuitLvl_ << endl;                // Number of PIs.
  // cout << "numFrame_ = " << atpgmgr.pCir->numFrame_ << endl;                // Number of PIs.
  // cout << "totalGate_ = " << atpgmgr.pCir->totalGate_ << endl;                // Number of PIs.
  // cout << "totalLvl_ = " << atpgmgr.pCir->totalLvl_ << endl;                // Number of PIs.
    
  int res = atpgmgr.runSingleSAF(pNtk, 8, Fault::SA0, 2, 1); // run single stuck at fault
  // Abc_Obj_t* pNode;
  // int i;
  // int count = 0;

  // Abc_NtkForEachNode(pNtk, pNode, i){
  //   cout <<pNode->Id << " stuck at 0" << endl;
  //   // int res = atpgmgr.myselftest(pNtk, pNode->Id);
  //   int res = atpgmgr.runSingleSAF(pNtk, pNode->Id, Fault::SA0); // run single stuck at fault
  //   // res = atpgmgr.runSingleSAF(pNtk, pNode->Id, Fault::SA0); // run single stuck at fault
    
  //   assert(stupid(pNtk,pNode->Id, 0) == res);
  //   // if(stupid(pNtk,pNode->Id, 0, 1) != res)
  //   //   count++;
  // }
  /*

  // cout << i << ' ' << count;
  // atpgmgr.runSingleSAF(52, Fault::SA1); // run single stuck at fault

  // Abc_Obj_t *pPo;
  //   int i;
  //   Abc_NtkForEachPo(pNtk, pPo, i){
  //       std::cout << pPo->fCompl0 << std::endl;
  //   }
  // atpgmgr.myselftest(pNtk);
  */

  return 0;
} 


int Lsv_CommandSimpleNodeSub(Abc_Frame_t* pAbc, int argc, char** argv){

  // extern void Abc_NtkCecFraig( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nSeconds, int fVerbose, int* isEqui);

  Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
  if(!Abc_NtkIsStrash(pNtk)){
    printf("this command can only be called after strashed.\n");
    return 0;
  }




  Abc_Obj_t* pNode;
  int i;
  // while(1){
  //   bool findSub = 0;
  Abc_Ntk_t* pNtkNew = Abc_NtkDup(pNtk);
    atpgmgr.init(pNtk);
    Abc_NtkForEachNodeReverse(pNtk, pNode, i){
      if(atpgmgr.runSingleSAF(pNtk, pNode->Id, CoreNs::Fault::SA0, -1, 1))
        continue;
      if(atpgmgr.runSingleSAF(pNtk, pNode->Id, CoreNs::Fault::SA1, -1, 1))
        continue;
      if(atpgmgr.simpleNodeSub(pNtkNew)){
        Abc_NtkReassignIds( pNtkNew );
        Abc_NtkStrash(pNtkNew, 0, 0, 0);
        int* isEqui = new int;
        *isEqui = 0;
        Abc_NtkCecFraig( pNtk, pNtkNew, 100, 1, isEqui);
        assert(*isEqui == 1 || *isEqui == 0);
        if(*isEqui == 0){
          Abc_NtkDelete(pNtkNew);
          pNtkNew = Abc_NtkDup(pNtk);
        }
        else{
          // Abc_NtkDelete(pNtk);
          Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);
          return 0;
        }
        delete isEqui;
        // findSub = 1;
        // atpgmgr.init(pNtk);
        // break;
      }
    }
    // if(!findSub)
      // break;
    // }

    // if(atpgmgr.simpleNodeSub(pNtk) != -1){
    //   Abc_AigReplace((Abc_Aig_t *)pNtk->pManFunc, Abc_NtkObj(pNtk, atpgmgr.NodeTarget()), Abc_NtkObj(pNtk, atpgmgr.simpleNodeSub(pNtk)), 0 );
      // Abc_Obj_t* fanin;
      // int j;
      // Abc_ObjForEachFanout(Abc_NtkObj(pNtk, 26), fanin, j)
      //   cout << fanin->Id << ' ';
      // cout << endl;
      // break;
      // atpgmgr.init(pNtk);
  
  return 0;


}
int Lsv_CommandConst1Searching(Abc_Frame_t* pAbc, int argc, char** argv){

  Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
  if(!Abc_NtkIsStrash(pNtk)){
    printf("this command can only be called after strashed.\n");
    return 0;
    }
  Abc_Obj_t* pNode;
  // Abc_NtkForEachNode(pNtk, pNode, i){
  for(int i = 1, n = Abc_NtkObjNum(pNtk); i < n; ++i){
    pNode = Abc_NtkObj(pNtk, i);
    if(Abc_ObjType(pNode) != ABC_OBJ_NODE)
      continue;
    std::cout <<"n = " << n << endl;
    int res = 0;
    Abc_Ntk_t* pNtkNew = Abc_NtkDup(pNtk);
    pNode = Abc_NtkObj(pNtkNew, i);
    atpgmgr.init(pNtkNew);
    res = atpgmgr.runSingleSAF(pNtkNew, pNode->Id, Fault::SA1); // run single stuck at fault
    assert(res!= -1);
    if( res == 1) // if return 1 --> undectable
    {
      cout << pNode->Id << "is replaced by const 1\n" ;
      Abc_AigReplace((Abc_Aig_t *)pNtkNew->pManFunc, pNode, Abc_AigConst1(pNtkNew), 0 , 0, -1);
      Abc_AigCleanup((Abc_Aig_t *)pNtkNew->pManFunc);
      Abc_NtkReassignIds( pNtkNew );
      Abc_NtkLevelize(pNtkNew);
      Abc_AigCheck( (Abc_Aig_t *)pNtkNew->pManFunc );
      Abc_NtkStrash(pNtkNew, 0, 0, 0);
      int* isEqui = new int;
      *isEqui = 0;
      Abc_NtkCecFraig( pNtk, pNtkNew, 100, 1, isEqui);
      assert(*isEqui == 1 || *isEqui == 0);
      if(*isEqui == 0){
        Abc_NtkDelete(pNtkNew);
        pNtkNew = Abc_NtkDup(pNtk);
      }
      else{
        // Abc_NtkDelete(pNtk);
        Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);
        return 1;
      }
      delete isEqui;
      // return 1;
    }
    res = -1;
    res = atpgmgr.runSingleSAF(pNtkNew, pNode->Id, Fault::SA0); // run single stuck at fault
    assert(res != -1);
    cout << "res = " << res << endl;
    if( res == 1) // if return 1 --> undectable
    {
      Abc_AigReplace((Abc_Aig_t *)pNtkNew->pManFunc, pNode, Abc_ObjNot(Abc_AigConst1(pNtkNew)), 0 , 0, -1);
      Abc_AigCleanup((Abc_Aig_t *)pNtkNew->pManFunc);
      Abc_NtkReassignIds( pNtkNew );
      Abc_NtkLevelize(pNtkNew);
      Abc_AigCheck( (Abc_Aig_t *)pNtkNew->pManFunc );
      int* isEqui = new int;
      *isEqui = 0;
      Abc_NtkCecFraig( pNtk, pNtkNew, 100, 1, isEqui);
      assert(*isEqui == 1 || *isEqui == 0);
      if(*isEqui == 0){
        Abc_NtkDelete(pNtkNew);
        pNtkNew = Abc_NtkDup(pNtk);
      }
      else{
        // Abc_NtkDelete(pNtk);
        Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);
        return 1;
      }
      delete isEqui;
      return 1;

    }
  }


  return 0;

}

void Lsv_Ntk2Chain(Abc_Ntk_t* pNtk) {
  if (!Abc_NtkIsStrash(pNtk)) {
    printf("Error: This command is only applicable to strashed networks.\n");
    return;
  }

  booleanChain.Ntk2Chain(pNtk);
  booleanChain.genTT();
}

int Lsv_CommandNtk2Chain(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF) {
    switch (c) {
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (!pNtk) {
    Abc_Print(-1, "Empty network.\n");
    return 1;
  }
  if (argc >= 2) {
    booleanChain.setVerbose(atoi(argv[1]));
  }
  Lsv_Ntk2Chain(pNtk);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_ntk2chain [-h]\n");
  Abc_Print(-2, "\t        convert the ntk to boolean chain\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}

int Lsv_CommandChainTimeLimit(Abc_Frame_t* pAbc, int argc, char** argv) {
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF) {
    switch (c) {
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (argc >= 2) {
    booleanChain.setNTimeLimit(atoi(argv[1]));
  }
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_chain_time_limit [-h]\n");
  Abc_Print(-2, "<timeLimit(sec)>\t        set the time limit of SAT solving\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}


void Lsv_PrintChain(int fTT) {
  booleanChain.print(fTT);
}

int Lsv_CommandPrintChain(Abc_Frame_t* pAbc, int argc, char** argv) {
  int c;
  int fTT = 1;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "ht")) != EOF) {
    switch (c) {
      case 't':
        fTT ^= 1;
        break;
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  Lsv_PrintChain(fTT);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_printChain [-h]\n");
  Abc_Print(-2, "\t        prints the boolean chain\n");
  Abc_Print(-2, "\t-t    : toggle to print the truth table (default = 1)\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}


void Lsv_ChainReduce(Abc_Ntk_t* pNtk, bool fAll, bool fDC, bool fSmart, int reduceSize) {
  set<int> subCircuit;
  set<int> constraintCut;
  // fAll, fDC, fSmart can at most 
  int check = fAll + fDC + fSmart;
  if (check > 1) {
    cerr << "Error: fAll, fDC, fSmart can only select one" << endl;
    return;
  }
  if (!fAll && !fDC && !fSmart) {
    cout << "Enter subCircuit index: ";
    int subIndex;
    while (cin >> subIndex) {
      if (subIndex == -1)
        break;
      subCircuit.insert(subIndex);
    }
    cout << "Enter constraintCut index: ";
    int consttraintCutIndex;
    while (cin >> consttraintCutIndex) {
      if (consttraintCutIndex == -1)
        break;
      constraintCut.insert(consttraintCutIndex);
    }
  }
  if (fAll) {
    Abc_Obj_t* pObj;
    int i;
    Abc_NtkForEachObj(pNtk, pObj, i) {
      if (Abc_ObjIsNode(pObj)) {
        // printf("subCircuit: %d\n", i); 
        subCircuit.insert(i);
      }
      if (Abc_ObjIsPo(pObj)) {
        // printf("constraintCut: %d\n", i); 
        constraintCut.insert(i);
      }
    }
  }
  if (fSmart) {
    size_t prevTimeLimit = booleanChain.getNTimeLimit();
    int maxIteration = 3;
    for (int i = 0; i < maxIteration; ++i) {
      int result = booleanChain.reduce(6, reduceSize);
      if (result > 0)
        return;
      if (result == -1) {
        booleanChain.setNTimeLimit(booleanChain.getNTimeLimit() * 1.2);
      }
    }
    for (int i = 0; i < maxIteration; ++i) {
      if (booleanChain.reduce(5, reduceSize) > 0)
        break;
    }
    booleanChain.setNTimeLimit(prevTimeLimit);

    // if (booleanChain.reduce(5, 1) < 0)
    //   booleanChain.reduce(5, 0);
    // while (booleanChain.reduce(6, -1) < 0) {
    //   cout << "Reduce ......." << endl;
    // }
    return;
  }
  if (fDC) {
    int length = Abc_NtkObjNum(pNtk);
    bool* cone = new bool[length];
    bool* input = new bool[length];
    for (int i = 0; i < length; ++i) {
      cone[i] = false;
      input[i] = false;
    }
    int root = getCone(pNtk, cone, input, booleanChain.coneUpperBound, booleanChain.coneLowerBound, booleanChain.badConeRoot);
    if (root == -1) {
      Abc_Print(-2, "Cannot find required cone\n");
      return;
    }
    booleanChain.badConeRoot.insert(root);
    cout << "Cone Root = " << root << ",";
    for (int i = 0; i < length; ++i) {
      if (cone[i]) {
        subCircuit.insert(i);
        cout << " " << i;
      }
    }
    cout << endl;
    delete[] cone;
    delete[] input;
    
    Abc_Obj_t* pObjFanout;
    int i;
    Abc_ObjForEachFanout(Abc_NtkObj(pNtk, root), pObjFanout, i) {
      constraintCut.insert(pObjFanout->Id);
    }
    // TODO: get DC and add DC
    Abc_Ntk_t* pNtkTemp;
    int mintermCount = Boolean_Chain_Insertion(pNtkTemp, pNtk, Abc_NtkObj(pNtk, root), false, booleanChain);
    cout << "mintermCount: " << mintermCount << endl;
    if (mintermCount == 0) {
      cerr << "minterm = 0" << endl;
      return;
    }
  }
  booleanChain.reduce(subCircuit, constraintCut, reduceSize);
  // printf("exec time: %d\n", Abc_Clock() - t);
}

int Lsv_CommandChainReduce(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int fAll = 0;
  int fDC = 0;
  int fSmart = 0;
  int reduceSize = -1;
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "adsh")) != EOF) {
    switch (c) {
      case 'a':
        fAll ^= 1;
        break;
      case 'd':
        fDC ^= 1;
        break;
      case 's':
        fSmart ^= 1;
        break;
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (argc == 3) {
    reduceSize = atoi(argv[2]);
    cerr << "reduceSize: " << reduceSize << endl;
  }
  Lsv_ChainReduce(pNtk, fAll, fDC, fSmart, reduceSize);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_chain_reduce [-h]\n");
  Abc_Print(-2, "\t        reduce the boolean chain\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}

int Lsv_CommandChainBadRootClear(Abc_Frame_t* pAbc, int argc, char** argv) {
  booleanChain.badConeRoot.clear();
  return 0;
}

int Lsv_CommandChainDCClear(Abc_Frame_t* pAbc, int argc, char** argv) {
  booleanChain.clearDC();
  return 0;
}

void Lsv_Chain2Ntk(Abc_Ntk_t* pNtk) {
  Abc_Ntk_t* pNtkNew = booleanChain.Chain2Ntk(pNtk);
  Abc_FrameReplaceCurrentNetwork(Abc_FrameReadGlobalFrame(), pNtkNew);
  // Abc_FrameClearVerifStatus( Abc_FrameReadGlobalFrame() );
}
int Lsv_CommandChainDCBound(Abc_Frame_t* pAbc, int argc, char** argv) {
  if (argc != 3) {
    Abc_Print(-1, "Usage: lsv_chain_dc_bound <upperBound> <lowerBound>\n");
    return 1;
  }
  booleanChain.coneUpperBound = atoi(argv[1]);
  booleanChain.coneLowerBound = atoi(argv[2]);
}


int Lsv_CommandChain2Ntk(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF) {
    switch (c) {
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  Lsv_Chain2Ntk(pNtk);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_chain2ntk [-h]\n");
  Abc_Print(-2, "\t        convert the boolean chain to ntk\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}


static int test_Command(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int length = Abc_NtkObjNum(pNtk);
  bool* cone = new bool[length];
  bool* input = new bool[length];
  for (int i = 0; i < length; ++i) {
    cone[i] = false;
    input[i] = false;
  }
  set<int> badCone;
  getCone(pNtk, cone, input, 7, 3, badCone);
  for (int i = 0; i < length; ++i){
    if (cone[i])
      Abc_Print(-2, "node %d\n", i);
  }
  return 0;
}
int XDC_simp(Abc_Frame_t* pAbc, int argc, char** argv){
  if(argc!=2){
    Abc_Print(-1, "Usage: xtest <target>\n");
    return 1;
  }
  int target=atoi(argv[1]);
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  Abc_Print(-2, "ntk type: %d\n",pNtk->ntkType);
  if(Abc_NtkIsStrash(pNtk)==NULL){
    Abc_Print(-1, "The network is not Aig logic.\n");
  }
  int i;
  Abc_Obj_t* pNode;
  BooleanChain bc;

  Abc_Ntk_t* newntk;
  int cubecnt=Boolean_Chain_Insertion(newntk, pNtk, Abc_NtkObj(pNtk, target),false, bc);
  Abc_Print(-2, "cube count: %d\n", cubecnt);
  return 0;
}

// // test2 id sa? 1
// static int test2_Command(Abc_Frame_t* pAbc, int argc, char** argv) {
//   Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
//   Abc_Ntk_t* pNtkSA = Abc_NtkDup(pNtk);
//   if (argc != 4) {
//     Abc_Print(-2, "usage: test2 <node> <sa(1/0)> <undetectable(1/0)>\n");
//     Abc_NtkDelete(pNtkSA);
//     return 0;
//   }
//   int id = atoi(argv[1]);
//   int saValue = atoi(argv[2]);
//   int undetectable = atoi(argv[3]);
//   if (id <= Abc_NtkPiNum(pNtk) + Abc_NtkPoNum(pNtk) || id > Abc_NtkObjNum(pNtk)) {
//     Abc_Print(-2, "Error: id is out of bound\n");
//     Abc_NtkDelete(pNtkSA);
//     return 0;
//   }
//   Abc_NtkDelete(pNtkSA);
//   return stupid(pNtk, id, saValue, undetectable);
//   Abc_Obj_t* pTarget = Abc_NtkObj(pNtkSA, id);
//   Abc_Obj_t* pFanout;
//   // cerr << pTarget->Type << endl;
//   int i;
//   Abc_ObjForEachFanout(pTarget, pFanout, i) {
//     Abc_ObjDeleteFanin(pFanout, pTarget);
//     Abc_ObjAddFanin(pFanout, Abc_ObjNotCond(Abc_AigConst1(pNtkSA), saValue == 0));
//   }
//   // cerr << pTarget->Type << endl;

//   Abc_NtkDeleteObj(pTarget);
//   Abc_Ntk_t* pNtkTemp = pNtkSA;
//   pNtkSA = Abc_NtkStrash(pNtkTemp, 1, 1, 0); // fAllNodes, fCleanup, fRecord
//   Abc_NtkDelete(pNtkTemp);
//   int temp = Abc_AigCleanup( (Abc_Aig_t *)pNtkSA->pManFunc ); // ???
// // cerr << temp << endl;
//   // Abc_FrameReplaceCurrentNetwork(Abc_FrameReadGlobalFrame(), pNtkSA);
//   // return 0;
//   Aig_Man_t* pMan = Abc_NtkToDar(pNtk, 0, 0); // 0 -> fExors, 0 -> fRegisters
//   Cnf_Dat_t* pCnf = Cnf_Derive(pMan, Aig_ManCoNum(pMan));
// // cerr << "pNtk to Cnf done" << endl; 

//   Aig_Man_t* pManSA = Abc_NtkToDar(pNtkSA, 0, 0); // 0 -> fExors, 0 -> fRegisters
//   Cnf_Dat_t* pCnfSA = Cnf_Derive(pManSA, Aig_ManCoNum(pManSA));
// // cerr << "pNtkSA to Cnf done" << endl; 

//   int liftNum = Aig_ManObjNum(pMan);
//   Cnf_DataLift(pCnfSA, liftNum);

//   sat_solver* pSat = sat_solver_new();
//   pSat = (sat_solver *)Cnf_DataWriteIntoSolverInt( pSat, pCnf, 1, 0 ); // 1 -> nFrames, 0 -> fInit 
//   pSat = (sat_solver *)Cnf_DataWriteIntoSolverInt( pSat, pCnfSA, 1, 0 ); // 1 -> nFrames, 0 -> fInit 

//   assert(Abc_NtkPiNum(pNtk) == Abc_NtkPiNum(pNtkSA));
//   assert(Abc_NtkPoNum(pNtk) == Abc_NtkPoNum(pNtkSA));
//   Abc_Obj_t* pObj;
//   Abc_NtkForEachPi(pNtk, pObj, i) {
//     lit clause[2];
//     // (a + ~b)(~a + b)
//     clause[0] = toLitCond(pCnf->pVarNums[pObj->Id]                  , 0); // Pi in pNtk
//     clause[1] = toLitCond(pCnfSA->pVarNums[Abc_NtkPi(pNtkSA, i)->Id], 1); // Pi in pNtkSA
//     sat_solver_addclause(pSat, clause, clause + 2);
//     clause[0] = toLitCond(pCnf->pVarNums[pObj->Id]                  , 1); // Pi in pNtk
//     clause[1] = toLitCond(pCnfSA->pVarNums[Abc_NtkPi(pNtkSA, i)->Id], 0); // Pi in pNtkSA
//     sat_solver_addclause(pSat, clause, clause + 2);
//   }

//   Abc_NtkForEachPo(pNtk, pObj, i) {
//     lit clause[2];
//     clause[0] = toLitCond(pCnf->pVarNums[pObj->Id]                  , !undetectable); // Po in pNtk
//     clause[1] = toLitCond(pCnfSA->pVarNums[Abc_NtkPo(pNtkSA, i)->Id], 1);     // Po in pNtkSA
//     sat_solver_addclause(pSat, clause, clause + 2);
//     clause[0] = toLitCond(pCnf->pVarNums[pObj->Id]                  , undetectable);  // Po in pNtk
//     clause[1] = toLitCond(pCnfSA->pVarNums[Abc_NtkPo(pNtkSA, i)->Id], 0);     // Po in pNtkSA
//     sat_solver_addclause(pSat, clause, clause + 2);
//   }
//   lit* clause = new lit[Abc_NtkPiNum(pNtk)];
//   int index = 0;
//   if (undetectable) {
//     int value;
//     cout << "Enter PI assignment(index start from 1): ";
//     while (cin >> value) {
//       if (value == 0)
//         break;
//       if (index == Abc_NtkPiNum(pNtk)) {
//         cerr << "Error: index out of bound" << endl;
//         break;
//       }
//       clause[index++] = toLitCond(pCnf->pVarNums[abs(value)], value < 0);
//     }
//   }
//   bool result = (1 == sat_solver_solve(pSat, clause, clause + index, 0, 0, 0, 0)); // l_True = 1 in MiniSat
//   if (!result) {
//     cout << "The result is UNSAT" << endl;
//   }
//   else {
//     cout << "The result is SAT, you fuck up" << endl;
//     cout << "--------- CEX ---------" << endl;
//     Abc_NtkForEachPi(pNtk, pObj, i) {
//       int var = pCnf->pVarNums[pObj->Id];
//       cout << sat_solver_var_value(pSat, var);
//     }
//     cout << endl;
//     cout << "--------- CEX END ---------" << endl;
//   }
//   delete[] clause;
//   return 0;
// }