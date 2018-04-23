#pragma once

#include "circuit.h"

namespace FlatCircuit {

  void foldConstants(CellDefinition& def);

  void deleteDeadInstances(CellDefinition& def);

}
