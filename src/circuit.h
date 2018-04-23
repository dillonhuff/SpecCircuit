#pragma once

#include "coreir.h"
#include "algorithm.h"

using namespace dbhc;

namespace FlatCircuit {

  enum Parameter {
    PARAM_PORT_TYPE,
    PARAM_WIDTH,
    PARAM_HIGH,
    PARAM_LOW,
    PARAM_IN_WIDTH,
    PARAM_IN0_WIDTH,
    PARAM_IN1_WIDTH,
    PARAM_OUT_WIDTH,
    PARAM_SEL_WIDTH,
    PARAM_CLK_POSEDGE,
    PARAM_ARST_POSEDGE,
    PARAM_INIT_VALUE,
    PARAM_MEM_WIDTH,
    PARAM_MEM_DEPTH,
    PARAM_HAS_INIT
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
#define CELL_TYPE_UGT 11
#define CELL_TYPE_ULT 12
#define CELL_TYPE_EQ 13
#define CELL_TYPE_NEQ 14
#define CELL_TYPE_REG 15
#define CELL_TYPE_REG_ARST 16
#define CELL_TYPE_MUX 17
#define CELL_TYPE_CONST 18
#define CELL_TYPE_LSHR 19
#define CELL_TYPE_ASHR 20
#define CELL_TYPE_SHL 21
#define CELL_TYPE_ZEXT 22
#define CELL_TYPE_MUL 23
#define CELL_TYPE_SLICE 24
#define CELL_TYPE_UGE 25
#define CELL_TYPE_ULE 26
#define CELL_TYPE_MEM 27

