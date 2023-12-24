
#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "bdd/cudd/cudd.h"
#include <string>
#include <cmath>
#include "sat/cnf/cnf.h"
#include "bdd/extrab/extraBdd.h"
#include <list>
#include "booleanChain.h"

extern "C"{
    Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
    Abc_Ntk_t * Abc_NtkFromGlobalBdds( Abc_Ntk_t * pNtk, int fReverse );
    void Abc_NtkShowBdd( Abc_Ntk_t * pNtk, int fCompl, int fReorder );
}


DdNode* getDC_bdd(Abc_Ntk_t* pNtkNew, Abc_Obj_t* pNode, DdManager*& dd){
  assert(dd==NULL);
  int target=Abc_ObjId(pNode);
  int i;
  Abc_Obj_t* newpNode=Abc_NtkObj(pNtkNew, Abc_ObjId(pNode));
  Abc_Obj_t * newpi=Abc_NtkCreatePi(pNtkNew);

  Abc_ObjRemoveFanins(newpNode);
  Abc_ObjAddFanin(newpNode, newpi);
  Abc_ObjAddFanin(newpNode,Abc_AigConst1(pNtkNew));
  Abc_Print(-2, "pinum: %d\n", Abc_ObjFaninNum(newpNode));
  dd  = (DdManager *) Abc_NtkBuildGlobalBdds(pNtkNew, ABC_INFINITY, 0, 0, 0, 1);
  
  if(dd==NULL){
    fprintf(stderr, "build bdd fail\n");
  }
  DdNode* dccheck=NULL;
  //int *sup= new int[dd->size];
  Abc_Obj_t* pCo;
  Abc_NtkForEachCo(pNtkNew, pCo, i){
    DdNode* ddnode1=Cudd_Cofactor(dd, (DdNode *)Abc_ObjGlobalBdd(pCo), (DdNode *)Abc_ObjGlobalBdd(newpi));
    Cudd_Ref(ddnode1);
    DdNode* ddnode2=Cudd_Cofactor(dd, (DdNode *)Abc_ObjGlobalBdd(pCo), Cudd_Not((DdNode *)Abc_ObjGlobalBdd(newpi)));
    Cudd_Ref(ddnode2);
    DdNode* dcnode=Cudd_bddXnor(dd, ddnode1, ddnode2);
    Cudd_Ref(dcnode);
    Cudd_RecursiveDeref(dd, ddnode1);
    Cudd_RecursiveDeref(dd, ddnode2);
    //sup=Cudd_SupportIndex(dd, dcnode);
    //Abc_Print(-2, "output %d: ",i);
    //for(int j=0;j<dd->size;j++){
    //  Abc_Print(-2, "%d ", sup[j]);
    //}
    //Abc_Print(-2, "\n");
    if(dccheck==NULL)
      dccheck=dcnode;
    else{
      dccheck=Cudd_bddAnd(dd, dcnode, dccheck);
      Cudd_Ref(dccheck);
    }
  }
  //delete[] sup;
  //Abc_Print(-2, "dccheck is 1: %d\n", dccheck==Cudd_ReadOne(dd));
  return dccheck;
}
int DCIntGen(int* cube, BooleanChain& bc, int piNum){
  int count=0;
  int dcint=0;
  bool undef[20]={false};
  for(int i=0;i<piNum;i++){
    if(cube[i]==2){
      undef[i]=true;
      count++;
    }else{
      //Abc_Print(-2, "%d %d\n", cube[i], cube[i]<<piNum-i-1);
      dcint|=cube[i]<<piNum-i-1;    
    }
  }
  for(int i=0;i<(1<<count);i++){
    int tmp=i;
    for(int j=0;j<piNum;j++){
      if(undef[j]){
        dcint|=((tmp&1)<<piNum-j-1);
        if(tmp&1)
          dcint|=(1<<piNum-j-1);
        else
          dcint&=~(1<<piNum-j-1);
        tmp>>=1;
      }
    }
    //for(int j=0;j<piNum;j++){
    //  Abc_Print(-2, "%d ", (dcint>>(piNum-j-1))&1);
    //}
    //Abc_Print(-2, "\n");
    //Abc_Print(-2, "%d\n", dcint);
    bc.addDC(dcint);
  }
  return 1<<count;
}
int Boolean_Chain_Insertion(Abc_Ntk_t*& retntk ,Abc_Ntk_t* pNtk, Abc_Obj_t* pNode,bool needntk, BooleanChain& bc){
  int target=Abc_ObjId(pNode);
  Abc_Ntk_t* pNtkNew=Abc_NtkDup(pNtk);
  DdManager* dd=NULL;
  DdNode* dccheck=getDC_bdd(pNtkNew, pNode, dd);
  if(dd==NULL){
    Abc_Print(-1, "build bdd fail\n");
  }
  DdGen *gen;
  int *cube;
  CUDD_VALUE_TYPE value;
  int count=0;
  Cudd_ForeachCube(dd, dccheck, gen,cube,value ){
    Abc_Print(-2, "cube: ");
    for(int j=0;j<dd->size;j++){
      if(cube[j]==0)
        Abc_Print(-2, "!%d ", j);
      else if(cube[j]==1)
        Abc_Print(-2, "%d ", j);
    }
    Abc_Print(-2, "\n");
    count+=DCIntGen(cube, bc,dd->size-1);
    Abc_Print(-2,"%d\n",DCIntGen(cube, bc,dd->size-1));
  }
  Cudd_RecursiveDeref(dd, dccheck);

  if(needntk){
    retntk=Abc_NtkFromGlobalBdds(pNtkNew, 0);
    if(retntk==NULL){
      Abc_Print(-1, "build ntk fail\n");
    }else{
      Abc_NtkFreeGlobalBdds(pNtkNew, 1);
      Abc_NtkDelete(pNtkNew);
    }
    return count;
  }
  else{
    Abc_NtkFreeGlobalBdds(pNtkNew, 1);
    Abc_NtkDelete(pNtkNew);
    retntk=NULL;
    return count;
  }
}