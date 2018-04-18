#pragma once

#include "coreir.h"

#include "algorithm.h"

using namespace dbhc;

namespace FlatCircuit {

  enum Parameter {
    PARAM_PORT_TYPE,
    PARAM_WIDTH,
    PARAM_IN0_WIDTH,
    PARAM_IN1_WIDTH,
    PARAM_OUT_WIDTH,
    PARAM_SEL_WIDTH,
    PARAM_CLK_POSEDGE,
    PARAM_CLK_NEGEDGE,
    PARAM_INIT_VALUE
  };

  typedef uint64_t CellType;

#define CELL_TYPE_PORT 0
#define CELL_TYPE_PASSTHROUGH 1
#define CELL_TYPE_OR 2
#define CELL_TYPE_AND 3
#define CELL_TYPE_XOR 4
#define CELL_TYPE_NOT 5
#define CELL_TYPE_ORR 6
#define CELL_TYPE_ANDR 7
#define CELL_TYPE_XORR 8
#define CELL_TYPE_ADD 9
#define CELL_TYPE_SUB 10
#define CELL_TYPE_GT 11
#define CELL_TYPE_LT 12
#define CELL_TYPE_EQ 13
#define CELL_TYPE_NEQ 14
#define CELL_TYPE_REG 15
#define CELL_TYPE_REG_ARST 16
#define CELL_TYPE_MUX 17
#define CELL_TYPE_CONST 18

  static inline std::string toString(const CellType cellTp) {
    if (cellTp == CELL_TYPE_CONST) {
      return "CELL_TYPE_CONST";
    } else if (cellTp == CELL_TYPE_MUX) {
      return "CELL_TYPE_MUX";
    }

    std::cout << "No string for cell type " << cellTp << std::endl;
    assert(false);
  }

  typedef uint64_t CellId;

  typedef uint64_t PortId;

#define PORT_ID_IN 0
#define PORT_ID_IN0 1
#define PORT_ID_IN1 2
#define PORT_ID_OUT 3
#define PORT_ID_SEL 4
#define PORT_ID_CLK 5
#define PORT_ID_ARST 6

#define PORT_CELL_FOR_INPUT 0
#define PORT_CELL_FOR_OUTPUT 1

  static inline bool isUnop(const CellType tp) {
    std::vector<CellType> unops{CELL_TYPE_PORT,
        CELL_TYPE_PASSTHROUGH,
        CELL_TYPE_NOT,
        CELL_TYPE_ORR,
        CELL_TYPE_ANDR,
        CELL_TYPE_XORR};

    return elem(tp, unops);
  }

  static inline bool isBinop(const CellType tp) {
    std::vector<CellType> binops{
      CELL_TYPE_OR,
        CELL_TYPE_AND,
        CELL_TYPE_XOR,
        CELL_TYPE_ADD,
        CELL_TYPE_SUB,
        CELL_TYPE_GT,
        CELL_TYPE_LT,
        CELL_TYPE_EQ,
        CELL_TYPE_NEQ
    };

    return elem(tp, binops);
  }
  
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

  static inline bool operator==(const SignalBit a, const SignalBit b) {
    return (a.cell == b.cell) && (a.port == b.port) && (a.offset == b.offset);
  }

  static inline std::string toString(const SignalBit bit) {
    return "( " + std::to_string(bit.cell) + ", " + std::to_string(bit.port) + ", " + std::to_string(bit.offset) + " )";
  }

  static inline bool notEmpty(const SignalBit bit) {
    return !(bit.cell == 0);
  }

  class SignalBus {
  public:
    std::vector<SignalBit> signals;

    SignalBus() : signals() {}
    
