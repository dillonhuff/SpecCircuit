#include "transformations.h"

#include "simulator.h"

using namespace std;

namespace FlatCircuit {


  static inline BitVector doUnop(const CellType tp,
                                 const BitVector& in) {
    switch (tp) {
    case CELL_TYPE_PASSTHROUGH:
      return in;

    case CELL_TYPE_ORR:
      return orr(in);

    case CELL_TYPE_ANDR:
      return andr(in);

    case CELL_TYPE_NOT:
      return ~in;

    default:
      std::cout << "Error: Unsupported unop = " << toString(tp) << std::endl;
      assert(false);
    }
  }

  static inline BitVector doBinop(const CellType tp,
                                  const BitVector& in0,
                                  const BitVector& in1) {
    switch (tp) {
    case CELL_TYPE_EQ:
      return BitVector(1, in0 == in1);

    case CELL_TYPE_NEQ:
      return BitVector(1, in0 != in1);

    case CELL_TYPE_ULT:
      return BitVector(1, in0 < in1);

    case CELL_TYPE_UGE:
      return BitVector(1, (in0 > in1) || (in0 == in1));

    case CELL_TYPE_ULE:
      return BitVector(1, (in0 < in1) || (in0 == in1));

    case CELL_TYPE_SHL:
      return shl(in0, in1);

    case CELL_TYPE_ASHR:
      return ashr(in0, in1);

    case CELL_TYPE_LSHR:
      return lshr(in0, in1);

    case CELL_TYPE_SUB:
      return sub_general_width_bv(in0, in1);
      
    case CELL_TYPE_XOR:
      return in0 ^ in1;
      
    case CELL_TYPE_OR:
      return in0 | in1;

    case CELL_TYPE_AND:
      return in0 & in1;

    case CELL_TYPE_ADD:
      return add_general_width_bv(in0, in1);

    case CELL_TYPE_MUL:
      return mul_general_width_bv(in0, in1);
      
    default:
      std::cout << "Error: Unsupported binop = " << toString(tp) << std::endl;
      assert(false);
    }
  }

  // Get the output of this register if its output drives its input
  // otherwise return Nothing
  maybe<BitVector> materializeRegisterInput(const CellId cid,
                                            CellDefinition& def,
                                            const BitVector& value) {
    const Cell& cell = def.getCellRefConst(cid);
    assert(isRegister(cell.getCellType()));

    auto drivers = cell.getDrivers(PORT_ID_IN);

    assert(value.bitLength() == drivers.signals.size());
    
    for (int offset = 0; offset < drivers.signals.size(); offset++) {
      SignalBit driverBit = drivers.signals[offset];

      SignalBit outputValue{cid, PORT_ID_OUT, offset};

      if (driverBit != outputValue) {
        return {};
      }
    }

    return value;
  }

