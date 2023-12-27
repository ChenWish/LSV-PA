// **************************************************************************
// File       [ circuit.cpp ]
// Author     [ littleshamoo ]
// Synopsis   [ Function definitions for circuit.h ]
// Date       [ 2011/07/05 created ]
// **************************************************************************

#include "circuit.h"

// using namespace IntfNs;
using namespace CoreNs;
int CoreNs::Gate::_globalRef_s = 0;

// **************************************************************************
// Function   [ Circuit::buildCircuit ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Map the circuit to our data structure.
//              description:
//              	We build the circuit with the input netlist. Also, determine
//              	the number of time frames and connection type for the circuit.
//              arguments:
//              	[in] pNetlist : The netlist we build the circuit from.
//              	[in] numFrame : The number of time frames.
//              	[in] timeFrameConnectType : The connection type of time frames.
//              	[out] bool : Indicate that we have constructed the circuit successfully.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
// bool Circuit::buildCircuit(Netlist *const pNetlist, const int &numFrame,
// 													 const TIME_FRAME_CONNECT_TYPE &timeFrameConnectType)
bool Circuit::buildCircuit(Abc_Ntk_t* pNtk, const int &numFrame, const int numTotalGates)
{
	// Cell *top = pNetlist->getTop();
	// if (!top)
	// {
	// 	std::cerr << "**ERROR Circuit::buildCircuit(): no top module set\n";
		// 	return false;
	// }
	// Techlib *techlib = pNetlist->getTechlib();
	// if (!techlib)
	// {
	// 	std::cerr << "**ERROR Circuit::buildCircuit(): no technology library\n";
		// 	return false;
	// }
	pNtk_ = pNtk;
	numFrame_ = numFrame;

	// Levelize.
	// pNetlist->levelize();
	// techlib->levelize();

	// Map the netlist to the circuit.
	mapNetlistToCircuit(numTotalGates);

	// Allocate gate memory.
	circuitGates_.resize(numGate_ * numFrame);

	// Create gates in the circuit.
	createCircuitGates();
	connectMultipleTimeFrame(); // For multiple time frames.
	assignMinLevelOfFanins();
	
	return true;
}

// **************************************************************************
// Function   [ Circuit::mapNetlistToCircuit ]
// Commenter  [ littleshamoo, PYH ]
// Synopsis   [ usage: Map cells and ports (in verilog netlist) to gates (in the circuit).
//              description:
//              	Call functions to determine some class variables, such as
//              	the number of gates and the mapping between netlist and circuit.
//              	Gate layout :
//              	frame 1                           frame 2
//              	|----- ------ ------ ----- ------|----- ------ ------
//              	| PI1 | PPI1 | gate | PO1 | PPO1 | PI2 | PPI2 | gate | ...
//              	|----- ------ ------ ----- ------|----- ------ ------
//            ]
// Date       [ Ver. 1.0 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::mapNetlistToCircuit(const int numTotalGates)
{
	calculateNumGate(numTotalGates);
	// calculateNumNet();
}

// **************************************************************************
// Function   [ Circuit::calculateNumGate ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Calculate the number of gates in circuit.
//              description:
//              	Determine the number of PI, PPI(PPO), PO and combinational
//              	gates, and determine the mapping from cell and port to gate.
//            ]
// Date       [ Ver. 1.0 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::calculateNumGate(const int numTotalGates)
{
	// Cell *top = pNetlist_->getTop();
	// Techlib *techlib = pNetlist_->getTechlib();

	numPI_ = Abc_NtkPiNum(pNtk_);
	numPPI_ = 0;
	numComb_ = numTotalGates;
	numPO_ = Abc_NtkPoNum(pNtk_);
	numGate_ = numTotalGates + Abc_NtkPiNum(pNtk_) + Abc_NtkPoNum(pNtk_); // should not have Const1 node
	// numGate_ = 0; // number of gates

	// Map PI to pseudo gates.
	// portIndexToGateIndex_.resize(top->getNPort());
	// numPI_ = 0;
	// for (int i = 0; i < (int)top->getNPort(); ++i)
	// {
	// 	if (top->getPort(i)->type_ == Port::INPUT) // Input port
		// 	{
	// 		if (strcmp(top->getPort(i)->name_, "CK") && strcmp(top->getPort(i)->name_, "test_si") && strcmp(top->getPort(i)->name_, "test_se")) // Not clock or test input
			// 		{
	// 			portIndexToGateIndex_[i] = numPI_;
				// 			++numGate_;
				// 			++numPI_;
			// 		}
	// 	}
	// }

	// Map cell to gates.
	// A single cell can have more than one gate in it.
	// numPPI_ = 0;
	// cellIndexToGateIndex_.resize(top->getNCell());
	// for (int i = 0; i < (int)top->getNCell(); ++i)
	// {
	// 	cellIndexToGateIndex_[i] = numGate_;

		// 	if (techlib->hasPmt(top->getCell(i)->libc_->id_, Pmt::DFF)) // D flip flop, indicates a PPI (PPO).
		// 	{
	// 		++numPPI_;
			// 		++numGate_;
		// 	}
	// 	else
		// 	{
	// 		numGate_ += top->getCell(i)->libc_->getNCell();
		// 	}
	// }

	// Calculate the number of combinational gates.
	// numComb_ = numGate_ - numPI_ - numPPI_;

	// Map PO to pseudo gates.
	// numPO_ = 0;
	// for (int i = 0; i < (int)top->getNPort(); ++i)
	// {
	// 	if (top->getPort(i)->type_ == Port::OUTPUT) // Output port
		// 	{
	// 		if (strcmp(top->getPort(i)->name_, "test_so")) // Not test output
			// 		{
	// 			portIndexToGateIndex_[i] = numGate_;
				// 			++numGate_;
	// 			++numPO_;
	// 		}
	// 	}
	// }
	/*
	// Add the number of PPOs (PPIs) to the number of gates.
	numGate_ += numPPI_;
*/
}

