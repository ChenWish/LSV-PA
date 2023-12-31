#ifndef LSVUTILS_H
#define LSVUTILS_H

#include "base/abc/abc.h"
#include "sat/bsat/satSolver.h"
#include <unordered_set>
#include <vector>
// #include "base/abc/abcAig.h"

// extern "C"{
//   // structural hash table procedures
//   Abc_Obj_t * Abc_AigAndCreate( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
//   Abc_Obj_t * Abc_AigAndCreateFrom( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1, Abc_Obj_t * pAnd );
//   void        Abc_AigAndDelete( Abc_Aig_t * pMan, Abc_Obj_t * pThis );
//   void        Abc_AigResize( Abc_Aig_t * pMan );
//   // incremental AIG procedures
//   void        Abc_AigReplace_int( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, int fUpdateLevel );
//   void        Abc_AigUpdateLevel_int( Abc_Aig_t * pMan );
//   void        Abc_AigUpdateLevelR_int( Abc_Aig_t * pMan );
//   void        Abc_AigRemoveFromLevelStructure( Vec_Vec_t * vStruct, Abc_Obj_t * pNode );
//   void        Abc_AigRemoveFromLevelStructureR( Vec_Vec_t * vStruct, Abc_Obj_t * pNode );
// }

static void collectTFO(std::unordered_set<Abc_Obj_t*>& vTFO, Abc_Obj_t* pNode);
static bool isFanoutC(Abc_Obj_t* pNode, Abc_Obj_t* pFanout);
static int Lsv_AigNodeHasPositiveFanoutEdge( Abc_Obj_t * pNode );
static int stupid(Abc_Ntk_t* pNtk, int id, int saValue, int id2 = -1, int saValue2 = -1, unordered_map<int, bool>* PiAssertion = 0);
static bool compareNode(const Abc_Obj_t* pNode1, const Abc_Obj_t* pNode2);
// static int Lsv_AigReplace( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, Abc_Obj_t* pFanout, int fUpdateLevel );
// static void Lsv_AigReplace_int( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, Abc_Obj_t* pFanout, int fUpdateLevel );


void collectTFO(std::unordered_set<Abc_Obj_t*>& vTFO, Abc_Obj_t* pNode){
  Abc_Obj_t* pFanout;
  int i;
  Abc_ObjForEachFanout(pNode, pFanout, i){
    collectTFO(vTFO, pFanout);
    vTFO.insert(pFanout);
  }
  return;
}

// check if the fanout edge is complmented
bool isFanoutC(Abc_Obj_t* pNode, Abc_Obj_t* pFanout){
  int iFanin = Vec_IntFind( &pFanout->vFanins, pNode->Id );
  assert( iFanin >= 0 );
  if ( Abc_ObjFaninC( pFanout, iFanin ) )
      return 1;
  return 0;
}

