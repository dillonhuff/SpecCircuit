#pragma once

#include "circuit.h"

namespace FlatCircuit {

  void foldConstants(CellDefinition& def,
                     const std::map<CellId, BitVector>& registerValues);

  void deleteDeadInstances(CellDefinition& def);

  void cullZexts(CellDefinition& def);

  void removeConstDuplicates(CellDefinition& def);

  void allInputsToConstants(CellDefinition& def);

  void cullPassthroughs(CellDefinition& def);

  void deDuplicate(CellDefinition& def);

}