// **************************************************************************
// Function   [ Circuit::calculateNumNet ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Calculate the number of nets in circuit.
//              description:
//              	Determine the number of nets in the circuit.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::calculateNumNet()
{
/*
Cell *top = pNetlist_->getTop();
	Techlib *techlib = pNetlist_->getTechlib();

	numNet_ = 0;

	for (int i = 0; i < (int)top->getNNet(); ++i)
	{
		NetSet eqvs = top->getEqvNets(i);
		int nports = 0;
		bool smallest = true;
		for (NetSet::iterator it = eqvs.begin(); it != eqvs.end(); ++it)
		{
			if ((*it)->id_ < i)
			{
				smallest = false;
				break;
			}
			nports += (*it)->getNPort();
		}
		if (smallest)
		{
			numNet_ += nports - 1;
		}
	}

	// Add internal nets.
	for (int i = 0; i < (int)top->getNCell(); ++i)
	{
		Cell *cellInTop = top->getCell(i);
		if (!techlib->hasPmt(cellInTop->typeName_, Pmt::DFF))
		{
			numNet_ += cellInTop->libc_->getNNet() - cellInTop->libc_->getNPort();
		}
	}
*/	
}

// **************************************************************************
// Function   [ Circuit::createCircuitGates ]
// Commenter  [ Bill, PYH ]
// Synopsis   [ usage: Create gate's PI,PPI,PP,PPO,Comb and initialize all
//                     kind of gate's data.
//              description:
//              	Call functions to create different types of gates.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::createCircuitGates()
{
	std::vector<Gate> unorderedGateswINV(circuitGates_);
	createCircuitPI(unorderedGateswINV);
	createCircuitComb(unorderedGateswINV);
	createCircuitPO(unorderedGateswINV);
	insertINV(unorderedGateswINV);
	DFSreorder(unorderedGateswINV);
	levelize();
	// for(auto gate: circuitGates_){
	// 	std::cout << "ID = "<<gate.gateId_ << std::endl;
	// 	std::cout << "gate type = " << gate.gateType_ << std::endl;
	// 	std::cout << "gate level = " << gate.numLevel_ << std::endl;
	// 	std::cout << "fanin num = " << gate.numFI_ << std::endl;
	// 	std::cout << "fanin = " << std::endl;
	// 	for(auto fanin: gate.faninVector_)
	// 		std::cout << circuitGates_[fanin].gateId_ << ' ';
	// 	std::cout << std::endl;
	// 	std::cout << "fanout num = " << gate.numFO_ << std::endl;
	// 	std::cout << "fanout = " << std::endl;
	// 	for(auto fanout: gate.fanoutVector_)
	// 		std::cout << circuitGates_[fanout].gateId_ << ' ';
	// 	std::cout << std::endl;
	// 	std::cout << "==================================="<< std::endl;

	// }
	// exit(0);
	// createCircuitPPI();
	// createCircuitPPO();
}


void Circuit::insertINV(std::vector<Gate>& gateVec){
	Abc_Obj_t* pNode;
	int i;

	int currentTmpId = Abc_NtkObjNum(pNtk_) - 1;
	// std::cout << "currentTmpId = " << currentTmpId << std::endl;
	Abc_NtkForEachObj(pNtk_, pNode, i){
		if(Abc_ObjType(pNode) == ABC_OBJ_PI || Abc_ObjType(pNode) == ABC_OBJ_CONST1)
			continue;
		if(Abc_ObjFaninC0(pNode)){
			int INVid = currentTmpId++;
			insertINVbetweenNodeandNode(gateVec,INVid, pNode, Abc_ObjFanin0(pNode));
		}
		if(Abc_ObjFaninC1(pNode) && Abc_ObjType(pNode) != ABC_OBJ_PO){
			int INVid = currentTmpId++;
			insertINVbetweenNodeandNode(gateVec,INVid, pNode, Abc_ObjFanin1(pNode));
		}
	}
	// for(auto gate: gateVec){
	// 	std::cout << gate.gateId_ << std::endl;
	// 	std::cout << "gate type = " << gate.gateType_ << std::endl;
	// 	std::cout << "fanin num = " << gate.numFI_ << std::endl;
	// 	std::cout << "fanin = " << std::endl;
	// 	for(auto fanin: gate.faninVector_)
	// 		std::cout << gateVec[fanin].gateId_ << ' ';
	// 	std::cout << std::endl;
	// 	std::cout << "fanout num = " << gate.numFO_ << std::endl;
	// 	std::cout << "fanout = " << std::endl;
	// 	for(auto fanout: gate.fanoutVector_)
	// 		std::cout << gateVec[fanout].gateId_ << ' ';
	// 	std::cout << std::endl;
	// 	std::cout << "==================================="<< std::endl;

	// }
	return;
	// exit(0);
	// Abc_NtkForEachNodeReverse(pNtk_, pNode, i){
	
	// 	// if a node is Po's fanin & the edge connecting them is complemented
	// 	// if(Abc_ObjType(Abc_ObjFanout0(pNode)) == ABC_OBJ_PO && Abc_ObjFaninC0(Abc_ObjFanout0(pNode)) == 1){	
	// 	// 	insertINVbetweenPOandNode(currentId, Abc_ObjFanout0(pNode));
	// 		// int invId = currentId, nodeId = invId - 1;
	// 		// circuitGates_[invId].gateId_ =  invId;
	// 		// circuitGates_[invId].fanoutVector_.push_back(Abc_ObjFanout0(pNode)->Id); // connect the INV's fanout to the Po
	// 		// ++circuitGates_[invId].numFO_;
	// 		// circuitGates_[Abc_ObjFanout0(pNode)->Id].faninVector_.push_back(invId); // connect the Po's fanin to the INV
	// 		// ++circuitGates_[Abc_ObjFanout0(pNode)->Id].numFI_;

	// 		// circuitGates_[nodeId].gateId_ = nodeId;
	// 		// circuitGates_[nodeId].fanoutVector_.push_back(invId); 
	// 		// ++circuitGates_[nodeId].numFO_; // connect the node's fanout to the INV
	// 		// circuitGates_[invId].faninVector_.push_back(nodeId); // connect the INV's fanin to the node
	// 		// ++circuitGates_[invId].numFI_;

	// 		// --currentId;
	// 	// }

	// 	// assert(Abc_ObjFanin1(pNode)->Id > Abc_ObjFanin0(pNode)->Id);

	// 	// if(Abc_ObjFaninC1(pNode) == 1){
	// 	// 	int InvId = currentId - 1;

		
	// }
}
void Circuit::insertINVbetweenPOandNode(const int currentId, Abc_Obj_t* pPO, Abc_Obj_t* pNode){
	int POId = pPO->Id - 1, invId = currentId, nodeId = pNode->Id;
	circuitGates_[invId].gateId_ =  invId;
	circuitGates_[invId].fanoutVector_.push_back(POId); // connect the INV's fanout to the Po
	++circuitGates_[invId].numFO_;
	circuitGates_[POId].faninVector_.push_back(invId); // connect the Po's fanin to the INV
	++circuitGates_[POId].numFI_;

	circuitGates_[nodeId].gateId_ = nodeId;
	circuitGates_[nodeId].fanoutVector_.push_back(invId); 
	++circuitGates_[nodeId].numFO_; // connect the node's fanout to the INV
	circuitGates_[invId].faninVector_.push_back(nodeId); // connect the INV's fanin to the node
	++circuitGates_[invId].numFI_;
}

