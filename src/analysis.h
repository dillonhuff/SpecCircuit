#pragma once

#include "circuit.h"

namespace FlatCircuit {

  dbhc::maybe<PortId> getTrueClockPort(const CellDefinition& def);

  bool allPosedge(const CellDefinition& def);

}
