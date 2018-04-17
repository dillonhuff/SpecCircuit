#pragma once

#include "coreir.h"

namespace FlatCircuit {

  enum Parameter {
    PARAM_IN0_WIDTH,
    PARAM_IN1_WIDTH,
    PARAM_OUT_WIDTH,
    PARAM_SEL_WIDTH,
    PARAM_CLK_POSEDGE,
    PARAM_CLK_NEGEDGE,
    PARAM_INIT_VALUE
  };

  typedef uint64_t CellType;
  typedef uint64_t CellId;
  typedef uint64_t PortId;

  enum PortType {
    PORT_TYPE_IN,
    PORT_TYPE_OUT
  };

  class Port {
  public:
    int width;
    PortType type;
  };

  class SignalBit {
  public:
    CellId cell;
    PortId port;
    int offset;
  };

  class SignalBus {
  public:
    std::vector<SignalBit> signals;
  };

  class Cell {
  protected:
    std::map<Parameter, BitVector> parameters;
    CellType cellType;
    std::map<PortId, Port> portWidths;
    std::map<PortId, SignalBus> drivers;
    std::map<PortId, std::vector<std::vector<SignalBit> > > receivers;

  public:
    Cell() {}

    Cell(const CellType cellType_,
         const std::map<Parameter, BitVector> & parameters_) {}

    BitVector getParameterValue(const Parameter val) const {
      return parameters.at(val);
    }

    int getPortWidth(const PortId port) const {
      return portWidths.at(port).width;
    }

    int getPortType(const PortId port) const {
      return portWidths.at(port).type;
    }
    
    CellType getCellType() const {
      return cellType;
    }

    void addReceiver(const PortId port, const int offset, const SignalBit receiver) {
      receivers[port][offset].push_back(receiver);
    }
    
    void setDriver(const PortId port, const int offset, const SignalBit driver) {
      drivers[port].signals[offset] = driver;
    }
  };

  // Random: Maybe each port on each cell should have its own unique identifier?

  class CellDefinition {
    std::map<CellId, Cell> cells;
    std::map<std::string, PortId> portNames;

    // How to represent connections? Map from ports to drivers? Map
    // from PortIds to receivers as well?

    // Q: What do I want to do with this code?
    // A: For a given port get all receiver ports (and offsets)
    //    For a given port get the list of driver ports (and offsets)

  public:

    void setDriver(const SignalBit receiver,
                   const SignalBit driver) {
      cells[receiver.cell].setDriver(receiver.port, receiver.offset, driver);
      cells[driver.cell].addReceiver(driver.port, driver.offset, receiver);
    }
  };

  // Problem: How to deal with module parameters?
  // I cant just store one piece of code that represents
  // each cell type. I have to store a different piece of code
  // (or data structure that represents how to simulate the cell)
  // for each cell type based on each parameter value, 16 bit add,
  // 8 bit add, etc.

  // Q: Can modargs be passed in to the execution code for simulation?

  // Q: How do we isolate pieces of code that we can compile into simulatable
  //    fragments?

  // A: Sortable combinational portions are an easy one
  //    Problems: Clock gated circuits, sequential elements

  // Issue: All the combinational loops (forgetting about clock gating)
  //        should not get feedback from outputs (if that makes sense),
  //        so the output of one of the muxes in the loop does not change
  //        the output of any of the muxes in the rest of the circuit


  // All loops could be broken by setting a mux (programmable switch)
  // The output of any loop mux does not affect the select on any other loop mux
  // Assumption: No data will be put down the datapath until configuration is finished
  // Q: Can any dangerous loops form in this scheme?

  // Q: Can I prove that given this setup I dont need full event based simulation
  //    even during configuration loading?
  //    Really what I want to check is whether each loop contains a mux that breaks
  //    the loop at each stage of configuration

  class Env {
  protected:
    std::map<CellType, CellDefinition> cellDefs;
    std::map<std::string, CellType> cellTypeNames;

    CellType nextType;

  public:

    // Save space for 100 primitive operations
    Env() : nextType(100) {}

    const std::map<CellType, CellDefinition>& getCellDefs() const {
      return cellDefs;
    }

    std::string toString() const {
      std::string str =  "--- Environment\n";
      return str;
    }
    
  };
  
}
