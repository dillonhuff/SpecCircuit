#include "value_store.h"

#include "ir.h"

using namespace std;
using namespace bsim;

namespace FlatCircuit {

  ValueStore::ValueStore(CellDefinition& def_) : compiledRaw(false), def(def_) {
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (isRegister(cell.getCellType())) {
        // BitVector bv = unknown_bv(cell.getPortWidth(PORT_ID_OUT));
        // unsigned long nextInd = simValueTable.size();
        // registerOffsets[cid] = nextInd;
        // simValueTable.push_back(bv);

        registerOffsets[cid] =
          simValueTable.addBitVector(cell.getPortWidth(PORT_ID_OUT));
      }

      for (auto port : cell.outputPorts()) {
        portOffsets[{cid, port}] =
          simValueTable.addBitVector(cell.getPortWidth(port));
        
        // BitVector bv = unknown_bv(cell.getPortWidth(port));
        // unsigned long nextInd = simValueTable.size();
        // portOffsets[{cid, port}] = nextInd;
        // simValueTable.push_back(bv);

        if (sequentialDependencies(cell, port).size() > 0) {
          pastValueOffsets[{cid, port}] =
            simValueTable.addBitVector(cell.getPortWidth(port));

          // BitVector bv = unknown_bv(cell.getPortWidth(port));
          // unsigned long nextInd = simValueTable.size();
          // pastValueOffsets[{cid, port}] = nextInd;
          // simValueTable.push_back(bv);
        }
      }

      if (cell.isOutputPortCell()) {
        portOffsets[{cid, port}] =
          simValueTable.addBitVector(cell.getPortWidth(PORT_ID_IN));

        // BitVector bv = unknown_bv(cell.getPortWidth(PORT_ID_IN));
        // unsigned long nextInd = simValueTable.size();
        // portOffsets[{cid, PORT_ID_IN}] = nextInd;
        // simValueTable.push_back(bv);
      }

      if (cell.getCellType() == CELL_TYPE_MEM) {
        int memWidth = cell.getMemWidth();
        int memDepth = cell.getMemDepth();

        //BitVector defaultValue(memWidth, 0);

        //unsigned long nextInd = simValueTable.size();
        memoryOffsets[cid] =
          simValueTable.addBitVector(cell.getPortWidth(memWidth));

        for (unsigned long i = 1; i < (unsigned long) memDepth; i++) {
          simValueTable.addBitVector(cell.getPortWidth(memWidth));
          //simValueTable.push_back(defaultValue);
        }
      }
    }
  }

  void ValueStore::debugPrintTableValues() const {
    // cout << "Table values" << endl;
    // for (int i = 0; i < (int) simValueTable.size(); i++) {
    //   cout << "\t" << i << " = " << simValueTable.getBitVector(i) << endl;
    // }
  }

  std::vector<IRInstruction*>
  ValueStore::codeToMaterializeOffset(const CellId cid,
                                      const PortId pid,
                                      const std::string& argName,
                                      const std::map<SigPort, unsigned long>& offsets,
                                      const bool isPast) const {
    const Cell& cell = def.getCellRefConst(cid);
    auto drivers = cell.getDrivers(pid);
    vector<IRInstruction*> instrs;

    bool canDirectCopy = true;
    CellId singleDriverCell = 0;
    PortId singleDriverPort = 0;
    set<CellId> driverCells;

    for (int offset = 0; offset < (int) drivers.signals.size(); offset++) {
      SignalBit driverBit = drivers.signals[offset];
      driverCells.insert(driverBit.cell);

      singleDriverCell = driverBit.cell;
      singleDriverPort = driverBit.port;

      if (driverCells.size() > 1) {
        canDirectCopy = false;
        break;
      }

      if (driverBit.offset != offset) {
        canDirectCopy = false;
        break;
      }
    }

    if (def.getCellRefConst(singleDriverCell).getPortWidth(singleDriverPort) !=
        cell.getPortWidth(pid)) {
      canDirectCopy = false;
    }

    SigPort sp = {singleDriverCell, singleDriverPort};
    if (canDirectCopy && !contains_key(sp, offsets)) {
      cout << "Error: " << sigPortString(def, sp) << endl;
      assert(contains_key(sp, offsets));
    }

    if (canDirectCopy) {
      instrs.push_back(new IRPortLoad(argName, sp.cell, sp.port, isPast));
      // instrs.push_back(new IRAssign(argName,
      //                               "values[" +
      //                               to_string(map_find(sp, offsets)) +
      //                               "]"));
    } else {

      for (int offset = 0; offset < (int) drivers.signals.size(); offset++) {
        SignalBit driverBit = drivers.signals[offset];
        //string valString = "values[" + to_string(map_find({driverBit.cell, driverBit.port}, offsets)) + "].get(" + to_string(driverBit.offset) + ")";

        //instrs.push_back(new IRSetBit(argName, offset, driverBit, isPast)); //offset, valString));
        instrs.push_back(new IRSetBit(argName, cid, pid, offset, driverBit, isPast)); //offset, valString));
      }
    }

    return instrs;
    
  }

  std::vector<IRInstruction*>
  ValueStore::codeToMaterialize(const CellId cid,
                                            const PortId pid,
                                            const std::string& argName) const {
    return codeToMaterializeOffset(cid, pid, argName, portOffsets, false);
  }

  IRInstruction* ValueStore::codeToAssignRegister(const CellId cid,
                                                  const std::string& assignCode) {
    return new IRRegisterStore(cid, assignCode);
    // return
    //   new IRAssign("values[" + std::to_string(map_find(cid, registerOffsets)) + "]",
    //                assignCode);
  }
    

  void ValueStore::debugPrintRawValueTable() const {
    cout << "value table" << endl;
    for (unsigned long i = 0; i < rawTableSize; i++) {
      cout << "\trawTable[" << i << "] = " << (int) rawSimValueTable[i] << endl;
    }
  }
}
