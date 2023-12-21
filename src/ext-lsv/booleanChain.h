#ifndef BOOLEAN_CHAIN_H
#define BOOLEAN_CHAIN_H

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "sat/glucose2/Solver.h"


#include <vector>
#include <set>

/* for SAT */
#include "sat/cnf/cnf.h"
// extern "C"{
//     Aig_Man_t* Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
// }

using namespace std;

class Gate {
// all members are private, only friend to BooleanChain

friend class BooleanChain;

static int getFaninId(int fanin) { return fanin >> 1; }
static int getFaninComplement(int fanin) { return fanin & 1; }
static void setLogicZero(lit l) { logicZero = l; }

static lit logicZero;
static int piNum; // number of Pi var
public:
    Gate() {
        deleted = false;
    }
    ~Gate() {}
private:
    void init();
    void setVar(int& start);
    lit getIthTT(int ith) const;
    void initTT() { tt.resize(((1<<piNum) - 1)/64 + 1, 0); } // init to zero
    // void addTTConstraint(vector<vector<lit>>& cnf);

    // for circuit network
    int fanin1; // last bit for fanin complement
    int fanin2; // last bit for fanin complement
    set<int> fanouts; // no record complement information, directly use it as index
    bool deleted;
    vector<size_t> tt;

    // for SAT variable
    // int enable;
    vector<int> select1Var; // one hot vector
    vector<int> select2Var; // one hot vector
    int c1Var;
    int c2Var;
    vector<int> var;
    vector<int> ithPo;
};

class BooleanChain {
public:
    BooleanChain() { piNum = 0; poNum = 0; verbose = 0;}
    ~BooleanChain() {};

    void setVerbose(int v) { verbose = v; }
    bool Ntk2Chain(Abc_Ntk_t* pNtk);
    Abc_Ntk_t*  Chain2Ntk(Abc_Ntk_t* pNtkOld) const;
    void genTT() {updateTT(true); };
    int reduce(int circuitSize, int reduceSize);
    int reduce(const set<int>& originSubCircuit, int reduceSize);
    int reduce(const set<int>& subCircuit, const set<int>& constraint, int reduceSize = -1);
    void print(bool tt = true) const;


private:
    int genCNF(vector<vector<lit>>& cnf, const set<int>& constraintCut);
    void addGateCNF(const set<int>&TFO, vector<vector<lit>>& cnf, int index);
    set<int> smartSelection(int circuitSize) const;
    int reduceInt(int nofNode, const vector<vector<lit>>& cnf, int l, const set<int>& originSubCircuit);
    void replace(sat_solver* pSat, const set<int>& originSubCircuit);
    void updateTopological();
    // void swapGate(int p, int q);
    bool checkValid(const set<int>& s, bool canBePo = false) const;
    bool isPo(int index) const { return (piNum < index) && (index <= piNum + poNum); }
    bool updateTT(bool initial = false);
    
    int verbose; // default=0, 1 for intermediate information, 2 for debug
    int piNum; // number of Pi var
    int poNum; // number of Po
    vector<Gate> chain;
    vector<Gate> subCircuit;
    set<int> subCircuitPiIndex;
    set<int> subCircuitPoIndex;
    // vector<int> PoIndex;
};
#endif
// BOOLEAN_CHAIN_H