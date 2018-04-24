#include "transformations.h"

using namespace std;

namespace FlatCircuit {

  maybe<BitVector> materializeConstPort(const SigPort sp,
                                        CellDefinition& def) {
    return {};
  }

  BitVector doBinop(const CellType tp,
                    const BitVector& in0,
                    const BitVector& in1) {
    switch (tp) {
    case CELL_TYPE_EQ:
      return BitVector(1, in0 == in1);
    default:
      assert(false);
    }
  }
  
  maybe<BitVector> getOutput(const CellId cid,
                             CellDefinition& def) {
    const Cell& nextCell = def.getCellRef(cid);
    
    if (!nextCell.hasPort(PORT_ID_OUT)) {
      return {};
    }

    if (isBinop(nextCell.getCellType())) {
      maybe<BitVector> in0m = materializeConstPort({cid, PORT_ID_IN0}, def);
      maybe<BitVector> in1m = materializeConstPort({cid, PORT_ID_IN1}, def);

      if (in0m.has_value() &&
          in1m.has_value()) {
        BitVector in0 = in0m.get_value();
        BitVector in1 = in1m.get_value();

        BitVector out = doBinop(nextCell.getCellType(), in0, in1);
        return out;
      }

      return {};
    }

    cout << "Error: Unsupported cell " << def.getCellName(cid) << " : " << toString(nextCell.getCellType()) << endl;
    assert(false);
  }

  maybe<BitVector> getInput(Cell& cell,
                            const PortId pid,
                            CellDefinition& def) {
    BitVector bv(cell.getPortWidth(pid), 0);

    auto& drivers = cell.getDrivers(pid);
    for (int offset = 0; offset < drivers.signals.size(); offset++) {
      SignalBit driver = drivers.signals.at(offset);
      if (notEmpty(driver)) {
        CellType driverTp = def.getCellRef(driver.cell).getCellType();
        if (driverTp != CELL_TYPE_CONST) {
          return {};
        }

        assert(driver.port == PORT_ID_OUT);

        Cell& driverCell = def.getCellRef(driver.cell);
        BitVector driverValue = driverCell.getParameterValue(PARAM_INIT_VALUE);

        bv.set(offset, driverValue.get(offset));
      } else {
        return {};
      }
    }

    return bv;
  }

  void foldConstants(CellDefinition& def) {
    set<CellId> candidates;
    for (auto cellPair : def.getCellMap()) {
      if (cellPair.second.getCellType() == CELL_TYPE_CONST) {
        cout << "Found const cell" << endl;
        for (auto sigBus : cellPair.second.getPortReceivers(PORT_ID_OUT)) {
          for (auto sigBit : sigBus) {
            candidates.insert(sigBit.cell);
          }
        }
      }
    }

    while (candidates.size() > 0) {
      cout << "# of candidates = " << candidates.size() << endl;

      CellId next = *std::begin(candidates);
      candidates.erase(next);

      Cell& nextCell = def.getCellRef(next);

      if (nextCell.getCellType() == CELL_TYPE_MUX) {
        maybe<BitVector> bv = getInput(nextCell, PORT_ID_SEL, def);

        if (bv.has_value()) {
          BitVector bitVec = bv.get_value();

          assert(bitVec.bitLength() == 1);

          // Maybe this isnt needed? If x value constant just pick one?
          if (bitVec.get(0).is_binary()) {

            for (auto sigBus : nextCell.getPortReceivers(PORT_ID_OUT)) {
              for (auto sigBit : sigBus) {
                candidates.insert(sigBit.cell);
              }
            }

            bool selIn1 = bitVec.get(0).binary_value() == 1;

            PortId inPort = selIn1 ? PORT_ID_IN1 : PORT_ID_IN0;
            int width = nextCell.getPortWidth(inPort);

            vector<SignalBit> inDrivers = nextCell.getDrivers(inPort).signals;
            auto& outReceivers = nextCell.getPortReceivers(PORT_ID_OUT);

            assert(outReceivers.size() == inDrivers.size());
            
            for (int offset = 0; offset < width; offset++) {
              SignalBit newDriver = inDrivers[offset];
              auto offsetReceivers = outReceivers[offset];

              for (auto receiverBit : offsetReceivers) {
                def.setDriver(receiverBit, newDriver);
              }
            }

            nextCell.clearReceivers(PORT_ID_OUT);
          }
        }
      } else {
        maybe<BitVector> bv = getOutput(next, def);

        if (bv.has_value()) {
          for (auto sigBus : nextCell.getPortReceivers(PORT_ID_OUT)) {
            for (auto sigBit : sigBus) {
              candidates.insert(sigBit.cell);
            }
          }

          def.replaceCellPortWithConstant(next, PORT_ID_OUT, bv.get_value());
        
        }
      }
    }

    assert(candidates.size() == 0);
  }

  void deleteDeadInstances(CellDefinition& def) {
    bool deleted = true;
    while (deleted) {
      deleted = false;
      for (auto cellPair : def.getCellMap()) {

        Cell& cell = cellPair.second;
        bool allOutputsHaveNoReceivers = true;

        int numOutputPorts = 0;

        for (auto portPair : cell.getPorts()) {
          PortId port = portPair.first;

          if (cell.getPortType(port) == PORT_TYPE_OUT) {
            numOutputPorts++;

            for (auto sigBus : cell.getPortReceivers(port)) {
              for (auto sigBit : sigBus) {
                if (notEmpty(sigBit)) {
                  allOutputsHaveNoReceivers = false;
                  break;
                }
              }

              if (!allOutputsHaveNoReceivers) {
                break;
              }
            }
          }

          if (!allOutputsHaveNoReceivers) {
            break;
          }
        }

        if ((numOutputPorts > 0) && allOutputsHaveNoReceivers && !def.isPortCell(cellPair.first)) {
          cout << "Deleting " << def.cellName(cellPair.first) << endl;
          def.deleteCell(cellPair.first);
          deleted = true;
          break;
        }

      }
    }
    
  }

}
