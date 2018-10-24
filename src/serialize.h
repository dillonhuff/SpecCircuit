#pragma once

#include "circuit.h"

namespace FlatCircuit {

  void saveToFile(const Env& e,
                  const CellDefinition& def,
                  const std::string& fileName);

  void loadFromFile(Env& e,
                    const std::string& fileName);
  
}
