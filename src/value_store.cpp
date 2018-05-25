#include "value_store.h"

#include "ir.h"

using namespace std;

namespace FlatCircuit {

  void ValueStore::debugPrintTableValues() const {
    cout << "Table values" << endl;
    for (int i = 0; i < (int) simValueTable.size(); i++) {
      cout << "\t" << i << " = " << simValueTable.at(i) << endl;
    }
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
    CellId singleDriverCell;
    PortId singleDriverPort;
    set<CellId> driverCells;

    for (int offset = 0; offset < drivers.signals.size(); offset++) {
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
      instrs.push_back(new IRAssign(argName,
                                    "values[" +
                                    to_string(map_find(sp, offsets)) +
                                    "]"));
    } else {

      for (int offset = 0; offset < drivers.signals.size(); offset++) {
        SignalBit driverBit = drivers.signals[offset];
        //string valString = "values[" + to_string(map_find({driverBit.cell, driverBit.port}, offsets)) + "].get(" + to_string(driverBit.offset) + ")";

        instrs.push_back(new IRSetBit(argName, offset, driverBit, isPast)); //offset, valString));
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
    
  
}