int Lsv_AigNodeHasPositiveFanoutEdge( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanout;
    int i, iFanin;
    Abc_ObjForEachFanout( pNode, pFanout, i )
    {
        iFanin = Vec_IntFind( &pFanout->vFanins, pNode->Id );
        assert( iFanin >= 0 );
        if ( !Abc_ObjFaninC( pFanout, iFanin ) )
            return 1;
    }
    return 0;
}
// return 0 if PATTERN FOUND, 1 if UNDETECTABLE
int stupid(Abc_Ntk_t* pNtk, int id, int saValue, int id2, int saValue2, unordered_map<int, bool>* PiAssertion) {
  // cout << "id = " << id << ", saValue = " << saValue << ", undetectable = " << undetectable << endl;
  unordered_map<int, int> id2Var;
  unordered_map<int, int> id2VarSA;
  sat_solver* pSat = sat_solver_new();
  Abc_Obj_t* pObj;
  int i;
  Abc_NtkForEachPi(pNtk, pObj, i) {
    id2Var[pObj->Id] = sat_solver_addvar(pSat);
    id2VarSA[pObj->Id] = id2Var[pObj->Id];
  }
  Abc_NtkForEachNode(pNtk, pObj, i) {
    // cerr << "NodeID = " << pObj->Id << endl;
    id2Var[pObj->Id] = sat_solver_addvar(pSat);
    id2VarSA[pObj->Id] = sat_solver_addvar(pSat);
    int fanin0 = Abc_ObjFanin0(pObj)->Id;
    int fanin1 = Abc_ObjFanin1(pObj)->Id;
    int c0 = Abc_ObjFaninC0(pObj);
    int c1 = Abc_ObjFaninC1(pObj);
    // cerr << "Fanin0 = " << Abc_ObjFanin0(pObj)->Id << ", C0 = " << c0 << endl;
    // cerr << "Fanin1 = " << Abc_ObjFanin1(pObj)->Id << ", C1 = " << c1 << endl;
    // cerr << endl;
    sat_solver_add_and(pSat, id2Var[pObj->Id], id2Var[fanin0], id2Var[fanin1], c0, c1, 0);
    if (pObj->Id != id && pObj->Id != id2)
      sat_solver_add_and(pSat, id2VarSA[pObj->Id], id2VarSA[fanin0], id2VarSA[fanin1], c0, c1, 0);
  }
  int* miterVar = new int[Abc_NtkPoNum(pNtk)];
  lit* clause = new lit[Abc_NtkPoNum(pNtk)];
  Abc_NtkForEachPo(pNtk, pObj, i) {
    int var = id2Var[Abc_ObjFanin0(pObj)->Id];
    int varSA = id2VarSA[Abc_ObjFanin0(pObj)->Id];
    miterVar[i] = sat_solver_addvar(pSat);
    sat_solver_add_xor(pSat, miterVar[i], var, varSA, 0); // my content
    clause[i] = toLitCond(miterVar[i], 0);
    // assert(id2Var.find(Abc_ObjFanin0(pObj)->Id) != id2Var.end());
    // assert(id2VarSA.find(Abc_ObjFanin0(pObj)->Id) != id2VarSA.end());
    // cerr << "Fanin0 = " << Abc_ObjFanin0(pObj)->Id << endl;
  }
  sat_solver_addclause(pSat, clause, clause + Abc_NtkPoNum(pNtk));
  delete[] miterVar;
  delete[] clause;
  clause = new lit[Abc_NtkPiNum(pNtk) + 2];
  int index = 0;
  clause[index++] = toLitCond(id2VarSA[id], saValue == 0);
  if(id2 != -1){
    clause[index++] = toLitCond(id2VarSA[id2], saValue2 == 0);
    for(auto i : *PiAssertion)
        clause[index++] = toLitCond(id2VarSA[i.first], i.second);
  }


  int result = (1 == sat_solver_solve(pSat, clause, clause + index, 0, 0, 0, 0)); // l_True = 1 in MiniSat
  cerr << "Result = " << result << endl;
  if (!result) {
    cout << "The result is UNSAT" << endl;
  }
  else {
    cout << "The result is SAT, you fuck up" << endl;
    cout << "--------- CEX ---------" << endl;
    Abc_NtkForEachPi(pNtk, pObj, i) {
      int var = id2Var[pObj->Id];
      cout << sat_solver_var_value(pSat, var);
    }
    cout << endl;
    cout << "--------- CEX END ---------" << endl;
  }
  delete[] clause;
  sat_solver_delete(pSat);
  return (result != 1);

  // return 0;
}
static bool compareNode(Abc_Obj_t* pNode1, Abc_Obj_t* pNode2){
  // First, compare based on FANINNUM
    if (Abc_ObjFanoutNum(pNode1) < Abc_ObjFanoutNum(pNode2)) 
        return true;
    else if (Abc_ObjFanoutNum(pNode1) > Abc_ObjFanoutNum(pNode2)) 
        return false;
    

    // If FANINNUM is equal, compare based on LEVEL
    return pNode1->Id < pNode2->Id;
}

#endif