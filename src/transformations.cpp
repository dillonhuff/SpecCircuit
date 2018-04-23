#include "transformations.h"

using namespace std;

namespace FlatCircuit {

  maybe<BitVector> getOutput(Cell& nextCell,
                             CellDefinition& def) {
    
    return {};
  }

  void foldConstants(CellDefinition& def) {
    set<CellId> candidates;
    for (auto cellPair : def.getCellMap()) {
      if (cellPair.first == CELL_TYPE_CONST) {
        for (auto sigBus : cellPair.second.getPortReceivers(PORT_ID_OUT)) {
          for (auto sigBit : sigBus) {
            candidates.insert(sigBit.cell);
          }
        }
      }
    }

    while (candidates.size() > 0) {
      CellId next = *std::begin(candidates);
      candidates.erase(next);

      Cell& nextCell = def.getCellRef(next);

      maybe<BitVector> bv = getOutput(nextCell, def);

      if (bv.has_value()) {
        for (auto sigBus : nextCell.getPortReceivers(PORT_ID_OUT)) {
          for (auto sigBit : sigBus) {
            candidates.insert(sigBit.cell);
          }
        }

        def.replaceCellPortWithConstant(next, PORT_ID_OUT, bv.get_value());
        
      }
    }

    assert(candidates.size() == 0);
  }

}
