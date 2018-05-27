#include "analysis.h"

using namespace std;

namespace FlatCircuit {

  bool allPosedge(const CellDefinition& def) {
    for (auto cid : def.getCellList()) {
      const Cell& cell = def.getCellRefConst(cid);
      CellType tp = cell.getCellType();
      if (isRegister(tp)) {
        if (!cell.clkPosedge()) {
          return false;
        }
      } else {
        // Only support posedge memories for now
        assert((tp == CELL_TYPE_MEM) || !cell.hasPort(PORT_ID_CLK));
      }
    }

    return true;
  }

  dbhc::maybe<PortId> getTrueClockPort(const CellDefinition& def) {
    SignalBit clkDriver{0, 0, 0};
    bool foundClk = false;

    for (auto cid : def.getCellList()) {
      const Cell& cell = def.getCellRefConst(cid);
      CellType tp = cell.getCellType();
      if (isRegister(tp) || (tp == CELL_TYPE_MEM)) {
        auto clkDrivers = cell.getDrivers(PORT_ID_CLK);

        assert(clkDrivers.size() == 1);

        SignalBit tmpDriver = clkDrivers.signals[0];

        if (!foundClk) {
          foundClk = true;
          clkDriver = tmpDriver;
        } else {
          if (tmpDriver != clkDriver) {
            return {};
          }
        }
      } else {
        assert(!cell.hasPort(PORT_ID_CLK));
      }
    }

    if (foundClk) {
      cout << "Found clock " << def.getCellName(clkDriver.cell) << endl;
    }

    const Cell& clkCell = def.getCellRefConst(clkDriver.cell);

    assert(clkCell.isInputPortCell());

    return def.portIdForCell(clkDriver.cell);
  }

}
