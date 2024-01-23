#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "booleanChain.h"
#include "sat/glucose2/AbcGlucose2.h"
#include "sat/glucose2/SimpSolver.h"
#include <iostream>
#include <set>
#include <map>
#include "lsvCone.h"

#include "sat/cnf/cnf.h"
extern "C"{
    Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
}

using namespace std;
static int resub_test(Abc_Frame_t* pAbc, int argc, char** argv);
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
extern int Resubsitution(Abc_Frame_t*& pAbc ,Abc_Ntk_t*& retntk ,Abc_Ntk_t* pNtk, int nodeid,bool needntk, set<int>& badConeRoot);
static int test2_Command(Abc_Frame_t* pAbc, int argc, char** argv);

static BooleanChain booleanChain;

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
  Cmd_CommandAdd(pAbc, "LSV", "test2", test2_Command, 0);
  srand(5487);
  Cmd_CommandAdd(pAbc, "LSV", "rrr", resub_test, 0);
}

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

static int stupid(Abc_Ntk_t* pNtk, int id, int saValue, int undetectable) {
  // cout << "id = " << id << ", saValue = " << saValue << ", undetectable = " << undetectable << endl;
  map<int, int> id2Var;
  map<int, int> id2VarSA;
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
    if (pObj->Id != id)
      sat_solver_add_and(pSat, id2VarSA[pObj->Id], id2VarSA[fanin0], id2VarSA[fanin1], c0, c1, 0);
  }
  int* miterVar = new int[Abc_NtkPoNum(pNtk)];
  lit* clause = new lit[Abc_NtkPoNum(pNtk)];
  Abc_NtkForEachPo(pNtk, pObj, i) {
    int var = id2Var[Abc_ObjFanin0(pObj)->Id];
    int varSA = id2VarSA[Abc_ObjFanin0(pObj)->Id];
    miterVar[i] = sat_solver_addvar(pSat);
    sat_solver_add_xor(pSat, miterVar[i], var, varSA, !undetectable);
    clause[i] = toLitCond(miterVar[i], 0);
    // assert(id2Var.find(Abc_ObjFanin0(pObj)->Id) != id2Var.end());
    // assert(id2VarSA.find(Abc_ObjFanin0(pObj)->Id) != id2VarSA.end());
    // cerr << "Fanin0 = " << Abc_ObjFanin0(pObj)->Id << endl;
  }
  sat_solver_addclause(pSat, clause, clause + Abc_NtkPoNum(pNtk));
  delete[] miterVar;
  delete[] clause;
  clause = new lit[Abc_NtkPiNum(pNtk)];
  int index = 0;
  clause[index++] = toLitCond(id2VarSA[id], saValue == 0);
  if (!undetectable) {
    int value;
    cout << "Enter PI assignment(index start from 1): ";
    while (cin >> value) {
      if (value == 0)
        break;
      if (index == Abc_NtkPiNum(pNtk) + 1) {
        cerr << "Error: index out of bound" << endl;
        break;
      }
      clause[index++] = toLitCond(id2Var[abs(value)], value < 0);
    }
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
  return 0;
}
static int test2_Command(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  Abc_Ntk_t* pNtkSA = Abc_NtkDup(pNtk);
  if (argc != 4) {
    Abc_Print(-2, "usage: test2 <node> <sa(1/0)> <undetectable(1/0)>\n");
    Abc_NtkDelete(pNtkSA);
    return 0;
  }
  int id = atoi(argv[1]);
  int saValue = atoi(argv[2]);
  int undetectable = atoi(argv[3]);
  if (id <= Abc_NtkPiNum(pNtk) + Abc_NtkPoNum(pNtk) || id > Abc_NtkObjNum(pNtk)) {
    Abc_Print(-2, "Error: id is out of bound\n");
    Abc_NtkDelete(pNtkSA);
    return 0;
  }
  Abc_NtkDelete(pNtkSA);
  return stupid(pNtk, id, saValue, undetectable);
  Abc_Obj_t* pTarget = Abc_NtkObj(pNtkSA, id);
  Abc_Obj_t* pFanout;
  // cerr << pTarget->Type << endl;
  int i;
  Abc_ObjForEachFanout(pTarget, pFanout, i) {
    Abc_ObjDeleteFanin(pFanout, pTarget);
    Abc_ObjAddFanin(pFanout, Abc_ObjNotCond(Abc_AigConst1(pNtkSA), saValue == 0));
  }
  // cerr << pTarget->Type << endl;

  Abc_NtkDeleteObj(pTarget);
  Abc_Ntk_t* pNtkTemp = pNtkSA;
  pNtkSA = Abc_NtkStrash(pNtkTemp, 1, 1, 0); // fAllNodes, fCleanup, fRecord
  Abc_NtkDelete(pNtkTemp);
  int temp = Abc_AigCleanup( (Abc_Aig_t *)pNtkSA->pManFunc ); // ???
// cerr << temp << endl;
  // Abc_FrameReplaceCurrentNetwork(Abc_FrameReadGlobalFrame(), pNtkSA);
  // return 0;
  Aig_Man_t* pMan = Abc_NtkToDar(pNtk, 0, 0); // 0 -> fExors, 0 -> fRegisters
  Cnf_Dat_t* pCnf = Cnf_Derive(pMan, Aig_ManCoNum(pMan));
// cerr << "pNtk to Cnf done" << endl; 

  Aig_Man_t* pManSA = Abc_NtkToDar(pNtkSA, 0, 0); // 0 -> fExors, 0 -> fRegisters
  Cnf_Dat_t* pCnfSA = Cnf_Derive(pManSA, Aig_ManCoNum(pManSA));
// cerr << "pNtkSA to Cnf done" << endl; 

  int liftNum = Aig_ManObjNum(pMan);
  Cnf_DataLift(pCnfSA, liftNum);

  sat_solver* pSat = sat_solver_new();
  pSat = (sat_solver *)Cnf_DataWriteIntoSolverInt( pSat, pCnf, 1, 0 ); // 1 -> nFrames, 0 -> fInit 
  pSat = (sat_solver *)Cnf_DataWriteIntoSolverInt( pSat, pCnfSA, 1, 0 ); // 1 -> nFrames, 0 -> fInit 

  assert(Abc_NtkPiNum(pNtk) == Abc_NtkPiNum(pNtkSA));
  assert(Abc_NtkPoNum(pNtk) == Abc_NtkPoNum(pNtkSA));
  Abc_Obj_t* pObj;
  Abc_NtkForEachPi(pNtk, pObj, i) {
    lit clause[2];
    // (a + ~b)(~a + b)
    clause[0] = toLitCond(pCnf->pVarNums[pObj->Id]                  , 0); // Pi in pNtk
    clause[1] = toLitCond(pCnfSA->pVarNums[Abc_NtkPi(pNtkSA, i)->Id], 1); // Pi in pNtkSA
    sat_solver_addclause(pSat, clause, clause + 2);
    clause[0] = toLitCond(pCnf->pVarNums[pObj->Id]                  , 1); // Pi in pNtk
    clause[1] = toLitCond(pCnfSA->pVarNums[Abc_NtkPi(pNtkSA, i)->Id], 0); // Pi in pNtkSA
    sat_solver_addclause(pSat, clause, clause + 2);
  }

  Abc_NtkForEachPo(pNtk, pObj, i) {
    lit clause[2];
    clause[0] = toLitCond(pCnf->pVarNums[pObj->Id]                  , !undetectable); // Po in pNtk
    clause[1] = toLitCond(pCnfSA->pVarNums[Abc_NtkPo(pNtkSA, i)->Id], 1);     // Po in pNtkSA
    sat_solver_addclause(pSat, clause, clause + 2);
    clause[0] = toLitCond(pCnf->pVarNums[pObj->Id]                  , undetectable);  // Po in pNtk
    clause[1] = toLitCond(pCnfSA->pVarNums[Abc_NtkPo(pNtkSA, i)->Id], 0);     // Po in pNtkSA
    sat_solver_addclause(pSat, clause, clause + 2);
  }
  lit* clause = new lit[Abc_NtkPiNum(pNtk)];
  int index = 0;
  if (undetectable) {
    int value;
    cout << "Enter PI assignment(index start from 1): ";
    while (cin >> value) {
      if (value == 0)
        break;
      if (index == Abc_NtkPiNum(pNtk)) {
        cerr << "Error: index out of bound" << endl;
        break;
      }
      clause[index++] = toLitCond(pCnf->pVarNums[abs(value)], value < 0);
    }
  }
  bool result = (1 == sat_solver_solve(pSat, clause, clause + index, 0, 0, 0, 0)); // l_True = 1 in MiniSat
  if (!result) {
    cout << "The result is UNSAT" << endl;
  }
  else {
    cout << "The result is SAT, you fuck up" << endl;
    cout << "--------- CEX ---------" << endl;
    Abc_NtkForEachPi(pNtk, pObj, i) {
      int var = pCnf->pVarNums[pObj->Id];
      cout << sat_solver_var_value(pSat, var);
    }
    cout << endl;
    cout << "--------- CEX END ---------" << endl;
  }
  delete[] clause;
  return 0;
}
int resub_test(Abc_Frame_t* pAbc, int argc, char** argv){
  if(argc!=2){
    Abc_Print(-1, "rtest <nodeID>\n");
    return 0;
  }
  int nodeid=atoi(argv[1]);
  Abc_Ntk_t* pNtk =Abc_FrameReadNtk(pAbc);
  Abc_Ntk_t* pNewNtk;
  set<int> badConeRoot;
  for(int i=0;i<Abc_NtkObjNum(pNtk)*0.1;i++){
    Resubsitution(pAbc ,pNewNtk, pNtk,nodeid,false,badConeRoot);
    Abc_Ntk_t* pNtk =Abc_FrameReadNtk(pAbc);
  }
  return 0;
  
}