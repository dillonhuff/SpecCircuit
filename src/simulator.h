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
          
        }

        // Seq updates
      } while (combChanges.size() > 0);

      std::cout << "Done with update" << std::endl;

      assert(combChanges.size() == 0);
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
