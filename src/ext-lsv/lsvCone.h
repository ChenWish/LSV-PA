#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <string>
#include <bdd/cudd/cudd.h>
#include <set>

#define RATIO 8

using namespace std;

typedef struct coneobj 
{
  int out_count=0;
  Abc_Obj_t * node=NULL;
}coneobj_t;

bool forallfanout(coneobj_t* node, bool* set){
  Abc_Obj_t* pFanout;
  int i;
  if(node->out_count!=0){
    //Abc_Print(-2, "node:%d out_count=%d\n", Abc_ObjId(node->node),node->out_count);
    return false;
  }
  Abc_ObjForEachFanout(node->node, pFanout, i){
    if(!set[Abc_ObjId(pFanout)]){
      node->out_count+=1;
    }
  }
  if(node->out_count>0)
    return false;
  else
    return true;
}
int recursive_include(coneobj_t* cone,int node, bool* set,bool* input){

  int total=1;
  int i;
  Abc_Obj_t* pFanin;
  if(input[node])
    input[node]=false;
  Abc_ObjForEachFanin(cone[node].node, pFanin, i){
    if(cone[Abc_ObjId(pFanin)].out_count==0)
      continue;
    else{
      if(--cone[Abc_ObjId(pFanin)].out_count==0){
        set[Abc_ObjId(pFanin)]=true;
        //Abc_Print(-2, "node %d set in recursive\n", Abc_ObjId(pFanin));
        total+=recursive_include(cone,Abc_ObjId(pFanin), set,input);
      }
      if(cone[Abc_ObjId(pFanin)].out_count<0){
        Abc_Print(-1, "out_count=%d\n", cone[Abc_ObjId(pFanin)].out_count);
      }
    }
  }
  return total;
}
int zerofanout(coneobj_t* cone,int node, bool* set, bool* input,int length){
  Abc_Obj_t* pFanin;
  int i;
  int total=0;
  Abc_ObjForEachFanin(cone[node].node, pFanin, i){
    if (Abc_ObjIsPi(pFanin))
      continue;
    if(Abc_ObjFanoutNum(pFanin)==1 || forallfanout(&cone[Abc_ObjId(pFanin)], set)){
      set[Abc_ObjId(pFanin)]=true;
      //Abc_Print(-2, "node %d set in zerofanout\n", Abc_ObjId(pFanin));
      total+=recursive_include(cone,Abc_ObjId(pFanin), set, input);
      total+=zerofanout(cone,Abc_ObjId(pFanin), set, input, length);
    }else{
      input[Abc_ObjId(pFanin)]=true;
    }
  }
  return total;
}
//set is a bool array to record the node is in the window
int getCone(Abc_Ntk_t* pNtk, bool* coneRet, bool* input, int sizeup, int sizedown, set<int>& badConeRoot) {
  if (Abc_NtkIsStrash(pNtk) == NULL) {
    Abc_Print(-1, "The network is not Aig logic.\n");
  }
  int i;
  //Abc_NtkForEachObj(pNtk, pNode, i){
  //  Abc_Print(-2, "node %d type %d \n", Abc_ObjId(pNode), Abc_ObjType(pNode));
  //}
  //Abc_Obj_t* pCi;
  //Abc_NtkForEachCi(pNtk, pCi, i){
  //  Abc_Print(-2, "CI: %s %d\n", Abc_ObjName(pCi),Abc_ObjId(pCi));
  //}
  Abc_Obj_t* pNode;
  int length = Abc_NtkObjNum(pNtk);
  if (input == NULL) {
    input = new bool[length];
  }
  coneobj_t* cone = new coneobj_t[length];
  bool* coneset=new bool[length];
  for (i = 0; i < length; ++i) {
    cone[i].node = Abc_NtkObj(pNtk, i);
  } 
  int conemax=-1;
  int conemaxid=-1;
  int input_min=length;
  Abc_NtkForEachNode(pNtk, pNode, i) {
    if (badConeRoot.find(i) != badConeRoot.end()) {
      continue;
    }
    if(Abc_ObjIsCo(Abc_ObjFanout(pNode, 0))) {
      badConeRoot.insert(i);
      continue;
    }
    memset(coneset, false, length);
    memset(input, false, length);
    for (int j = 0; j < length; ++j) {
      cone[j].out_count = 0;
    }
    //find all 
    coneset[Abc_ObjId(pNode)] = true;
    int res = zerofanout(cone ,i, coneset, input, length) + 1;
    if (res > sizedown) {
      if (res > sizeup) {
        int count = 0;
        for (int j = 0; j < length; ++j) {
          if (coneset[j]) {
            count++;
            coneset[j] = false;
            Abc_Obj_t* pFanin;
            int k = 0;
            if(count == res-sizeup)
              break;
          }
        }
        res=sizeup;
      }

      int input_num = 0;
      for(int j = 0; j < length; ++j) {
        if (coneset[j]) {
          Abc_Obj_t* pFanin;
          int k = 0;
          Abc_ObjForEachFanin(Abc_NtkObj(pNtk, j), pFanin, k) {
            input[Abc_ObjId(pFanin)] = coneset[Abc_ObjId(pFanin)]?false:true;
          }
        }
      }
      for (int j = 0; j < length; ++j) {
        if (input[j]) {
          input_num++;
        }
      }

      if(input_num <= input_min && res >= conemax && ((rand() % RATIO) != 0)) {
        input_min = input_num;
        conemax = res;
        conemaxid = i;
        memcpy(coneRet, coneset, sizeof(bool) * length);
      }
/*
      Abc_Print(-2, "Cone %d num %d\n", Abc_ObjId(pNode), res);
      for(int i=0;i<length;i++){
        if(coneset[i]){
          Abc_Print(-2, "node %d ", i);
        }
      }
      Abc_Print(-2, "\n");
      Abc_Print(-2, "input num %d\n", input_num);
      for(int i=0;i<length;i++){
        if(input[i]){
          Abc_Print(-2, "node %d ", i);
        }
      }
      Abc_Print(-2, "\n");
*/
    }else{
      badConeRoot.insert(i);
    }
  }
  delete [] cone;
  return conemaxid;
}