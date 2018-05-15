#pragma once

#include "circuit.h"

namespace FlatCircuit {

  dbhc::maybe<PortId> getTrueClockPort(CellDefinition& def);

}