void Circuit::insertINVbetweenNodeandNode(std::vector<Gate>& gateVec, const int invId, Abc_Obj_t* pNode1, Abc_Obj_t* pNode2){	// node1 is the node with larger abc gate id
	int node1Id = myAbc_ObjId(pNode1) , node2Id = myAbc_ObjId(pNode2);
	gateVec[invId].gateId_ =  invId;
	gateVec[invId].gateType_ =  Gate::INV;
	gateVec[invId].fanoutVector_.push_back(node1Id); // connect the INV's fanout to the node1
	++gateVec[invId].numFO_;
	if(gateVec[node1Id].faninVector_[0] == node2Id)
		gateVec[node1Id].faninVector_[0] = invId;
	else
		gateVec[node1Id].faninVector_[1] = invId;
	// gateVec[node1Id].faninVector_.push_back(invId); // connect the node1's fanin to the INV
	// ++gateVec[node1Id].numFI_;

	gateVec[node2Id].gateId_ = node2Id;
	for(auto &j : gateVec[node2Id].fanoutVector_)
		if(j == node1Id)
			j = invId;
	// gateVec[node2Id].fanoutVector_.push_back(invId); 
	// ++gateVec[node2Id].numFO_; // connect the node2's fanout to the INV
	gateVec[invId].faninVector_.push_back(node2Id); // connect the INV's fanin to the node2
	++gateVec[invId].numFI_;

	// --currentId;
}

void Circuit::insertINVbetweenNodeandPI(const int NodeId, int& currentId, Abc_Obj_t* pPi){
	int nodeId = NodeId, invId = currentId, PiId = pPi->Id - 1;

	circuitGates_[invId].gateId_ =  invId;
	circuitGates_[invId].fanoutVector_.push_back(nodeId); // connect the INV's fanout to the node1
	++circuitGates_[invId].numFO_;
	circuitGates_[nodeId].faninVector_.push_back(invId); // connect the node1's fanin to the INV
	++circuitGates_[nodeId].numFI_;

	circuitGates_[PiId].gateId_ = PiId;
	circuitGates_[PiId].fanoutVector_.push_back(invId); 
	++circuitGates_[PiId].numFO_; // connect the node2's fanout to the INV
	circuitGates_[invId].faninVector_.push_back(PiId); // connect the INV's fanin to the node2
	++circuitGates_[invId].numFI_;

	--currentId;

}

