
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
#include <map>

extern "C"{
    Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
    Abc_Ntk_t * Abc_NtkFromGlobalBdds( Abc_Ntk_t * pNtk, int fReverse );
    void Abc_NtkShowBdd( Abc_Ntk_t * pNtk, int fCompl, int fReorder );
}

static DdNode * my_NodeGlobalBdds_rec( DdManager * dd, Abc_Obj_t * pNode,int * pCounter, bool fDropInternal)
{
    int nBddSizeMax = ABC_INFINITY;
    DdNode * bFunc, * bFunc0, * bFunc1, * bFuncC;
    int fDetectMuxes = 0;
    assert( !Abc_ObjIsComplement(pNode) );
    if ( Cudd_ReadKeys(dd)-Cudd_ReadDead(dd) > (unsigned)nBddSizeMax )
    {
        printf( "The number of live nodes reached %d.\n", nBddSizeMax );
        fflush( stdout );
        return NULL;
    }
    // if the result is available return
    if ( Abc_ObjGlobalBdd(pNode) == NULL )
    {
      Abc_Obj_t * pNodeC, * pNode0, * pNode1;
      pNode0 = Abc_ObjFanin0(pNode);
      pNode1 = Abc_ObjFanin1(pNode);
      // check for the special case when it is MUX/EXOR
      
          // compute the result for both branches
      bFunc0 = my_NodeGlobalBdds_rec( dd, Abc_ObjFanin(pNode,0),pCounter ,fDropInternal); 
      if ( bFunc0 == NULL )
          return NULL;
      Cudd_Ref( bFunc0 );
      bFunc1 = my_NodeGlobalBdds_rec( dd, Abc_ObjFanin(pNode,1),pCounter ,fDropInternal);
      if ( bFunc1 == NULL )
          return NULL;
      Cudd_Ref( bFunc1 );
      bFunc0 = Cudd_NotCond( bFunc0, (int)Abc_ObjFaninC0(pNode) );
      bFunc1 = Cudd_NotCond( bFunc1, (int)Abc_ObjFaninC1(pNode) );
      // get the final result
      bFunc = Cudd_bddAndLimit( dd, bFunc0, bFunc1, nBddSizeMax );
      if ( bFunc == NULL )
          return NULL;
      Cudd_Ref( bFunc );
      Cudd_RecursiveDeref( dd, bFunc0 );
      Cudd_RecursiveDeref( dd, bFunc1 );
      // add the number of used nodes
      (*pCounter)++;
      // set the result
      assert( Abc_ObjGlobalBdd(pNode) == NULL );
      Abc_ObjSetGlobalBdd( pNode, bFunc );
      // increment the progress bar
    }
    // prepare the return value
    bFunc = (DdNode *)Abc_ObjGlobalBdd(pNode);
    // dereference BDD at the node
    if ( --pNode->vFanouts.nSize == 0 && fDropInternal )
    {
        Cudd_Deref( bFunc );
        Abc_ObjSetGlobalBdd( pNode, NULL );
    }
    return bFunc;
}
void * my_NtkBuildGlobalBdds( Abc_Ntk_t *& pNtk, int fDropInternal, int fReorder){
    Abc_Obj_t * pObj, * pFanin;
    Vec_Att_t * pAttMan;
    DdManager * dd;
    DdNode * bFunc;
    int i, k, Counter;

    // remove dangling nodes

    // start the manager
    if( Abc_NtkGlobalBdd(pNtk) == NULL ){
      //Abc_Print(-2, "build bdd\n");
      dd = Cudd_Init( Abc_NtkCiNum(pNtk), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
      pAttMan = Vec_AttAlloc( Abc_NtkObjNumMax(pNtk) + 1, dd, (void (*)(void*))Extra_StopManager, NULL, (void (*)(void*,void*))Cudd_RecursiveDeref );
      Vec_PtrWriteEntry( pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD, pAttMan );
    }
    else{
      dd = (DdManager *)Abc_NtkGlobalBddMan(pNtk);
    }
    // set reordering
    if ( fReorder )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    // assign the constant node BDD
    pObj = Abc_AigConst1(pNtk);
    if ( Abc_ObjFanoutNum(pObj) > 0 )
    {
        bFunc = dd->one;
        Abc_ObjSetGlobalBdd( pObj, bFunc );   Cudd_Ref( bFunc );
    }
    //// set the elementary variables
    //Abc_NtkForEachNode( pNtk, pObj, i ){
    //  Abc_Print(-2, "node: %d null: %d\n",Abc_ObjId(pObj),Abc_ObjGlobalBdd(pObj)==NULL);
    //}
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
        {
            bFunc = dd->vars[i];
            Abc_ObjSetGlobalBdd( pObj, bFunc );  Cudd_Ref( bFunc );
        }

    // collect the global functions of the COs
    Counter = 0;
    // construct the BDDs
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        //Abc_Print(-2,"callingrec\n");
        bFunc = my_NodeGlobalBdds_rec( dd, Abc_ObjFanin0(pObj), &Counter,fDropInternal );
        if ( bFunc == NULL )
        {
            Abc_NtkFreeGlobalBdds( pNtk, 0 );
            Cudd_Quit( dd ); 

            // reset references
            Abc_NtkForEachObj( pNtk, pObj, i )
                if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
                    pObj->vFanouts.nSize = 0;
            Abc_NtkForEachObj( pNtk, pObj, i )
                if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
                    Abc_ObjForEachFanin( pObj, pFanin, k )
                        pFanin->vFanouts.nSize++;
            return NULL;
        }
        bFunc = Cudd_NotCond( bFunc, (int)Abc_ObjFaninC0(pObj) );  Cudd_Ref( bFunc ); 
        Abc_ObjSetGlobalBdd( pObj, bFunc );
    }
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
            pObj->vFanouts.nSize = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                pFanin->vFanouts.nSize++;

    // reorder one more time
    if ( fReorder )
    {
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
//        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
        Cudd_AutodynDisable( dd );
    }
//    Cudd_PrintInfo( dd, stdout );
    return dd;
}

