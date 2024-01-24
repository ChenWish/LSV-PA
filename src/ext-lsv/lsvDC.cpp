
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
#include "aig/hop/hop.h"

#define CONERATIO 0.1
extern "C"{
    Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
    Abc_Ntk_t * Abc_NtkFromGlobalBdds( Abc_Ntk_t * pNtk, int fReverse );
    void Abc_NtkShowBdd( Abc_Ntk_t * pNtk, int fCompl, int fReorder );
    Hop_Obj_t * Abc_ConvertSopToAig( Hop_Man_t * pMan, char * pSop );
    char * Abc_ConvertBddToSop( Mem_Flex_t * pMan, DdManager * dd, DdNode * bFuncOn, DdNode * bFuncOnDc, int nFanins, int fAllPrimes, Vec_Str_t * vCube, int fMode );
    void Abc_NtkStrashPerform( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew, int fAllNodes, int fRecord );
    void Abc_NtkShow( Abc_Ntk_t * pNtk0, int fGateNames, int fSeq, int fUseReverse, int fKeepDot );
}

extern int getCone(Abc_Ntk_t* pNtk, bool* coneRet, bool* input, int sizeup, int sizedown, set<int>& badConeRoot,bool verbose=false);
int RandomPickSet(set<int>& candidates);

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
      // set the resultf
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
DdNode* getDC_bdd(Abc_Ntk_t*& pNtk, Abc_Obj_t* tNode, DdManager*& dd,bool fReorder){
  assert(dd==NULL);
  int target=Abc_ObjId(tNode);
  int i;
  Abc_Obj_t* pNode;

  int fid0=Abc_ObjFaninId0(tNode);
  int fic0=Abc_ObjFaninC0(tNode);
  int fid1=Abc_ObjFaninId1(tNode);
  int fic1=Abc_ObjFaninC1(tNode);

  Abc_Obj_t * newpi=Abc_NtkCreatePi(pNtk);
  Abc_ObjRemoveFanins(tNode);
  Abc_ObjAddFanin(tNode, newpi);
  Abc_ObjAddFanin(tNode,Abc_AigConst1(pNtk));
  dd  = (DdManager *) my_NtkBuildGlobalBdds(pNtk, false, fReorder);
  
  //Abc_NtkForEachNode(pNtk, pNode, i){
  //  if(Abc_ObjGlobalBdd(pNode)==NULL){
  //    Abc_Print(-2, "node %d is null\n", i);
  //  }
  //}
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

  Abc_ObjRemoveFanins(tNode);
  assert(!Abc_ObjIsPi(tNode));
  assert(!Abc_ObjIsPo(Abc_ObjRegular(Abc_ObjNotCond(Abc_NtkObj(pNtk, fid0), fic0))));
  Abc_ObjAddFanin(tNode, Abc_ObjNotCond(Abc_NtkObj(pNtk, fid0), fic0));
  Abc_ObjAddFanin(tNode, Abc_ObjNotCond(Abc_NtkObj(pNtk, fid1), fic1));
  Abc_ObjSetGlobalBdd(tNode, NULL);
  Abc_NtkDeleteObj(newpi);
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
int TFO(Abc_Ntk_t* pNtk, Abc_Obj_t* pNode, set<int>& tfoSet){
  int i;
  Abc_Obj_t* pFanout;
  int count=0;
  set<Abc_Obj_t*> temp;
  temp.insert(pNode);
  tfoSet.insert(Abc_ObjId(pNode));
  while(!temp.empty()){
    Abc_Obj_t* pTemp=*(temp.begin());
    temp.erase(pTemp);
    Abc_ObjForEachFanout(pTemp, pFanout, i){
      if(tfoSet.find(Abc_ObjId(pFanout))==tfoSet.end()){
        tfoSet.insert(Abc_ObjId(pFanout));
        temp.insert(pFanout);
        count++;
      }
    }
  }
  return count;
}
bool Check(DdNode*& hon,Abc_Ntk_t* pNtk,DdNode* fon,DdNode* foff, DdNode* yBdd, DdNode* abs, DdManager* dd,bool verbose=false){
  int i;
  Abc_Obj_t* pCo;
  
  DdNode* hoff;
  DdNode* temp=Cudd_bddAnd(dd, fon, yBdd);
  Cudd_Ref(temp);
  //Abc_Print(-2, "abs\n");
  //Cudd_PrintMinterm(dd, abs);
  //Abc_Print(-2, "temp\n");
  //Cudd_PrintMinterm(dd, temp);
  hon=Cudd_bddExistAbstract(dd, temp, abs);
  Cudd_Ref(hon);
  Cudd_RecursiveDeref(dd, temp);
  //Abc_Print(-2, "hon\n");
  //Cudd_PrintMinterm(dd, hon);
  if(hon==Cudd_ReadZero(dd)){
    Abc_Print(-2, "hon is zero\n");
  }


  temp=Cudd_bddAnd(dd, foff, yBdd);
  Cudd_Ref(temp);
  hoff=Cudd_bddExistAbstract(dd, temp, abs);
  Cudd_Ref(hoff);
  //Abc_Print(-2, "hoff\n");
  //Cudd_PrintMinterm(dd, hoff);
  Cudd_RecursiveDeref(dd, temp);

  DdNode* intersec=Cudd_bddAnd(dd, hon, hoff);
  Cudd_Ref(intersec);
  DdGen* gen;
  int* cube;
  CUDD_VALUE_TYPE value;
  if(verbose){
    Abc_Print(-2, "hon\n");
    if(Cudd_PrintMinterm(dd, hon)==0){
      Abc_Print(-2, "memory out\n");
    }
    Abc_Print(-2, "hoff\n");
    if(Cudd_PrintMinterm(dd, hoff)==0){
      Abc_Print(-2, "memory out\n");
    }
  }

  if(Cudd_IsConstant(intersec)){
    Abc_Print(-2, "intersec is constant\n");
  }
  if(Cudd_IsConstant(intersec) && Cudd_IsComplement(intersec)){
    Cudd_RecursiveDeref(dd, intersec);
    Cudd_RecursiveDeref(dd, hoff);
    
    return true;
  }else{
    //Abc_Print(-2, "onset and offset intersects\n");
    Cudd_RecursiveDeref(dd, intersec);
    Cudd_RecursiveDeref(dd, hon);
    Cudd_RecursiveDeref(dd, hoff);
    return false;
  }
}
DdNode* CheckRemove(DdNode*& hon,Abc_Ntk_t* pNtk,DdNode* fon,DdNode* foff, DdNode* yBdd, DdNode* abs, DdManager* dd, DdNode* ithvar,bool verbose=false){
  DdNode* temp=Cudd_Cofactor(dd, yBdd, ithvar);
  Cudd_Ref(temp);
  DdNode* temp2=Cudd_Cofactor(dd, yBdd, Cudd_Not(ithvar));
  Cudd_Ref(temp2);
  DdNode* temp3=Cudd_bddOr(dd, temp, temp2);
  Cudd_Ref(temp3);
  Cudd_RecursiveDeref(dd, temp);
  Cudd_RecursiveDeref(dd, temp2);
  if(Check(hon,pNtk, fon, foff, temp3, abs, dd,verbose)){
    return temp3;
  }else{
    Cudd_RecursiveDeref(dd, temp3);
    return NULL;
  }
}
int randomReplace(Abc_Ntk_t* pNtk,DdNode* yBdd, int replacer,map<int, DdNode*>& nodeid2ithvar, DdManager* dd, set<int> & selected){
  int tar= RandomPickSet(selected);
  DdNode* ithvar=nodeid2ithvar[tar];

  // remove target from yBdd
  DdNode* temp=Cudd_Cofactor(dd, yBdd, ithvar);
  Cudd_Ref(temp);
  DdNode* temp2=Cudd_Cofactor(dd, yBdd, Cudd_Not(ithvar));
  Cudd_Ref(temp2);
  DdNode* temp3=Cudd_bddOr(dd, temp, temp2);
  Cudd_Ref(temp3);
  //Cudd_recursiveDeref(dd, temp);
  //Cudd_recursiveDeref(dd, temp2);
  //Cudd_recursiveDeref(dd, yBdd);
  yBdd=temp3;
  nodeid2ithvar.erase(tar);
  selected.erase(tar);

  // add replacer to yBdd
  DdNode* replacerBdd=(DdNode*) Abc_ObjGlobalBdd(Abc_NtkObj(pNtk, replacer));
  nodeid2ithvar[replacer]=ithvar;
  selected.insert(replacer);
  temp=Cudd_bddXnor(dd, replacerBdd, ithvar);
  Cudd_Ref(temp);
  temp2=Cudd_bddAnd(dd, yBdd, temp);
  Cudd_Ref(temp2);
  //Cudd_recursiveDeref(dd, yBdd);
  //Cudd_recursiveDeref(dd, temp);
  yBdd=temp2;
  return tar;
}

int RandomPickSet(set<int>& candidates){
  set<int>::iterator itr;
  itr=candidates.begin();
  advance(itr, rand()%candidates.size());
  return *itr;
}
int Build_ImageBdd(DdNode*& hon,Abc_Ntk_t*& pNtk, Abc_Obj_t* pNode ,DdNode* DC,set<int>& candidates,set<int>& selected,map<int, DdNode*>& nodeid2ithvar, int maxinput=50){
  int pinum=Abc_NtkPiNum(pNtk);
  DdManager* dd=(DdManager*)Abc_NtkGlobalBddMan(pNtk);
  int iteration=(candidates.size()-maxinput)*maxinput/10;
  Abc_Print(-2, "iteration %d\n", iteration);
  if(iteration<0)
    iteration=0;
  int count=Abc_NtkPiNum(pNtk);
  //construct original ybdd
  for(int i=0;i<maxinput;i++){
    int temp=RandomPickSet(candidates);
    nodeid2ithvar[temp]=Cudd_bddIthVar(dd, count);
    Abc_Print(-2, "node %d ithvar %d\n", temp, count);
    selected.insert(temp);
    count++;
    Cudd_Ref(nodeid2ithvar[temp]);
    candidates.erase(temp);
  }
  set<int>::iterator itr;
  DdNode* yBdd;
  
  for(itr=selected.begin();itr!=selected.end();itr++){
    if(itr==selected.begin()){
      yBdd=Cudd_bddXnor(dd, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkObj(pNtk,*itr)), nodeid2ithvar[*itr]);
      Cudd_Ref(yBdd);
    }else{
      DdNode* temp=Cudd_bddXnor(dd, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkObj(pNtk,*itr)), nodeid2ithvar[*itr]);
      Cudd_Ref(temp);
      DdNode* temp2=Cudd_bddAnd(dd, yBdd, temp);
      Cudd_Ref(temp2);
      //Cudd_RecursiveDeref(dd, yBdd);
      //Cudd_recursiveDeref(dd, temp);
      yBdd=temp2;
    }
  }
  int i;
  DdNode* fon;
  DdNode* foff;
  //generate fon and foff
  //Abc_Print(-2, "DC\n");
  //Cudd_PrintMinterm(dd, DC);
  fon=Cudd_bddAnd(dd, (DdNode *)Abc_ObjGlobalBdd(pNode), Cudd_Not(DC));
  Cudd_Ref(fon);
  //Abc_Print(-2, "fon\n");
  //Cudd_PrintMinterm(dd, fon);
  foff=Cudd_bddAnd(dd, Cudd_Not((DdNode *)Abc_ObjGlobalBdd(pNode)), Cudd_Not(DC));
  Cudd_Ref(foff);
  //Abc_Print(-2, "foff\n");
  //Cudd_PrintMinterm(dd, foff);
  //generate abs for existance quantification
  DdNode* abs=Cudd_bddIthVar(dd, 0);
  for(int i=1;i<Abc_NtkPiNum(pNtk);i++){
    DdNode* temp=Cudd_bddAnd(dd, abs, Cudd_bddIthVar(dd, i));
    Cudd_Ref(temp);
    abs=temp;
  }
  bool success=false;
  //Abc_Print(-2, "yBdd\n");
  //Cudd_PrintMinterm(dd, yBdd);

  //do check and random replace iteration times
  set<int> erased;
  for(int i=0;i<iteration+1;i++){
    if(Check(hon,pNtk, fon, foff, yBdd, abs, dd)){
      for(itr=selected.begin();itr!=selected.end();itr++){
        Abc_Print(-2, "selected %d\n", *itr);
        Abc_Print(-2, "try remove %d\n", *itr);
        bool verbose=false;
        //if(*itr==32||*itr==31){
        //  verbose=true;
        //}
        DdNode* temp=CheckRemove(hon,pNtk, fon, foff, yBdd, abs, dd, nodeid2ithvar[*itr],verbose);
        if(temp!=NULL){
          Abc_Print(-2, "remove success\n");
          //Cudd_recursiveDeref(dd, yBdd);
          yBdd=temp;
          erased.insert(*itr);
        }
      }
      success=true;
      for(itr=erased.begin();itr!=erased.end();itr++){
        selected.erase(*itr);
      }
      Abc_Print(-2, "resub success\n");
      for(itr=selected.begin();itr!=selected.end();itr++){
        Abc_Print(-2, "selected %d\n", *itr);
      }
      break;
    }else{
      Abc_Print(-2, "resub fail\n");
      int replacer=RandomPickSet(candidates);
      candidates.erase(replacer);
      int replaced=randomReplace(pNtk, yBdd, replacer, nodeid2ithvar, dd, selected);
      candidates.insert(replaced);
      //Abc_Print(-2, "replacer %d replaced %d\n", replacer, replaced);
    }
  }
  Abc_Print(-2,"Iteration over\n");

  //if(!success){
  //  Cudd_recursiveDeref(dd, yBdd);
  //}
  DdNode* temp=Cudd_bddAnd(dd, fon, yBdd);
  Cudd_Ref(temp);
  hon=Cudd_bddExistAbstract(dd, temp, abs);
  Cudd_Ref(hon);
  Cudd_RecursiveDeref(dd, temp);
  //free resources
  Cudd_RecursiveDeref(dd, abs);
  Cudd_RecursiveDeref(dd, fon);
  Cudd_RecursiveDeref(dd, foff);

  return success;
}

