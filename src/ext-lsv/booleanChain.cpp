#include "booleanChain.h"

#include <algorithm>
#include <queue>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>


int Gate::piNum = 0;
Lit Gate::logicZero = mkLit(0);

// static void oneHot(sat_solver* pSat, lit* begin, lit* end) {
static void oneHot(Solver& s, const vec<Lit>& lits) {
    // sat_solver_addclause(pSat, begin, end);
    // lit clause[2];
    // for (lit* j = begin; j < end; ++j) {
    //     clause[0] = lit_neg(*j);
    //     for (lit* i = begin; i < j; ++i) {
    //         clause[1] = lit_neg(*i);
    //         sat_solver_addclause(pSat, clause, clause+2);
    //     }
    // }
    s.addClause(lits);
    for (int j = 0; j < lits.size(); ++j) {
        for (int i = 0; i < j; ++i) {
            s.addClause(~lits[i], ~lits[j]);
        }
    }
}

void 
Gate::init() {
    fanouts.clear();
    tt.clear();
    select1Var.clear();
    select2Var.clear();
    var.clear();
    ithPo.clear();
}

void
Gate::setVar(int& start) {
    int ttLen = (1 << piNum); 
    var.resize(ttLen);
    for (int i = 0; i < ttLen; ++i)
        var[i] = start++;
}

Lit
Gate::getIthTT(int ith) const {
    assert(ith < (1 << piNum));
    bool isOne = (tt[ith/64] >> ((size_t)63 - (size_t)(ith%64))) & (size_t)1;
    return isOne ? ~logicZero : logicZero;
}


/******************** public function ********************/

bool
BooleanChain::Ntk2Chain(Abc_Ntk_t* pNtk) {
    if (!Abc_NtkIsStrash(pNtk)) {
        printf("Error: ntk should be strashed networks.\n");
        return false;
    }
    if (verbose >= 2)
        cout << "Begin ntk2chain" << endl;


    chain.clear();
    dc.clear();
    Gate::piNum = piNum = Abc_NtkPiNum(pNtk);
    poNum = Abc_NtkPoNum(pNtk);

    // NtkObj in the order of: const_1, PI, PO, Node
    Abc_Obj_t* pObj;
    int i;
    chain.resize(Abc_NtkObjNum(pNtk));
    Abc_NtkForEachObj(pNtk, pObj, i) {
        assert(pObj->Id == i);
        if (i == 0)
            assert(pObj->Type == Abc_ObjType_t::ABC_OBJ_CONST1);
        else if (i <= piNum) 
            assert(Abc_ObjIsPi(pObj));
        else if (i <= piNum + poNum) {
            assert(Abc_ObjIsPo(pObj));
            Abc_Obj_t* f0 = Abc_ObjFanin0(pObj);
            int c0 = Abc_ObjFaninC0(pObj);
            chain[i].fanin1 = ((f0->Id) << 1) | c0;
            chain[f0->Id].fanouts.insert(i);
            chain[i].fanin2 = -1; // if get this index, chain[-1] will out of bound and trigger seg fault
        }
        else {
            assert(Abc_ObjIsNode(pObj));
            Abc_Obj_t* f0 = Abc_ObjFanin0(pObj);
            Abc_Obj_t* f1 = Abc_ObjFanin1(pObj);
            int c0 = Abc_ObjFaninC0(pObj);
            int c1 = Abc_ObjFaninC1(pObj);
            chain[i].fanin1 = ((f0->Id) << 1) | c0;
            chain[i].fanin2 = ((f1->Id) << 1) | c1;
            chain[f0->Id].fanouts.insert(i);
            chain[f1->Id].fanouts.insert(i);
        }
    }
    updateTT(true);
    if (verbose >= 2)
        cout << "End ntk2chain" << endl;
    return true;
}

Abc_Ntk_t*  
BooleanChain::Chain2Ntk(Abc_Ntk_t* pNtkOld) const {


// Abc_ObjDeleteFanin()
// Abc_ObjRemoveFanins
// Abc_NtkDeleteObj( Abc_Obj_t * pObj )

    Abc_Ntk_t* pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    if (verbose >= 2)
        cout << "Begin chain2ntk" << endl;
    pNtkNew->pName = Extra_UtilStrsav( pNtkOld->pName );
    pNtkNew->pSpec = Extra_UtilStrsav( pNtkOld->pSpec );
    pNtkNew->nConstrs = pNtkOld->nConstrs;

    map<int, Abc_Obj_t*> chainId2pObj;

    if (verbose >= 2)
        cout << "Begin create node" << endl;
    Abc_Obj_t* pObj;
    int i;
    for (i = 0; i < chain.size(); ++i) {
        if (chain[i].deleted)
            continue;
        if (verbose >= 2)
            cout << "i=" << i << endl;
        if (i == 0) {
            pObj = Abc_AigConst1(pNtkNew);
        }
        else if (i <= piNum) { // Pi
            pObj = Abc_NtkCreatePi(pNtkNew);    
        }
        else if (i <= piNum + poNum) { // Po
            pObj = Abc_NtkCreatePo(pNtkNew);  
        }
        else { // internal Node
            int fanin1 = Gate::getFaninId(chain[i].fanin1);
            int fanin2 = Gate::getFaninId(chain[i].fanin2);
            int c1 = Gate::getFaninComplement(chain[i].fanin1);
            int c2 = Gate::getFaninComplement(chain[i].fanin2);
            Abc_Obj_t* pNode1 = Abc_ObjNotCond(chainId2pObj[fanin1], c1);
            Abc_Obj_t* pNode2 = Abc_ObjNotCond(chainId2pObj[fanin2], c2);
            pObj = Abc_AigAnd((Abc_Aig_t *)pNtkNew->pManFunc, pNode1, pNode2);
        }
        chainId2pObj[i] = pObj;
    }
    if (verbose >= 2)
        cout << "After create node" << endl;

    if (verbose >= 2)
        cout << "Before add Po fanin" << endl;
    // add Po fanin
    for (i = piNum + 1; i <= piNum + poNum; ++i) {
        assert(isPo(i));
        int fanin1 = Gate::getFaninId(chain[i].fanin1);
        int c1 = Gate::getFaninComplement(chain[i].fanin1);
        Abc_Obj_t* pNode1 = Abc_ObjNotCond(chainId2pObj[fanin1], c1);
        Abc_ObjAddFanin(chainId2pObj[i], pNode1);
    }
    if (verbose >= 2)
        cout << "After add Po fanin" << endl;

    // remove the extra nodes
    Abc_AigCleanup( (Abc_Aig_t *)pNtkNew->pManFunc );

    int Counter = 0;
    // assign the remaining names
    Abc_NtkForEachPi(pNtkNew, pObj, i) {
        Abc_ObjAssignName(pObj, Abc_ObjName(pObj), NULL);
        Counter++;
    }
    Abc_NtkForEachPo(pNtkNew, pObj, i) {
        Abc_ObjAssignName(pObj, Abc_ObjName(pObj), NULL);
        Counter++;
    }

    // if (!Abc_NtkCheckRead(pNtkNew)) {
    //     printf( "Chain2Ntk: The network check has failed.\n" );
    //     Abc_NtkDelete( pNtkNew );
    //     return NULL;
    // }

    if (!Abc_NtkCheck( pNtkNew )) {
        cerr << "NtkCheck faild" << endl;
    }
    return pNtkNew;
    
}

int
BooleanChain::reduce(int circuitSize, int reduceSize) {
    set<int> originSubCircuit = smartSelection(circuitSize);
    return reduce(originSubCircuit, reduceSize);
}


int
BooleanChain::reduce(const set<int>& originSubCircuit, int reduceSize) {
    set<int> constraintCut;
    for (int i = piNum + 1; i <= piNum + poNum; ++i)
        constraintCut.insert(i);
    return reduce(originSubCircuit, constraintCut, reduceSize);
}

