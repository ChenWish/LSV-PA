// **************************************************************************
// File       [ circuit.cpp ]
// Author     [ littleshamoo ]
// Synopsis   [ Function definitions for circuit.h ]
// Date       [ 2011/07/05 created ]
// **************************************************************************

#include "circuit1.h"
#include <climits>
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
// 													 const TIME_frame_CONNECT_TYPE &timeFrameConnectType)
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
	nframe_ = numFrame;

	// Levelize.
	// pNetlist->levelize();
	// techlib->levelize();

	// Map the netlist to the circuit.
	mapNetlistToCircuit(numTotalGates);

	// Allocate gate memory.
	// gates_.resize(ngate_ * numFrame);
	gates_ = new Gate[(ngate_ * numFrame)];

	// Create gates in the circuit.
	createCircuitGates();
	connectMultipleTimeFrame(); // For multiple time frames.
	assignMinLevelOfFanins();
	// for(int i = 0, ni = ngate_; i < ni; ++i){
	// 	gates_[i].print();
	// }
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

	npi_ = Abc_NtkPiNum(pNtk_);
	nppi_ = 0;
	ncomb_ = numTotalGates;
	npo_ = Abc_NtkPoNum(pNtk_);
	ngate_ = numTotalGates + Abc_NtkPiNum(pNtk_) + Abc_NtkPoNum(pNtk_); // should not have Const1 node
	// ngate_ = 0; // number of gates

	// Map PI to pseudo gates.
	// portIndexToGateIndex_.resize(top->getNPort());
	// npi_ = 0;
	// for (int i = 0; i < (int)top->getNPort(); ++i)
	// {
	// 	if (top->getPort(i)->type_ == Port::INPUT) // Input port
		// 	{
	// 		if (strcmp(top->getPort(i)->name_, "CK") && strcmp(top->getPort(i)->name_, "test_si") && strcmp(top->getPort(i)->name_, "test_se")) // Not clock or test input
			// 		{
	// 			portIndexToGateIndex_[i] = npi_;
				// 			++ngate_;
				// 			++npi_;
			// 		}
	// 	}
	// }

	// Map cell to gates.
	// A single cell can have more than one gate in it.
	// nppi_ = 0;
	// cellIndexToGateIndex_.resize(top->getNCell());
	// for (int i = 0; i < (int)top->getNCell(); ++i)
	// {
	// 	cellIndexToGateIndex_[i] = ngate_;

		// 	if (techlib->hasPmt(top->getCell(i)->libc_->id_, Pmt::DFF)) // D flip flop, indicates a PPI (PPO).
		// 	{
	// 		++nppi_;
			// 		++ngate_;
		// 	}
	// 	else
		// 	{
	// 		ngate_ += top->getCell(i)->libc_->getNCell();
		// 	}
	// }

	// Calculate the number of combinational gates.
	// ncomb_ = ngate_ - npi_ - nppi_;

	// Map PO to pseudo gates.
	// npo_ = 0;
	// for (int i = 0; i < (int)top->getNPort(); ++i)
	// {
	// 	if (top->getPort(i)->type_ == Port::OUTPUT) // Output port
		// 	{
	// 		if (strcmp(top->getPort(i)->name_, "test_so")) // Not test output
			// 		{
	// 			portIndexToGateIndex_[i] = ngate_;
				// 			++ngate_;
	// 			++npo_;
	// 		}
	// 	}
	// }
	/*
	// Add the number of PPOs (PPIs) to the number of gates.
	ngate_ += nppi_;
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

	nnet_ = 0;

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
			nnet_ += nports - 1;
		}
	}

	// Add internal nets.
	for (int i = 0; i < (int)top->getNCell(); ++i)
	{
		Cell *cellInTop = top->getCell(i);
		if (!techlib->hasPmt(cellInTop->typeName_, Pmt::DFF))
		{
			nnet_ += cellInTop->libc_->getNNet() - cellInTop->libc_->getNPort();
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
	// size_t tmpsize = sizeof(gates_) / sizeof(gates_[0]);
	size_t tmpsize = ngate_;
	std::vector<Gate> unorderedGateswINV(gates_, gates_ + tmpsize);
	createCircuitPI(unorderedGateswINV);
	createCircuitComb(unorderedGateswINV);
	createCircuitPO(unorderedGateswINV);
	insertINV(unorderedGateswINV);
	DFSreorder(unorderedGateswINV);
	levelize();
	// for(auto gate: gates_){
	// 	std::cout << "ID = "<<gate.id_ << std::endl;
	// 	std::cout << "gate type = " << gate.type_ << std::endl;
	// 	std::cout << "gate level = " << gate.lvl_ << std::endl;
	// 	std::cout << "fanin num = " << gate.nfi_ << std::endl;
	// 	std::cout << "fanin = " << std::endl;
	// 	for(auto fanin: gate.*fis_)
	// 		std::cout << gates_[fanin].id_ << ' ';
	// 	std::cout << std::endl;
	// 	std::cout << "fanout num = " << gate.nfo_ << std::endl;
	// 	std::cout << "fanout = " << std::endl;
	// 	for(auto fanout: gate.*fos_)
	// 		std::cout << gates_[fanout].id_ << ' ';
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
	// 	std::cout << gate.id_ << std::endl;
	// 	std::cout << "gate type = " << gate.type_ << std::endl;
	// 	std::cout << "fanin num = " << gate.nfi_ << std::endl;
	// 	std::cout << "fanin = " << std::endl;
	// 	for(auto fanin: gate.*fis_)
	// 		std::cout << gateVec[fanin].id_ << ' ';
	// 	std::cout << std::endl;
	// 	std::cout << "fanout num = " << gate.nfo_ << std::endl;
	// 	std::cout << "fanout = " << std::endl;
	// 	for(auto fanout: gate.*fos_)
	// 		std::cout << gateVec[fanout].id_ << ' ';
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
	// 		// gates_[invId].id_ =  invId;
	// 		// gates_[invId].*fos_.push_back(Abc_ObjFanout0(pNode)->Id); // connect the INV's fanout to the Po
	// 		// ++gates_[invId].nfo_;
	// 		// gates_[Abc_ObjFanout0(pNode)->Id].*fis_.push_back(invId); // connect the Po's fanin to the INV
	// 		// ++gates_[Abc_ObjFanout0(pNode)->Id].nfi_;

	// 		// gates_[nodeId].id_ = nodeId;
	// 		// gates_[nodeId].*fos_.push_back(invId); 
	// 		// ++gates_[nodeId].nfo_; // connect the node's fanout to the INV
	// 		// gates_[invId].*fis_.push_back(nodeId); // connect the INV's fanin to the node
	// 		// ++gates_[invId].nfi_;

	// 		// --currentId;
	// 	// }

	// 	// assert(Abc_ObjFanin1(pNode)->Id > Abc_ObjFanin0(pNode)->Id);

	// 	// if(Abc_ObjFaninC1(pNode) == 1){
	// 	// 	int InvId = currentId - 1;

		
	// }
}
// void Circuit::insertINVbetweenPOandNode(const int currentId, Abc_Obj_t* pPO, Abc_Obj_t* pNode){
// 	int POId = pPO->Id - 1, invId = currentId, nodeId = pNode->Id;
// 	gates_[invId].id_ =  invId;
// 	gates_[invId].*fos_.push_back(POId); // connect the INV's fanout to the Po
// 	++gates_[invId].nfo_;
// 	gates_[POId].*fis_.push_back(invId); // connect the Po's fanin to the INV
// 	++gates_[POId].nfi_;

// 	gates_[nodeId].id_ = nodeId;
// 	gates_[nodeId].*fos_.push_back(invId); 
// 	++gates_[nodeId].nfo_; // connect the node's fanout to the INV
// 	gates_[invId].*fis_.push_back(nodeId); // connect the INV's fanin to the node
// 	++gates_[invId].nfi_;
// }

void Circuit::insertINVbetweenNodeandNode(std::vector<Gate>& gateVec, const int invId, Abc_Obj_t* pNode1, Abc_Obj_t* pNode2){	// node1 is the node with larger abc gate id
	int node1Id = myAbc_ObjId(pNode1) , node2Id = myAbc_ObjId(pNode2);
	gateVec[invId].id_ =  invId;
	gateVec[invId].type_ =  Gate::INV;
	gateVec[invId].fos_ = new int[1]; // connect the INV's fanout to the node1
	gateVec[invId].fis_ = new int[1]; // connect the INV's fanout to the node1

	// gateVec[invId].*fos_.push_back(node1Id); // connect the INV's fanout to the node1
	gateVec[invId].fos_[0] = node1Id; // connect the INV's fanout to the node1
	++gateVec[invId].nfo_;
	if(gateVec[node1Id].fis_[0] == node2Id)
		gateVec[node1Id].fis_[0] = invId;
	else
		gateVec[node1Id].fis_[1] = invId;
	// gateVec[node1Id].*fis_.push_back(invId); // connect the node1's fanin to the INV
	// ++gateVec[node1Id].nfi_;

	gateVec[node2Id].id_ = node2Id;
	// for(auto &j : gateVec[node2Id].*fos_)
	for(int k = 0, nk = gateVec[node2Id].nfo_; k < nk; ++k){
		int& j = gateVec[node2Id].fos_[k];
		if(j == node1Id)
			j = invId;
	}
	// gateVec[node2Id].*fos_.push_back(invId); 
	// ++gateVec[node2Id].nfo_; // connect the node2's fanout to the INV
	gateVec[invId].fis_[0] = node2Id; // connect the INV's fanin to the node2
	// gateVec[invId].*fis_.push_back(node2Id); // connect the INV's fanin to the node2
	++gateVec[invId].nfi_;

	// --currentId;
}

// void Circuit::insertINVbetweenNodeandPI(const int NodeId, int& currentId, Abc_Obj_t* pPi){
// 	int nodeId = NodeId, invId = currentId, PiId = pPi->Id - 1;

// 	gates_[invId].id_ =  invId;
// 	gates_[invId].*fos_.push_back(nodeId); // connect the INV's fanout to the node1
// 	++gates_[invId].nfo_;
// 	gates_[nodeId].*fis_.push_back(invId); // connect the node1's fanin to the INV
// 	++gates_[nodeId].nfi_;

// 	gates_[PiId].id_ = PiId;
// 	gates_[PiId].*fos_.push_back(invId); 
// 	++gates_[PiId].nfo_; // connect the node2's fanout to the INV
// 	gates_[invId].*fis_.push_back(PiId); // connect the INV's fanin to the node2
// 	++gates_[invId].nfi_;

// 	--currentId;

// }

void Circuit::DFSreorder(std::vector<Gate>& gateVec){
	// int count = Abc_NtkPiNum(pNtk_) + Abc_NtkPoNum(pNtk_);
	int count = Abc_NtkPiNum(pNtk_);
	int poOffset = count + ncomb_;
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
		gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].id_ = poOffset + i;
		gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].type_ = gateVec[myAbc_ObjId(pPo)].type_;
		gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].nfi_ = gateVec[myAbc_ObjId(pPo)].nfi_;
		gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].nfo_ = gateVec[myAbc_ObjId(pPo)].nfo_;

		gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].fis_ = new int[1];
		// gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].*fis_.push_back(unorderedIdtoOrderId.at(gateVec[myAbc_ObjId(pPo)].*fis_[0]));
		gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].fis_[0] = (unorderedIdtoOrderId.at(gateVec[myAbc_ObjId(pPo)].fis_[0]));
	}

	// deal with Pi
	Abc_NtkForEachPi(pNtk_, pPo, i){
		gates_[myAbc_ObjId(pPo)].id_ = gateVec[myAbc_ObjId(pPo)].id_;
		gates_[myAbc_ObjId(pPo)].type_ = gateVec[myAbc_ObjId(pPo)].type_;
		gates_[myAbc_ObjId(pPo)].nfi_ = gateVec[myAbc_ObjId(pPo)].nfi_;
		// gates_[myAbc_ObjId(pPo)].nfo_ = gateVec[myAbc_ObjId(pPo)].nfo_;
		// for(auto fanout : gateVec[myAbc_ObjId(pPo)].*fos_)
		gates_[myAbc_ObjId(pPo)].fos_ = new int[gateVec[myAbc_ObjId(pPo)].nfo_];
		for(int k = 0, nk = gateVec[myAbc_ObjId(pPo)].nfo_; k < nk; ++k){
			int& fanout = gateVec[myAbc_ObjId(pPo)].fos_[k];
			gates_[myAbc_ObjId(pPo)].fos_[gates_[myAbc_ObjId(pPo)].nfo_++] = unorderedIdtoOrderId.at(fanout);
			// gates_[myAbc_ObjId(pPo)].*fos_.push_back(unorderedIdtoOrderId.at(fanout));
		}
	}

	// deal with node
	// for(int j = Abc_NtkPiNum(pNtk_) + Abc_NtkPoNum(pNtk_) , nj = gates_.size(); j < nj; ++j){
	for(int j = Abc_NtkPiNum(pNtk_), nj = j + ncomb_; j < nj; ++j){
		int oldId = orderIdtoUnorderedId.at(j);
		gates_[j].id_ = j;
		gates_[j].type_ = gateVec[oldId].type_;
		// gates_[j].nfi_ = gateVec[oldId].nfi_;
		// gates_[j].nfo_ = gateVec[oldId].nfo_;
		gates_[j].fis_ = new int[gateVec[oldId].nfi_];
		gates_[j].fos_ = new int[gateVec[oldId].nfo_];
		// for(auto fanout : gateVec[oldId].*fos_)
		for(int k = 0, nk = gateVec[oldId].nfo_; k < nk; ++k){
			int& fanout = gateVec[oldId].fos_[k];
			gates_[j].fos_[gates_[j].nfo_++] = unorderedIdtoOrderId.at(fanout);
			// gates_[j].*fos_.push_back(unorderedIdtoOrderId.at(fanout));
		}
		for(int k = 0, nk = gateVec[oldId].nfi_; k < nk; ++k)
			gates_[j].fis_[gates_[j].nfi_++] = unorderedIdtoOrderId.at(gateVec[oldId].fis_[k]);
		// for(auto fanin : gateVec[oldId].*fis_)
		// gates_[j].*fis_.push_back(unorderedIdtoOrderId.at(fanin));
	}

	// std::cout << "unorderedIdtoOrderId = " << std::endl;
	// for(auto j : unorderedIdtoOrderId)
	// 	std::cout << j.first << ' ' << j.second << std::endl;
	
}
void Circuit::DFS(std::vector<Gate>& gateVec, Gate& _gate, int& tmp){
	// for(auto fanin : _gate.*fis_){
	for(int i = 0, ni = _gate.nfi_; i < ni; ++i){
		int fanin = _gate.fis_[i];
		Gate& it = gateVec[fanin];
		if (!it.isGlobalRef()){
			it.setToGlobalRef();
			DFS(gateVec, it, tmp);
			// std::cout << it.id_ << ' ';
			if(it.type_ != Gate::PI && it.type_ != Gate::PO){
				assert(umapinsert(unorderedIdtoOrderId, it.id_, tmp));
				assert(umapinsert(orderIdtoUnorderedId, tmp++, it.id_));
				// std::cout << tmp - 1 << std::endl;
				// std::cout << tmp - 1 << ' ';
			}
			// std::cout << it.id_ << std::endl;
		}
	}
}

void Circuit::levelize(){
	// for(auto &i : gates_){
	for(int j = 0, nj = ngate_; j < nj; ++j){
		Gate& i = gates_[j];
		if(i.type_ == Gate::PO)
			continue;
		int maxLevel = -1;
		// for(auto fanin : i.*fis_){
		for(int k = 0, nk = i.nfi_; k < nk; ++k){
			int fanin = i.fis_[k];
			assert(gates_[fanin].lvl_ != -10);
			maxLevel = maxLevel < gates_[fanin].lvl_ ? gates_[fanin].lvl_ : maxLevel;
		}
		i.lvl_ = maxLevel + 1;
	}
	Abc_Obj_t* pPo;
	int i, maxLevel = -1;
	Abc_NtkForEachPo(pNtk_, pPo, i){
		gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].lvl_ = gates_[gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].fis_[0]].lvl_ + 1;
		maxLevel = gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].lvl_  > maxLevel ? gates_[unorderedIdtoOrderId.at(myAbc_ObjId(pPo))].lvl_  : maxLevel;
	}
	lvl_ = maxLevel + 1; // +1 ?? // my content
	// lvl_ = maxLevel; // +1 ?? // my content

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
		gateVec[piGateID].id_ = piGateID;
		gateVec[piGateID].cid_ = i;
		gateVec[piGateID].pmtid_ = 0;
		gateVec[piGateID].lvl_ = 0;
		gateVec[piGateID].type_ = Gate::PI;
		// gateVec[piGateID].*fos_.reserve(top->getNetPorts(piPort->inNet_->id_).size() - 1);
		// gateVec[piGateID].*fos_.reserve(Abc_ObjFanoutNum(pPi));
		gateVec[piGateID].fos_ = new int[Abc_ObjFanoutNum(pPi)];
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
		gates_[piGateID].id_ = piGateID;
		gates_[piGateID].cid_ = i;
		gates_[piGateID].pmtid_ = 0;
		gates_[piGateID].lvl_ = 0;
		gates_[piGateID].type_ = Gate::PI;
		gates_[piGateID].*fos_.reserve(top->getNetPorts(piPort->inNet_->id_).size() - 1);
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
	for (int i = 0; i < nppi_; ++i)
	{
		Cell *ppiCell = top->getCell(i);
		int ppiGateID = cellIndexToGateIndex_[i];
		gates_[ppiGateID].id_ = ppiGateID;
		gates_[ppiGateID].cid_ = i;
		gates_[ppiGateID].pmtid_ = 0;
		gates_[ppiGateID].lvl_ = 0;
		gates_[ppiGateID].type_ = Gate::PPI;

		int qPortID = 0;
		int fanoutSize = 0; // Calculate size of *fos_ of gates_[ppiGateID].
		while (strcmp(ppiCell->getPort(qPortID)->name_, "Q"))
		{
			++qPortID;
		}
		fanoutSize += top->getNetPorts(ppiCell->getPort(qPortID)->exNet_->id_).size() - 2;
		if (nframe_ > 1 && connType_ == SHIFT && i < nppi_ - 1)
		{
			++fanoutSize; // Reserve *fos_ size for multiple time frame PPI fanout in SHIFT connection type.
		}
		gates_[ppiGateID].*fos_.reserve(fanoutSize);
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
	Abc_NtkForEachNode(pNtk_, pObj, i){
		// if(Abc_ObjType(pObj) == ABC_OBJ_CONST1 || Abc_ObjType(pObj) == ABC_OBJ_PI || Abc_ObjType(pObj) == ABC_OBJ_PO)
		// if(Abc_ObjType(pObj) != ABC_OBJ_NODE)
		// 	continue;
		// int combGateID = pObj->Id;
		int combGateID = myAbc_ObjId(pObj);
		gateVec[combGateID].id_ = combGateID;
		// gates_[combGateID].cid_ = cellInTop->id_;
		// gates_[combGateID].pmtid_ = j;

		// Pmt *pmt = (Pmt *)cellInTop->libc_->getCell(j);
		createCircuitPmt(gateVec, combGateID);
	}
	// my content
	// lvl_ = Abc_NtkPo(pNtk_, 0)->Level;

	/*
	Cell *top = pNetlist_->getTop();

	for (int i = nppi_; i < (int)top->getNCell(); ++i)
	{
		Cell *cellInTop = top->getCell(i);
		for (int j = 0; j < (int)cellInTop->libc_->getNCell(); ++j)
		{
			int combGateID = cellIndexToGateIndex_[i] + j;
			gates_[combGateID].id_ = combGateID;
			gates_[combGateID].cid_ = cellInTop->id_;
			gates_[combGateID].pmtid_ = j;

			Pmt *pmt = (Pmt *)cellInTop->libc_->getCell(j);
			createCircuitPmt(combGateID, cellInTop, pmt);
		}
	}
	if ((int)top->getNCell() > 0)
	{
		lvl_ = gates_[cellIndexToGateIndex_[top->getNCell() - 1]].lvl_ + 2;
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
	// determineGateType(gateVec , gateID + 1, 0);
	determineGateType(gateVec , gateID, 0); // my content // should not add 1?
	Abc_Obj_t* pObj = Abc_NtkObj(pNtk_, gateID + 1);

	// Abc_NtkForEachObj(pNtk_, pObj, i){
		// if(Abc_ObjType(pObj) == ABC_OBJ_CONST1 || Abc_ObjType(pObj) == ABC_OBJ_PI || Abc_ObjType(pObj) == ABC_OBJ_PO)
		// if(Abc_ObjType(pObj) == ABC_OBJ_CONST1 || Abc_ObjType(pObj) == ABC_OBJ_PI)
		// if(Abc_ObjType(pObj) == ABC_OBJ_PI)
		// if(Abc_ObjType(pObj) != ABC_OBJ_NODE)
		// 	continue;
	// gateVec[gateID].*fis_.reserve(Abc_ObjFaninNum(pObj));
	gateVec[gateID].fis_ = new int[Abc_ObjFaninNum(pObj)];
	Abc_Obj_t* pFanin;
	int j;
	Abc_ObjForEachFanin(pObj, pFanin, j){
		// gateVec[gateID].*fis_.push_back(pFanin->Id);
		// gateVec[gateID].*fis_.push_back(myAbc_ObjId(pFanin));
		gateVec[gateID].fis_[gateVec[gateID].nfi_] = myAbc_ObjId(pFanin);
		++gateVec[gateID].nfi_;
		// gateVec[gateID].nfi_ = Abc_ObjFaninNum(pObj);
		gateVec[myAbc_ObjId(pFanin)].fos_[gateVec[myAbc_ObjId(pFanin)].nfo_] = gateID;
		++gateVec[myAbc_ObjId(pFanin)].nfo_;
		// gateVec[myAbc_ObjId(pFanin)].nfo_ = Abc_ObjFanoutNum(pFanin);

		// gateVec[myAbc_ObjId(pFanin)].*fos_.push_back(gateID);
		// gateVec[pFanin->Id].*fos_.push_back(gateID);
		// gateVec[pFanin->Id].nfo_ = Abc_ObjFanoutNum(pFanin);
	}

	gateVec[gateID].lvl_ = pObj->Level;
	gateVec[gateID].fos_ = new int[Abc_ObjFanoutNum(pObj)];
	// gateVec[gateID].*fos_.reserve(Abc_ObjFanoutNum(pObj));
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
		gates_[gateID].*fis_.reserve(nin->getNPort());
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
			gates_[gateID].*fis_.push_back(faninID);
			++gates_[gateID].nfi_;
			gates_[faninID].*fos_.push_back(gateID);
			++gates_[faninID].nfo_;

			if (gates_[faninID].lvl_ > maxLvl)
			{
				maxLvl = gates_[faninID].lvl_;
			}
		}
	}
	gates_[gateID].lvl_ = maxLvl + 1;

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
	gates_[gateID].*fos_.reserve(fanoutSize);
*/
}