int replace(Abc_Ntk_t*& pNtkNew, Abc_Ntk_t*& pNtk, int root,bool* cone,set<int> selected){
  
  set<int>::iterator itr;
  itr=selected.begin();
  Abc_Obj_t* pNode;
  int i;
  map<Abc_Obj_t*, Abc_Obj_t*> nodenew2node;
  Abc_Obj_t* pPo;
  Abc_NtkForEachPi(pNtkNew, pNode, i){
    nodenew2node[pNode]=Abc_NtkObj(pNtk, *itr);
    itr++;
  }
  Abc_ObjRemoveFanins(Abc_NtkObj(pNtk, root));
  for(int i=Abc_NtkObjNum(pNtk)-1;i>=0;i--){
    if(i!=root && cone[i]){
      Abc_NtkDeleteObj(Abc_NtkObj(pNtk, i));
    }
  }

  
  pPo=Abc_NtkPo(pNtkNew, 0);
  bool shown=false;
  Abc_NtkForEachNode(pNtkNew, pNode, i){
    if(pNode==Abc_ObjFanin0(pPo) || pNode==Abc_ObjFanin1(pPo)){
      if(shown==false){
        shown=true;
      }else{
      }
      nodenew2node[pNode]=Abc_NtkObj(pNtk, root);
    }else{
      Abc_Obj_t* temp;
      temp=Abc_NtkCreateNode(pNtk);
      nodenew2node[pNode]=temp;
    }
  }
  Abc_NtkForEachNode(pNtkNew, pNode, i){
    Abc_Obj_t* temp=nodenew2node[pNode];
    Abc_Obj_t* fanin0=nodenew2node[Abc_ObjFanin0(pNode)];
    Abc_Obj_t* fanin1=nodenew2node[Abc_ObjFanin1(pNode)];
    //Abc_ObjSetCopy(temp, temp);

    if(temp==NULL || fanin0==NULL || fanin1==NULL){
      Abc_Print(-1, "null pointer\n");
    }
    Abc_ObjAddFanin(temp, Abc_ObjNotCond(fanin0,pNode->fCompl0));
    Abc_ObjAddFanin(temp, Abc_ObjNotCond(fanin1,pNode->fCompl1));
  }
  map<Abc_Obj_t*, Abc_Obj_t*> node2retnode;
  Abc_Ntk_t* pNtkRet;
  pNtkRet=Abc_NtkStartFrom(pNtk,ABC_NTK_STRASH,ABC_FUNC_AIG);
  Abc_NtkForEachPi(pNtk, pNode, i){
    node2retnode[pNode]=Abc_NtkPi(pNtkRet, i);
  }
  Abc_NtkForEachPo(pNtk, pNode, i){
    node2retnode[pNode]=Abc_NtkPo(pNtkRet, i);
  }
  for(int j=0;j<Abc_NtkNodeNum(pNtk);j++){
    Abc_NtkForEachNode(pNtk, pNode, i){
      if(node2retnode[pNode]==NULL && node2retnode[Abc_ObjFanin0(pNode)]!=NULL && node2retnode[Abc_ObjFanin1(pNode)]!=NULL){
        Abc_Obj_t* temp;
        temp=Abc_NtkCreateNode(pNtkRet);
        node2retnode[pNode]=temp;
        Abc_ObjAddFanin(temp, Abc_ObjNotCond(node2retnode[Abc_ObjFanin0(pNode)],pNode->fCompl0));
        Abc_ObjAddFanin(temp, Abc_ObjNotCond(node2retnode[Abc_ObjFanin1(pNode)],pNode->fCompl1));
        Abc_Print(-2, "nodecreated original %d ret %d\n", i, Abc_ObjId(temp));
      }
    } 
  }
  for(int i=0;i<Abc_NtkNodeNum(pNtk);i++){
    Abc_NtkForEachNode(pNtk, pNode, i){
      if(node2retnode[pNode]==NULL){
        Abc_Print(-2, "node %d is null\n", i);
        Abc_Print(-2,"it's children is %d %d\n",Abc_ObjId(Abc_ObjFanin0(pNode)),Abc_ObjId(Abc_ObjFanin1(pNode)));
      }
    } 
  }
  Abc_NtkForEachPo(pNtk, pNode, i){
    Abc_ObjRemoveFanins(node2retnode[pNode]);
    Abc_ObjAddFanin(node2retnode[pNode], Abc_ObjNotCond(node2retnode[Abc_ObjFanin0(pNode)],pNode->fCompl0));
  }
  Abc_Obj_t* pObj;
  Abc_NtkShow(pNtkNew,0, 0, 1, 0);
  Abc_NtkShow(pNtkRet,0, 0, 1, 0);
  pNtk=pNtkRet;
  return 0;
}
int Resubsitution(Abc_Frame_t*& pAbc ,Abc_Ntk_t*& retntk ,Abc_Ntk_t* pNtk, int num,bool needntk,set<int>& badConeRoot,int maxinput){
  int ithPo, ithPi;
  Abc_Obj_t* pPo, *pPi, *pNode;
  
  Abc_Print(-2, "nodenum %d\n",Abc_NtkObjNum(pNtk));
  bool fDropInternal=false;
  bool fReorder=false;
  Abc_Ntk_t* pNtkNew=Abc_NtkDup(pNtk);
  DdManager* dd=NULL;

  bool cone[Abc_NtkObjNum(pNtkNew)]= {false};
  bool input[Abc_NtkObjNum(pNtkNew)]= {false};
  int root=getCone(pNtkNew, cone, input, int(Abc_NtkObjNum(pNtkNew)*CONERATIO), 3, badConeRoot);
  int conesize=0;
  for(int i=0;i<Abc_NtkObjNum(pNtkNew);i++){
    if(cone[i])
      conesize++;
  }
  set<int> candidates;
  set<int> selected;
  set<int> tfoSet;
  map<int, DdNode*> nodeid2ithvar;
  TFO(pNtkNew, Abc_NtkObj(pNtkNew, root), tfoSet);
  for(int i=0;i<Abc_NtkObjNum(pNtkNew);i++){
    if(!cone[i] && (tfoSet.find(i)==tfoSet.end()) && (Abc_ObjIsPi(Abc_NtkObj(pNtkNew, i))||Abc_ObjIsNode(Abc_NtkObj(pNtkNew, i)))){
      candidates.insert(i);
      //Abc_Print(-2, "candidate insert%d\n", i);
    }
  }
  Abc_Print(-2, "root: %d\n", root);
  Abc_Obj_t* Nodenew=Abc_NtkObj(pNtkNew, root);
  DdNode* dc=getDC_bdd(pNtkNew, Nodenew, dd,fReorder);
  //Abc_Print(-2, "dc\n");
  //Cudd_PrintMinterm(dd, dc);
  if(dd==NULL){
    Abc_Print(-1, "build bdd fail\n");
  }
  //Abc_NtkForEachNode(pNtkNew, pNode, ithPo){
  //  if(Abc_ObjGlobalBdd(pNode)==NULL){
  //    Abc_Print(-2, "node %d is null\n", ithPo);
  //  }
  //}

  Abc_NtkForEachNode(pNtkNew, pNode, ithPo){
    if(Abc_ObjGlobalBdd(Abc_ObjFanin0(pNode))==NULL || Abc_ObjGlobalBdd(Abc_ObjFanin1(pNode))==NULL){
      if(Abc_ObjGlobalBdd(pNode)!=NULL){
        //Cudd_recursiveDeref(dd,(DdNode*)(Abc_ObjGlobalBdd(pNode)));
        Abc_ObjSetGlobalBdd(pNode, NULL);
      }
    }
  }
  //construct original bdd
  my_NtkBuildGlobalBdds(pNtkNew, fDropInternal, fReorder);
  dd=(DdManager*)Abc_NtkGlobalBddMan(pNtkNew);
  //testing image bdd
  
  DdNode* hon;
  int success=Build_ImageBdd(hon, pNtkNew, Nodenew, dc, candidates,selected,nodeid2ithvar, maxinput);
  Abc_Print(-2, "success %d\n", success);
  
  if(success==0){
    Abc_Print(-2, "resub fail\n");
    Abc_NtkDelete(pNtkNew);
    return 0;
  }
  badConeRoot.clear();
  //build aig from bdd
  DdGen *gen;
  int *cube;
  set<int>::iterator itr;
  string sopstr="";
  CUDD_VALUE_TYPE value;
  //Abc_Print(-2, "root: %d\n", root);
  //Cudd_PrintMinterm(dd, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkObj(pNtkNew, root)));
  //for(itr=selected.begin();itr!=selected.end();itr++){
  //  Abc_Print(-2, "node: %d, ithvar: %d\n", *itr, nodeid2ithvar[*itr]->index);
  //  //Cudd_PrintMinterm(dd, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkObj(pNtkNew, *itr)));
  //}
  Cudd_ForeachCube(dd, hon, gen,cube,value ){
    DdNode* temp=Cudd_ReadOne(dd);
    for(itr=selected.begin();itr!=selected.end();itr++){
      int id=nodeid2ithvar[*itr]->index;
      if(cube[id]==0){
        temp=Cudd_bddAnd(dd, temp, Cudd_Not((DdNode*)Abc_ObjGlobalBdd(Abc_NtkObj(pNtkNew, *itr))));
        sopstr.append("0");
      }
      else if(cube[id]==1){
        temp=Cudd_bddAnd(dd, temp, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkObj(pNtkNew, *itr)));
        sopstr.append("1");
      }
      else
        sopstr.append("-");
    }
    Cudd_Ref(temp);
    //Abc_Print(-2, "cube: ");
    //Cudd_PrintMinterm(dd, temp);
    sopstr.append(" 1\n");
  }
  Hop_Man_t * pMan;
  pMan = Hop_ManStart();
  int Max=selected.size();
  if ( Max ) Hop_IthVar( pMan, Max-1 );
  Abc_Ntk_t* pNtkNewNew=Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_AIG, 1 );
  pNode=Abc_NtkCreateNode( pNtkNewNew );
  Abc_Obj_t* pCo=Abc_NtkCreatePo( pNtkNewNew );
  Abc_ObjAddFanin(pCo, pNode);
  for(int i=0;i<selected.size();i++){
    Abc_Obj_t* pPi=Abc_NtkCreatePi( pNtkNewNew );
    Abc_ObjAddFanin(pNode, pPi);
  }
  Abc_Print(-2, "sopstr: %s\n", sopstr.c_str());
  pNode->pData = Abc_ConvertSopToAig( pMan, (char*)sopstr.c_str() );
  pNtkNewNew->pManFunc=pMan;
  pNtkNewNew->ntkFunc = ABC_FUNC_AIG;

  pNtkNewNew=Abc_NtkStrash(pNtkNewNew, 0, 1, 0);
  pNtkNewNew=Abc_NtkBalance( pNtkNewNew, 0, 0, 1 );
  Abc_NtkRewrite( pNtk, 1, 0, 0, 0, 0 );
  Abc_NtkRewrite( pNtk, 1, 1, 0, 0, 0 );
  pNtkNewNew=Abc_NtkBalance( pNtkNewNew, 0, 0, 1 );
  Abc_NtkRewrite( pNtk, 1, 1, 0, 0, 0 );
  int i;
  if(Abc_NtkNodeNum(pNtkNewNew)<=conesize){
    replace(pNtkNewNew, pNtkNew, root,cone,selected);
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkNew);
    Abc_NtkDelete(pNtk);
  }
  else{
    Abc_Print(-2, "after resub is larger\n");
  }
  //Abc_Ntk_t* pNtkAig = Abc_NtkStartFrom( pNtkNewNew, ABC_NTK_STRASH, ABC_FUNC_AIG );
  //Abc_NtkStrashPerform( pNtkNewNew, pNtkAig, 0, 0 );
  //Abc_NtkFinalize( pNtkNewNew, pNtkAig );
  Abc_Print(-2, "ddvarnum %d\n",dd->size);
  //Cudd_recursiveDeref(dd, dc);

  Abc_NtkForEachObj(pNtkNew, pNode, ithPo){
    if(Abc_ObjGlobalBdd(pNode)!=NULL){
      //Cudd_recursiveDeref(dd,(DdNode*)(Abc_ObjGlobalBdd(pNode)));
      Abc_ObjSetGlobalBdd(pNode, NULL);
    }
  }
  Abc_NtkFreeGlobalBdds(pNtkNew, 1);
  Abc_NtkDelete(pNtkNewNew);

  return 0;
}