int
BooleanChain::reduce(const set<int>& originSubCircuit, const set<int>& constraintCut, int reduceSize) {
    // default reduceSize = -1, which means iterative reduction
    // assert there is no path from Po in subCircuit to Pi in subCircuit in the original circuit
    // Pi/Po dummy node in the original circuit cannot be in the subCircuit

    if (!checkValid(originSubCircuit) || !checkValid(constraintCut, true)) {
        // both cannot contain deleted node
        // subCircuit cannot contain Pi/Po, while constraintCut cannot contain Pi
        cerr << "Error: in reduce(booleanChain.cpp), the given subCircuit is invalid" << endl;
        return -1;
    }

    subCircuitPiIndex.clear();
    subCircuitPoIndex.clear();
    for (set<int>::const_iterator it = originSubCircuit.begin(); it != originSubCircuit.end(); ++it) {
        int fanin1 = Gate::getFaninId(chain[*it].fanin1);
        int fanin2 = Gate::getFaninId(chain[*it].fanin2);
        if (originSubCircuit.find(fanin1) == originSubCircuit.end()) subCircuitPiIndex.insert(fanin1);
        if (originSubCircuit.find(fanin2) == originSubCircuit.end()) subCircuitPiIndex.insert(fanin2);
        for (set<int>::iterator it2 = chain[*it].fanouts.begin(); it2 != chain[*it].fanouts.end(); ++it2) {
            if (originSubCircuit.find(*it2) == originSubCircuit.end()) {
                subCircuitPoIndex.insert(*it);
                break;
            }
        }
    }
    if (verbose >= 1) {
        cout << "subCircuit PI index:";
        for (set<int>::iterator it = subCircuitPiIndex.begin(); it != subCircuitPiIndex.end(); ++it) {
            cout << " " << *it;
        }
        cout << endl;
        cout << "subCircuit PO index:";
        for (set<int>::iterator it = subCircuitPoIndex.begin(); it != subCircuitPoIndex.end(); ++it) {
            cout << " " << *it;
        }
        cout << endl;
    }

    
    // vector<vector<lit>> cnf;
    vector<vector<Lit>> cnf;
    int nofVar = genCNF(cnf, constraintCut); 
    // cerr << "Here" << endl;
    // return 0;
    int result = 0; // glucose undef
    if (reduceSize == -1) {
        for (int l = 1; l <= originSubCircuit.size(); ++l) { // modify < to <=
            cout << "subCircuit with size " << l << endl;
            result = reduceInt(nofVar, cnf, l, originSubCircuit);
            // l_Undef = 0, l_True = 1, l_False = -1
            if (result == 1) { // how about l_Undef ?
                break;
            }
        }
    }
    else {
        int l = max(int(originSubCircuit.size()) - reduceSize, 1);
        cout << "subCircuit with size " << l << endl;
        result = reduceInt(nofVar, cnf, l, originSubCircuit);
    }

    if (result == 1) {
        printf("Success reduce subcircuit from %ld to %ld\n", originSubCircuit.size(), subCircuit.size());
        // return originSubCircuit.size() - subCircuit.size();
        return 1;
    }
    else if (result == -1) {
        return -1;
    }
    else { // result == l_Undef
        if (verbose >= 1) {
            cout << "In reduce, the result is l_Undef" << endl;
        }
        return 0;
    }
}

void
BooleanChain::print(bool tt) const {
    // default tt = true
    for (int i = 0; i < chain.size(); ++i) {
        if (chain[i].deleted) {
            continue;
        }
        cout << "Id = " << setw(chain.size()/10 + 2) << i << ", ";
        stringstream ss;
        string name;
        if (i == 0)
            ss << "const_1";
        else if (i <= piNum) 
            ss << "Pi_" + to_string(i);
        else if (i <= piNum + poNum) {
            int fanin1 = Gate::getFaninId(chain[i].fanin1);
            int c1 = Gate::getFaninComplement(chain[i].fanin1);
            ss << "Po_" + to_string(i-piNum) << "(" << setw(chain.size()/10 + 1) << (c1 ? "~" : " ") << fanin1 << ")";
        }
        else {
            int fanin1 = Gate::getFaninId(chain[i].fanin1);
            int fanin2 = Gate::getFaninId(chain[i].fanin2);
            int c1 = Gate::getFaninComplement(chain[i].fanin1);
            int c2 = Gate::getFaninComplement(chain[i].fanin2);
            string s1 = (c1 ? "~" : " ") + to_string(fanin1);
            string s2 = (c2 ? "~" : " ") + to_string(fanin2);
            ss << " " << setw(chain.size()/10 + 2) << s1 << " & " << setw(chain.size()/10 + 2) << s2;
        }
        cout << setw(3 + max(7, int(2 * chain.size()/10 + 6))) << ss.str() << "  ";
        if (tt) {
            for (int j = 0; j < (1 << piNum); ++j) {
                // cout << (chain[i].getIthTT(j));
                cout << toInt(chain[i].getIthTT(j));
            }
        }
        cout << endl;
    }
}

/******************** private function ********************/

int
BooleanChain::genCNF(vector<vector<Lit>>& cnf, const set<int>& constraintCut) {
    // add cnf var in reverse topological order
    cnf.clear();
    set<int> TFO;
    set<int> careCut;
    queue<int> q;
    for (set<int>::const_iterator it = subCircuitPoIndex.begin(); it != subCircuitPoIndex.end(); ++it) {
        // if (constraintCut.find(*it) == constraintCut.end())
            q.push(*it);
    }
    while (!q.empty()) {
        int index = q.front();
        q.pop();
        if (constraintCut.find(index) != constraintCut.end()) {
            careCut.insert(index);
            if (verbose >= 2)
                cout << "careCut: " << index << endl;
        }
        else { // index is not in the constraintCut
            TFO.insert(index);
            if (verbose >= 2)
                cout << "TFO: " << index << endl;
            for (set<int>::iterator it = chain[index].fanouts.begin(); it != chain[index].fanouts.end(); ++it) {
                if (TFO.find(*it) == TFO.end())
                    q.push(*it);
            }
        }
    }
    if (verbose >= 2) {
        cout << "TFO:";
        for (set<int>::iterator it = TFO.begin(); it != TFO.end(); ++it) {
            cout << " " << *it;
        }
        cout << endl;

        cout << "careCut:";
        for (set<int>::iterator it = careCut.begin(); it != careCut.end(); ++it) {
            cout << " " << *it;
        }
        cout << endl;
    }

    // sat var start from 0
    // reverse topological order
    int nowVar = 0;
    cnf.push_back(vector<Lit>{mkLit(nowVar++, 1)}); // first var is logic zero
    // Gate::setLogicZero(toLit(0));
    Gate::setLogicZero(mkLit(0));
    // for (set<int>::const_reverse_iterator rit = careCut.rbegin(); rit != careCut.rend(); ++rit) {
    //     chain[*rit].setVar(nowVar); // BUGGY
    // }
    for (set<int>::const_reverse_iterator rit = TFO.rbegin(); rit != TFO.rend(); ++rit) {
        chain[*rit].setVar(nowVar);
    }
    for (set<int>::const_reverse_iterator rit = careCut.rbegin(); rit != careCut.rend(); ++rit) {
        addGateCNF(TFO, cnf, *rit);
    }
    for (set<int>::const_reverse_iterator rit = TFO.rbegin(); rit != TFO.rend(); ++rit) {
        addGateCNF(TFO, cnf, *rit);
    }
    return nowVar;
}

set<int>
BooleanChain::smartSelection(int circuitSize) const {
    set<int> badRoot;
    while (1) {
        int root = selectRoot(badRoot);
        set<int> subCircuit = selectSubCircuit(root, circuitSize);
        int subCircuitPoNum = 0;
        for (set<int>::iterator it = subCircuit.begin(); it != subCircuit.end(); ++it) {
            for (set<int>::iterator it2 = chain[*it].fanouts.begin(); it2 != chain[*it].fanouts.end(); ++it2) {
                if (subCircuit.find(*it2) == subCircuit.end()) {
                    subCircuitPoNum++;
                    break;
                }
            }
        }
        if (subCircuitPoNum >= subCircuit.size()-1) {
            badRoot.insert(root);
            continue;
        }
        if (subCircuit.size() < 4) {
            badRoot.insert(root);
            continue;
        }
        if (verbose >= 1) {
            cout << "SmartSelection with subCircuit:";
            for (set<int>::iterator it = subCircuit.begin(); it != subCircuit.end(); ++it)
                cout << " " << *it;
            cout << ", with subCircuitPoNum = " << subCircuitPoNum << endl;
        }
        return subCircuit;
    }
    assert(0);
    return set<int>();
}

int 
BooleanChain::selectRoot(set<int>& badRoot) const {
    srand(time(NULL));
    int root = 0;
    while (chain[root].deleted || root <= piNum + poNum || badRoot.find(root) != badRoot.end()) {
        root = rand() % chain.size();
    }
    if (verbose >= 1)
        cout << "SmartSelection: Root = " << root << endl;
    return root;
}