void Circuit::DFSreorder(std::vector<Gate>& gateVec){
	// int count = Abc_NtkPiNum(pNtk_) + Abc_NtkPoNum(pNtk_);
	int count = Abc_NtkPiNum(pNtk_);
	int poOffset = count + numComb_;
	Abc_Obj_t* pPo;
	int i;
	Gate::setGlobalRef();
	// DFS
	Abc_NtkForEachPo(pNtk_, pPo, i){
		DFS(gateVec, gateVec[myAbc_ObjId(pPo)], count);
		assert(umapinsert(unorderedIdtoOrderId, myAbc_ObjId(pPo), poOffset + i));
		assert(umapinsert(orderIdtoUnorderedId, poOffset + i, myAbc_ObjId(pPo)));
	}
	Abc_NtkForEachPi(pNtk_, pPo, i){
		assert(umapinsert(unorderedIdtoOrderId, myAbc_ObjId(pPo), myAbc_ObjId(pPo)));
		assert(umapinsert(orderIdtoUnorderedId, myAbc_ObjId(pPo), myAbc_ObjId(pPo)));
	}

	// deal with PO
	Abc_NtkForEachPo(pNtk_, pPo, i){
		circuitGates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].gateId_ = poOffset + i;
		circuitGates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].gateType_ = gateVec[myAbc_ObjId(pPo)].gateType_;
		circuitGates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].numFI_ = gateVec[myAbc_ObjId(pPo)].numFI_;
		circuitGates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].numFO_ = gateVec[myAbc_ObjId(pPo)].numFO_;
		circuitGates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].faninVector_.push_back(unorderedIdtoOrderId.at(gateVec[myAbc_ObjId(pPo)].faninVector_[0]));
	}

	// deal with Pi
	Abc_NtkForEachPi(pNtk_, pPo, i){
		circuitGates_[myAbc_ObjId(pPo)].gateId_ = gateVec[myAbc_ObjId(pPo)].gateId_;
		circuitGates_[myAbc_ObjId(pPo)].gateType_ = gateVec[myAbc_ObjId(pPo)].gateType_;
		circuitGates_[myAbc_ObjId(pPo)].numFI_ = gateVec[myAbc_ObjId(pPo)].numFI_;
		circuitGates_[myAbc_ObjId(pPo)].numFO_ = gateVec[myAbc_ObjId(pPo)].numFO_;
		for(auto fanout : gateVec[myAbc_ObjId(pPo)].fanoutVector_)
			circuitGates_[myAbc_ObjId(pPo)].fanoutVector_.push_back(unorderedIdtoOrderId.at(fanout));
	}

	// deal with node
	// for(int j = Abc_NtkPiNum(pNtk_) + Abc_NtkPoNum(pNtk_) , nj = circuitGates_.size(); j < nj; ++j){
	for(int j = Abc_NtkPiNum(pNtk_), nj = j + numComb_; j < nj; ++j){
		int oldId = orderIdtoUnorderedId.at(j);
		circuitGates_[j].gateId_ = j;
		circuitGates_[j].gateType_ = gateVec[oldId].gateType_;
		circuitGates_[j].numFI_ = gateVec[oldId].numFI_;
		circuitGates_[j].numFO_ = gateVec[oldId].numFO_;
		for(auto fanout : gateVec[oldId].fanoutVector_)
			circuitGates_[j].fanoutVector_.push_back(unorderedIdtoOrderId.at(fanout));
		for(auto fanin : gateVec[oldId].faninVector_)
			circuitGates_[j].faninVector_.push_back(unorderedIdtoOrderId.at(fanin));
	}

	// std::cout << "unorderedIdtoOrderId = " << std::endl;
	// for(auto j : unorderedIdtoOrderId)
	// 	std::cout << j.first << ' ' << j.second << std::endl;
	
}
void Circuit::DFS(std::vector<Gate>& gateVec, Gate& _gate, int& tmp){
	for(auto fanin : _gate.faninVector_){
		Gate& it = gateVec[fanin];
		if (!it.isGlobalRef()){
			it.setToGlobalRef();
			DFS(gateVec, it, tmp);
			// std::cout << it.gateId_ << ' ';
			if(it.gateType_ != Gate::PI && it.gateType_ != Gate::PO){
				assert(umapinsert(unorderedIdtoOrderId, it.gateId_, tmp));
				assert(umapinsert(orderIdtoUnorderedId, tmp++, it.gateId_));
				// std::cout << tmp - 1 << std::endl;
				// std::cout << tmp - 1 << ' ';
			}
			// std::cout << it.gateId_ << std::endl;
		}
	}
}

void Circuit::levelize(){
	for(auto &i : circuitGates_){
		if(i.gateType_ == Gate::PO)
			continue;
		int maxLevel = -1;
		for(auto fanin : i.faninVector_){
			assert(circuitGates_[fanin].numLevel_ != -10);
			maxLevel = maxLevel < circuitGates_[fanin].numLevel_ ? circuitGates_[fanin].numLevel_ : maxLevel;
		}
		i.numLevel_ = maxLevel + 1;
	}
	Abc_Obj_t* pPo;
	int i, maxLevel = -1;
	Abc_NtkForEachPo(pNtk_, pPo, i){
		circuitGates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].numLevel_ = circuitGates_[circuitGates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].faninVector_[0]].numLevel_ + 1;
		maxLevel = circuitGates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].numLevel_  > maxLevel ? circuitGates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].numLevel_  : maxLevel;
	}
	circuitLvl_ = maxLevel + 1; // +1 ?? // my content
	// circuitLvl_ = maxLevel; // +1 ?? // my content

}
// **************************************************************************
// Function   [ Circuit::createCircuitPI ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Create PI gates of the circuit.
//              description:
//              	Create PI gates of the circuit from the input ports in the netlist.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::createCircuitPI(std::vector<Gate>& gateVec)
{
	// Cell *top = pNetlist_->getTop();
	Abc_Obj_t *pPi;
	int i;
	Abc_NtkForEachPi( pNtk_, pPi, i ){
		// printf("Pi's fanin num %d\n", Abc_ObjFaninNum(pPi));
		// printf("Pi's level %d\n", pPi->Level);
		// pPi = Abc_ObjFanout0(pPi);
		int piGateID = myAbc_ObjId(pPi); // pPi->Id;
		gateVec[piGateID].gateId_ = piGateID;
		gateVec[piGateID].cellId_ = i;
		gateVec[piGateID].primitiveId_ = 0;
		gateVec[piGateID].numLevel_ = 0;
		gateVec[piGateID].gateType_ = Gate::PI;
		// gateVec[piGateID].fanoutVector_.reserve(top->getNetPorts(piPort->inNet_->id_).size() - 1);
		gateVec[piGateID].fanoutVector_.reserve(Abc_ObjFanoutNum(pPi));
		// printf("circuit.cpp::createCircuitPI()\n");
		// printf("%d\n", Abc_ObjFanoutNum(pPi));
	}
	/*
	for (int i = 0; i < (int)top->getNPort(); ++i)
	{
		Port *piPort = top->getPort(i);
		// Ignore clock and test inputs.
		if (!strcmp(piPort->name_, "CK") || !strcmp(piPort->name_, "test_si") || !strcmp(piPort->name_, "test_se"))
		{
			continue;
		}
		int piGateID = portIndexToGateIndex_[i];
		if (piPort->type_ != Port::INPUT || piGateID < 0)
		{
			continue;
		}
		circuitGates_[piGateID].gateId_ = piGateID;
		circuitGates_[piGateID].cellId_ = i;
		circuitGates_[piGateID].primitiveId_ = 0;
		circuitGates_[piGateID].numLevel_ = 0;
		circuitGates_[piGateID].gateType_ = Gate::PI;
		circuitGates_[piGateID].fanoutVector_.reserve(top->getNetPorts(piPort->inNet_->id_).size() - 1);
	}
*/
}

