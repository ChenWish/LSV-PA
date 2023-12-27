#ifndef LSVUTILS_H
#define LSVUTILS_H

#include "base/abc/abc.h"
#include <unordered_set>

static void collectTFO(std::unordered_set<Abc_Obj_t*>& vTFO, Abc_Obj_t* pNode);
static bool isFanoutC(Abc_Obj_t* pNode, Abc_Obj_t* pFanout);


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

#endif