set<int>
BooleanChain::selectSubCircuit(int root, int subCircuitSize) const {
    set<int> subCircuit;
    subCircuit.insert(root);
    while (1) {
        int nextIndex = -1;
        int nextAddInputNum = INT32_MAX;
        int nextAddOutputNum = INT32_MAX;
        for (set<int>::iterator it = subCircuit.begin(); it != subCircuit.end(); ++it) {
            int fanin1 = Gate::getFaninId(chain[*it].fanin1);
            int fanin2 = Gate::getFaninId(chain[*it].fanin2);
            
            int addInputNum;
            int addOutputNum;
            auto evalNode = [&] (int nodeIndex) {
                addInputNum = 0;
                addOutputNum = 0;
                for (set<int>::iterator it = chain[nodeIndex].fanouts.begin(); it != chain[nodeIndex].fanouts.end(); ++it) {
                    if (subCircuit.find(*it) == subCircuit.end())
                        addOutputNum ++;
                }
                if (subCircuit.find(Gate::getFaninId(chain[nodeIndex].fanin1)) == subCircuit.end())
                    addInputNum ++;
                if (subCircuit.find(Gate::getFaninId(chain[nodeIndex].fanin2)) == subCircuit.end())
                    addInputNum ++;
            };

            if (fanin1 > piNum + poNum && subCircuit.find(fanin1) == subCircuit.end()) {
                evalNode(fanin1);
                if (addOutputNum < nextAddOutputNum || (addOutputNum == nextAddOutputNum && addInputNum < nextAddInputNum)) {
                    nextIndex = fanin1;
                    nextAddInputNum = addInputNum;
                    nextAddOutputNum = addOutputNum;
                }
            }
            if (fanin2 > piNum + poNum && subCircuit.find(fanin2) == subCircuit.end()) {
                evalNode(fanin2);
                if (addOutputNum < nextAddOutputNum || (addOutputNum == nextAddOutputNum && addInputNum < nextAddInputNum)) {
                    nextIndex = fanin2;
                    nextAddInputNum = addInputNum;
                    nextAddOutputNum = addOutputNum;
                }
            }
        }
        // assert(nextIndex != -1);
        if (nextIndex == -1)
            break;
        subCircuit.insert(nextIndex);
        if (subCircuit.size() > subCircuitSize) {
            break;
        }
    }
    return subCircuit;
}