    SignalBus(const int width) : signals() {
      signals.resize(width);
    }
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
         const std::map<Parameter, BitVector> & parameters_) :
      parameters(parameters_), cellType(cellType_) {
      //std::cout << "Creating cell type " << cellType << std::endl;

      // NOTE: Need to include input port type and output port type
      if (cellType == CELL_TYPE_PORT) {
        BitVector width = parameters.at(PARAM_OUT_WIDTH);
        int wd = width.to_type<int>();

        assert(width.bitLength() == 32);

        BitVector ptp = parameters.at(PARAM_PORT_TYPE);
        int ptpInt = ptp.to_type<int>();

        assert((ptpInt == PORT_CELL_FOR_INPUT) ||
               (ptpInt == PORT_CELL_FOR_OUTPUT));

        if (ptpInt == PORT_CELL_FOR_INPUT) {

          portWidths.insert({PORT_ID_OUT, {width.to_type<int>(), PORT_TYPE_OUT}});
          
          std::vector<std::vector<SignalBit> > bus(wd);
          receivers.insert({PORT_ID_OUT, bus});
          
        } else {
          assert(ptpInt == PORT_CELL_FOR_OUTPUT);

          portWidths.insert({PORT_ID_IN, {width.to_type<int>(), PORT_TYPE_IN}});
          drivers.insert({PORT_ID_IN, SignalBus(wd)});

        }
      } else if (isBinop(cellType)) {

        BitVector width = parameters.at(PARAM_WIDTH);
        assert(width.bitLength() == 32);

        int wd = width.to_type<int>();

        portWidths.insert({PORT_ID_OUT, {width.to_type<int>(), PORT_TYPE_OUT}});
        std::vector<std::vector<SignalBit> > bus(wd);
        receivers.insert({PORT_ID_OUT, bus});

        portWidths.insert({PORT_ID_IN0, {width.to_type<int>(), PORT_TYPE_IN}});
        drivers.insert({PORT_ID_IN0, SignalBus(wd)});

        portWidths.insert({PORT_ID_IN1, {width.to_type<int>(), PORT_TYPE_IN}});
        drivers.insert({PORT_ID_IN1, SignalBus(wd)});
        
      } else if (cellType == CELL_TYPE_REG_ARST) {

        BitVector width = parameters.at(PARAM_WIDTH);
        int wd = width.to_type<int>();
        assert(width.bitLength() == 32);

        portWidths.insert({PORT_ID_OUT, {width.to_type<int>(), PORT_TYPE_OUT}});
        std::vector<std::vector<SignalBit> > bus(wd);
        receivers.insert({PORT_ID_OUT, bus});

        portWidths.insert({PORT_ID_IN, {width.to_type<int>(), PORT_TYPE_IN}});
        drivers.insert({PORT_ID_IN, SignalBus(wd)});

        portWidths.insert({PORT_ID_CLK, {1, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_CLK, SignalBus(wd)});

        portWidths.insert({PORT_ID_ARST, {1, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_ARST, SignalBus(wd)});
        
      } else if (cellType == CELL_TYPE_MUX) {

        BitVector width = parameters.at(PARAM_WIDTH);
        int wd = width.to_type<int>();
        assert(width.bitLength() == 32);

        portWidths.insert({PORT_ID_OUT, {width.to_type<int>(), PORT_TYPE_OUT}});
        std::vector<std::vector<SignalBit> > bus(wd);
        receivers.insert({PORT_ID_OUT, bus});

        portWidths.insert({PORT_ID_IN0, {width.to_type<int>(), PORT_TYPE_IN}});
        drivers.insert({PORT_ID_IN0, SignalBus(wd)});

        portWidths.insert({PORT_ID_IN1, {width.to_type<int>(), PORT_TYPE_IN}});
        drivers.insert({PORT_ID_IN1, SignalBus(wd)});

        portWidths.insert({PORT_ID_SEL, {1, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_SEL, SignalBus(wd)});
        
      } else if (isUnop(cellType)) {

        BitVector width = parameters.at(PARAM_WIDTH);
        int wd = width.to_type<int>();
        assert(width.bitLength() == 32);

        portWidths.insert({PORT_ID_OUT, {width.to_type<int>(), PORT_TYPE_OUT}});
        std::vector<std::vector<SignalBit> > bus(wd);
        receivers.insert({PORT_ID_OUT, bus});

        portWidths.insert({PORT_ID_IN, {width.to_type<int>(), PORT_TYPE_IN}});
        drivers.insert({PORT_ID_IN, SignalBus(wd)});

      } else if (cellType == CELL_TYPE_CONST) {

        BitVector width = parameters.at(PARAM_WIDTH);
        int wd = width.to_type<int>();
        assert(width.bitLength() == 32);
        portWidths.insert({PORT_ID_OUT, {width.to_type<int>(), PORT_TYPE_OUT}});
        std::vector<std::vector<SignalBit> > bus(wd);
        receivers.insert({PORT_ID_OUT, bus});

      } else {
        std::cout << "No processing rule for cell type " << cellType << std::endl;
        assert(false);
      }

      //std::cout << "Done creating cell" << cellType << std::endl;
    }

    const SignalBus& getDrivers(const PortId port) const {
      return drivers.at(port);
    }

    const std::vector<std::vector<SignalBit> >&
    getPortReceivers(const PortId pid) const {
      assert(contains_key(pid, receivers));

      return receivers.at(pid);
    }

    BitVector getParameterValue(const Parameter val) const {
      return parameters.at(val);
    }

    int getPortWidth(const PortId port) const {
      return portWidths.at(port).width;
    }

    PortType getPortType(const PortId port) const {
      return portWidths.at(port).type;
    }

    bool hasPort(const PortId port) const {
      return contains_key(port, portWidths);
    }
    
    CellType getCellType() const {
      return cellType;
    }

    void addReceiver(const PortId port, const int offset, const SignalBit receiver) {
      if (!contains_key(port, receivers)) {
        std::cout << "All ports: " << std::endl;
        for (auto port : receivers) {
          std::cout << "\t" << port.first << std::endl;
        }
      }

      assert(contains_key(port, receivers));

      auto& rcv = receivers[port];

      assert(rcv.size() > offset);
      
      rcv[offset].push_back(receiver);
    }
    
    void setDriver(const PortId port, const int offset, const SignalBit driver) {
      if (!contains_key(port, drivers)) {
        std::cout << "All ports: " << std::endl;
        for (auto port : drivers) {
          std::cout << "\t" << port.first << std::endl;
        }
      }
      assert(contains_key(port, drivers));

      auto& sigBus = drivers[port];

      assert(sigBus.signals.size() > offset);

      sigBus.signals[offset] = driver;

      assert(getDrivers(port).signals[offset] == driver);
    }
  };

  // Random: Maybe each port on each cell should have its own unique identifier?

  class CellDefinition {
    std::map<CellId, Cell> cells;
    std::map<PortId, Port> ports;
    std::map<PortId, CellId> portsToCells;

    std::map<std::string, PortId> portNames;
    std::map<std::string, CellId> cellNames;
    std::map<CellId, std::string> cellIdsToNames;

    CellId next;
    PortId nextPort;

    // How to represent connections? Map from ports to drivers? Map
    // from PortIds to receivers as well?

    // Q: What do I want to do with this code?
    // A: For a given port get all receiver ports (and offsets)
    //    For a given port get the list of driver ports (and offsets)

  public:

    // Cell 0 is reserved to indicated a non-existant cell
    CellDefinition() : next(1), nextPort(0) {}

    const std::map<CellId, Cell>& getCellMap() const {
      return cells;
    }

    std::vector<std::string> getPortNames() const {
      std::vector<std::string> names;
      for (auto p : portNames) {
        names.push_back(p.first);
      }
      return names;
    }

    PortId addPort(const std::string& name, const int portWidth, const PortType tp) {
      PortId pid = nextPort;

      // TODO: Add cell ports
      ports[pid] = Port{portWidth, tp};
      portNames[name] = pid;

      CellType cellTp = CELL_TYPE_PORT;

      CellId cid;

      if (tp == PORT_TYPE_IN) {

        cid = addCell(name, cellTp, {{PARAM_OUT_WIDTH, BitVector(32, portWidth)}, {PARAM_PORT_TYPE, BitVector(2, PORT_CELL_FOR_INPUT)}});

      } else {

        cid = addCell(name, cellTp, {{PARAM_OUT_WIDTH, BitVector(32, portWidth)}, {PARAM_PORT_TYPE, BitVector(2, PORT_CELL_FOR_OUTPUT)}});

      }
      portsToCells[pid] = cid;
      nextPort++;
      return pid;
    }

    Cell& getCellRef(const CellId c) {
      return cells.at(c);
    }

    std::string cellName(const CellId id) const {
      return cellIdsToNames.at(id);
    }

    CellId getPortCellId(const std::string& name) const {
      assert(contains_key(name, portNames));
      auto pid = portNames.at(name);

      assert(contains_key(pid, portsToCells));

      auto cellId = portsToCells.at(pid);

      return cellId;
    }

    const Cell& getPortCell(const std::string& name) const {
      return cells.at(getPortCellId(name));
    }

    CellId addCell(const std::string& name, const CellType cell, const std::map<Parameter, BitVector>& params) {
      auto id = next;
      next++;

      cells[id] = Cell(cell, params);

      assert(!contains_key(name, cellNames));

      cellNames[name] = id;
      cellIdsToNames[id] = name;
      return id;
    }

    void setDriver(const SignalBit receiver,
                   const SignalBit driver) {
      cells[receiver.cell].setDriver(receiver.port, receiver.offset, driver);
      cells[driver.cell].addReceiver(driver.port, driver.offset, receiver);
    }

    int numCells() const {
      return cells.size();
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

    CellType addCellType(const std::string& name) {
      auto tp = nextType;
      cellDefs[tp] = {};
      cellTypeNames[name] = tp;

      nextType++;

      return tp;
    }

    CellDefinition& getDef(const CellType tp) {
      return cellDefs.at(tp);
    }
    
    CellDefinition& getDef(const std::string& cellName) {
      return cellDefs.at(cellTypeNames.at(cellName));
    }

    const std::map<CellType, CellDefinition>& getCellDefs() const {
      return cellDefs;
    }

    std::string toString() const {
      std::string str =  "--- Environment\n";
      return str;
    }
    
  };
  
}
