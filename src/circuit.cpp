#include "circuit.h"

using namespace std;

namespace FlatCircuit {

  bool definitionIsConsistent(const CellDefinition& def) {
    for (auto cellPair : def.getCellMap()) {
      CellId cid = cellPair.first;
      const Cell& c = def.getCellRefConst(cid);

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
                cout << "Error: Receiver " << toString(sigBit) << " no longer exists" << endl;
              }

              assert(def.hasCell(sigBit.cell));
            }
          }

        }
      }
    }

    return true;
  }

}