// **************************************************************************
// Function   [ Circuit::createCircuitPPI ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Create PPI gates of the circuit.
//              description:
//              	Create PPI gates of the circuit from the cells(DFF) in the netlist.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::createCircuitPPI()
{
/*
	Cell *top = pNetlist_->getTop();
	for (int i = 0; i < numPPI_; ++i)
	{
		Cell *ppiCell = top->getCell(i);
		int ppiGateID = cellIndexToGateIndex_[i];
		circuitGates_[ppiGateID].gateId_ = ppiGateID;
		circuitGates_[ppiGateID].cellId_ = i;
		circuitGates_[ppiGateID].primitiveId_ = 0;
		circuitGates_[ppiGateID].numLevel_ = 0;
		circuitGates_[ppiGateID].gateType_ = Gate::PPI;

		int qPortID = 0;
		int fanoutSize = 0; // Calculate size of fanoutVector_ of circuitGates_[ppiGateID].
		while (strcmp(ppiCell->getPort(qPortID)->name_, "Q"))
		{
			++qPortID;
		}
		fanoutSize += top->getNetPorts(ppiCell->getPort(qPortID)->exNet_->id_).size() - 2;
		if (numFrame_ > 1 && timeFrameConnectType_ == SHIFT && i < numPPI_ - 1)
		{
			++fanoutSize; // Reserve fanoutVector_ size for multiple time frame PPI fanout in SHIFT connection type.
		}
		circuitGates_[ppiGateID].fanoutVector_.reserve(fanoutSize);
	}
*/
}

// **************************************************************************
// Function   [ Circuit::createCircuitComb ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Create combinational logic gates of the circuit.
//              description:
//              	Call createCircuitPmt() to construct the gates. Finally,
//              	determine the level of the circuit.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::createCircuitComb(std::vector<Gate>& gateVec)
{
Abc_Obj_t* pObj;
	int i;
	Abc_NtkForEachObj(pNtk_, pObj, i){
		// if(Abc_ObjType(pObj) == ABC_OBJ_CONST1 || Abc_ObjType(pObj) == ABC_OBJ_PI || Abc_ObjType(pObj) == ABC_OBJ_PO)
		if(Abc_ObjType(pObj) != ABC_OBJ_NODE)
			continue;
		// int combGateID = pObj->Id;
		int combGateID = myAbc_ObjId(pObj);
		gateVec[combGateID].gateId_ = combGateID;
		// circuitGates_[combGateID].cellId_ = cellInTop->id_;
		// circuitGates_[combGateID].primitiveId_ = j;

		// Pmt *pmt = (Pmt *)cellInTop->libc_->getCell(j);
		createCircuitPmt(gateVec, combGateID);
	}
	// my content
	// circuitLvl_ = Abc_NtkPo(pNtk_, 0)->Level;

	/*
	Cell *top = pNetlist_->getTop();

	for (int i = numPPI_; i < (int)top->getNCell(); ++i)
	{
		Cell *cellInTop = top->getCell(i);
		for (int j = 0; j < (int)cellInTop->libc_->getNCell(); ++j)
		{
			int combGateID = cellIndexToGateIndex_[i] + j;
			circuitGates_[combGateID].gateId_ = combGateID;
			circuitGates_[combGateID].cellId_ = cellInTop->id_;
			circuitGates_[combGateID].primitiveId_ = j;

			Pmt *pmt = (Pmt *)cellInTop->libc_->getCell(j);
			createCircuitPmt(combGateID, cellInTop, pmt);
		}
	}
	if ((int)top->getNCell() > 0)
	{
		circuitLvl_ = circuitGates_[cellIndexToGateIndex_[top->getNCell() - 1]].numLevel_ + 2;
	}
*/
}

// **************************************************************************
// Function   [ Circuit::createCircuitPmt ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Create primitives(gates) of the circuit.
//              description:
//              	Primitive is from Mentor .mdt . We have the relationship
//              	cell => primitive => gate. But primitive is not actually in
//              	our data structure. We then use the primitive to create the gate.
//              arguments:
//              	[in] gateID : The gateID of the gate we are going to create.
//              	[in] cell : The cell which the gate located at.
//              	[in] pmt : The primitive. It is an intermediate data type
//              	           between cell and gate. We create the gate from
//              	           this primitive.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
// void Circuit::createCircuitPmt(const int &gateID, const Cell *const cell,
                               //                                const Pmt *const pmt)
