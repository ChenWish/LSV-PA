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
using namespace Gluco2;

class Gate {
// all members are private, only friend to BooleanChain

friend class BooleanChain;

static int getFaninId(int fanin) { return fanin >> 1; }
static int getFaninComplement(int fanin) { return fanin & 1; }
static void setLogicZero(Lit l) { logicZero = l; }

static Lit logicZero;
static int piNum; // number of Pi var
public:
    Gate() {
        deleted = false;
    }
    ~Gate() {}
private:
    void init();
    void setVar(int& start);
    Lit getIthTT(int ith) const;
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
    BooleanChain() { piNum = 0; poNum = 0; verbose = 0; nTimeLimit = 0;}
    ~BooleanChain() {};

    void setVerbose(int v) { verbose = v; }
    void setNTimeLimit(size_t t) { nTimeLimit = t; }
    size_t getNTimeLimit() const { return nTimeLimit; }
    void addDC(int d) { dc.insert(d); }
    void clearDC() { dc.clear(); }
    bool Ntk2Chain(Abc_Ntk_t* pNtk);
    Abc_Ntk_t*  Chain2Ntk(Abc_Ntk_t* pNtkOld) const;
    void genTT() {updateTT(true); };
    int reduce(int circuitSize, int reduceSize);
    int reduce(const set<int>& originSubCircuit, int reduceSize);
    int reduce(const set<int>& subCircuit, const set<int>& constraint, int reduceSize = -1);
    void print(bool tt = true) const;

    set<int> badConeRoot;

private:
    int genCNF(vector<vector<Lit>>& cnf, const set<int>& constraintCut);
    void addGateCNF(const set<int>&TFO, vector<vector<Lit>>& cnf, int index);
    set<int> smartSelection(int circuitSize) const;
    int selectRoot(set<int>& badRoot) const;
    set<int> selectSubCircuit(int root, int subCircuitSize) const;
    int reduceInt(int nofNode, const vector<vector<Lit>>& cnf, int l, const set<int>& originSubCircuit);
    void replace(Solver& solver, const set<int>& originSubCircuit);
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
    size_t nTimeLimit;
    
    // for FFC & dc
    set<int> dc;
};
#endif
// BOOLEAN_CHAIN_H