  static inline std::string toString(const CellType cellTp) {
    if (cellTp == CELL_TYPE_CONST) {
      return "CELL_TYPE_CONST";
    } else if (cellTp == CELL_TYPE_MUX) {
      return "CELL_TYPE_MUX";
    } else if (cellTp == CELL_TYPE_PORT) {
      return "CELL_TYPE_PORT";
    } else if (cellTp == CELL_TYPE_OR) {
      return "CELL_TYPE_OR";
    } else if (cellTp == CELL_TYPE_ORR) {
      return "CELL_TYPE_ORR";
    } else if (cellTp == CELL_TYPE_EQ) {
      return "CELL_TYPE_EQ";
    } else if (cellTp == CELL_TYPE_REG_ARST) {
      return "CELL_TYPE_REG_ARST";
    } else if (cellTp == CELL_TYPE_PASSTHROUGH) {
      return "CELL_TYPE_PASSTHROUGH";
    } else if (cellTp == CELL_TYPE_REG) {
      return "CELL_TYPE_REG";
    } else if (cellTp == CELL_TYPE_ADD) {
      return "CELL_TYPE_ADD";
    } else if (cellTp == CELL_TYPE_ULT) {
      return "CELL_TYPE_ULT";
    } else if (cellTp == CELL_TYPE_XOR) {
      return "CELL_TYPE_XOR";
    } else if (cellTp == CELL_TYPE_AND) {
      return "CELL_TYPE_AND";
    } else if (cellTp == CELL_TYPE_MUL) {
      return "CELL_TYPE_MUL";
    } else if (cellTp == CELL_TYPE_ZEXT) {
      return "CELL_TYPE_ZEXT";
    } else if (cellTp == CELL_TYPE_SLICE) {
      return "CELL_TYPE_SLICE";
    } else if (cellTp == CELL_TYPE_ASHR) {
      return "CELL_TYPE_ASHR";
    } else if (cellTp == CELL_TYPE_LSHR) {
      return "CELL_TYPE_LSHR";
    } else if (cellTp == CELL_TYPE_NOT) {
      return "CELL_TYPE_NOT";
    } else if (cellTp == CELL_TYPE_NEQ) {
      return "CELL_TYPE_NEQ";
    } else if (cellTp == CELL_TYPE_MEM) {
      return "CELL_TYPE_MEM";
    } else if (cellTp == CELL_TYPE_UGE) {
      return "CELL_TYPE_UGE";
    } else if (cellTp == CELL_TYPE_ULE) {
      return "CELL_TYPE_ULE";
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
#define PORT_ID_RADDR 7
#define PORT_ID_RDATA 8
#define PORT_ID_WADDR 9
#define PORT_ID_WDATA 10
#define PORT_ID_WEN 11  

  static inline std::string portIdString(const PortId portId) {
    if (portId == PORT_ID_IN0) {
      return "PORT_ID_IN0";
    }

    if (portId == PORT_ID_IN1) {
      return "PORT_ID_IN1";
    }

    if (portId == PORT_ID_IN) {
      return "PORT_ID_IN";
    }

    if (portId == PORT_ID_OUT) {
      return "PORT_ID_OUT";
    }

    if (portId == PORT_ID_SEL) {
      return "PORT_ID_SEL";
    }

    if (portId == PORT_ID_CLK) {
      return "PORT_ID_CLK";
    }

    if (portId == PORT_ID_ARST) {
      return "PORT_ID_ARST";
    }

    if (portId == PORT_ID_RADDR) {
      return "PORT_ID_RADDR";
    }

    if (portId == PORT_ID_WADDR) {
      return "PORT_ID_WADDR";
    }

    if (portId == PORT_ID_WDATA) {
      return "PORT_ID_WDATA";
    }

    if (portId == PORT_ID_RDATA) {
      return "PORT_ID_RDATA";
    }

    if (portId == PORT_ID_WEN) {
      return "PORT_ID_WEN";
    }
    
    std::cout << "No string for port id " << portId << std::endl;
    assert(false);
  }
  
#define PORT_CELL_FOR_INPUT 0
#define PORT_CELL_FOR_OUTPUT 1

  static inline int bvToInt(const BitVector& bv) {
    return bv.to_type<int>();
  }

  static inline bool isUnop(const CellType tp) {
    // Why is cell type port here?
    std::vector<CellType> unops{
        CELL_TYPE_PASSTHROUGH,
        CELL_TYPE_NOT,
        CELL_TYPE_ORR,
        CELL_TYPE_ANDR,
        CELL_TYPE_XORR,
        CELL_TYPE_ZEXT};

    return elem(tp, unops);
  }

  static inline bool isBinop(const CellType tp) {
    std::vector<CellType> binops{
      CELL_TYPE_OR,
        CELL_TYPE_AND,
        CELL_TYPE_XOR,
        CELL_TYPE_ADD,
        CELL_TYPE_SUB,
        CELL_TYPE_MUL,
        CELL_TYPE_UGT,
        CELL_TYPE_ULT,
        CELL_TYPE_EQ,
        CELL_TYPE_NEQ,
        CELL_TYPE_ASHR,
        CELL_TYPE_LSHR,
        CELL_TYPE_SHL,
        CELL_TYPE_UGE,
        CELL_TYPE_ULE
    };

    return elem(tp, binops);
  }

  static inline bool isComparator(const CellType tp) {
    std::vector<CellType> comparators{
      CELL_TYPE_UGT,
        CELL_TYPE_ULT,
        CELL_TYPE_EQ,
        CELL_TYPE_NEQ,
        CELL_TYPE_UGE,
        CELL_TYPE_ULE
        };

    return elem(tp, comparators);
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
    return "( " + std::to_string(bit.cell) + ", " + portIdString(bit.port) + ", " + std::to_string(bit.offset) + " )";
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
      if (cellType == CELL_TYPE_SLICE) {
        BitVector width = map_find(PARAM_WIDTH, parameters);
        int wd = width.to_type<int>();

        BitVector hi_b = map_find(PARAM_HIGH, parameters);
        int hi = hi_b.to_type<int>();

        BitVector lo_b = map_find(PARAM_LOW, parameters);
        int lo = lo_b.to_type<int>();
        
        portWidths.insert({PORT_ID_OUT, {hi - lo, PORT_TYPE_OUT}});
        std::vector<std::vector<SignalBit> > bus(hi - lo);
        receivers.insert({PORT_ID_OUT, bus});

        portWidths.insert({PORT_ID_IN, {wd, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_IN, SignalBus(wd)});

      } else if (cellType == CELL_TYPE_MEM) {
        // TODO: Fill in the memory cell
        BitVector depth = map_find(PARAM_MEM_DEPTH, parameters);

        BitVector width = map_find(PARAM_MEM_WIDTH, parameters);
        int addrWidth = ceil(log2(depth.to_type<int>()));

        portWidths.insert({PORT_ID_RADDR, {addrWidth, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_RADDR, SignalBus(addrWidth)});

        portWidths.insert({PORT_ID_WADDR, {addrWidth, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_WADDR, SignalBus(addrWidth)});

        portWidths.insert({PORT_ID_WDATA, {width.to_type<int>(), PORT_TYPE_IN}});
        drivers.insert({PORT_ID_WDATA, SignalBus(width.to_type<int>())});

        portWidths.insert({PORT_ID_CLK, {1, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_CLK, SignalBus(1)});

        portWidths.insert({PORT_ID_WEN, {1, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_WEN, SignalBus(1)});
        
        portWidths.insert({PORT_ID_RDATA, {width.to_type<int>(), PORT_TYPE_OUT}});
        std::vector<std::vector<SignalBit> > bus(width.to_type<int>());
        receivers.insert({PORT_ID_RDATA, bus});
        
      } else if (cellType == CELL_TYPE_ZEXT) {
        BitVector out_width = map_find(PARAM_OUT_WIDTH, parameters);
        int out_wd = out_width.to_type<int>();

        BitVector in_width = map_find(PARAM_IN_WIDTH, parameters);
        int in_wd = in_width.to_type<int>();

        assert(in_wd <= out_wd);

        portWidths.insert({PORT_ID_OUT, {out_wd, PORT_TYPE_OUT}});
        std::vector<std::vector<SignalBit> > bus(out_wd);
        receivers.insert({PORT_ID_OUT, bus});

        portWidths.insert({PORT_ID_IN, {in_wd, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_IN, SignalBus(in_wd)});
        
      } else if (cellType == CELL_TYPE_PORT) {
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

        // TODO: Distinguish comparators like eq, gt that have out width 1 from
        // operands with same in and out width
        BitVector width = parameters.at(PARAM_WIDTH);
        assert(width.bitLength() == 32);

        int wd = width.to_type<int>();

        if (!isComparator(cellType)) {
          portWidths.insert({PORT_ID_OUT, {width.to_type<int>(), PORT_TYPE_OUT}});
          std::vector<std::vector<SignalBit> > bus(wd);
          receivers.insert({PORT_ID_OUT, bus});
        } else {
          portWidths.insert({PORT_ID_OUT, {1, PORT_TYPE_OUT}});
          std::vector<std::vector<SignalBit> > bus(1);
          receivers.insert({PORT_ID_OUT, bus});

        }

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
        drivers.insert({PORT_ID_CLK, SignalBus(1)});

        portWidths.insert({PORT_ID_ARST, {1, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_ARST, SignalBus(1)});
        
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
        drivers.insert({PORT_ID_SEL, SignalBus(1)});
        
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

      } else if (cellType == CELL_TYPE_REG) {

        BitVector width = parameters.at(PARAM_WIDTH);
        int wd = width.to_type<int>();
        assert(width.bitLength() == 32);

        portWidths.insert({PORT_ID_OUT, {width.to_type<int>(), PORT_TYPE_OUT}});
        std::vector<std::vector<SignalBit> > bus(wd);
        receivers.insert({PORT_ID_OUT, bus});

        portWidths.insert({PORT_ID_IN, {width.to_type<int>(), PORT_TYPE_IN}});
        drivers.insert({PORT_ID_IN, SignalBus(wd)});

        portWidths.insert({PORT_ID_CLK, {1, PORT_TYPE_IN}});
        drivers.insert({PORT_ID_CLK, SignalBus(1)});

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
      return map_find(val, parameters); //parameters.at(val);
    }

    int getPortWidth(const PortId port) const {
      return map_find(port, portWidths).width;// .at(port).width;
    }

    PortType getPortType(const PortId port) const {
      return map_find(port, portWidths).type; //portWidths.at(port).type;
    }

    bool isInputPortCell() const {
      int pt = bvToInt(getParameterValue(PARAM_PORT_TYPE));
      return pt == PORT_CELL_FOR_INPUT;
    }

    bool hasPort(const PortId port) const {
      return contains_key(port, portWidths);
    }
    
    CellType getCellType() const {
      return cellType;
    }

    BitVector initValue() const {
      return getParameterValue(PARAM_INIT_VALUE);
    }

    int getMemWidth() const {
      BitVector bv = getParameterValue(PARAM_MEM_WIDTH);
      return bv.to_type<int>();
    }

    int getMemDepth() const {
      BitVector bv = getParameterValue(PARAM_MEM_DEPTH);
      return bv.to_type<int>();
    }
    
    bool rstPosedge() const {
      BitVector bv = getParameterValue(PARAM_ARST_POSEDGE);
      return bv.get(0).is_binary() && (bv.get(0).binary_value() == 1);
    }
    
    bool clkPosedge() const {
      BitVector bv = getParameterValue(PARAM_CLK_POSEDGE);
      return bv.get(0).is_binary() && (bv.get(0).binary_value() == 1);
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

    void clearReceivers(const PortId port) {
      assert(contains_key(port, receivers));

      auto& rcv = receivers[port];
      rcv = {};
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

  class CellDefinition {
    std::map<CellId, Cell> cells;
    std::map<PortId, Port> ports;
    std::map<PortId, CellId> portsToCells;

    std::map<std::string, PortId> portNames;
    std::map<std::string, CellId> cellNames;
    std::map<CellId, std::string> cellIdsToNames;

    CellId next;
    PortId nextPort;

  public:

    // Cell 0 is reserved to indicated a non-existant cell
    CellDefinition() : next(1), nextPort(0) {}

    const std::map<CellId, Cell>& getCellMap() const {
      return cells;
    }

    void connect(const CellId driverCellId, const PortId driverPID,
                 const CellId receiverCellId, const PortId receiverPID) {

      Cell& driverCell = getCellRef(driverCellId);
      Cell& receiverCell = getCellRef(receiverCellId);

      int driverWidth = driverCell.getPortWidth(driverPID);
      int receiverWidth = receiverCell.getPortWidth(receiverPID);

      assert(driverWidth == receiverWidth);

      for (int offset = 0; offset < driverWidth; offset++) {
        SignalBit driverBit{driverCellId, driverPID, offset};
        SignalBit receiverBit{receiverCellId, receiverPID, offset};

        setDriver(receiverBit, driverBit);
      }
      
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

    const Cell& getCellRefConst(const CellId c) const {
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

    Cell& getPortCell(const std::string& name) {
      return cells.at(getPortCellId(name));
    }
    
    const Cell& getPortCellConst(const std::string& name) const {
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

    std::string getCellName(const CellId cellId) {
      assert(contains_key(cellId, cellIdsToNames));
      return cellIdsToNames.at(cellId);
    }

    void setDriver(const SignalBit receiver,
                   const SignalBit driver) {
      cells[receiver.cell].setDriver(receiver.port, receiver.offset, driver);
      cells[driver.cell].addReceiver(driver.port, driver.offset, receiver);
    }

    void replacePortWithConstant(const std::string& portName,
                                 const BitVector& constValue) {
      CellId constId = addCell(portName + "_const_replacement",
                               CELL_TYPE_CONST,
                               {{PARAM_WIDTH, BitVector(32, constValue.bitLength())},
                                   {PARAM_INIT_VALUE, constValue}});

      //Cell& constCell = getCellRef(constId);

      CellId portId = getPortCellId(portName);

      Cell& portCell = getCellRef(portId);

      
      std::vector<std::vector<SignalBit> > receivers =
        portCell.getPortReceivers(PORT_ID_OUT);
      
      for (int offset = 0; offset < receivers.size(); offset++) {
        auto& sigBus = receivers.at(offset);
        SignalBit driverSignal{constId, PORT_ID_OUT, offset};
        for (auto receiverSignal : sigBus) {
          setDriver(receiverSignal, driverSignal);
        }
      }

      // NOTE: Drivers and receivers are not kept in sync here
      portCell.clearReceivers(PORT_ID_OUT);
    }

    int numCells() const {
      return cells.size();
    }
  };

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

  static inline std::string toString(const CellDefinition& def,
                                     const SignalBit bit) {
    return "( " + def.cellName(bit.cell) + ", " + toString(def.getCellRefConst(bit.cell).getCellType()) + ", " + portIdString(bit.port) + ", " + std::to_string(bit.offset) + " )";
  }
  
}
