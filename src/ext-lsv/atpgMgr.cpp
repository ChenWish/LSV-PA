#include "atpgMgr.h"
// #include "atpgPKG/atpg.h"

using namespace CoreNs;



int AtpgMgr::runSingleSAF(int gateID, CoreNs::Fault::FAULT_TYPE faultType){
    return atpg->runSingleSAF(pPatPro, gateID, faultType);

}