DdNode* getDC_bdd(Abc_Ntk_t*& pNtk, Abc_Obj_t* pNode, DdManager*& dd,bool fReorder){
  assert(dd==NULL);
  int target=Abc_ObjId(pNode);
  int i;

  int fid0=Abc_ObjFaninId0(pNode);
  int fic0=Abc_ObjFaninC0(pNode);
  int fid1=Abc_ObjFaninId1(pNode);
  int fic1=Abc_ObjFaninC1(pNode);

  Abc_Obj_t * newpi=Abc_NtkCreatePi(pNtk);
  Abc_ObjRemoveFanins(pNode);
  Abc_ObjAddFanin(pNode, newpi);
  Abc_ObjAddFanin(pNode,Abc_AigConst1(pNtk));
  dd  = (DdManager *) my_NtkBuildGlobalBdds(pNtk, false, fReorder);
  
  if(dd==NULL){
    fprintf(stderr, "build bdd fail\n");
  }
  DdNode* dccheck=NULL;
  //int *sup= new int[dd->size];
  Abc_Obj_t* pCo;
  Abc_NtkForEachCo(pNtk, pCo, i){
    DdNode* ddnode1=Cudd_Cofactor(dd, (DdNode *)Abc_ObjGlobalBdd(pCo), (DdNode *)Abc_ObjGlobalBdd(newpi));
    Cudd_Ref(ddnode1);
    DdNode* ddnode2=Cudd_Cofactor(dd, (DdNode *)Abc_ObjGlobalBdd(pCo), Cudd_Not((DdNode *)Abc_ObjGlobalBdd(newpi)));
    Cudd_Ref(ddnode2);
    DdNode* dcnode=Cudd_bddXnor(dd, ddnode1, ddnode2);
    Cudd_Ref(dcnode);
    Cudd_RecursiveDeref(dd, ddnode1);
    Cudd_RecursiveDeref(dd, ddnode2);
    if(dccheck==NULL)
      dccheck=dcnode;
    else{
      DdNode* temp=Cudd_bddAnd(dd, dcnode, dccheck);
      Cudd_Ref(temp);
      Cudd_RecursiveDeref(dd, dccheck);
      Cudd_RecursiveDeref(dd, dcnode);
      dccheck=temp;
    }
  }
  //delete[] sup;
  //Abc_Print(-2, "dccheck is 1: %d\n", dccheck==Cudd_ReadOne(dd));

  Abc_ObjRemoveFanins(pNode);
  Abc_ObjAddFanin(pNode, Abc_ObjNotCond(Abc_NtkObj(pNtk, fid0), fic0));
  Abc_ObjAddFanin(pNode, Abc_ObjNotCond(Abc_NtkObj(pNtk, fid1), fic1));

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
  Abc_Obj_t* pNodeNew=Abc_NtkObj(pNtkNew, target);
  DdManager* dd=NULL;
  DdNode* dccheck=getDC_bdd(pNtkNew, pNodeNew, dd,false);
  if(dd==NULL){
    Abc_Print(-1, "build bdd fail\n");
  }
  DdGen *gen;
  int *cube;
  CUDD_VALUE_TYPE value;
  int count=0;
  Cudd_ForeachCube(dd, dccheck, gen,cube,value ){
    //Abc_Print(-2, "cube: ");
    //for(int j=0;j<dd->size;j++){
    //  if(cube[j]==0)
    //    Abc_Print(-2, "!%d ", j);
    //  else if(cube[j]==1)
    //    Abc_Print(-2, "%d ", j);
    //}
    //Abc_Print(-2, "\n");
    count+=DCIntGen(cube, bc,dd->size-1);
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
//ref: Abc_NtkBuildGlobalBdds
DdNode* Build_ImageBdd(Abc_Ntk_t*& pNtk, Abc_Obj_t* pNode ,DdNode* DC,int* roots,int rootnum){
  int pinum=Abc_NtkPiNum(pNtk);
  DdManager* dd=(DdManager*)Abc_NtkGlobalBddMan(pNtk);
  map<int, DdNode*> nodeid2ithvar;
  for(int i=0;i<rootnum;i++){
    nodeid2ithvar[roots[i]]=Cudd_bddIthVar(dd, i+pinum);
  }
  DdNode* yBdd;
  for(int i=0;i<rootnum;i++){
    if(i==0){
      yBdd=Cudd_bddXnor(dd, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkObj(pNtk,roots[i])), nodeid2ithvar[roots[i]]);
      Cudd_Ref(yBdd);
    }else{
      DdNode* temp=Cudd_bddXnor(dd, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkObj(pNtk,roots[i])), nodeid2ithvar[roots[i]]);
      Cudd_Ref(temp);
      DdNode* temp2=Cudd_bddAnd(dd, yBdd, temp);
      Cudd_Ref(temp2);
      Cudd_RecursiveDeref(dd, yBdd);
      Cudd_RecursiveDeref(dd, temp);
      yBdd=temp2;
    }
  }
  Abc_Obj_t* pCo;
  int i;
  DdNode** hon=new DdNode*[Abc_NtkCoNum(pNtk)];
  DdNode** hoff=new DdNode*[Abc_NtkCoNum(pNtk)];
  Abc_NtkForEachCo(pNtk, pCo, i){
    DdNode* ddnode1=Cudd_Cofactor(dd, (DdNode *)Abc_ObjGlobalBdd(pCo), (DdNode *)yBdd);
    Cudd_Ref(ddnode1);
    DdNode* ddnode2=Cudd_Cofactor(dd, (DdNode *)Abc_ObjGlobalBdd(pCo), Cudd_Not((DdNode *)yBdd));
    Cudd_Ref(ddnode2);
    DdNode* dcnode=Cudd_bddXnor(dd, ddnode1, ddnode2);
    Cudd_Ref(dcnode);
    Cudd_RecursiveDeref(dd, ddnode1);
    Cudd_RecursiveDeref(dd, ddnode2);
    DdNode* temp=Cudd_bddAnd(dd, DC, dcnode);
    Cudd_Ref(temp);
    Cudd_RecursiveDeref(dd, DC);
    Cudd_RecursiveDeref(dd, dcnode);
    DC=temp;
  }
}