int 
BooleanChain::reduceInt(int nofVar, const vector<vector<Lit>>& cnf, int len, const set<int>& originSubCircuit) {
    // sat_solver_setnvars to init nVar
    // can benefit from incremental SAT?
    size_t startTime = Abc_Clock();
    // sat_solver* pSat = sat_solver_new();
    Solver solver;
    solver.setIncrementalMode();



    // add cnf clause into solver
    // sat_solver_setnvars(pSat, nofVar);
    solver.addVar(nofVar-1); // -1 or not
    for (int i = 0; i < cnf.size(); ++i) {
        // lit* clause = new lit[cnf[i].size()];
        vec<Lit> clause(cnf[i].size());
        for (int j = 0; j < cnf[i].size(); ++j) {
            clause[j] = cnf[i][j];
        }
        // sat_solver_addclause(pSat, clause, clause + cnf[i].size());
        solver.addClause(clause);
        // delete[] clause;  // WISH
    }
    

    // (¬A ∨ ¬B ∨ P) ∧ (¬P ∨ A) ∧ (¬P ∨ B)
    // i < j
    // (s1_i & s2_j & ~c1 & ~c2) -> (~var_i + ~var_j + var)(~var + var_i)(~var + var_j)
    // (s1_i & s2_j & ~c1 &  c2) ->
    // (s1_i & s2_j &  c1 & ~c2) ->
    // (s1_i & s2_j &  c1 &  c2) ->

    // TODO: check complment of lit
    // TODO: s1 < s2, break symmetric
    // WISH: need to block trivial projection
    subCircuit.resize(len);
    for (int l = 0; l < len; ++l) {
        Gate& gate = subCircuit[l];
        gate.init();
        gate.setVar(nofVar);
        // sat_solver_setnvars(pSat, nofVar);
        // gate.c1Var = sat_solver_addvar(pSat);
        // gate.c2Var = sat_solver_addvar(pSat);
        solver.addVar(nofVar-1);
        gate.c1Var = solver.newVar();
        gate.c2Var = solver.newVar();
        nofVar += 2;

        // init select var and add one-hot constraint
        // lit* oneHotS1 = new lit[subCircuitPiIndex.size() + l];
        // lit* oneHotS2 = new lit[subCircuitPiIndex.size() + l];
        vec<Lit> oneHotS1(subCircuitPiIndex.size() + l);
        vec<Lit> oneHotS2(subCircuitPiIndex.size() + l);
        for (int i = 0; i < subCircuitPiIndex.size() + l; ++i) {
            // gate.select1Var.push_back(sat_solver_addvar(pSat));
            // gate.select2Var.push_back(sat_solver_addvar(pSat));
            // oneHotS1[i] = toLit(gate.select1Var.back());
            // oneHotS2[i] = toLit(gate.select2Var.back());
            gate.select1Var.push_back(solver.newVar());
            gate.select2Var.push_back(solver.newVar());
            oneHotS1[i] = mkLit(gate.select1Var.back());
            oneHotS2[i] = mkLit(gate.select2Var.back());
            nofVar += 2;
        }
        // oneHot(pSat, oneHotS1, oneHotS1 + subCircuitPiIndex.size() + l);
        // oneHot(pSat, oneHotS2, oneHotS2 + subCircuitPiIndex.size() + l);
        // WISH
        // delete[] oneHotS1;
        // delete[] oneHotS2;

        oneHot(solver, oneHotS1);
        oneHot(solver, oneHotS2);

        for (int j = 0; j < subCircuitPiIndex.size() + l; ++j) {
            for (int i = 0; i < subCircuitPiIndex.size() + l; ++i) {
                if (i >= j) {
                    // forbid s1 >= s2 cases
                    // s1[i] -> ~s2[j]
                    // cout << "buggy, i=" << i << ", j=" << j << "s1Size=" << gate.select1Var.size() << " s2Size=" << gate.select2Var.size() << endl;
                    // lit clause[2] = {toLitCond(gate.select1Var[i], 1), toLitCond(gate.select2Var[j], 1)};
                    // sat_solver_addclause(pSat, clause, clause+2);
                    solver.addClause(~mkLit(gate.select1Var[i]), ~mkLit(gate.select2Var[j]));
                    continue;
                }
                bool c1[4] = {false, false, true, true};
                bool c2[4] = {false, true, false, true};
/*
                const lit s1 = toLit(gate.select1Var[i]);
                const lit s2 = toLit(gate.select2Var[j]);
                for (int k = 0; k < (1 << piNum); ++k) {
                    const lit gateLit = toLit(gate.var[k]);
                    for (int m = 0; m < 4; ++m) {
                        const lit c1Lit = toLitCond(gate.c1Var, c1[m]);
                        const lit c2Lit = toLitCond(gate.c2Var, c2[m]);
                        const set<int>::const_iterator it = subCircuitPiIndex.begin();
                        lit temp = (i < subCircuitPiIndex.size()) ? (chain[*next(it, i)].getIthTT(k)) : toLit(subCircuit[i - subCircuitPiIndex.size()].var[k]); // modify for Pi, which is const
                        const lit iLit = c1[m] ? lit_neg(temp) : temp;
                            temp = (j < subCircuitPiIndex.size()) ? (chain[*next(it, j)].getIthTT(k)) : toLit(subCircuit[j - subCircuitPiIndex.size()].var[k]); // modify for Pi, which is const
                        const lit jLit = c2[m] ? lit_neg(temp) : temp;
                        lit clause[7];
                        clause[0] = lit_neg(s1);
                        clause[1] = lit_neg(s2);
                        clause[2] = c1Lit;
                        clause[3] = c2Lit;

                        // (~var_i + ~var_j + var)
                        clause[4] = lit_neg(iLit);
                        clause[5] = lit_neg(jLit);
                        clause[6] = gateLit;
                        sat_solver_addclause(pSat, clause, clause+7);

                        // (~var + var_i)
                        clause[4] = lit_neg(gateLit);
                        clause[5] = iLit;
                        sat_solver_addclause(pSat, clause, clause+6);

                        // (~var + var_j)
                        clause[4] = lit_neg(gateLit);
                        clause[5] = jLit;
                        sat_solver_addclause(pSat, clause, clause+6);
                    }
                }
*/
                const Lit s1 = mkLit(gate.select1Var[i]);
                const Lit s2 = mkLit(gate.select2Var[j]);
                for (int k = 0; k < (1 << piNum); ++k) {
                    // modify for dc
                    if (dc.find(k) != dc.end())
                        continue;
                    const Lit gateLit = mkLit(gate.var[k]);
                    for (int m = 0; m < 4; ++m) {
                        const Lit c1Lit = mkLit(gate.c1Var, c1[m]);
                        const Lit c2Lit = mkLit(gate.c2Var, c2[m]);
                        const set<int>::const_iterator it = subCircuitPiIndex.begin();
                        Lit temp = (i < subCircuitPiIndex.size()) ? (chain[*next(it, i)].getIthTT(k)) : mkLit(subCircuit[i - subCircuitPiIndex.size()].var[k]); // modify for Pi, which is const
                        const Lit iLit = c1[m] ? ~temp : temp;
                            temp = (j < subCircuitPiIndex.size()) ? (chain[*next(it, j)].getIthTT(k)) : mkLit(subCircuit[j - subCircuitPiIndex.size()].var[k]); // modify for Pi, which is const
                        const Lit jLit = c2[m] ? ~temp : temp;
                        vec<Lit> clause(7);
                        clause[0] = ~s1;
                        clause[1] = ~s2;
                        clause[2] = c1Lit;
                        clause[3] = c2Lit;

                        // (~var_i + ~var_j + var)
                        clause[4] = ~iLit;
                        clause[5] = ~jLit;
                        clause[6] = gateLit;
                        solver.addClause(clause);

                        clause.pop(); // resize to 6
                        // (~var + var_i)
                        clause[4] = ~gateLit;
                        clause[5] = iLit;
                        solver.addClause(clause);
                        

                        // (~var + var_j)
                        clause[4] = ~gateLit;
                        clause[5] = jLit;
                        solver.addClause(clause);
                    }
                }
            }
        }
    }
    // subCircuit Po
    // lit* clause = new lit[len];
    vec<Lit> clause(len);
    for (set<int>::iterator it = subCircuitPoIndex.begin(); it != subCircuitPoIndex.end(); ++it) {
        // cout << "*it: " << *it << endl;
        for (int l = 0; l < len; ++l) {
            // int var = sat_solver_addvar(pSat);
            int var = solver.newVar();
            subCircuit[l].ithPo.push_back(var);
            // clause[l] = toLit(var);
            clause[l] = mkLit(var);
            for (int j = 0; j < (1 << piNum); ++j) {
                // modify for dc
                if (dc.find(j) != dc.end())
                    continue;
                // sat_solver_add_buffer_enable(pSat, subCircuit[l].var[j], chain[*it].var[j], var, 0);
                // var -> (a == b) == ~var + (a + ~b)(~a + b) == (~var + a + ~b)(~var + ~a + b)
                solver.addClause(~mkLit(var), mkLit(subCircuit[l].var[j]), ~mkLit(chain[*it].var[j])); // ~var + a + ~b
                solver.addClause(~mkLit(var), ~mkLit(subCircuit[l].var[j]), mkLit(chain[*it].var[j])); // ~var + ~a + b
            }
        }
        // oneHot(pSat, clause, clause+len);
        oneHot(solver, clause);
    }
    // delete[] clause; // WISH

    // acyclic constraint
    map<int, set<int>> outPath2Pi; // Pi, Po are saved in ith index in subCircuitPi/PoIndex
    for (int i = 0; i < subCircuitPiIndex.size(); ++i) {
        set<int> visited;
        queue<int> q;
        q.push(*next(subCircuitPiIndex.begin(), i));
        while (!q.empty()) {
            int index = q.front();
            q.pop();
            if (index <= piNum)
                continue;
            if (visited.find(index) != visited.end())
                continue;
            visited.insert(index);
            for (int j = 0; j < subCircuitPoIndex.size(); ++j) {
                if (*next(subCircuitPoIndex.begin(), j) == index) {
                    outPath2Pi[i].insert(j);
                    break;
                }
            }
            int fanin1 = Gate::getFaninId(chain[index].fanin1);
            int fanin2 = Gate::getFaninId(chain[index].fanin2);
            q.push(fanin1);
            q.push(fanin2);
        }
    }
    if (!outPath2Pi.empty()) {
        // cerr << "not empty" << endl;
        vector<map<int, int>> connect2Pi(subCircuit.size());
        for (int l = 0; l < connect2Pi.size(); ++l) {
            Gate& gate = subCircuit[l];
            for (map<int, set<int>>::iterator it = outPath2Pi.begin(); it != outPath2Pi.end(); ++it) {
                const int ithPi = it->first;
                // connect2Pi[l][ithPi] = sat_solver_addvar(pSat);
                connect2Pi[l][ithPi] = solver.newVar();
                // lit clause[3];
                // clause[0] = toLitCond(gate.select1Var[ithPi], 1); // ~s1
                // clause[1] = toLit(connect2Pi[l][ithPi]); // c
                // sat_solver_addclause(pSat, clause, clause + 2);
                // clause[0] = toLitCond(gate.select2Var[ithPi], 1); // ~s2
                // sat_solver_addclause(pSat, clause, clause + 2);
                solver.addClause(~mkLit(gate.select1Var[ithPi]), mkLit(connect2Pi[l][ithPi])); // ~s1 + c
                solver.addClause(~mkLit(gate.select2Var[ithPi]), mkLit(connect2Pi[l][ithPi])); // ~s2 + c


                // (s_i & c_i) -> c_l == ~s_i + ~c_i + c_l, for all i < l
                for (int subCircuitIndex = 0; subCircuitIndex < l; ++subCircuitIndex) {
                    // clause[0] = toLitCond(gate.select1Var[subCircuitIndex + subCircuitPiIndex.size()], 1); // ~si
                    // clause[1] = toLitCond(connect2Pi[subCircuitIndex][ithPi], 1); // ~ci
                    // clause[2] = toLit(connect2Pi[l][ithPi]); // c_l
                    // sat_solver_addclause(pSat, clause, clause + 3);
                    solver.addClause(~mkLit(gate.select1Var[subCircuitIndex + subCircuitPiIndex.size()]), ~mkLit(connect2Pi[subCircuitIndex][ithPi]), mkLit(connect2Pi[l][ithPi])); // ~s1 + ~c_i + c_l

                    // clause[0] = toLitCond(gate.select2Var[subCircuitIndex + subCircuitPiIndex.size()], 1); // ~si
                    // sat_solver_addclause(pSat, clause, clause + 3);
                    solver.addClause(~mkLit(gate.select2Var[subCircuitIndex + subCircuitPiIndex.size()]), ~mkLit(connect2Pi[subCircuitIndex][ithPi]), mkLit(connect2Pi[l][ithPi])); // ~s2 + ~c_i + c_l
                }

                // Po constraint
                // c_l -> ~ithPo == ~c_l + ~ithPo
                for (set<int>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                    // lit clause[2] = {toLitCond(connect2Pi[l][ithPi], 1), toLitCond(gate.ithPo[*it2], 1)};
                    // sat_solver_addclause(pSat, clause, clause + 2);
                    solver.addClause(~mkLit(connect2Pi[l][ithPi]), ~mkLit(gate.ithPo[*it2])); // ~c_l + ~ithPo
                }
            }
        }
    }

    // lbool result = sat_solver_solve(pSat, 0, 0, 0, 0, 0, 0);
    // solver.nRuntimeLimit = xxx; set the runTimeLimit
    if (nTimeLimit != 0)
        solver.nRuntimeLimit = Abc_Clock() + nTimeLimit * CLOCKS_PER_SEC;
    // bool result = solver.solve();
    // solver.solveLimited();
    // solver.nRuntimeLimit = 1000000;
    Gluco2::lbool lboolResult = solver.solveLimited(vec<Lit>()); // no assumption
    int result = (lboolResult == l_True) ? 1 : (lboolResult == l_False ? -1 : 0);
    Abc_PrintTime( 1, "Time", Abc_Clock() - startTime );

    // cerr << "after solve" << endl;
    if (result == 1) {
/*
        if (verbose >= 2) {
            for (set<int>::iterator it = subCircuitPiIndex.begin(); it != subCircuitPiIndex.end(); ++it) {
                cout << "----------- subCircuitPiIndex " << *it << endl;
                // cout << "tt: ";
                // for (int i = 0; i < (1 << piNum); ++i) {
                //     cout << chain[*it].getIthTT(i);
                // }
                // cout << endl;
            }

            for (int l = 0; l < subCircuit.size(); ++l) {
                cout << "----------- subCircuit " << l << endl;
                cout << "c1: " << (sat_solver_var_value(pSat, subCircuit[l].c1Var) == 1) << endl;
                cout << "c2: " << (sat_solver_var_value(pSat, subCircuit[l].c2Var) == 1) << endl;
                cout << "s1: ";
                for (int i = 0; i < subCircuit[l].select1Var.size(); ++i) {
                    cout << (sat_solver_var_value(pSat, subCircuit[l].select1Var[i]) == 1);
                }
                cout << endl;
                cout << "s2: ";
                for (int i = 0; i < subCircuit[l].select2Var.size(); ++i) {
                    cout << (sat_solver_var_value(pSat, subCircuit[l].select2Var[i]) == 1);
                }
                cout << endl;
                cout << "ithPo: ";
                for (int i = 0; i < subCircuit[l].ithPo.size(); ++i) {
                    cout << (sat_solver_var_value(pSat, subCircuit[l].ithPo[i]) == 1);
                }
                cout << endl;
                // cout << "tt: ";
                // for (int i = 0; i < subCircuit[l].var.size(); ++i) {
                //     cout << (sat_solver_var_value(pSat, subCircuit[l].var[i]) == l_True);
                // }
                // cout << endl;
            }
            for (set<int>::iterator it = subCircuitPoIndex.begin(); it != subCircuitPoIndex.end(); ++it) {
                cout << *it << " tt: ";
                // for (int i = 0; i < chain[*it].var.size(); ++i) {
                //     cout << (sat_solver_var_value(pSat, chain[*it].var[i]) == l_True);
                // }
                cout << endl;
            }

            // DEBUG
            // cout << "Enter TFO index: ";
            // set<int> TFO;
            // int temp;
            // while (cin >> temp) {
            //     if (temp == -1)
            //         break;
            //     TFO.insert(temp);
            // }
            // for (set<int>::iterator it = TFO.begin(); it != TFO.end(); ++it) {
            //     cout << "TFO: " << *it << ": ";
            //     for (int i = 0; i < chain[*it].var.size(); ++i) {
            //         cout << (sat_solver_var_value(pSat, chain[*it].var[i]) == l_True);
            //     }
            //     cout << endl;
            // }
            // return result;
        }
*/ 
        replace(solver, originSubCircuit);
        if (verbose >= 1) {
            cout << "--------------- print boolean chain ---------------" << endl;
            print(false);
        }
    }
    // sat_solver_delete(pSat);
    // cout << "Exec Time: " << (Abc_Clock() - startTime)/1000000 << " sec" << endl;
    return result;
    // l_Undef = 0, l_True = 1, l_False = -1
}

