#pragma once

#include "coreir.h"
#include "coreir/libs/rtlil.h"
#include "coreir/libs/commonlib.h"

#include "circuit.h"

namespace FlatCircuit {

  Env convertFromCoreIR(CoreIR::Context* const c,
                        CoreIR::Module* const top);
  
  Env loadFromCoreIR(const std::string& topName,
                     const std::string& fileName);

  std::vector<std::pair<unsigned int, unsigned int> >
  loadBitStream(const std::string& fileName);
  
}
