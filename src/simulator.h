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
        if (c.second.getCellType() == CELL_TYPE_CONST) {
          // TODO: Insert real values
          CellId cid = c.first;
          Cell cl = c.second;

          BitVector initVal = cl.getParameterValue(PARAM_INIT_VALUE);
          portValues[{cid, PORT_ID_OUT}] = initVal;
          combChanges.insert({cid, PORT_ID_OUT});
        }
      }

      std::cout << "End init" << std::endl;
    }

    void update() {
      std::cout << "Starting with update" << std::endl;
      
      // Add user inputs to combChanges
      for (auto in : userInputs) {
        combChanges.insert(in);
      }

      userInputs = {};

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

        BitVector oldOut = portValues.at(sigPort);

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
        val.set(i, portValues.at({b.cell, b.port}).get(b.offset));
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
      return portValues.at({cid, pid});
    }    

    BitVector getBitVec(const std::string& cellName,
                        const PortId id) {
      CellId cid = def.getPortCellId(cellName);
      return getBitVec(cid, id);
    }

  };
}
