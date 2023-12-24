#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "booleanChain.h"
#include "sat/glucose2/AbcGlucose2.h"
#include "sat/glucose2/SimpSolver.h"
#include <iostream>
#include <set>
#include "lsvCone.h"
using namespace std;

static int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandNtk2Chain(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainTimeLimit(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandPrintChain(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainReduce(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainBadRootClear(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChain2Ntk(Abc_Frame_t* pAbc, int argc, char** argv);
static int test_Command(Abc_Frame_t* pAbc, int argc, char** argv);

static BooleanChain booleanChain;

void init(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "LSV", "lsv_print_nodes", Lsv_CommandPrintNodes, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_ntk2chain", Lsv_CommandNtk2Chain, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_time_limit", Lsv_CommandChainTimeLimit, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_print_chain", Lsv_CommandPrintChain, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_reduce", Lsv_CommandChainReduce, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_bad_root_clear", Lsv_CommandChainBadRootClear, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain2ntk", Lsv_CommandChain2Ntk, 1);
  Cmd_CommandAdd(pAbc, "LSV", "test", test_Command, 0);
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
        break;
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
    int root = getCone(pNtk, cone, input, 11, 3, booleanChain.badConeRoot);
    if (root == -1) {
      Abc_Print(-2, "Cannot find required cone\n");
      return;
    }
    booleanChain.badConeRoot.insert(root);
    for (int i = 0; i < length; ++i) {
      if (cone[i])
        subCircuit.insert(i);
    }
    delete[] cone;
    delete[] input;
    
    Abc_Obj_t* pObjFanout;
    int i;
    Abc_ObjForEachFanout(Abc_NtkObj(pNtk, root), pObjFanout, i) {
      constraintCut.insert(pObjFanout->Id);
    }
    // TODO: get DC and add DC
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

void Lsv_Chain2Ntk(Abc_Ntk_t* pNtk) {
  Abc_Ntk_t* pNtkNew = booleanChain.Chain2Ntk(pNtk);
  Abc_FrameReplaceCurrentNetwork(Abc_FrameReadGlobalFrame(), pNtkNew);
  // Abc_FrameClearVerifStatus( Abc_FrameReadGlobalFrame() );
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
  getCone(pNtk, cone, input, 7, 3, set<int>());
  for (int i = 0; i < length; ++i){
    if (cone[i])
      Abc_Print(-2, "node %d\n", i);
  }
  return 0;
}
