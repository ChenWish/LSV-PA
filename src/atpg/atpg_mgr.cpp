/*
 * =====================================================================================
 *
 *       Filename:  atpg_mgr.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/22/2015 08:33:49 PM
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  xenia-cjen (xc), jonah0604@gmail.com
 *        Company:  LaDS(I), GIEE, NTU
 *
 * =====================================================================================
 */

#include <cassert>

#include "atpg_mgr.h" 

using namespace std; 

using namespace CoreNs; 

bool comp_fault(Fault* f1, Fault* f2);  

int AtpgMgr::runSingleSAF(Circuit* pCir, int gateID, CoreNs::Fault::Type faultType, int targetGateId, Abc_Ntk_t* pNtk, std::vector<int>* maval) // run single stuck at fault
{
    int line = 0;
    if(targetGateId != -1){
        if(pCir->gates_[pCir->gates_[gateID].fis_[0]].type_ == Gate::AND2 && pCir->gates_[gateID].fis_[0] == targetGateId)
            line = 1;
        else if(pCir->gates_[pCir->gates_[gateID].fis_[0]].type_ == Gate::INV && pCir->gates_[pCir->gates_[gateID].fis_[0]].fis_[0] == targetGateId)
            line = 1;
        else
            line = 2;
    }
    else
        line = 0;
    Fault* f = new Fault(gateID, faultType, line);
    cir_ = pCir;
    atpg_ = new Atpg(pCir, f); 
    Atpg::GenStatus ret = atpg_->Tpg(); 
    std::cout << static_cast<int>(ret) << std::endl;
    if(ret==Atpg::TEST_FOUND && pNtk != 0){
        Pattern *p = new Pattern; 
        p->pi1_ = new Value[pCir->npi_];
        p->ppi_ = new Value[pCir->nppi_];
        p->po1_ = new Value[pCir->npo_];
        p->ppo_ = new Value[pCir->nppi_];
        // pcoll_->pats_.push_back(p);
        // atpg_->GetPiPattern(p, maval); 
        sim_ = new Simulator(pCir);
        getPoPattern(p, maval, pNtk); 
        delete sim_;
        delete p;
    }
    return static_cast<int>(ret);
}

void AtpgMgr::generation() { 
    pcoll_->init(cir_); 
    Fault *f = NULL; 
    for (int i=0; i<fListExtract_->faults_.size(); i++) 
        calc_fault_hardness(fListExtract_->faults_[i]); 

    FaultList flist = fListExtract_->current_; 
    flist.sort(comp_fault); 

    while (flist.begin()!=flist.end()) { 
        if (flist.front()->state_==Fault::DT) { 
            flist.pop_front(); 
            continue; 
        }
        // if (flist.front()->state_==Fault::AB) 
        if (flist.front()->state_==Fault::AB || flist.front()->state_==Fault::PT) 
            break; 

        if (f==flist.front()) { 
            f->state_ = Fault::PT; 
            flist.push_back(flist.front()); 
            flist.pop_front(); 
        }

        f = flist.front();  
        atpg_ = new Atpg(cir_, f); 
        Atpg::GenStatus ret = atpg_->Tpg(); 

        if (ret==Atpg::TEST_FOUND) { 
            Pattern *p = new Pattern; 
		    p->pi1_ = new Value[cir_->npi_];
		    p->ppi_ = new Value[cir_->nppi_];
		    p->po1_ = new Value[cir_->npo_];
		    p->ppo_ = new Value[cir_->nppi_];
		    pcoll_->pats_.push_back(p);
            atpg_->GetPiPattern(p); 

		if ((pcoll_->staticCompression_ == PatternProcessor::OFF) && (pcoll_->XFill_ == PatternProcessor::ON)){
			pcoll_->randomFill(pcoll_->pats_.back());
		}

            sim_->pfFaultSim(pcoll_->pats_.back(), flist); 
            getPoPattern(pcoll_->pats_.back()); 
        }
        else if (ret==Atpg::UNTESTABLE) { 
            flist.front()->state_ = Fault::AU; 
            flist.pop_front(); 
        }
        else { // ABORT 
            //TODO 
         // cout << "*** BACKTRACK NEEDED!! \n";  
            flist.front()->state_ = Fault::AB; 
            flist.push_back(flist.front()); 
            flist.pop_front(); 
        }

        delete atpg_; 
    }
}

bool comp_fault(Fault* f1, Fault* f2) {
    return f1->hard_ > f2->hard_; 
} 

void AtpgMgr::calc_fault_hardness(Fault* f1) {
    int t1; 
    
    t1 = (f1->type_==Fault::SA0 || f1->type_==Fault::STR)?cir_->gates_[f1->gate_].cc1_:cir_->gates_[f1->gate_].cc0_; 
    t1 *= (f1->line_)?cir_->gates_[f1->gate_].co_i_[f1->line_-1]:cir_->gates_[f1->gate_].co_o_;

    f1->hard_ = t1; 

}


void AtpgMgr::getPoPattern(Pattern *pat, vector<int>* maval, Abc_Ntk_t* pNtk) { 
    sim_->goodSim();
    for(int i = 0, ni = cir_->ngate_; i < ni; ++i){
        int index = cir_->orderIdtoUnorderedId.at(i) + 1;
        if(index >= Abc_NtkObjNum(pNtk))
            continue;
        if (cir_->gates_[i].gl_ == PARA_H)
            maval->at(index) = L;
        else if (cir_->gates_[i].gh_ == PARA_H)
            maval->at(index) = H;
        else
            maval->at(index) = X;
    }
    return ;
    int offset = cir_->ngate_ - cir_->npo_ - cir_->nppi_;
    for (int i = 0; i < cir_->npo_ ; i++) {
        if (cir_->gates_[offset + i].gl_ == PARA_H)
            pat->po1_[i] = L;
        else if (cir_->gates_[offset + i].gh_ == PARA_H)
            pat->po1_[i] = H;
        else
            pat->po1_[i] = X;
    }

	if(pat->po2_!=NULL && cir_->nframe_>1)
		for( int i = 0 ; i < cir_->npo_ ; i++ ){
			if (cir_->gates_[offset + i + cir_->ngate_].gl_ == PARA_H)
				pat->po2_[i] = L;
			else if (cir_->gates_[offset + i + cir_->ngate_].gh_ == PARA_H)
				pat->po2_[i] = H;
			else
				pat->po2_[i] = X;
		}

    offset = cir_->ngate_ - cir_->nppi_;
	if(cir_->nframe_>1)
		offset += cir_->ngate_;
    for (int i = 0; i < cir_->nppi_; i++) {
        if (cir_->gates_[offset + i].gl_ == PARA_H)
            pat->ppo_[i] = L;
        else if (cir_->gates_[offset + i].gh_ == PARA_H)
            pat->ppo_[i] = H;
        else
            pat->ppo_[i] = X;
    }
}