void
BooleanChain::addGateCNF(const set<int>&TFO, vector<vector<Lit>>& cnf, int index) {
    int fanin1 = Gate::getFaninId(chain[index].fanin1);
    int fanin2 = Gate::getFaninId(chain[index].fanin2);
    bool c1 = Gate::getFaninComplement(chain[index].fanin1);
    bool c2 = Gate::getFaninComplement(chain[index].fanin2);

    // node not in TFO is a constant node
    bool isConst0 = (TFO.find(index) == TFO.end());
    bool isConst1 = (TFO.find(fanin1) == TFO.end());
    bool isConst2 = (TFO.find(fanin2) == TFO.end());

    if (isConst1 && isConst2) { // both fanins are not in TFO -> chain[index] is the Po of subCircuit
        // do something
        // maybe do nothing because the Po in subCircuit is added in reduceInt
        if (verbose >= 2)
            cout << "id: " << index << " is the Po in subCircuit" << endl;
        return;
    }

    if (verbose >= 2) {
        cout << "-------- isConst0: " << isConst0 << ", index: " << index << endl;
        cout << " isConst1: " << isConst1 << ", fanin1: " << fanin1 << endl;
        cout << " isConst2: " << isConst2 << ", fanin2: " << fanin2 << endl;
        cout << "isPo: " << isPo(index) << endl;
    }

    for (int i = 0; i < (1 << piNum); ++i) {
        // modify for dc
        if (dc.find(i) != dc.end())
            continue;
/*
        // Po has only fanin1
        lit l0 = (isConst0 ? chain[index].getIthTT(i)  : toLit(chain[index].var[i]));
        lit l1 = isConst1 ? chain[fanin1].getIthTT(i) : toLit(chain[fanin1].var[i]);
        lit l2 = isPo(index) ? lit_neg(Gate::logicZero) : (isConst2 ? chain[fanin2].getIthTT(i) : toLit(chain[fanin2].var[i]));
        l1 = c1 ? lit_neg(l1) : l1;
        l2 = (!isPo(index) && c2) ? lit_neg(l2) : l2;

        // (p == a ^ b) =  (~p v a)(~p v b)(~a v ~b v p)
        cnf.push_back(vector<lit>{lit_neg(l0), l1});                // (~p v a)
        cnf.push_back(vector<lit>{lit_neg(l0), l2});                // (~p v b)
        cnf.push_back(vector<lit>{lit_neg(l1), lit_neg(l2), l0});   // (~a v ~b v p)
*/
        Lit l0 = (isConst0 ? chain[index].getIthTT(i) : mkLit(chain[index].var[i]));
        Lit l1 = isConst1 ? chain[fanin1].getIthTT(i) : mkLit(chain[fanin1].var[i]);
        Lit l2 = isPo(index) ? ~(Gate::logicZero) : (isConst2 ? chain[fanin2].getIthTT(i) : mkLit(chain[fanin2].var[i]));

        l1 = c1 ? ~l1 : l1;
        l2 = (!isPo(index) && c2) ? ~l2 : l2;

        // (p == a ^ b) =  (~p v a)(~p v b)(~a v ~b v p)
        cnf.push_back(vector<Lit>{~l0, l1});        // (~p v a)
        cnf.push_back(vector<Lit>{~l0, l2});        // (~p v b)
        cnf.push_back(vector<Lit>{~l1, ~l2, l0});   // (~a v ~b v p)
    }
}