int Resubsitution(Abc_Frame_t*& pAbc ,Abc_Ntk_t*& retntk ,Abc_Ntk_t* pNtk, int nodeid,bool needntk){
  int ithPo, ithPi;
  Abc_Obj_t* pPo, *pPi, *pNode;

  bool fDropInternal=false;
  bool fReorder=false;
  DdManager* dd=NULL;
  Abc_Ntk_t* pNtkNew=Abc_NtkDup(pNtk);
  Abc_Obj_t* Nodenew=Abc_NtkObj(pNtkNew, nodeid);

  DdNode* dc=getDC_bdd(pNtkNew, Nodenew, dd,fReorder);
  if(dd==NULL){
    Abc_Print(-1, "build bdd fail\n");
  }
  
  //Abc_Print(-2, "ddeqiv %d\n",dd==Abc_NtkGlobalBdd(pNtkNew));
  //Abc_Print(-2, "ddnull %d\n",Abc_NtkGlobalBdd(pNtkNew)==NULL);

  
  Abc_NtkForEachNode(pNtkNew, pNode, ithPo){
    if(Abc_ObjGlobalBdd(Abc_ObjFanin0(pNode))==NULL || Abc_ObjGlobalBdd(Abc_ObjFanin1(pNode))==NULL){
      assert((Abc_ObjGlobalBdd(pNode))!=NULL);
      Cudd_RecursiveDeref(dd,(DdNode*)(Abc_ObjGlobalBdd(pNode)));
      Abc_ObjSetGlobalBdd(pNode, NULL);
    }
  }
  //construct original bdd
  my_NtkBuildGlobalBdds(pNtkNew, fDropInternal, fReorder);
  dd=(DdManager*)Abc_NtkGlobalBddMan(pNtkNew);
  Abc_Print(-2, "ddvarnum %d\n",dd->size);
  Cudd_RecursiveDeref(dd, dc);
  return 0;
}