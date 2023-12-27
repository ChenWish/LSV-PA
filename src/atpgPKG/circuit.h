// **************************************************************************
// File       [ circuit.h ]
// Author     [ littleshamoo ]
// Synopsis   [ This file define the circuits. ]
// Date       [ 2010/12/29 created ]
// **************************************************************************

#ifndef _CORE_CIRCUIT_H_
#define _CORE_CIRCUIT_H_

// #include "interface/netlist.h"
#include "gate.h"
#include "base/abc/abc.h"
#include "unordered_map"

namespace CoreNs
{//
	class Circuit
	{
	public:
		// Specify how to connect multiple time frames of circuits.
		// CAPTURE means Launch-on-capture; SHIFT means launch-on-shift.
		enum TIME_FRAME_CONNECT_TYPE
		{
			CAPTURE = 0,
			SHIFT
		};

		inline Circuit();

		// Build the circuit from the netlist.
		// bool buildCircuit(IntfNs::Netlist *const pNetlist, const int &numFrame = 1,
		//                   const TIME_FRAME_CONNECT_TYPE &timeFrameConnectType = CAPTURE);
		// my buildCircuit() 
		// Build the circuit from the netlist.
		bool buildCircuit(Abc_Ntk_t* pNtk, const int &numFrame, const int numTotalGates);
		// Info for one time frame.
		// IntfNs::Netlist *pNetlist_; // Corresponding netlist.
		Abc_Ntk_t* pNtk_; // Corresponding netlist.
		int numPI_;                 // Number of PIs.
		int numPPI_;                // Number of PPIs (PPOs).
		int numPO_;                 // Number of POs.
		int numComb_;               // Number of combinational gates.
		int numGate_;               // Number of gates.
		int numNet_;                // Number of nets.
		int circuitLvl_;            // Circuit level, starting from inputs.

		// Info for multiple time frames.
		int numFrame_;                                 // Number of time frames.
		TIME_FRAME_CONNECT_TYPE timeFrameConnectType_; // Time frame connection type.
		int totalGate_;                                // Number of total gates. Equal to numGate_ * numFrame_.
		int totalLvl_;                                 // Total levels. Equal to circuitLvl_ * numFrame_.

		// Structure
		// **********************************************************************
		// frame 1                frame 2                      frame n
		// |--- ---- ---- --- ----|--- ---- ---- --- ----|     |--- ---- ----
		// |PI1|PPI1|cell|PO1|PPO1|PI2|PPI2|cell|PO2|PPO2| ... |PIn|PPIn|cell|...
		// |--- ---- ---- --- ----|--- ---- ---- --- ----|     |--- ---- ----
		// **********************************************************************

		std::vector<Gate> circuitGates_;        // Gates in the circuit.
		std::vector<int> cellIndexToGateIndex_; // Map cells in the netlist to gates.
		std::vector<int> portIndexToGateIndex_; // Map ports in the netlist to gates.
		std::unordered_map<int, int> abcGateidtoIntGateid_; // transform the abc gate id to consequtive internal gate(for the corresponding INV from aig's complemented Fanin)
		std::unordered_map<int, int> IntGateidtoabcGateid_;
		std::unordered_map<int, int> unorderedIdtoOrderId; // transform the INV-inserted gate id to DFS-ordered gateId
		std::unordered_map<int, int> orderIdtoUnorderedId; // transform the INV-inserted gate id to DFS-ordered gateId


	protected:

		// For circuit building.
		void mapNetlistToCircuit(const int numTotalGates);
		void calculateNumGate(const int numTotalGates);
		void calculateNumNet();
		void createCircuitGates();
		void createCircuitPI(std::vector<Gate>& gateVec);
		void createCircuitPPI();
		void createCircuitComb(std::vector<Gate>& gateVec);
		void insertINV(std::vector<Gate>& gateVec); // insert INV into the origin aig's complemented fanin and relabel the gateid
		void insertINVbetweenPOandNode(const int currentId, Abc_Obj_t* pPO, Abc_Obj_t* pNode);
		void insertINVbetweenNodeandNode(std::vector<Gate>& gateVec, const int NodeId, Abc_Obj_t* pNode1, Abc_Obj_t* pNode2);
		void insertINVbetweenNodeandPI(const int NodeId, int& currentId, Abc_Obj_t* pPi);
		// void createCircuitPmt(const int &gateID, const IntfNs::Cell *const cell,
		//                       const IntfNs::Pmt *const pmt);
		void createCircuitPmt(std::vector<Gate>& gateVec, const int &gateID);
		// void determineGateType(const int &gateID, const IntfNs::Cell *const cell,
		//                        const IntfNs::Pmt *const pmt);
		void determineGateType(std::vector<Gate>& gateVec, const int &gateID, bool isInv);
		void createCircuitPO(std::vector<Gate>& gateVec);
		void createCircuitPPO();
		void connectMultipleTimeFrame();
		void assignMinLevelOfFanins();
		bool umapinsert(std::unordered_map<int, int> &umap1,int key, int value){
			return umap1.insert(std::pair<int,int>(key, value)).second;
			// if(umap1.insert(std::pair<int,int>(key, value)).second == false){
			// 	return value == umap1.at(key);
			// }
			// return 1;
		}
		void DFSreorder(std::vector<Gate>& gateVec);
		void DFS(std::vector<Gate>& gateVec, Gate& _gate, int &tmp);
		void levelize(); // every node's level = MAX{its fanin nodes' level} + 1

	};

	inline Circuit::Circuit()
	{
		// pNetlist_ = NULL;
		pNtk_ = NULL;
		numPI_ = 0;
		numPPI_ = 0;
		numPO_ = 0;
		numComb_ = 0;
		numGate_ = 0;
		numNet_ = 0;
		circuitLvl_ = -1;
		numFrame_ = 1;
		timeFrameConnectType_ = CAPTURE;
	}
};

#endif