void Circuit::createCircuitPmt(std::vector<Gate>& gateVec, const int &gateID)
{
	// Determine gate type.
	determineGateType(gateVec , gateID + 1, 0);
	Abc_Obj_t* pObj = Abc_NtkObj(pNtk_, gateID + 1);

	// Abc_NtkForEachObj(pNtk_, pObj, i){
		// if(Abc_ObjType(pObj) == ABC_OBJ_CONST1 || Abc_ObjType(pObj) == ABC_OBJ_PI || Abc_ObjType(pObj) == ABC_OBJ_PO)
		// if(Abc_ObjType(pObj) == ABC_OBJ_CONST1 || Abc_ObjType(pObj) == ABC_OBJ_PI)
		// if(Abc_ObjType(pObj) == ABC_OBJ_PI)
		// if(Abc_ObjType(pObj) != ABC_OBJ_NODE)
		// 	continue;
	gateVec[gateID].faninVector_.reserve(Abc_ObjFaninNum(pObj));
	Abc_Obj_t* pFanin;
	int j;
	Abc_ObjForEachFanin(pObj, pFanin, j){
		// gateVec[gateID].faninVector_.push_back(pFanin->Id);
		gateVec[gateID].faninVector_.push_back(myAbc_ObjId(pFanin));
		gateVec[gateID].numFI_ = Abc_ObjFaninNum(pObj);
		gateVec[myAbc_ObjId(pFanin)].fanoutVector_.push_back(gateID);
		// gateVec[pFanin->Id].fanoutVector_.push_back(gateID);
		// gateVec[pFanin->Id].numFO_ = Abc_ObjFanoutNum(pFanin);
		gateVec[myAbc_ObjId(pFanin)].numFO_ = Abc_ObjFanoutNum(pFanin);
	}

	gateVec[gateID].numLevel_ = pObj->Level;
	gateVec[gateID].fanoutVector_.reserve(Abc_ObjFanoutNum(pObj));
	// }
	/*

	// Determine fanin and level.
	int maxLvl = -1;
	for (int i = 0; i < (int)pmt->getNPort(); ++i)
	{
		if (pmt->getPort(i)->type_ != Port::INPUT)
		{
			continue;
		}
		Net *nin = pmt->getPort(i)->exNet_;
		circuitGates_[gateID].faninVector_.reserve(nin->getNPort());
		for (int j = 0; j < (int)nin->getNPort(); ++j)
		{
			Port *port = nin->getPort(j);
			if (port == pmt->getPort(i))
			{
				continue;
			}

			int faninID = 0;
			// Internal connection.
			if (port->type_ == Port::OUTPUT && port->top_ != cell->libc_)
			{
				faninID = cellIndexToGateIndex_[cell->id_] + port->top_->id_;
			}
			else if (port->type_ == Port::INPUT && port->top_ == cell->libc_) // External connection.
			{
				Net *nex = cell->getPort(port->id_)->exNet_;
				PortSet ps = cell->top_->getNetPorts(nex->id_);
				PortSet::iterator it = ps.begin();
				for (; it != ps.end(); ++it)
				{
					Cell *cin = (*it)->top_;
					if ((*it)->type_ == Port::OUTPUT && cin != cell->top_)
					{
						CellSet cs = cin->libc_->getPortCells((*it)->id_);
						faninID = cellIndexToGateIndex_[cin->id_];
						if (pNetlist_->getTechlib()->hasPmt(cin->libc_->id_, Pmt::DFF)) // NE
						{
							break;
						}
						faninID += (*cs.begin())->id_;
						break;
					}
					else if ((*it)->type_ == Port::INPUT && cin == cell->top_)
					{
						faninID = portIndexToGateIndex_[(*it)->id_];
						break;
					}
				}
			}
			circuitGates_[gateID].faninVector_.push_back(faninID);
			++circuitGates_[gateID].numFI_;
			circuitGates_[faninID].fanoutVector_.push_back(gateID);
			++circuitGates_[faninID].numFO_;

			if (circuitGates_[faninID].numLevel_ > maxLvl)
			{
				maxLvl = circuitGates_[faninID].numLevel_;
			}
		}
	}
	circuitGates_[gateID].numLevel_ = maxLvl + 1;

	// Determine fanout size.
	Port *outp = NULL;
	int fanoutSize = 0;
	for (int i = 0; i < (int)pmt->getNPort() && !outp; ++i)
	{
		if (pmt->getPort(i)->type_ == Port::OUTPUT)
		{
			outp = pmt->getPort(i);
		}
	}
	PortSet ps = cell->libc_->getNetPorts(outp->exNet_->id_);
	PortSet::iterator it = ps.begin();
	for (; it != ps.end(); ++it)
	{
		if ((*it)->top_ != cell->libc_ && (*it)->top_ != pmt)
		{
			++fanoutSize;
		}
		else if ((*it)->top_ == cell->libc_)
		{
			int nid = cell->getPort((*it)->id_)->exNet_->id_;
			fanoutSize += cell->top_->getNetPorts(nid).size() - 1;
		}
	}
	circuitGates_[gateID].fanoutVector_.reserve(fanoutSize);
*/
}

// **************************************************************************
// Function   [ Circuit::determineGateType ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Determine the type of the gate.
//              description:
//              	Determine and set the gateType_ class variable of the gates
//              	with the gateID, cell and primitive.
//              arguments:
//              	[in] gateID : The gateID of the gate.
//              	[in] cell : The cell which the gate located at.
//              	[in] pmt : The primitive. It is an intermediate data type
//              	           between cell and gate. In fact, it is the gate.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
// void Circuit::determineGateType(const int &gateID, const Cell *const cell,
                                //                                 const Pmt *const pmt)
