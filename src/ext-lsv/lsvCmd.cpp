#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "booleanChain.h"
#include "sat/glucose2/AbcGlucose2.h"
#include "sat/glucose2/SimpSolver.h"
#include <iostream>

using namespace std;

static int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandNtk2Chain(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainTimeLimit(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandPrintChain(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChainReduce(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandChain2Ntk(Abc_Frame_t* pAbc, int argc, char** argv);

static BooleanChain booleanChain;

void init(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "LSV", "lsv_print_nodes", Lsv_CommandPrintNodes, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_ntk2chain", Lsv_CommandNtk2Chain, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_time_limit", Lsv_CommandChainTimeLimit, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_print_chain", Lsv_CommandPrintChain, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain_reduce", Lsv_CommandChainReduce, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_chain2ntk", Lsv_CommandChain2Ntk, 1);
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


void Lsv_ChainReduce(Abc_Ntk_t* pNtk, bool fAll, bool fSmart) {
  set<int> subCircuit;
  set<int> constraintCut;
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
  else {
    if (fSmart) {
      while (booleanChain.reduce(6, -1) <= 0) {} // 5

      // if (booleanChain.reduce(5, 1) < 0)
      //   booleanChain.reduce(5, 0);
      // while (booleanChain.reduce(6, -1) < 0) {
      //   cout << "Reduce ......." << endl;
      // }
      return;
    }
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
  int reduceSize;
  printf("Enter reduce size: ");
  scanf("%d", &reduceSize);
  booleanChain.reduce(subCircuit, constraintCut, reduceSize);
  // printf("exec time: %d\n", Abc_Clock() - t);
}

int Lsv_CommandChainReduce(Abc_Frame_t* pAbc, int argc, char** argv) {
  // bmcg2_sat_solver * s = bmcg2_sat_solver_start() ;


  // sat_solver_setnvars(pSat, 100);
  // int v = sat_solver_addvar(pSat);
  // printf("%d\n", v);
  // return 0;
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int fAll = 0;
  int fSmart = 1;
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "ash")) != EOF) {
    switch (c) {
      case 'a':
        fAll ^= 1;
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
  Lsv_ChainReduce(pNtk, fAll, fSmart);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_chain_reduce [-h]\n");
  Abc_Print(-2, "\t        reduce the boolean chain\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
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