  maybe<BitVector> materializeConstPort(const SigPort sp,
                                        CellDefinition& def) {

    CellId cid = sp.cell;
    Cell& cell = def.getCellRef(cid);
    PortId pid = sp.port;

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

        bv.set(offset, driverValue.get(driver.offset));

      } else {
        return {};
      }
    }

    return bv;
  }

  maybe<BitVector> getOutput(const CellId cid,
                             CellDefinition& def,
                             const std::map<CellId, BitVector>& registerValues) {
    const Cell& nextCell = def.getCellRef(cid);
    if (!nextCell.hasPort(PORT_ID_OUT)) {
      return {};
    }

    CellType tp = nextCell.getCellType();

    if (tp == CELL_TYPE_ZEXT) {

      maybe<BitVector> inm = materializeConstPort({cid, PORT_ID_IN}, def);

      if (inm.has_value()) {

        BitVector in = inm.get_value();

        int outWidth = nextCell.getPortWidth(PORT_ID_OUT);
        BitVector res(outWidth, 0);
        for (uint i = 0; i < in.bitLength(); i++) {
          res.set(i, in.get(i));
        }
        
        BitVector newOut = res;
        return newOut;
      }

      return {};

    } else if (tp == CELL_TYPE_SLICE) {
      maybe<BitVector> inm = materializeConstPort({cid, PORT_ID_IN}, def);

      if (inm.has_value()) {
        BitVector in = inm.get_value();

        uint lo = nextCell.getParameterValue(PARAM_LOW).to_type<int>();
        uint hi = nextCell.getParameterValue(PARAM_HIGH).to_type<int>();

        assert((hi - lo) > 0);

        BitVector res(hi - lo, 0);
        for (uint i = lo; i < hi; i++) {
          res.set(i - lo, in.get(i));
        }

        return res;
      }

      return {};
      
    } else if (isBinop(tp)) {
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
    } else if (tp == CELL_TYPE_REG_ARST) {
      maybe<BitVector> in0m =
        materializeRegisterInput(cid, def, map_find(cid, registerValues));
      maybe<BitVector> rstm = materializeConstPort({cid, PORT_ID_ARST}, def);

      if (in0m.has_value() &&
          rstm.has_value()) {
        BitVector in0 = in0m.get_value();

        return in0;
      }

      return {};
    } else if (tp == CELL_TYPE_REG) {
      maybe<BitVector> in0m =
        materializeRegisterInput(cid, def, map_find(cid, registerValues));

      if (in0m.has_value()) {
        BitVector in0 = in0m.get_value();

        return in0;
      }

      return {};
      
    } else if (isUnop(tp)) {

      maybe<BitVector> inm = materializeConstPort({cid, PORT_ID_IN}, def);

      if (inm.has_value()) {
        BitVector in = inm.get_value();

        BitVector out = doUnop(nextCell.getCellType(), in);
        return out;
      }

      return {};
    }

    cout << "Error: Unsupported cell " << def.getCellName(cid) << " : " << toString(nextCell.getCellType()) << endl;
    assert(false);
  }

  void foldConstants(CellDefinition& def,
                     const std::map<CellId, BitVector>& registerValues) {

    set<CellId> candidates;
    for (auto cellPair : def.getCellMap()) {
      if (cellPair.second.getCellType() == CELL_TYPE_CONST) {
        for (auto sigBus : cellPair.second.getPortReceivers(PORT_ID_OUT)) {
          for (auto sigBit : sigBus) {
            candidates.insert(sigBit.cell);
          }
        }
      }
    }

    int iterCount = 0;
    while (candidates.size() > 0) {

      if (iterCount % 100) {
        cout << "# of candidates = " << candidates.size() << endl;
      }
      iterCount++;

      CellId next = *std::begin(candidates);
      candidates.erase(next);

      //cout << "Checking cell " << def.cellName(next) << endl;

      Cell& nextCell = def.getCellRef(next);

      if (nextCell.getCellType() == CELL_TYPE_MUX) {
        maybe<BitVector> bv = materializeConstPort({next, PORT_ID_SEL}, def);

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

            if (outReceivers.size() != inDrivers.size()) {
              cout << "Error while folding " << def.getCellName(next) << ": outReceivers.size() = " << outReceivers.size() << ", but inDrivers = " << inDrivers.size() << endl;

              
            }
            assert(outReceivers.size() == inDrivers.size());
            
            for (int offset = 0; offset < width; offset++) {
              SignalBit newDriver = inDrivers[offset];
              auto offsetReceivers = outReceivers[offset];

              for (auto receiverBit : offsetReceivers) {
                def.setDriver(receiverBit, newDriver);
              }
            }

            nextCell.clearReceivers(PORT_ID_OUT);

            def.deleteCell(next);
            candidates.erase(next);
          }
        }
      } else {
        maybe<BitVector> bv = getOutput(next, def, registerValues);

        if (bv.has_value()) {
          for (auto sigBus : nextCell.getPortReceivers(PORT_ID_OUT)) {
            for (auto sigBit : sigBus) {
              candidates.insert(sigBit.cell);
            }
          }

          def.replaceCellPortWithConstant(next, PORT_ID_OUT, bv.get_value());
          def.deleteCell(next);
          candidates.erase(next);

        }
      }
    }

    assert(candidates.size() == 0);
  }

  // TODO: Change this to a scan algorithm that deletes in large batches
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
          def.deleteCell(cellPair.first);
          deleted = true;
          break;
        }

      }
    }
    
  }

}
