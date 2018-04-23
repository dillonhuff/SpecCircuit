#include "transformations.h"

using namespace std;

namespace FlatCircuit {

  maybe<BitVector> getOutput(Cell& nextCell,
                             CellDefinition& def) {
    return {};
  }

  void foldConstants(CellDefinition& def) {
    set<CellId> candidates;
    

    while (candidates.size() > 0) {
      CellId next = *std::begin(candidates);
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