void
BooleanChain::replace(Solver& s, const set<int>& originSubCircuit) {
    assert(originSubCircuit.size() >= subCircuit.size()); // could be equal
/*
    int reducedSize = originSubCircuit.size() - subCircuit.size();
    if (verbose >= 2)
        cerr << "reducedSize = " << reducedSize << endl;
    for (int i = 0; i < reducedSize; ++i) {
        chain[*prev(originSubCircuit.end(), i+1)].deleted = true;
        if (verbose >= 2)
            cerr << "chain[" << *prev(originSubCircuit.end(), i+1) << "].deleted = true" << endl;
    }
*/

    // remove gate fanouts to subCircuit in subCircuitPi
    for (set<int>::iterator it = subCircuitPiIndex.begin(); it != subCircuitPiIndex.end(); ++it) {
        Gate& gate = chain[*it];
        for (set<int>::iterator it2 = gate.fanouts.begin(); it2 != gate.fanouts.end(); ) {
            if (originSubCircuit.find(*it2) != originSubCircuit.end())
                gate.fanouts.erase(it2++);
            else
                ++it2;
        }
    }
    //
    int fanin1, fanin2;
    map<int, int> oldFanin2newFanin;
    for (int i = 0; i < subCircuitPiIndex.size(); ++i)
        oldFanin2newFanin[i] = *next(subCircuitPiIndex.begin(), i);

    auto getFaninId = [&](const Gate& gate) {
        fanin1 = -1;
        fanin2 = -1;
        for (int i = 0; i < gate.select1Var.size(); ++i) {
            if (s.modelValue(gate.select1Var[i]) == l_True) {
                fanin1 = i;
                break;
            }
        }
        for (int i = 0; i < gate.select2Var.size(); ++i) {
            if (s.modelValue(gate.select2Var[i]) == l_True) {
                fanin2 = i;
                break;
            }
        }
        assert(oldFanin2newFanin.find(fanin1) != oldFanin2newFanin.end());
        assert(oldFanin2newFanin.find(fanin2) != oldFanin2newFanin.end());
        fanin1 = oldFanin2newFanin[fanin1];
        fanin2 = oldFanin2newFanin[fanin2];
    };

    for (int l = 0; l < subCircuit.size(); ++l) {
        int index = -1;
        for (int i = 0; i < subCircuitPoIndex.size(); ++i) {
            if (s.modelValue(subCircuit[l].ithPo[i]) == l_True) {
                index = *next(subCircuitPoIndex.begin(), i);
                break;
            }
        }
        if (index == -1) { // not subCircuitPo
            index = chain.size();
            chain.push_back(Gate());
            chain.back().initTT();
        }
        getFaninId(subCircuit[l]);
        int c1 = s.modelValue(subCircuit[l].c1Var) == l_True;
        int c2 = s.modelValue(subCircuit[l].c2Var) == l_True;
        chain[index].fanin1 = (fanin1 << 1) | c1;
        chain[index].fanin2 = (fanin2 << 1) | c2;
        chain[fanin1].fanouts.insert(index);
        chain[fanin2].fanouts.insert(index);
        oldFanin2newFanin[subCircuitPiIndex.size() + l] = index;
    }
    for (set<int>::const_iterator it = originSubCircuit.begin(); it != originSubCircuit.end(); ++it) {
        if (subCircuitPoIndex.find(*it) == subCircuitPoIndex.end())
            chain[*it].deleted = true;
    }
    if (verbose >= 2)
        cout << "Before updateTopological" << endl;
    updateTopological();
    if (!updateTT()) {
        cerr << "Error in replace(booleanChain.cpp): replaced circuit violated Po tt" << endl;
    }
    return;

    map<int, int> originPo2newPo;
    for (int l = 0; l < subCircuit.size(); ++l) {
        // Gate* gatePtr = &chain[*next(originSubCircuit.begin(), l)];
        int nowGateIndex = *next(originSubCircuit.begin(), l);
        for (int i = 0; i < subCircuitPoIndex.size(); ++i) {
            // if (sat_solver_var_value(pSat, subCircuit[l].ithPo[i]) == 1) {
            if (s.modelValue(subCircuit[l].ithPo[i]) == l_Undef) {
                cerr << "/%/%//%//%/%/%/%/%/%/%/%/%/%/%/%/%/%/%/% HAHAHA, there are undefine in the final model" << endl;
            }
            if (s.modelValue(subCircuit[l].ithPo[i]) == l_True) {
                // newSubCircuitPoIndex[i] = nowGateIndex; // nowGateIndex is ith origin Po
                originPo2newPo[*next(subCircuitPoIndex.begin(), i)] = nowGateIndex;
                if (verbose >= 2)
                    cout << "originPo2newPo[" << *next(subCircuitPoIndex.begin(), i) << "] = " << nowGateIndex << endl;
                // chain[nowGateIndex].deleted = true;
                // nowGateIndex = *next(subCircuitPoIndex.begin(), i);
                break;
            }
        }

        // int c1 = sat_solver_var_value(pSat, subCircuit[l].c1Var);
        // int c2 = sat_solver_var_value(pSat, subCircuit[l].c2Var);
        int c1 = s.modelValue(subCircuit[l].c1Var) == l_True;
        int c2 = s.modelValue(subCircuit[l].c2Var) == l_True;
        int fanin1 = -1, fanin2 = -1;
        for (int i = 0; i < subCircuit[l].select1Var.size(); ++i) {
            // if (sat_solver_var_value(pSat, subCircuit[l].select1Var[i]) == 1) {
            if (s.modelValue(subCircuit[l].select1Var[i]) == l_True) {
                fanin1 = i;
                break;
            }
        }
        for (int i = 0; i < subCircuit[l].select2Var.size(); ++i) {
            // if (sat_solver_var_value(pSat, subCircuit[l].select2Var[i]) == 1) {
            if (s.modelValue(subCircuit[l].select2Var[i]) == l_True) {
                fanin2 = i;
                break;
            }
        }
        assert(fanin1 != -1); assert(fanin2 != -1);
        fanin1 = (fanin1 < subCircuitPiIndex.size()) ? *next(subCircuitPiIndex.begin(), fanin1) : *next(originSubCircuit.begin(), fanin1 - subCircuitPiIndex.size());
        fanin2 = (fanin2 < subCircuitPiIndex.size()) ? *next(subCircuitPiIndex.begin(), fanin2) : *next(originSubCircuit.begin(), fanin2 - subCircuitPiIndex.size());
        // gatePtr->fanin1 = (fanin1 << 1) | c1;
        // gatePtr->fanin2 = (fanin2 << 1) | c2;
        chain[nowGateIndex].fanin1 = (fanin1 << 1) | c1;
        chain[nowGateIndex].fanin2 = (fanin2 << 1) | c2;
        chain[nowGateIndex].fanouts.clear();
        chain[fanin1].fanouts.insert(nowGateIndex);
        chain[fanin2].fanouts.insert(nowGateIndex);


        // not update tt, after replacement, update tt for whole circuit at once
        // chain[nowGateIndex].initTT();
        // for (int i = 0; i < (1<<n); ++i) {
        //     if (sat_solver_var_value(pSat, subCircuit[l].var[i]) != l_True)
        //         continue;
        //     // gatePtr->tt[i/64] |= (1 << (63 - i%64));
        //     chain[nowGateIndex].tt[i/64] |= (1 << (63 - i%64));
        // }
    }

    // update subCircuitPo
    // assert(newSubCircuitPoIndex.size() == subCircuitPoFanoutCopy.size());
    // for (int i = 0; i < newSubCircuitPoIndex.size(); ++i) {
    //     const int& poIndex = newSubCircuitPoIndex[i];
    //     chain[poIndex].fanouts = subCircuitPoFanoutCopy[i]; // bug: should append, not replace
    //     for (set<int>::const_iterator it = subCircuitPoFanoutCopy[i].begin(); it != subCircuitPoFanoutCopy[i].end(); ++it) {
    //         bool updated = false;
    //         int originPoIndex = *next(subCircuitPoIndex.begin(), i);
    //         if (Gate::getFaninId(chain[*it].fanin1) == originPoIndex) {
    //             chain[*it].fanin1 = (poIndex << 1) | Gate::getFaninComplement(chain[*it].fanin1);
    //             updated = true;
    //         }
    //         if (Gate::getFaninId(chain[*it].fanin2) == originPoIndex) {
    //             chain[*it].fanin2 = (poIndex << 1) | Gate::getFaninComplement(chain[*it].fanin2);
    //             updated = true;
    //         }
    //         assert(updated);
    //     }
    // }

    for (int i = piNum + 1; i < chain.size(); ++i) {
        if (chain[i].deleted)
            continue;
        if (originSubCircuit.find(i) != originSubCircuit.end())
            continue;
        int fanin1 = Gate::getFaninId(chain[i].fanin1);
        if (originSubCircuit.find(fanin1) != originSubCircuit.end()) {
            assert(originPo2newPo.find(fanin1) != originPo2newPo.end());
            int newFanin1 = originPo2newPo[fanin1];
            int c1 = Gate::getFaninComplement(chain[i].fanin1);
            chain[i].fanin1 = (newFanin1 << 1) | c1;
            chain[newFanin1].fanouts.insert(i);
        }
        if (!isPo(i)) { // Po has no fanin2
            int fanin2 = Gate::getFaninId(chain[i].fanin2);
            if (originSubCircuit.find(fanin2) != originSubCircuit.end()) {
                assert(originPo2newPo.find(fanin2) != originPo2newPo.end());
                int newFanin2 = originPo2newPo[fanin2];
                int c2 = Gate::getFaninComplement(chain[i].fanin2);
                chain[i].fanin2 = (newFanin2 << 1) | c2;
                chain[newFanin2].fanouts.insert(i);
            }
        }
    }
    // cannot remove deleted gate because the index will be wrong
    if (verbose >= 2)
        cout << "Before updateTopological" << endl;
    updateTopological();
    if (!updateTT()) {
        cerr << "Error in replace(booleanChain.cpp): replaced circuit violated Po tt" << endl;
    }
}

void
BooleanChain::updateTopological() {
    // int nowIndex = piNum + poNum + 1;
    map<int, int> old2new;
    for (int i = 0; i <= piNum + poNum; ++i) {
        old2new[i] = i;
    }
    bool finished = false;
    int newIndex = piNum + poNum + 1;
    while (!finished) {
        finished = true;
        for (int i = piNum + poNum + 1; i < chain.size(); ++i) {
            if (chain[i].deleted)
                continue;
            if (old2new.find(i) != old2new.end())
                continue;
            finished = false;
            int fanin1 = Gate::getFaninId(chain[i].fanin1);
            int fanin2 = Gate::getFaninId(chain[i].fanin2);
            if (old2new.find(fanin1) != old2new.end() && old2new.find(fanin2) != old2new.end()) {
                old2new[i] = newIndex++;
            }
        }
    }
    vector<Gate> newChain(newIndex);
    for (int i = 0; i < newChain.size(); ++i) { // const_1, PI, PO
        if (i <= piNum + poNum)
            newChain[i].tt = chain[i].tt;
        else
            newChain[i].initTT();
    }
    for (int i = 0; i < chain.size(); ++i) {
        if (chain[i].deleted)
            continue;
        const int newIndex = old2new[i];
        if (!isPo(i)) {
            for (set<int>::iterator it = chain[i].fanouts.begin(); it != chain[i].fanouts.end(); ++it) {
                newChain[newIndex].fanouts.insert(old2new[*it]);
            }
        }
        if (i > piNum) { // not (const_1, PI)
            int fanin1 = Gate::getFaninId(chain[i].fanin1);
            int c1 = Gate::getFaninComplement(chain[i].fanin1);
            newChain[newIndex].fanin1 = (old2new[fanin1] << 1) | c1;
            if (isPo(i)) {
                newChain[newIndex].fanin2 = -1;
            }
            else {
                int fanin2 = Gate::getFaninId(chain[i].fanin2);
                int c2 = Gate::getFaninComplement(chain[i].fanin2);
                newChain[newIndex].fanin2 = (old2new[fanin2] << 1) | c2;
            }
        }
    }
    set<int> newBadConeRoot;
    for (set<int>::iterator it = badConeRoot.begin(); it != badConeRoot.end(); ++it) {
        if (chain[*it].deleted)
            continue;
        newBadConeRoot.insert(old2new[*it]);
    }
    chain = newChain;
    badConeRoot = newBadConeRoot;
    return;

/*
    for (int i = piNum + poNum + 1; i < chain.size(); ++i) {
        if (chain[i].deleted)
            continue;
        if (visited.find(i) != visited.end())
            continue;
        int fanin1 = Gate::getFaninId(chain[i].fanin1);
        int fanin2 = Gate::getFaninId(chain[i].fanin2);
        if (visited.find(fanin1) != visited.end() && visited.find(fanin2) != visited.end()) {

        }
    }

    for (int i = 0; i < chain.size(); ++i) {
        if (chain[i].deleted)
            continue;
        Gate gate;
        if (i <= piNum + poNum) {
            chainTemp.push_back(chain[i]);
            visited.insert(i);
        }
        else {
            int fanin1 = Gate::getFaninId(chain[i].fanin1);
            int fanin2 = Gate::getFaninId(chain[i].fanin2);
            if (visited.find(fanin1) != visited.end() && visited.find(fanin2) != visited.end()) {
                chainTemp.push_back(chain[i]);
                visited.insert(i);
                continue;
            }
            for (int j = i + 1; j < chain.size(); ++j) {
                if (chain[j].deleted)
                    ;
            }
        }
        chainTemp.push_back(gate);
    }
    for (int i = piNum + poNum + 1; i < chain.size(); ++i) {
        if (chain[i].deleted)
            continue;
        if (Gate::getFaninId(chain[i].fanin1) < i && Gate::getFaninId(chain[i].fanin2) < i)
            continue;
        cerr << "i=" << i << endl;
        for (int j = i + 1; j < chain.size(); ++j) {
            if (chain[j].deleted)
                continue;
            if (Gate::getFaninId(chain[j].fanin1) < i && Gate::getFaninId(chain[j].fanin2) < i) {
                cerr << "j=" << j << endl;
                swapGate(i, j);
                break;
            }
            assert(j != chain.size() - 1);
        }
    }
*/
}

