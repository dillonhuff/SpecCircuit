#pragma once

#include "simulator.h"

namespace FlatCircuit {

  typedef bsim::quad_value_bit_vector BitVec;

  typedef std::vector<std::pair<unsigned int, unsigned int> > CGRABitStream;

  void loadCGRAConfig(const CGRABitStream& configValues,
                      Simulator& sim);

  void loadMemoryTileConfig(const CGRABitStream& configValues,
                            Simulator& sim);

  void reset(const std::string& rstName, Simulator& sim);

  template<typename SimulatorType>
  void posedge(const std::string& clkName, SimulatorType& sim) {
    sim.setFreshValue(clkName, BitVec(1, 0));
    sim.update();
    sim.setFreshValue(clkName, BitVec(1, 1));
    sim.update();
  }

  //void posedge(const std::string& clkName, Simulator& sim);

  void negedge(const std::string& clkName, Simulator& sim);

  BitVector getCGRAOutput(const int sideNo, Simulator& sim);

  void printCGRAOutputs(Simulator& sim);

  void printCGRAInputs(Simulator& sim);

  void setCGRAInput(const BitVector& input, Simulator& sim);

  void setCGRAInput(const int side, const BitVector& input, Simulator& sim);

  void setCGRAInputTwoState(const int side,
                            const int width,
                            const unsigned long value,
                            Simulator& sim);

  std::vector<std::string> splitString(const std::string& str,
                                       const std::string& delimiter);
  
}
