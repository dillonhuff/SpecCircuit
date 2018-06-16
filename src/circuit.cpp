#include "circuit.h"

using namespace std;

namespace FlatCircuit {

  bool definitionIsConsistent(const CellDefinition& def) {
    for (auto cellPair : def.getCellMap()) {
      CellId cid = cellPair.first;
      const Cell& c = def.getCellRefConst(cid);

      // This code checks that every driver of every bit of every port of this cell
      // actually exists in the circuit, and that every receiver of every bit of
      // every port of this cell also exists.

      for (auto ptp : c.getPorts()) {

        PortId pid = ptp.first;

        if (c.getPortType(pid) == PORT_TYPE_IN) {

          auto drivers = c.getDrivers(pid);
          for (auto driverBit : drivers.signals) {
            assert(def.hasCell(driverBit.cell));
          }

        } else {

          auto receivers = c.getPortReceivers(pid);
          for (auto sigList : receivers) {
            for (auto sigBit : sigList) {
              if (!def.hasCell(sigBit.cell)) {
                cout << "Error: Receiver " << toString(sigBit) << " of cell " <<
                  def.cellName(cid) << " no longer exists" << endl;
              }

              assert(def.hasCell(sigBit.cell));
            }
          }

        }
      }
    }

    // Here we check to make sure that if a sigbit is in the receivers list
    // of one cell it is not in the receivers list of any other cell
    for (auto cellPair : def.getCellMap()) {
      CellId cid = cellPair.first;
      const Cell& c = def.getCellRefConst(cid);

      set<SignalBit> allSignalBits;
      
      for (auto ptp : c.getPorts()) {
        PortId pid = ptp.first;

        if (c.getPortType(pid) == PORT_TYPE_OUT) {

          auto receivers = c.getPortReceivers(pid);
          for (auto sigList : receivers) {
            for (auto sigBit : sigList) {
              if (elem(sigBit, allSignalBits)) {
                cout << "Error: signal bit " << toString(def, sigBit) << " is already driven!" << endl;
                assert(false);
              }
              allSignalBits.insert(sigBit);
            }
          }
          
        }
      }
    }


    return true;
  }


  std::vector<SigPort>
  combinationalDependencies(const Cell& cell,
                            const PortId pid,
                            const CellDefinition& def) {
    vector<SigPort> deps;
    
    for (auto sigPort : cell.receiverSigPorts(pid)) {
      const Cell& receiverCell = def.getCellRefConst(sigPort.cell);

      if ((receiverCell.getCellType() == CELL_TYPE_MEM) &&
          (sigPort.port != PORT_ID_RADDR)) {
        continue;
      }

      if (!isRegister(receiverCell.getCellType()) &&
          !((receiverCell.getCellType() == CELL_TYPE_MEM) &&
            (sigPort.port == PORT_ID_RADDR)) &&
          (sigPort.port != PORT_ID_ARST) &&
          (sigPort.port != PORT_ID_CLK)) {

        deps.push_back(sigPort);

      }
    }

    return deps;
  }

  std::vector<SigPort>
  sequentialDependencies(const Cell& cell,
                         const PortId pid) {
    vector<SigPort> deps;
    
    for (auto sigPort : cell.receiverSigPorts(pid)) {
      if ((sigPort.port == PORT_ID_ARST) ||
          (sigPort.port == PORT_ID_CLK)) {

        deps.push_back(sigPort);

      }
    }

    return deps;
  }

}