// void
// BooleanChain::swapGate(int p, int q) {
//     assert(p != q);
//     assert(piNum + poNum < p && p < chain.size());
//     assert(piNum + poNum < q && q < chain.size());
//     assert(!chain[p].deleted && !chain[q].deleted);
//     int fanin1Temp = chain[p].fanin1;
//     int fanin2Temp = chain[p].fanin2;
//     set<int> fanoutsTemp = chain[p].fanouts;
//     chain[Gate::getFaninId(chain[p].fanin1)].fanouts.erase(chain[Gate::getFaninId(chain[p].fanin1)].fanouts.find(p));
//     chain[Gate::getFaninId(chain[p].fanin2)].fanouts.erase(chain[Gate::getFaninId(chain[p].fanin2)].fanouts.find(p));
//     chain[Gate::getFaninId(chain[p].fanin1)].fanouts.insert(q);
//     chain[Gate::getFaninId(chain[p].fanin2)].fanouts.insert(q);

//     chain[Gate::getFaninId(chain[q].fanin1)].fanouts.erase(chain[Gate::getFaninId(chain[q].fanin1)].fanouts.find(q));
//     chain[Gate::getFaninId(chain[q].fanin2)].fanouts.erase(chain[Gate::getFaninId(chain[q].fanin2)].fanouts.find(q));
//     chain[Gate::getFaninId(chain[q].fanin1)].fanouts.insert(p);
//     chain[Gate::getFaninId(chain[q].fanin2)].fanouts.insert(p);

//     for (int i = piNum + 1; i < chain.size(); ++i) {
//         Gate& gate = chain[i];
//         if (gate.deleted)
//             continue;

//         if (Gate::getFaninId(gate.fanin1) == p) {
//             gate.fanin1 = (q << 1) | Gate::getFaninComplement(gate.fanin1);
//         }
//         else if (Gate::getFaninId(gate.fanin1) == q) {
//             gate.fanin1 = (p << 1) | Gate::getFaninComplement(gate.fanin1);
//         }
//         // Po has no fanin2
//         if (!isPo(i)) {
//             if (Gate::getFaninId(gate.fanin2) == p) {
//                 gate.fanin2 = (q << 1) | Gate::getFaninComplement(gate.fanin2);
//             }
//             else if (Gate::getFaninId(gate.fanin2) == q) {
//                 gate.fanin2 = (p << 1) | Gate::getFaninComplement(gate.fanin2);
//             }
//         }
//     }
//     chain[p].fanin1 = chain[q].fanin1;
//     chain[p].fanin2 = chain[q].fanin2;
//     chain[p].fanouts = chain[q].fanouts;

//     chain[q].fanin1 = fanin1Temp;
//     chain[q].fanin2 = fanin2Temp;
//     chain[q].fanouts = fanoutsTemp;

//     // for (set<int>::iterator it = chain[p].fanouts.begin(); it != chain[p].fanouts.end(); ++it) {
//     //     Gate& gate = chain[*it];
//     //     if (Gate::getFaninId(gate.fanin1) == p) {
//     //         gate.fanin1 = (q << 1) | Gate::getFaninComplement(gate.fanin1);
//     //     }
//     //     else if (Gate::getFaninId(gate.fanin2) == p) {
//     //         if (chain[q].fanouts.find(*it) != chain[q].fanouts.end()) {
//     //             int temp = gate.fanin1;
//     //             gate.fanin1 = gate.fanin2;
//     //             gate.fanin2 = temp;
//     //         }
//     //         else
//     //             gate.fanin2 = (q << 1) | Gate::getFaninComplement(gate.fanin2);
//     //     }
//     //     else
//     //         assert(0);
//     // }

//     // chain[Gate::getFaninId(chain[q].fanin1)].fanouts.erase(chain[Gate::getFaninId(chain[q].fanin1)].fanouts.find(q));
//     // chain[Gate::getFaninId(chain[q].fanin2)].fanouts.erase(chain[Gate::getFaninId(chain[q].fanin2)].fanouts.find(q));
//     // chain[Gate::getFaninId(chain[q].fanin1)].fanouts.insert(p);
//     // chain[Gate::getFaninId(chain[q].fanin2)].fanouts.insert(p);
//     // for (set<int>::iterator it = chain[q].fanouts.begin(); it != chain[q].fanouts.end(); ++it) {
//     //     Gate& gate = chain[*it];
//     //     if (Gate::getFaninId(gate.fanin1) == q) {
//     //         gate.fanin1 = (p << 1) | Gate::getFaninComplement(gate.fanin1);
//     //     }
//     //     else if (Gate::getFaninId(gate.fanin2) == p) {
//     //         gate.fanin2 = (q << 1) | Gate::getFaninComplement(gate.fanin2);
//     //     }
//     //     else
//     //         assert(0);
//     // }

//     //             gate.fanouts.erase(it2++);

// }

bool
BooleanChain::checkValid(const set<int>& s, bool canBePo) const {
    // default canBePo = false

    for (set<int>::const_iterator it = s.begin(); it != s.end(); ++it) {
    // check no gate is deleted
        if ((*it) >= chain.size()) {
            cerr << "Error in checkNoDeleted(booleanChain.cpp): index out of bound" << endl;
            return false;
        }
        if (chain[*it].deleted) {
            cerr << "Error in checkNoDeleted(booleanChain.cpp): chain[" << *it << "] is deleted" << endl;
            return false;
        }

    // check no gate is Pi/Po
        if ((*it) < piNum) {
            cerr << "Error in checkNoDeleted(booleanChain.cpp): subCircuit/constraintCut cannot contain Pi" << endl;
            return false;
        }
        if (!canBePo && (isPo(*it))) {
            cerr << "Error in checkNoDeleted(booleanChain.cpp): subCircuit cannot contain Po" << endl;
            return false;
        }
    }
    return true;
}