// **************************************************************************
// Function   [ Circuit::determineGateType ]
// Commenter  [ PYH ]
// Synopsis   [ usage: Determine the type of the gate.
//              description:
//              	Determine and set the type_ class variable of the gates
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
	gateVec[gateID].type_ = isInv ? (Gate::INV) : (Gate::AND2) ;
// 	gates_[gateID].phaseFanins[0] = Abc_NtkObj(pNtk_, gateID)->fCompl0;
// 	gates_[gateID].phaseFanins[1] = Abc_NtkObj(pNtk_, gateID)->fCompl1;

	/*
	switch (pmt->type_)
	{
		case Pmt::BUF:
			gates_[gateID].type_ = Gate::BUF;
			break;
		case Pmt::INV:
		case Pmt::INVF:
			gates_[gateID].type_ = Gate::INV;
			break;
		case Pmt::MUX:
			gates_[gateID].type_ = Gate::MUX;
			break;
		case Pmt::AND:
			if (cell->getNPort() == 3)
			{
				gates_[gateID].type_ = Gate::AND2;
			}
			else if (cell->getNPort() == 4)
			{
				gates_[gateID].type_ = Gate::AND3;
			}
			else if (cell->getNPort() == 5)
			{
				gates_[gateID].type_ = Gate::AND4;
			}
			else
			{
				gates_[gateID].type_ = Gate::NA;
			}
			break;
		case Pmt::NAND:
			if (cell->getNPort() == 3)
			{
				gates_[gateID].type_ = Gate::NAND2;
			}
			else if (cell->getNPort() == 4)
			{
				gates_[gateID].type_ = Gate::NAND3;
			}
			else if (cell->getNPort() == 5)
			{
				gates_[gateID].type_ = Gate::NAND4;
			}
			else
			{
				gates_[gateID].type_ = Gate::NA;
			}
			break;
		case Pmt::OR:
			if (cell->getNPort() == 3)
			{
				gates_[gateID].type_ = Gate::OR2;
			}
			else if (cell->getNPort() == 4)
			{
				gates_[gateID].type_ = Gate::OR3;
			}
			else if (cell->getNPort() == 5)
			{
				gates_[gateID].type_ = Gate::OR4;
			}
			else
			{
				gates_[gateID].type_ = Gate::NA;
			}
			break;
		case Pmt::NOR:
			if (cell->getNPort() == 3)
			{
				gates_[gateID].type_ = Gate::NOR2;
			}
			else if (cell->getNPort() == 4)
			{
				gates_[gateID].type_ = Gate::NOR3;
			}
			else if (cell->getNPort() == 5)
			{
				gates_[gateID].type_ = Gate::NOR4;
			}
			else
			{
				gates_[gateID].type_ = Gate::NA;
			}
			break;
		case Pmt::XOR:
			if (cell->getNPort() == 3)
			{
				gates_[gateID].type_ = Gate::XOR2;
			}
			else if (cell->getNPort() == 4)
			{
				gates_[gateID].type_ = Gate::XOR3;
			}
			else
			{
				gates_[gateID].type_ = Gate::NA;
			}
			break;
		case Pmt::XNOR:
			if (cell->getNPort() == 3)
			{
				gates_[gateID].type_ = Gate::XNOR2;
			}
			else if (cell->getNPort() == 4)
			{
				gates_[gateID].type_ = Gate::XNOR3;
			}
			else
			{
				gates_[gateID].type_ = Gate::NA;
			}
			break;
		case Pmt::TIE1:
			gates_[gateID].type_ = Gate::TIE1;
			break;
		case Pmt::TIE0:
			gates_[gateID].type_ = Gate::TIE0;
			break;
		case Pmt::TIEX:
			gates_[gateID].type_ = Gate::TIE0;
			break;
		case Pmt::TIEZ:
			gates_[gateID].type_ = Gate::TIEZ;
			break;
		default:
			gates_[gateID].type_ = Gate::NA;
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
		gateVec[poGateID].fis_ = new int[1];
		gateVec[poGateID].id_ = poGateID;
		gateVec[poGateID].cid_ = i;
		gateVec[poGateID].pmtid_ = 0;
		gateVec[poGateID].lvl_ = Abc_ObjFanin0(pPo)->Level + 1; // my content
		maxLevel = maxLevel < gateVec[poGateID].lvl_ ? gateVec[poGateID].lvl_: maxLevel;
		gateVec[poGateID].type_ = Gate::PO;
		// gateVec[poGateID].*fis_.push_back(Abc_ObjFanin0(pPo)->Id);
		// gateVec[poGateID].*fis_.push_back(myAbc_ObjId(Abc_ObjFanin0(pPo)));
		gateVec[poGateID].fis_[0] = myAbc_ObjId(Abc_ObjFanin0(pPo));
		gateVec[poGateID].nfi_ = Abc_ObjFaninNum(pPo);	
		gateVec[myAbc_ObjId(Abc_ObjFanin0(pPo))].fos_[gateVec[myAbc_ObjId(Abc_ObjFanin0(pPo))].nfo_] = poGateID;
		++gateVec[myAbc_ObjId(Abc_ObjFanin0(pPo))].nfo_;
		// gateVec[myAbc_ObjId(Abc_ObjFanin0(pPo))].*fos_.push_back(poGateID);
		// gateVec[myAbc_ObjId(Abc_ObjFanin0(pPo))].nfo_ = Abc_ObjFanoutNum(Abc_ObjFanin0(pPo));
		// gates_[Abc_ObjFanin0(pPo)->Id].*fos_.push_back(poGateID);
		// gates_[Abc_ObjFanin0(pPo)->Id].nfo_ = Abc_ObjFanoutNum(Abc_ObjFanin0(pPo));
	}
	lvl_ = maxLevel + 1; // +1 ?? // my content
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
		gates_[poGateID].id_ = poGateID;
		gates_[poGateID].cid_ = i;
		gates_[poGateID].pmtid_ = 0;
		gates_[poGateID].lvl_ = lvl_ - 1;
		gates_[poGateID].type_ = Gate::PO;

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

			gates_[poGateID].*fis_.push_back(faninID);
			++gates_[poGateID].nfi_;
			gates_[faninID].*fos_.push_back(poGateID);
			++gates_[faninID].nfo_;
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
	for (int i = 0; i < nppi_; ++i)
	{
		Cell *cell = top->getCell(i);
		int ppoGateID = ngate_ - nppi_ + i;
		gates_[ppoGateID].id_ = ppoGateID;
		gates_[ppoGateID].cid_ = i;
		gates_[ppoGateID].pmtid_ = 0;
		gates_[ppoGateID].lvl_ = lvl_ - 1;
		gates_[ppoGateID].type_ = Gate::PPO;

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

			gates_[ppoGateID].*fis_.push_back(faninID);
			++gates_[ppoGateID].nfi_;
			gates_[faninID].*fos_.push_back(ppoGateID);
			++gates_[faninID].nfo_;
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
	tlvl_ = lvl_ * nframe_;
	tgate_ = ngate_ * nframe_;
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
	// 	// Gate *gate = &gates_[pObj->Id];
	// 	Gate *gate = &gates_[myAbc_ObjId(pObj)];
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
	for (int i = 0; i < tgate_; ++i)
	{
		Gate *gate = &gates_[i];
		int minLvl = gate->lvl_;
		for (int j = 0; j < gate->nfi_; ++j)
		{
			if (gates_[gate->fis_[j]].lvl_ < minLvl)
			{
				minLvl = gates_[gate->fis_[j]].lvl_;
				gate->fiMinLvl_ = gate->fis_[j];
				// find the minimum among gate->fis_[j] and replace gate->fiMinLvl with it.
				//                                            **********
				//                             gate->nfi_   *        *
				// *************************                  *  gate  *
				// * gate->*fis_[j] * ---------------- *        *
				// *************************                  **********
			}
		}
	}
}

void Circuit::runScoap() {
    //calculate cc 
    for(int i=0; i<tgate_; ++i) {
        Gate& g = gates_[i]; 
        switch(g.type_) {
        case Gate::PI: 
        case Gate::PPI: 
            g.cc0_=g.cc1_=1; 
            break; 
        case Gate::PO: 
        case Gate::PPO: 
            g.cc0_=gates_[g.fis_[0]].cc0_; 
            g.cc1_=gates_[g.fis_[0]].cc1_; 
            break; 
        case Gate::INV: 
        case Gate::BUF: 
        {
            Gate fi = gates_[g.fis_[0]]; 
            g.cc0_ = (!g.isInverse())?fi.cc0_:fi.cc1_; 
            g.cc1_ = (!g.isInverse())?fi.cc1_:fi.cc0_; 

            g.cc0_+=1; 
            g.cc1_+=1; 
            break; 
        }
        case Gate::AND2: 
        case Gate::AND3: 
        case Gate::AND4: 
        case Gate::NAND2: 
        case Gate::NAND3: 
        case Gate::NAND4: 
        { 
            int cc1_sum = 0; 
            int cc0_min = INT_MAX; 
            for(int i=0; i<g.nfi_; i++) { 
                Gate fi = gates_[g.fis_[i]]; 
                cc1_sum+=fi.cc1_; 
                if(fi.cc0_ < cc0_min) 
                    cc0_min = fi.cc0_; 
            }

            g.cc0_ = (!g.isInverse())?cc0_min:cc1_sum; 
            g.cc1_ = (!g.isInverse())?cc1_sum:cc0_min; 

            g.cc0_+=1; 
            g.cc1_+=1; 
            break; 
        }
        case Gate::OR2: 
        case Gate::OR3: 
        case Gate::OR4: 
        case Gate::NOR2: 
        case Gate::NOR3: 
        case Gate::NOR4: 
        { 
            int cc0_sum = 0; 
            int cc1_min = INT_MAX; 
            for(int i=0; i<g.nfi_; i++) { 
                Gate fi = gates_[g.fis_[i]]; 
                cc0_sum+=fi.cc0_; 
                if(fi.cc1_ < cc1_min) 
                    cc1_min = fi.cc1_; 
            }

            g.cc0_ = (!g.isInverse())?cc0_sum:cc1_min; 
            g.cc1_ = (!g.isInverse())?cc1_min:cc0_sum; 

            g.cc0_+=1; 
            g.cc1_+=1; 
            break; 
        }
        default: 
            cout << "***WARNNING: Circuit::runScoap(): gate type "; 
            cout << g.type_; 
            cout << "currently not supported...\n"; 
            break; 
        }
    }

    //calculate co 
    for(int n=tgate_-1; n>=0; --n) {
        Gate *g = &gates_[n]; 
        //set co_o_
        bool co_o_set = false; 
        int co_o_min = INT_MAX; 
        for(int i=0; i<g->nfo_; ++i) {
            Gate *fo = &gates_[g->fos_[i]]; 
            for(int j=0; j<fo->nfi_; ++j) { 
                if(fo->fis_[j]==g->id_ && 
                     fo->co_i_[j]<co_o_min) { 
                        co_o_min = fo->co_i_[j]; 
                        co_o_set = true; 
                }
            }
        }
        g->co_o_ = (!co_o_set)?0:co_o_min; 

        //set co_i_[]; 
        switch(g->type_) { 
        case Gate::PI: 
        case Gate::PPI: 
            break; 
        case Gate::PO: 
        case Gate::PPO: 
            g->co_i_[0] = 0; 
            break; 
        case Gate::INV: 
        case Gate::BUF: 
            g->co_i_[0] = g->co_o_+1; 
            break; 
        case Gate::AND2: 
        case Gate::AND3: 
        case Gate::AND4: 
        case Gate::NAND2: 
        case Gate::NAND3: 
        case Gate::NAND4: 
        case Gate::OR2: 
        case Gate::OR3: 
        case Gate::OR4: 
        case Gate::NOR2: 
        case Gate::NOR3: 
        case Gate::NOR4: 
        {
            for(int i=0; i<g->nfi_; ++i) { 
                g->co_i_[i] = g->co_o_ + 1; 
                for(int j=0; j<g->nfi_; ++j) { 
                    if(j==i) continue; 
                    int cc = (g->getInputNonCtrlValue()==L)?gates_[g->fis_[j]].cc0_:gates_[g->fis_[j]].cc1_;  
                    g->co_i_[i]+=cc; 
                }
            }
            break; 
        }
        default: 
            cout << "***WARNNING: Circuit::runScoap(): gate type "; 
            cout << g->type_; 
            cout << "currently not supported...\n"; 
            break; 
        }
    }
}
