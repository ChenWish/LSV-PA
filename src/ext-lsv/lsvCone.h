
#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <string>
#include <bdd/cudd/cudd.h>
#include <set>

#define RATIO 4

using namespace std;
#ifndef LSVCONE_H
#define LSVCONE_H
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
int getCone(Abc_Ntk_t* pNtk, bool* coneRet, bool* input, int sizeup, int sizedown, set<int>& badConeRoot,bool verbose=false) {
  if (Abc_NtkIsStrash(pNtk) == NULL) {
    Abc_Print(-1, "The network is not Aig logic.\n");
  }
  int i;
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
  int ratio=RATIO;
  Abc_NtkForEachNode(pNtk, pNode, i) {
    if (badConeRoot.find(i) != badConeRoot.end()) {
      continue;
    }
    if(Abc_ObjIsCo(Abc_ObjFanout(pNode, 0))||Abc_ObjIsCi(pNode)) {
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
    zerofanout(cone ,i, coneset, input, length);
    int res = 0;
    for (int j = 0; j < length; ++j) {
      if (coneset[j]) {
        res++;
      }
    }
    if (res > sizedown) {
      if (res > sizeup) {
        int count = 0;
        for (int j = 0; j < length; ++j) {
          if (coneset[j]) {
            count++;
            coneset[j] = false;
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
            if(coneset[j]){
              if(coneset[Abc_ObjId(pFanin)]){
                input[Abc_ObjId(pFanin)] = false;
              }
              else{
                input[Abc_ObjId(pFanin)] = (coneset[Abc_ObjId(pFanin)]?false:true);
                Abc_Print(-2, "node %d set %d  in input\n",j, Abc_ObjId(pFanin));
              }
            }
          }
        }
      }
      for (int j = 0; j < length; ++j) {
        if (input[j]) {
          input_num++;
        }
      }

      if(((res == conemax && input_num < input_min)&&((rand() % ratio) != 0))||(res > conemax) ) {
        if(verbose){
          Abc_Print(-2, "node %d res=%d: ", i, res);
          for(int j=0;j<length;++j){
            if(coneset[j])
              Abc_Print(-2, "%d ", j);
          }
          Abc_Print(-2, "\n");
        } 
        input_min = input_num;
        conemax = res;
        conemaxid = i;
        memcpy(coneRet, coneset, sizeof(bool) * length);
        ratio+=1;
      }
    }else{
      badConeRoot.insert(i);
    }
  }
  badConeRoot.insert(conemaxid);
  delete [] cone;
  return conemaxid;
}
#endif