bool
BooleanChain::updateTT(bool initial) { // return false if violate Po's TT
    // const_1 and Pi
    if (initial) {
        chain[0].initTT();
        for (int j = 0; j < chain[0].tt.size(); ++j)
            chain[0].tt[j] = ~0;
        for (int i = 1; i <= piNum; ++i) { // chain[0] is const_1, 1~n is Pi
            chain[i].initTT();
            // TODO: how to init var TT
            // naive way: exhaustive enumeration
            for (int j = 0; j < (1 << piNum); ++j) {
                if ((j >> (piNum-i)) & 1) {
                    chain[i].tt[j/64] |= ((size_t)1 << (63 - (j%64)));
                }
            }
        }
    }
    
    // internal node
    for (int i = piNum+poNum + 1; i < chain.size(); ++i) {
        if (chain[i].deleted)
            continue;
        int fanin1 = Gate::getFaninId(chain[i].fanin1);
        int fanin2 = Gate::getFaninId(chain[i].fanin2);
        int c1 = Gate::getFaninComplement(chain[i].fanin1);
        int c2 = Gate::getFaninComplement(chain[i].fanin2);
        if (initial)
            chain[i].initTT();
        // cerr << "index: " << i << endl;
        assert(chain[i].tt.size() == ((1<<piNum) - 1)/64 + 1);
        // cerr << "fanin1: " << fanin1 << endl;
        // cerr << "fanin2: " << fanin2 << endl;

        // bool isPo = find(PoIndex.begin(), PoIndex.end(), i) != PoIndex.end();
        // cerr <<"isPo: " << int(isPo) << endl;
        for (int j = 0; j < chain[i].tt.size(); ++j) {
            size_t tt1 = c1 ? ~chain[fanin1].tt[j] : chain[fanin1].tt[j];
            size_t tt2 = c2 ? ~chain[fanin2].tt[j] : chain[fanin2].tt[j];
            size_t tt = tt1 & tt2;
            chain[i].tt[j] = tt;
        }
    }

    // Po
    for (int i = piNum+1; i <= piNum+poNum; ++i) {
        if (initial)
            chain[i].initTT();    
        for (int j = 0; j < chain[i].tt.size(); ++j) {
            int fanin1 = Gate::getFaninId(chain[i].fanin1);
            int c1 = Gate::getFaninComplement(chain[i].fanin1);
            // cout << "Po fanin1: " << fanin1 << endl;
            size_t tt1 = c1 ? ~chain[fanin1].tt[j] : chain[fanin1].tt[j];
            if (initial)
                chain[i].tt[j] = tt1;
            else {
                int shiftNum = (j == chain[i].tt.size()-1) ? (63 - (((1<<piNum) - 1) % 64)) : 0;
                if (chain[i].tt[j] >> shiftNum != tt1 >> shiftNum) {
                    cerr << "Error in updateTT(booleanChain.cpp): updated TT is violated with Po tt" << endl;
                    return false;
                }
            }
        }
    }
           
    return true;
}
/*
Enter subCircuit index:  48 52 53 44 54 55 46 45 43 -1
Enter constraintCut index: 15 16 17 18 19 20 21 -1
Enter reduce size: 1
*/


/*
chain[55].deleted = true                                                                                               
originPo2newPo[43] = 43                                                                                                
originPo2newPo[45] = 46                                                                                                
originPo2newPo[48] = 52                                                                                                
originPo2newPo[55] = 54   

Id =         0,                  const_1                                                                               
Id =         1,                     Pi_1                                                                               
Id =         2,                     Pi_2                                                                               
Id =         3,                     Pi_3                                                                               
Id =         4,                     Pi_4                                                                               
Id =         5,                     Pi_5                                                                               
Id =         6,                     Pi_6                                                                               
Id =         7,                     Pi_7                                                                               
Id =         8,                     Pi_8  
Id =         9,                     Pi_9                                                                     [101/1996]
Id =        10,                    Pi_10                                                                               
Id =        11,                    Pi_11                                                                               
Id =        12,                    Pi_12                                                                               
Id =        13,                    Pi_13                                                                               
Id =        14,                    Pi_14                                                                               
Id =        15,          Po_1(       ~6)                                                                               
Id =        16,         Po_2(        77)                                                                               
Id =        17,         Po_3(        34)                                                                               
Id =        18,         Po_4(        12)                                                                               
Id =        19,         Po_5(        14)                                                                               
Id =        20,          Po_6(       ~9)                                                                               
Id =        21,         Po_7(       ~39)                                                                               
Id =        22,           ~5 &        ~7                                                                               
Id =        23,           10 &        22                                                                               
Id =        24,           ~4 &        13                                                                               
Id =        25,           23 &       ~24                                                                               
Id =        26,           ~3 &         8 
Id =        27,            5 &        22                                                                      [83/1996]
Id =        28,            4 &        27                                                                               
Id =        29,            3 &        13                                                                               
Id =        30,          ~27 &       ~29                                                                               
Id =        31,           ~8 &       ~30                                                                               
Id =        32,           ~4 &        10                                                                               
Id =        33,           ~1 &        32                                                                               
Id =        34,            8 &       ~13                                                                               
Id =        35,          ~14 &       ~26                                                                               
Id =        36,          ~34 &       ~35                                                                               
Id =        37,          ~33 &       ~36                                                                               
Id =        38,          ~31 &       ~37                                                                               
Id =        39,          ~28 &       ~38                                                                               
Id =        40,          ~26 &       ~39                                                                               
Id =        41,           ~4 &       ~23                                                                               
Id =        42,          ~30 &       ~41                                                                               
Id =        43,          ~40 &       ~42  o
Id =        44,            8 &        43  -
Id =        45,           ~8 &       ~47    BUGGY: why use Id=47 ?????                                                                            
Id =        46,            1 &        44                                                                               
Id =        47,            4 &        ~8                                                                               
Id =        48,           ~1 &       ~43                                                                               
Id =        49,            2 &       ~11                                                                               
Id =        50,           ~7 &        49                                                                               
Id =        51,            5 &       ~50                                                                               
Id =        52,          ~47 &       ~46                                                                               
Id =        53,          ~45 &        46                                                                               
Id =        54,          ~48 &        52                                                                               
Id =        56,          ~25 &        54                                                                               
Id =        57,           ~1 &       ~52                                                                               
Id =        58,          ~46 &       ~57                                                                               
Id =        59,          ~10 &        51                                                                               
Id =        60,          ~58 &       ~59                                                                               
Id =        61,          ~28 &       ~60                                                                               
Id =        62,           29 &        43                                                                               
Id =        63,           ~1 &        40                                                                               
Id =        64,           10 &       ~51                                                                               
Id =        65,          ~27 &       ~64  
Id =        66,           22 &        49  
Id =        67,          ~33 &       ~38  
Id =        68,           ~3 &         4  
Id =        69,           10 &        68  
Id =        70,          ~66 &       ~69  
Id =        71,          ~67 &        70  
Id =        72,          ~65 &       ~71  
Id =        73,          ~54 &        72  
Id =        74,          ~62 &       ~63  
Id =        75,          ~73 &        74  
Id =        76,          ~61 &       ~75  
Id =        77,          ~56 &       ~76  
*/
/*
Begin create node
Segmentation fault (core dumped)

i=45
Segmentation fault (core dumped)
*/
/*
Id =        40,          ~26 &       ~39                                                                               
Id =        41,           ~4 &       ~23                                                                               
Id =        42,          ~30 &       ~41                                                                               
Id =        43,          ~40 &       ~42  x                                                                             
Id =        44,           ~1 &       ~43  x                                                                             
Id =        45,            1 &         8  x                                                                             
Id =        46,           43 &        45  x                                                                             
Id =        47,            4 &        ~8                                                                               
Id =        48,          ~27 &       ~47  x                                                                            
Id =        49,            2 &       ~11                                                                               
Id =        50,           ~7 &        49                                                                               
Id =        51,            5 &       ~50  
Id =        52,            8 &       ~51  x
Id =        53,          ~48 &       ~52  x
Id =        54,          ~44 &       ~53  x                                                                             
Id =        55,          ~46 &        54  x                                                                             
Id =        56,          ~25 &        55   
*/
/*
----------- subCircuitPiIndex 1                                                                                        
----------- subCircuitPiIndex 8                                                                                        
----------- subCircuitPiIndex 27                                                                                       
----------- subCircuitPiIndex 40                                                                                       
----------- subCircuitPiIndex 42                                                                                       
----------- subCircuitPiIndex 47                                                                                       
----------- subCircuitPiIndex 51  
----------- subCircuit 0                                                                                               
c1: 1                                                                                                                  
c2: 1                                                                                                                  
s1: 0001000                                                                                                            
s2: 0000100                                                                                                            
ithPo: 1000                                                                                                            
----------- subCircuit 1                                                                                               
c1: 0                                                                                                                  
c2: 0                                                                                                                  
s1: 01000000                                                                                                           
s2: 00000001                                                                                                           
ithPo: 0000 
----------- subCircuit 2                                                                                               
c1: 1                                                                                                                  
c2: 1                                                                                                                  
s1: 010000000                                                                                                          
s2: 000001000                                                                                                          
ithPo: 0000                                                                                                            
----------- subCircuit 3                                                                                               
c1: 0                                                                                                                  
c2: 0                                                                                                                  
s1: 1000000000                                                                                                         
s2: 0000000010                                                                                                         
ithPo: 0100 
----------- subCircuit 4                                                                                               
c1: 1                                                                                                                  
c2: 1                                                                                                                  
s1: 10000000000                                                                                                        
s2: 00000001000                                                                                                        
ithPo: 0000                                                                                                            
----------- subCircuit 5                                                                                               
c1: 1                                                                                                                  
c2: 1                                                                                                                  
s1: 000001000000                                                                                                       
s2: 000000000010                                                                                                       
ithPo: 0010    
----------- subCircuit 6                                                                                               
c1: 1                                                                                                                  
c2: 0                                                                                                                  
s1: 0000000001000                                                                                                      
s2: 0000000000100                                                                                                      
ithPo: 0000                                                                                                            
----------- subCircuit 7                                                                                               
c1: 1                                                                                                                  
c2: 0                                                                                                                  
s1: 00000000000100                                                                                                     
s2: 00000000000010
ithPo: 0001
*/