/*
 * =====================================================================================
 *
 *       Filename:  atpg_mgr.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/17/2015 02:53:37 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xenia-cjen (xc), jonah0604@gmail.com
 *        Company:  LaDS(I), GIEE, NTU
 *
 * =====================================================================================
 */

#ifndef _CORE_ATPG_MGR_H_
#define _CORE_ATPG_MGR_H_ 

#include "atpg.h" 

namespace CoreNs {

typedef std::vector<Atpg*> AtpgVec; 

class AtpgMgr {
public: 
    AtpgMgr();  
    ~AtpgMgr();  

    void                generation(); 
    // if target gate != -1 --> gateId is the fanout gate of the target gate(i.e., the wire that happens stuck at fault is the wire between them)
	int runSingleSAF(Circuit* pCir, int gateID, CoreNs::Fault::Type faultType, int targetGateId = -1,  Abc_Ntk_t* pNtk = 0, std::vector<int>* maval = 0); // run single stuck at fault

    
    FaultListExtract    *fListExtract_;
    PatternProcessor    *pcoll_;
    Circuit             *cir_;
    Simulator           *sim_;
private: 
    void                getPoPattern(Pattern *pat, vector<int>* maval = 0, Abc_Ntk_t* pNtk = 0);  
    // bool                comp_fault(Fault* f1, Fault* f2); 
    void                calc_fault_hardness(Fault* f1); 

    Atpg                *atpg_; 

    //AtpgVec             atpgs_; 
}; 
 
inline AtpgMgr::AtpgMgr() {
        fListExtract_ = NULL;
        pcoll_        = NULL;
        cir_          = NULL;
        sim_          = NULL;
        atpg_         = NULL;
    }

inline AtpgMgr::~AtpgMgr() {}
    
}; // CoreNs

#endif // _CORE_ATPG_MGR_H_