void Circuit::determineGateType(std::vector<Gate>& gateVec, const int &gateID, bool isInv)
{
	gateVec[gateID].gateType_ = isInv ? (Gate::INV) : (Gate::AND2) ;
// 	circuitGates_[gateID].phaseFanins[0] = Abc_NtkObj(pNtk_, gateID)->fCompl0;
// 	circuitGates_[gateID].phaseFanins[1] = Abc_NtkObj(pNtk_, gateID)->fCompl1;

	/*
	switch (pmt->type_)
	{
		case Pmt::BUF:
			circuitGates_[gateID].gateType_ = Gate::BUF;
			break;
		case Pmt::INV:
		case Pmt::INVF:
			circuitGates_[gateID].gateType_ = Gate::INV;
			break;
		case Pmt::MUX:
			circuitGates_[gateID].gateType_ = Gate::MUX;
			break;
		case Pmt::AND:
			if (cell->getNPort() == 3)
			{
				circuitGates_[gateID].gateType_ = Gate::AND2;
			}
			else if (cell->getNPort() == 4)
			{
				circuitGates_[gateID].gateType_ = Gate::AND3;
			}
			else if (cell->getNPort() == 5)
			{
				circuitGates_[gateID].gateType_ = Gate::AND4;
			}
			else
			{
				circuitGates_[gateID].gateType_ = Gate::NA;
			}
			break;
		case Pmt::NAND:
			if (cell->getNPort() == 3)
			{
				circuitGates_[gateID].gateType_ = Gate::NAND2;
			}
			else if (cell->getNPort() == 4)
			{
				circuitGates_[gateID].gateType_ = Gate::NAND3;
			}
			else if (cell->getNPort() == 5)
			{
				circuitGates_[gateID].gateType_ = Gate::NAND4;
			}
			else
			{
				circuitGates_[gateID].gateType_ = Gate::NA;
			}
			break;
		case Pmt::OR:
			if (cell->getNPort() == 3)
			{
				circuitGates_[gateID].gateType_ = Gate::OR2;
			}
			else if (cell->getNPort() == 4)
			{
				circuitGates_[gateID].gateType_ = Gate::OR3;
			}
			else if (cell->getNPort() == 5)
			{
				circuitGates_[gateID].gateType_ = Gate::OR4;
			}
			else
			{
				circuitGates_[gateID].gateType_ = Gate::NA;
			}
			break;
		case Pmt::NOR:
			if (cell->getNPort() == 3)
			{
				circuitGates_[gateID].gateType_ = Gate::NOR2;
			}
			else if (cell->getNPort() == 4)
			{
				circuitGates_[gateID].gateType_ = Gate::NOR3;
			}
			else if (cell->getNPort() == 5)
			{
				circuitGates_[gateID].gateType_ = Gate::NOR4;
			}
			else
			{
				circuitGates_[gateID].gateType_ = Gate::NA;
			}
			break;
		case Pmt::XOR:
			if (cell->getNPort() == 3)
			{
				circuitGates_[gateID].gateType_ = Gate::XOR2;
			}
			else if (cell->getNPort() == 4)
			{
				circuitGates_[gateID].gateType_ = Gate::XOR3;
			}
			else
			{
				circuitGates_[gateID].gateType_ = Gate::NA;
			}
			break;
		case Pmt::XNOR:
			if (cell->getNPort() == 3)
			{
				circuitGates_[gateID].gateType_ = Gate::XNOR2;
			}
			else if (cell->getNPort() == 4)
			{
				circuitGates_[gateID].gateType_ = Gate::XNOR3;
			}
			else
			{
				circuitGates_[gateID].gateType_ = Gate::NA;
			}
			break;
		case Pmt::TIE1:
			circuitGates_[gateID].gateType_ = Gate::TIE1;
			break;
		case Pmt::TIE0:
			circuitGates_[gateID].gateType_ = Gate::TIE0;
			break;
		case Pmt::TIEX:
			circuitGates_[gateID].gateType_ = Gate::TIE0;
			break;
		case Pmt::TIEZ:
			circuitGates_[gateID].gateType_ = Gate::TIEZ;
			break;
		default:
			circuitGates_[gateID].gateType_ = Gate::NA;
	}
*/
}

// **************************************************************************
// Function   [ Circuit::createCircuitPO ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Create PO gates of the circuit.
//              description:
//              	Create PO gates of the circuit from the ports in the netlist.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::createCircuitPO(std::vector<Gate>& gateVec)
{
Abc_Obj_t* pPo;
	int i, maxLevel = -1;
	Abc_NtkForEachPo(pNtk_, pPo, i){
		// pPo = Abc_ObjFanin0(pPo);
		// printf("Po's fanin num %d\n", Abc_ObjFaninNum(pPo));
		// printf("Po's level %d\n", pPo->Level);
		// int poGateID = pPo->Id;
		int poGateID = myAbc_ObjId(pPo);
		gateVec[poGateID].gateId_ = poGateID;
		gateVec[poGateID].cellId_ = i;
		gateVec[poGateID].primitiveId_ = 0;
		gateVec[poGateID].numLevel_ = Abc_ObjFanin0(pPo)->Level + 1; // my content
		maxLevel = maxLevel < gateVec[poGateID].numLevel_ ? gateVec[poGateID].numLevel_: maxLevel;
		gateVec[poGateID].gateType_ = Gate::PO;
		// gateVec[poGateID].faninVector_.push_back(Abc_ObjFanin0(pPo)->Id);
		gateVec[poGateID].faninVector_.push_back(myAbc_ObjId(Abc_ObjFanin0(pPo)));
		gateVec[poGateID].numFI_ = Abc_ObjFaninNum(pPo);	
		gateVec[myAbc_ObjId(Abc_ObjFanin0(pPo))].fanoutVector_.push_back(poGateID);
		gateVec[myAbc_ObjId(Abc_ObjFanin0(pPo))].numFO_ = Abc_ObjFanoutNum(Abc_ObjFanin0(pPo));
		// circuitGates_[Abc_ObjFanin0(pPo)->Id].fanoutVector_.push_back(poGateID);
		// circuitGates_[Abc_ObjFanin0(pPo)->Id].numFO_ = Abc_ObjFanoutNum(Abc_ObjFanin0(pPo));
	}
	circuitLvl_ = maxLevel + 1; // +1 ?? // my content
	/*
	Cell *top = pNetlist_->getTop();

	for (int i = 0; i < (int)top->getNPort(); ++i)
	{
		Port *poPort = top->getPort(i);
		if (!strcmp(poPort->name_, "test_so"))
		{
			continue;
		}
		int poGateID = portIndexToGateIndex_[i];
		if (poPort->type_ != Port::OUTPUT || poGateID < 0)
		{
			continue;
		}
		circuitGates_[poGateID].gateId_ = poGateID;
		circuitGates_[poGateID].cellId_ = i;
		circuitGates_[poGateID].primitiveId_ = 0;
		circuitGates_[poGateID].numLevel_ = circuitLvl_ - 1;
		circuitGates_[poGateID].gateType_ = Gate::PO;

		PortSet ps = top->getNetPorts(poPort->inNet_->id_);
		PortSet::iterator it = ps.begin();
		for (; it != ps.end(); ++it)
		{
			if ((*it) == poPort)
			{
				continue;
			}
			int faninID = 0;
			if ((*it)->top_ == top && (*it)->type_ == Port::INPUT)
			{
				faninID = portIndexToGateIndex_[(*it)->id_];
			}
			else if ((*it)->top_ != top && (*it)->type_ == Port::OUTPUT)
			{
				faninID = cellIndexToGateIndex_[(*it)->top_->id_];
				Cell *libc = (*it)->top_->libc_;
				if (!pNetlist_->getTechlib()->hasPmt(libc->id_, Pmt::DFF)) // NE
				{
					faninID += (*libc->getPortCells((*it)->id_).begin())->id_;
				}
			}
			else
			{
				continue;
			}

			circuitGates_[poGateID].faninVector_.push_back(faninID);
			++circuitGates_[poGateID].numFI_;
			circuitGates_[faninID].fanoutVector_.push_back(poGateID);
			++circuitGates_[faninID].numFO_;
		}
	}
*/
}

