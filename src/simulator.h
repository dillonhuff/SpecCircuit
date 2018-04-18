#pragma once

#include "circuit.h"

namespace FlatCircuit {

  class SigPort {
  public:
    CellId cell;
    PortId port;
  };

  static inline bool operator<(const SigPort a, const SigPort b) {
    if (a.cell > b.cell) {
      return false;
    }

    if (a.cell == b.cell) {
      return a.port < b.port;
    }

    return true;
  }

  class Simulator {

    CellDefinition& def;

    std::map<SigPort, BitVector> portValues;
    std::set<SigPort> userInputs;
    std::set<SigPort> combChanges;
    std::set<SigPort> seqChanges;
    
    
  public:

    Simulator(Env& e_, CellDefinition& def_) : def(def_) {

      std::cout << "Start init" << std::endl;
      for (auto c : def.getCellMap()) {
        auto tp = c.second.getCellType();

        CellId cid = c.first;
        Cell cl = c.second;


        if (tp == CELL_TYPE_CONST) {
          BitVector initVal = cl.getParameterValue(PARAM_INIT_VALUE);
          portValues[{cid, PORT_ID_OUT}] = initVal;

          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }
        } else if (tp == CELL_TYPE_PORT) {
          if (cl.hasPort(PORT_ID_OUT)) {
            int width = cl.getParameterValue(PARAM_OUT_WIDTH).to_type<int>();
            BitVector initVal = bsim::unknown_bv(width);
            portValues[{cid, PORT_ID_OUT}] = initVal;

            const Cell& c = def.getCellRef(cid);
            for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
              for (auto& sigBit : receiverBus) {
                combChanges.insert({sigBit.cell, sigBit.port});
              }
            }

          }
        } else if (isBinop(tp) || isUnop(tp) || (tp == CELL_TYPE_MUX) || (tp == CELL_TYPE_REG_ARST)) {

          int width = cl.getParameterValue(PARAM_WIDTH).to_type<int>();
          BitVector initVal = bsim::unknown_bv(width);
          portValues[{cid, PORT_ID_OUT}] = initVal;

          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }

        } else {
          std::cout << "No initialization for cell type " << toString(tp) << std::endl;
          assert(false);
        }
      }

      std::cout << "End init" << std::endl;
    }

    void update() {
      std::cout << "Starting with update" << std::endl;
      
      // Add user inputs to combChanges
      for (auto in : userInputs) {
        const Cell& c = def.getCellRef(in.cell);
        for (auto& receiverBus : c.getPortReceivers(in.port)) {
          for (auto& sigBit : receiverBus) {
            combChanges.insert({sigBit.cell, sigBit.port});
          }
        }

        //combChanges.insert(in);
      }

      userInputs = {};

      std::cout << "Comb values than changed" << std::endl;
      for (auto val : combChanges) {
        std::cout << "\t" << def.getCellName(val.cell) << ", " << portIdString(val.port) << std::endl;
      }

      std::cout << "Done with values" << std::endl;

      // do while updates
      do {

        // Comb updates
        while (combChanges.size() > 0) {
          SigPort nextComb = *std::begin(combChanges);
          combChanges.erase(nextComb);

          bool changed = updatePort(nextComb);

          if (changed) {
            const Cell& c = def.getCellRef(nextComb.cell);
            for (auto& receiverBus : c.getPortReceivers(nextComb.port)) {
              for (auto& sigBit : receiverBus) {
                combChanges.insert({sigBit.cell, sigBit.port});
              }
            }
          }

        }

        // TODO: Add sequential updates
      } while (combChanges.size() > 0);

      std::cout << "Done with update" << std::endl;

      assert(combChanges.size() == 0);
    }

    bool updatePort(const SigPort sigPort) {
      CellType tp = def.getCellRef(sigPort.cell).getCellType();
      if ((tp == CELL_TYPE_PORT) || (tp == CELL_TYPE_CONST)) {
        return false;
      } else if (tp == CELL_TYPE_MUX) {
        
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});
        BitVector sel = materializeInput({sigPort.cell, PORT_ID_SEL});

        BitVector oldOut = getBitVec(sigPort.cell, sigPort.port);

        assert(sel.bitLength() == 1);

        BitVector newOut = oldOut;
        return !same_representation(oldOut, newOut);
      } else {
        std::cout << "No update for cell type " << toString(tp) << std::endl;
        assert(false);
      }

      
    }

    BitVector materializeInput(const SigPort sigPort) {
      int width = def.getCellRef(sigPort.cell).getPortWidth(sigPort.port);

      BitVector val(width, 0);

      auto& sigBus = def.getCellRef(sigPort.cell).getDrivers(sigPort.port);

      assert(sigBus.signals.size() == width);

      for (int i = 0; i < (int) sigBus.signals.size(); i++) {
        SignalBit b = sigBus.signals.at(i);
        val.set(i, getBitVec(b.cell, b.port).get(b.offset));
      }
        
      return val;
    }

    void setFreshValue(const std::string& cellName,
                       const PortId id,
                       const BitVector& bv) {
      CellId cid = def.getPortCellId(cellName);
      setFreshValue(cid, id, bv);
    }

    void setFreshValue(const CellId cid,
                       const PortId pid,
                       const BitVector& bv) {
      portValues[{cid, pid}] = bv;
      userInputs.insert({cid, pid});
    }

    BitVector getBitVec(const CellId cid,
                        const PortId pid) {
      assert(contains_key({cid, pid}, portValues));
      return portValues.at({cid, pid});
    }    

    BitVector getBitVec(const std::string& cellName,
                        const PortId id) {
      CellId cid = def.getPortCellId(cellName);
      return getBitVec(cid, id);
    }

  };
}
