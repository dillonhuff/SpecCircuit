#pragma once

// 25 at 2:36
#include "circuit.h"
#include "value_store.h"

namespace FlatCircuit {

  void foldConstants(CellDefinition& def,
                     const std::map<CellId, BitVector>& registerValues);

  void foldConstantsWRTState(CellDefinition& def,
                             const ValueStore& valueStore);
  
  void deleteDeadInstances(CellDefinition& def);

  void cullZexts(CellDefinition& def);

  void removeConstDuplicates(CellDefinition& def);

  void allInputsToConstants(CellDefinition& def);

  void cullPassthroughs(CellDefinition& def);

  void deDuplicate(CellDefinition& def);

}