// **************************************************************************
// Function   [ Circuit::createCircuitPPO ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Create PPO gates of the circuit.
//              description:
//              	Create PPO gates of the circuit from the cells(DFF) in the netlist.
//            ]
// Date       [ Ver. 1.0 started 2013/08/11 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::createCircuitPPO()
{
/*
	Cell *top = pNetlist_->getTop();
	for (int i = 0; i < numPPI_; ++i)
	{
		Cell *cell = top->getCell(i);
		int ppoGateID = numGate_ - numPPI_ + i;
		circuitGates_[ppoGateID].gateId_ = ppoGateID;
		circuitGates_[ppoGateID].cellId_ = i;
		circuitGates_[ppoGateID].primitiveId_ = 0;
		circuitGates_[ppoGateID].numLevel_ = circuitLvl_ - 1;
		circuitGates_[ppoGateID].gateType_ = Gate::PPO;

		int dPortID = 0;
		while (strcmp(cell->getPort(dPortID)->name_, "D"))
		{
			++dPortID;
		}
		Port *inp = cell->getPort(dPortID);
		PortSet ps = top->getNetPorts(cell->getPort(inp->id_)->exNet_->id_);
		PortSet::iterator it = ps.begin();
		for (; it != ps.end(); ++it)
		{
			if ((*it) == inp)
			{
				continue;
			}
			int faninID = 0;
			if ((*it)->top_ == top && (*it)->type_ == Port::INPUT)
			{
				faninID = portIndexToGateIndex_[(*it)->id_];
			}
			else if ((*it)->top_ != top && (*it)->type_ == Port::OUTPUT)
			{
				faninID = cellIndexToGateIndex_[(*it)->top_->id_];
				Cell *libc = (*it)->top_->libc_;
				if (!pNetlist_->getTechlib()->hasPmt(libc->id_, Pmt::DFF)) // NE
				{
					faninID += (*libc->getPortCells((*it)->id_).begin())->id_;
				}
			}
			else
			{
				continue;
			}

			circuitGates_[ppoGateID].faninVector_.push_back(faninID);
			++circuitGates_[ppoGateID].numFI_;
			circuitGates_[faninID].fanoutVector_.push_back(ppoGateID);
			++circuitGates_[faninID].numFO_;
		}
	}
*/
}

// **************************************************************************
// Function   [ Circuit::connectMultipleTimeFrame ]
// Commenter  [ LingYunHsu, PYH ]
// Synopsis   [ usage: Connect gates in different time frames. This for multiple
//                     time frame ATPG.
//              description:
//              	For every new time frame, connect new time frame PPIs with old
//              	time frame PPOs (CAPTURE) or old time frame PPIs (SHIFT), and copy
//              	other gates to the new frame. Finally, remove old time PPO if needed.
//            ]
// Date       [ Ver. 1.0 started 2013/08/18 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::connectMultipleTimeFrame()
{
	totalLvl_ = circuitLvl_ * numFrame_;
	totalGate_ = numGate_ * numFrame_;
}

// **************************************************************************
// Function   [ Circuit::assignMinLevelOfFanins ]
// Commenter  [ Jun-Han Pan, PYH ]
// Synopsis   [ usage: For every gate, find the minimum level from all fanins' level.
//              description:
//              	For every gate, find the minimum level from all fanins' level
//              	and assign it to the gate->MinLevelOfFanins. This is used for
//              	headLine justification (atpg.h).
//            ]
// Date       [ Ver. 1.0 started 2013/08/18 last modified 2023/01/05 ]
// **************************************************************************
void Circuit::assignMinLevelOfFanins()
{
	// Abc_Obj_t* pObj;
	// int i;
	// Abc_NtkForEachObj(pNtk_, pObj, i){
	// 	// Gate *gate = &circuitGates_[pObj->Id];
	// 	Gate *gate = &circuitGates_[myAbc_ObjId(pObj)];
	// 	int minLevel = 2147483647;
	// 	Abc_Obj_t* pFanin;
	// 	int j;
	// 	Abc_ObjForEachFanin(pObj, pFanin, j){
	// 		if(pFanin->Level < minLevel){
	// 			minLevel = pFanin->Level;
	// 			gate->minLevelOfFanins_ = myAbc_ObjId(pFanin);
	// 			// gate->minLevelOfFanins_ = pFanin->Id;
	// 		}
	// 	}

	// }
	for (int i = 0; i < totalGate_; ++i)
	{
		Gate *gate = &circuitGates_[i];
		int minLvl = gate->numLevel_;
		for (int j = 0; j < gate->numFI_; ++j)
		{
			if (circuitGates_[gate->faninVector_[j]].numLevel_ < minLvl)
			{
				minLvl = circuitGates_[gate->faninVector_[j]].numLevel_;
				gate->minLevelOfFanins_ = gate->faninVector_[j];
				// find the minimum among gate->fis_[j] and replace gate->fiMinLvl with it.
				//                                            **********
				//                             gate->numFI_   *        *
				// *************************                  *  gate  *
				// * gate->faninVector_[j] * ---------------- *        *
				// *************************                  **********
			}
		}
	}
}
