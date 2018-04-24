#pragma once

#include "circuit.h"

namespace FlatCircuit {

  void foldConstants(CellDefinition& def,
                     const std::map<CellId, BitVector>& registerValues);

  void deleteDeadInstances(CellDefinition& def);


}
