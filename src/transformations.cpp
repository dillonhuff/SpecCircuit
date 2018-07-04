#include "transformations.h"

#include "simulator.h"

using namespace std;

namespace FlatCircuit {


  static inline BitVector doUnop(const CellType tp,
                                 const BitVector& in) {
    switch (tp) {
    case CELL_TYPE_PASSTHROUGH:
      return in;

    case CELL_TYPE_ORR:
      return orr(in);

    case CELL_TYPE_ANDR:
      return andr(in);

    case CELL_TYPE_NOT:
      return ~in;

    default:
      std::cout << "Error: Unsupported unop = " << toString(tp) << std::endl;
      assert(false);
    }
  }

  static inline BitVector doBinop(const CellType tp,
                                  const BitVector& in0,
                                  const BitVector& in1) {
    switch (tp) {
    case CELL_TYPE_EQ:
      return BitVector(1, in0 == in1);

    case CELL_TYPE_NEQ:
      return BitVector(1, in0 != in1);

    case CELL_TYPE_ULT:
      return BitVector(1, in0 < in1);

    case CELL_TYPE_UGT:
      return BitVector(1, in0 > in1);
      
    case CELL_TYPE_UGE:
      return BitVector(1, (in0 > in1) || (in0 == in1));

    case CELL_TYPE_ULE:
      return BitVector(1, (in0 < in1) || (in0 == in1));

    case CELL_TYPE_SHL:
      return shl(in0, in1);

    case CELL_TYPE_ASHR:
      return ashr(in0, in1);

    case CELL_TYPE_LSHR:
      return lshr(in0, in1);

    case CELL_TYPE_SUB:
      return sub_general_width_bv(in0, in1);
      
    case CELL_TYPE_XOR:
      return in0 ^ in1;
      
    case CELL_TYPE_OR:
      return in0 | in1;

    case CELL_TYPE_AND:
      return in0 & in1;

    case CELL_TYPE_ADD:
      return add_general_width_bv(in0, in1);

    case CELL_TYPE_MUL:
      return mul_general_width_bv(in0, in1);
      
    default:
      std::cout << "Error: Unsupported binop = " << toString(tp) << std::endl;
      assert(false);
    }
  }

  maybe<BitVector> materializeConstPort(const SigPort sp,
                                        CellDefinition& def);
  
  // Get the output of this register if its output drives its input
  // otherwise return Nothing
  maybe<BitVector> materializeRegisterInput(const CellId cid,
                                            CellDefinition& def,
                                            const BitVector& value) {
    const Cell& cell = def.getCellRefConst(cid);
    assert(isRegister(cell.getCellType()));
    // HASTY WARNING: Really ought to check whether the input constant matches
    // the current state
    maybe<BitVector> inValue = materializeConstPort({cid, PORT_ID_IN}, def);
    if (inValue.has_value()) {
      return inValue;
    }

    auto drivers = cell.getDrivers(PORT_ID_IN);

    assert(value.bitLength() == (int) drivers.signals.size());
    
    for (int offset = 0; offset < (int) drivers.signals.size(); offset++) {
      SignalBit driverBit = drivers.signals[offset];

      SignalBit outputValue{cid, PORT_ID_OUT, offset};

      if (driverBit != outputValue) {
        return {};
      }
    }

    return value;
  }

  maybe<BitVector> materializeConstPort(const SigPort sp,
                                        CellDefinition& def) {

    CellId cid = sp.cell;
    Cell& cell = def.getCellRef(cid);
    PortId pid = sp.port;

    BitVector bv(cell.getPortWidth(pid), 0);

    auto& drivers = cell.getDrivers(pid);

    for (int offset = 0; offset < (int) drivers.signals.size(); offset++) {

      SignalBit driver = drivers.signals.at(offset);

      if (notEmpty(driver)) {
        CellType driverTp = def.getCellRef(driver.cell).getCellType();
        if (driverTp != CELL_TYPE_CONST) {
          return {};
        }

        assert(driver.port == PORT_ID_OUT);

        Cell& driverCell = def.getCellRef(driver.cell);
        BitVector driverValue = driverCell.getParameterValue(PARAM_INIT_VALUE);

        bv.set(offset, driverValue.get(driver.offset));

      } else {
        return {};
      }
    }

    return bv;
  }

  maybe<BitVector> getOutput(const CellId cid,
                             CellDefinition& def,
                             const std::map<CellId, BitVector>& registerValues) {
    const Cell& nextCell = def.getCellRef(cid);
    if (!nextCell.hasPort(PORT_ID_OUT)) {

      if (!def.isPortCell(cid)) {
        cout << "Cell " << def.cellName(cid) << " has no output port!" << endl;
        assert(false);
      }

      return {};
    }

    CellType tp = nextCell.getCellType();

    if (tp == CELL_TYPE_ZEXT) {

      maybe<BitVector> inm = materializeConstPort({cid, PORT_ID_IN}, def);

      if (inm.has_value()) {

        BitVector in = inm.get_value();

        int outWidth = nextCell.getPortWidth(PORT_ID_OUT);
        BitVector res(outWidth, 0);
        for (uint i = 0; i < (uint) in.bitLength(); i++) {
          res.set(i, in.get(i));
        }
        
        BitVector newOut = res;
        return newOut;
      }

      return {};

    } else if (tp == CELL_TYPE_SLICE) {
      maybe<BitVector> inm = materializeConstPort({cid, PORT_ID_IN}, def);

      if (inm.has_value()) {
        BitVector in = inm.get_value();

        uint lo = nextCell.getParameterValue(PARAM_LOW).to_type<int>();
        uint hi = nextCell.getParameterValue(PARAM_HIGH).to_type<int>();

        assert((hi - lo) > 0);

        BitVector res(hi - lo, 0);
        for (uint i = lo; i < hi; i++) {
          res.set(i - lo, in.get(i));
        }

        return res;
      }

      return {};
      
    } else if (isBinop(tp)) {
      maybe<BitVector> in0m = materializeConstPort({cid, PORT_ID_IN0}, def);
      maybe<BitVector> in1m = materializeConstPort({cid, PORT_ID_IN1}, def);

      if (in0m.has_value() &&
          in1m.has_value()) {
        BitVector in0 = in0m.get_value();
        BitVector in1 = in1m.get_value();

        BitVector out = doBinop(nextCell.getCellType(), in0, in1);

        return out;
      }

      if (in0m.has_value() &&
          (tp == CELL_TYPE_AND) &&
          (in0m.get_value().bitLength() == 1)) {
        BitVector in0 = in0m.get_value();

        if (in0 == BitVector(1, 0)) {
          //cout << "Partially evaluated an and to 0" << endl;
          return BitVector(1, 0);
        }

        return {};
      }

      if (in1m.has_value() &&
          (tp == CELL_TYPE_AND) &&
          (in1m.get_value().bitLength() == 1)) {
        BitVector in1 = in1m.get_value();

        if (in1 == BitVector(1, 0)) {
          //cout << "Partially evaluated an and to 0" << endl;
          return BitVector(1, 0);
        }

        return {};
      }

      if (in0m.has_value() &&
          (tp == CELL_TYPE_OR) &&
          (in0m.get_value().bitLength() == 1)) {
        BitVector in0 = in0m.get_value();

        if (in0 == BitVector(1, 1)) {
          //cout << "Partially evaluated an or to 1" << endl;
          return BitVector(1, 1);
        }

        return {};
      }

      if (in1m.has_value() &&
          (tp == CELL_TYPE_OR) &&
          (in1m.get_value().bitLength() == 1)) {
        BitVector in1 = in1m.get_value();

        if (in1 == BitVector(1, 1)) {
          //cout << "Partially evaluated an and to 1" << endl;
          return BitVector(1, 1);
        }

        return {};
      }
      
      return {};
    } else if (tp == CELL_TYPE_REG_ARST) {
      maybe<BitVector> in0m =
        materializeRegisterInput(cid, def, map_find(cid, registerValues));
      maybe<BitVector> rstm = materializeConstPort({cid, PORT_ID_ARST}, def);

      if (in0m.has_value() &&
          rstm.has_value()) {
        BitVector in0 = in0m.get_value();

        return in0;
      }

      return {};
    } else if (tp == CELL_TYPE_REG) {
      maybe<BitVector> in0m =
        materializeRegisterInput(cid, def, map_find(cid, registerValues));

      if (in0m.has_value()) {
        BitVector in0 = in0m.get_value();

        return in0;
      }

      return {};
      
    } else if (isUnop(tp)) {

      maybe<BitVector> inm = materializeConstPort({cid, PORT_ID_IN}, def);

      if (inm.has_value()) {
        BitVector in = inm.get_value();

        BitVector out = doUnop(nextCell.getCellType(), in);
        return out;
      }

      return {};
    }

    cout << "Error: Unsupported cell " << def.getCellName(cid) << " : " << toString(nextCell.getCellType()) << endl;
    assert(false);
  }

  void insertNewCandidates(const Cell& cell,
                           const PortId pid,
                           const CellDefinition& def,
                           std::set<CellId>& candidates) {
                           //                           std::set<CellId>& alreadyDeleted) {
    for (auto sigBus : cell.getPortReceivers(pid)) {
      for (auto sigBit : sigBus) {
        if (//!elem(sigBit.cell, alreadyDeleted) &&
            notEmpty(sigBit)) { // &&
          //            (//def.getCellRefConst(sigBit.cell).getCellType() != CELL_TYPE_CONST)) {
          candidates.insert(sigBit.cell);
        }
      }
    }
    
  }

  // Other constant folding mechanisms:
  // 1. Partially evaluating ands / ors, x + 0, etc.
  // 2. Evaluating registers whose inputs are constant and whose current value
  //    equals the value of the input
  // 3. Maybe there are loops that are disconnected from any output, none of
  //    the elements are disconnected??
  void foldConstants(CellDefinition& def,
                     const std::map<CellId, BitVector>& registerValues) {

    set<CellId> candidates;
    //set<CellId> cellsToDelete;
    
    for (auto& cellPair : def.getCellMap()) {
      Cell& cell = def.getCellRef(cellPair.first);
      if (cell.getCellType() == CELL_TYPE_CONST) {
        //insertNewCandidates(cell, PORT_ID_OUT, def, candidates, cellsToDelete);

        insertNewCandidates(cell, PORT_ID_OUT, def, candidates);

      }
    }

    int iterCount = 0;
    while (candidates.size() > 0) {

      if ((iterCount % 10000) == 0) {
        cout << "# of candidates = " << candidates.size() << endl;
      }
      iterCount++;

      CellId next = *std::begin(candidates);
      candidates.erase(next);

      //      cout << "Checking cell " << def.cellName(next) << endl;

      Cell& nextCell = def.getCellRef(next);

      if (nextCell.getCellType() == CELL_TYPE_MUX) {

        maybe<BitVector> bv = materializeConstPort({next, PORT_ID_SEL}, def);

        if (bv.has_value()) {
          BitVector bitVec = bv.get_value();

          assert(bitVec.bitLength() == 1);

          insertNewCandidates(nextCell, PORT_ID_OUT, def, candidates);
          //insertNewCandidates(nextCell, PORT_ID_OUT, def, candidates, cellsToDelete);
          bool selIn1 = 0;
          if (bitVec.get(0).is_binary()) {
            selIn1 = bitVec.get(0).binary_value() == 1;
          }

          PortId inPort = selIn1 ? PORT_ID_IN1 : PORT_ID_IN0;
          int width = nextCell.getPortWidth(inPort);

          vector<SignalBit> inDrivers = nextCell.getDrivers(inPort).signals;
          auto& outReceivers = nextCell.getPortReceivers(PORT_ID_OUT);

          if (outReceivers.size() != inDrivers.size()) {
            cout << "Error while folding " << def.getCellName(next)
                 << ": outReceivers.size() = " << outReceivers.size()
                 << ", but inDrivers = " << inDrivers.size() << endl;
          }
          assert(outReceivers.size() == inDrivers.size());
            
          for (int offset = 0; offset < width; offset++) {
            SignalBit newDriver = inDrivers[offset];
            auto offsetReceivers = outReceivers[offset];

            for (auto receiverBit : offsetReceivers) {
              def.setDriver(receiverBit, newDriver);
            }
          }

          def.deleteCell(next);
          //cellsToDelete.insert(next);
          candidates.erase(next);
        }
      } else if (nextCell.getCellType() == CELL_TYPE_MEM) {
        //cout << "Warning: Cannot fold constant memory right now" << endl;

        maybe<BitVector> wen = materializeConstPort({next, PORT_ID_WEN}, def);
        maybe<BitVector> clk = materializeConstPort({next, PORT_ID_CLK}, def);
        maybe<BitVector> raddr = materializeConstPort({next, PORT_ID_RADDR}, def);
        maybe<BitVector> waddr = materializeConstPort({next, PORT_ID_WADDR}, def);
        maybe<BitVector> wdata = materializeConstPort({next, PORT_ID_WDATA}, def);

        if (wen.has_value() &&
            clk.has_value() && 
            raddr.has_value() &&
            waddr.has_value() &&
            wdata.has_value()) {
          cout << "Warning: Constant folding memory does not use memory state!"
               << endl;

          int width = nextCell.getMemWidth();
          def.replaceCellPortWithConstant(next, PORT_ID_RDATA, BitVector(width, 0));

          def.deleteCell(next);
          //cellsToDelete.insert(next);
          candidates.erase(next);
        }
        
      } else {
        maybe<BitVector> bv = getOutput(next, def, registerValues);

        if (bv.has_value()) {
          insertNewCandidates(nextCell, PORT_ID_OUT, def, candidates);
          //insertNewCandidates(nextCell, PORT_ID_OUT, def, candidates, cellsToDelete);

          def.replaceCellPortWithConstant(next, PORT_ID_OUT, bv.get_value());
          def.deleteCell(next);
          //cellsToDelete.insert(next);
          candidates.erase(next);

        }
      }
    }

    //def.bulkDelete(cellsToDelete);

    // Checking the result has no unfolded sources
    for (auto& ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      Cell& cell = def.getCellRef(ctp.first);

      bool allInputsConst = true;
      int numInputPorts = 0;
      for (auto ptp : cell.getPorts()) {
        PortId pid = ptp.first;

        if (cell.getPortType(pid) == PORT_TYPE_IN) {
          numInputPorts++;
          
          maybe<BitVector> bv = materializeConstPort({cid, pid}, def);
          if (!bv.has_value()) {
            allInputsConst = false;
            break;
          }
        }
      }

      if (allInputsConst && !def.isPortCell(cid) && (numInputPorts > 0)) {
        cout << "Error: All inputs to " << def.cellName(cid) << " are constants, but it wasnt removed by constant folding" << endl;
        assert(false);
      }
    }

    assert(candidates.size() == 0);
  }

  std::set<CellId> allDrivers(const Cell& cell) {
    set<CellId> allDrivers;

    for (auto portPair : cell.getPorts()) {
      PortId port = portPair.first;

      if (cell.getPortType(port) == PORT_TYPE_IN) {
        for (auto sigBit : cell.getDrivers(port).signals) {
          if (notEmpty(sigBit)) {
            allDrivers.insert(sigBit.cell);
          }
        }
            
      }
    }

    return allDrivers;
  }

  std::set<CellId> innerCellsWithNoReceivers(CellDefinition& def) {
    std::set<CellId> toDelete;
    for (auto& cellPair : def.getCellMap()) {

      Cell& cell = def.getCellRef(cellPair.first);
      bool allOutputsHaveNoReceivers = true;

      int numOutputPorts = 0;
      for (auto portPair : cell.getPorts()) {
        PortId port = portPair.first;

        if (cell.getPortType(port) == PORT_TYPE_OUT) {
          numOutputPorts++;

          for (auto sigBus : cell.getPortReceivers(port)) {
            for (auto sigBit : sigBus) {
              if (notEmpty(sigBit)) {
                allOutputsHaveNoReceivers = false;
                break;
              }
            }
          }
            
        }

        if (!allOutputsHaveNoReceivers) {
          break;
        }
      }

      if ((numOutputPorts > 0) &&
          allOutputsHaveNoReceivers &&
          !def.isPortCell(cellPair.first)) {
        toDelete.insert(cellPair.first);
      }

    }

    return toDelete;
  }

  std::set<CellId> isolatedCells(CellDefinition& def) {
    set<CellId> connectedCells;
    for (auto portCell : def.getPortCells()) {
      connectedCells.insert(portCell);
    }

    set<CellId> toConsider = connectedCells;
    while (toConsider.size() > 0) {
      CellId cid = *std::begin(toConsider);
      toConsider.erase(cid);

      const Cell& cell = def.getCellRefConst(cid);

      for (auto& ptp : cell.getPorts()) {
        PortId pid = ptp.first;
        if (cell.getPortType(pid) == PORT_ID_IN) {
          for (auto driverBit : cell.getDrivers(pid).signals) {
            if (notEmpty(driverBit) && !elem(driverBit.cell, connectedCells)) {
              connectedCells.insert(driverBit.cell);
              toConsider.insert(driverBit.cell);
            }
          }
        } else {
          assert(cell.getPortType(pid) == PORT_TYPE_OUT);

          for (auto sigBus : cell.getPortReceivers(pid)) {
            for (auto sigBit : sigBus) {
              if (notEmpty(sigBit) && !elem(sigBit.cell, connectedCells)) {
                connectedCells.insert(sigBit.cell);
                toConsider.insert(sigBit.cell);
              }
            }
          }
        }
      }
    }

    cout << "# of cells connected to inputs = " << connectedCells.size() << endl;

    set<CellId> innerCells;
    for (auto& ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      if (!elem(cid, connectedCells)) {
        innerCells.insert(cid);
      }
    }

    return innerCells;
  }

  void removeConstDuplicates(CellDefinition& def) {

    vector<CellId> zeroConstants;
    vector<CellId> oneConstants;

    for (auto& ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      if (def.getCellRefConst(cid).getCellType() == CELL_TYPE_CONST) {
        BitVector initVal = def.getCellRef(cid).getParameterValue(PARAM_INIT_VALUE);

        if (initVal.is_binary()) {
          if (initVal.bitLength() == 1) {
            if (bvToInt(initVal) == 0) {
              zeroConstants.push_back(cid);
            } else if (bvToInt(initVal) == 1) {
              oneConstants.push_back(cid);
            }
          }
        }
      }
    }

    cout << "# of zero constants = " << zeroConstants.size() << endl;
    cout << "# of one constants  = " << oneConstants.size() << endl;

    if (zeroConstants.size() > 1) {
      CellId zc = zeroConstants.back();
      Cell& zeroCell = def.getCellRef(zc);
      zeroConstants.pop_back();

      for (auto toReplaceId : zeroConstants) {
        Cell& toReplace = def.getCellRef(toReplaceId);

        assert(toReplace.getCellType() == CELL_TYPE_CONST);

        auto receivers = toReplace.getPortReceivers(PORT_ID_OUT);

        for (int offset = 0; offset < (int) receivers.size(); offset++) {
          SignalBit newDriver{zc, PORT_ID_OUT, offset};
          for (auto receiverBit : receivers[offset]) {
            zeroCell.addReceiver(PORT_ID_OUT, offset, receiverBit);

            def.setDriver(receiverBit, newDriver);
          }
        }

        toReplace.clearReceivers(PORT_ID_OUT);
      }
    }

    cout << "Done removing bitconsts" << endl;
    
  }

  void elidePort(const CellId cid,
                 const PortId in,
                 const PortId out,
                 CellDefinition& def) {
    Cell& cell = def.getCellRef(cid);
    auto inDrivers = cell.getDrivers(in);

    assert(cell.getPortWidth(in) == cell.getPortWidth(out));

    auto& receivers = cell.getPortReceivers(out);
    for (int offset = 0; offset < (int) receivers.size(); offset++) {
      SignalBit newDriver = inDrivers.signals[offset];
      auto offsetReceivers = receivers[offset];

      for (auto receiverBit : offsetReceivers) {
        def.setDriver(receiverBit, newDriver);
      }
    }

    cell.clearReceivers(out);
  }

  void cullPassthroughs(CellDefinition& def) {
    std::set<CellId> passthroughs;
    for (auto& ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if ((cell.getCellType() == CELL_TYPE_PASSTHROUGH)) {
        passthroughs.insert(cid);
      }
    }

    cout << "Deleting " << passthroughs.size() << " passthroughs" << endl;
    for (auto cid : passthroughs) {
      elidePort(cid, PORT_ID_IN, PORT_ID_OUT, def);
    }
  }

  void cullZexts(CellDefinition& def) {
    std::set<CellId> idZexts;
    for (auto& ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.getCellType() == CELL_TYPE_ZEXT) {
        int inWidth = bvToInt(cell.getParameterValue(PARAM_IN_WIDTH));
        int outWidth = bvToInt(cell.getParameterValue(PARAM_OUT_WIDTH));

        if (inWidth == outWidth) {
          idZexts.insert(cid);
        }
      }
    }

    cout << "Deleting " << idZexts.size() << " cells" << endl;
    for (auto cid : idZexts) {
      elidePort(cid, PORT_ID_IN, PORT_ID_OUT, def);
    }

  }
  
  void deleteDeadInstances(CellDefinition& def) {

    // TODO: Create proper dataflow analysis

    std::set<CellId> toDelete =
      innerCellsWithNoReceivers(def);

    while (toDelete.size() > 0) {
      cout << "Starting to delete " << toDelete.size() << " cells " << endl;

      def.bulkDelete(toDelete);

      cout << "Done deleting" << endl;

      toDelete = innerCellsWithNoReceivers(def);
    }

    std::set<CellId> unconnectedCells =
      isolatedCells(def);

    cout << "# of inner cells to delete = " << unconnectedCells.size() << endl;

    def.bulkDelete(unconnectedCells);

    std::set<CellId> leftOver =
      innerCellsWithNoReceivers(def);
    
    cout << "# of leftovers = " << leftOver.size() << endl;
    for (auto c : leftOver) {
      cout << "\t" << def.cellName(c) << endl;
    }

    assert(leftOver.size() == 0);

    // Checking that the there are no inputs unconnected to outputs
    set<CellId> connectedToOutputs;
    for (auto cid : def.getPortCells()) {
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.isOutputPortCell()) {
        connectedToOutputs.insert(cid);
      }
    }

    bool foundCell = true;
    while (foundCell) {
      foundCell = false;

      cout << "# of cells connected to outputs = " << connectedToOutputs.size()
           << endl;
      
      for (auto ctp : def.getCellMap()) {
        CellId cid = ctp.first;
        const Cell& cell = def.getCellRefConst(cid);

        for (auto outPort : cell.outputPorts()) {
          for (auto rSp : cell.receiverSigPorts(outPort)) {
            if (dbhc::elem(rSp.cell, connectedToOutputs) &&
                !dbhc::elem(cid, connectedToOutputs)) {

              connectedToOutputs.insert(cid);
              foundCell = true;

            }
          }
        }
      }
    }

    cout << "# of cells connected to outputs = " << connectedToOutputs.size()
         << endl;

    cout << "# of cells                      = " << def.numCells()
         << endl;

    toDelete = {};
    for (auto ctp : def.getCellMap()) {
      const CellId cid = ctp.first;
      if (!elem(cid, connectedToOutputs) && !def.isPortCell(cid)) {
        toDelete.insert(cid);
      }
    }

    cout << "Deleting unconnected instances" << endl;

    def.bulkDelete(toDelete);

    cout << "Done deleting dead instances" << endl;
  }

  void allInputsToConstants(CellDefinition& def) {
    for (auto cid : def.getPortCells()) {
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.isInputPortCell()) {
        int width = cell.getPortWidth(PORT_ID_OUT);
        cout << "Replacing " << def.getCellName(cid) << ", width = " << width << endl;
        def.replaceCellPortWithConstant(cid, PORT_ID_OUT, BitVector(width, 0));
        cout << "Done with replacement" << endl;
      }
    }
  }

  bool driversMatch(const SignalBus& repIn1Drivers,
                    const SignalBus& otherIn1Drivers) {
    auto& rep = repIn1Drivers.signals;
    auto& other = otherIn1Drivers.signals;

    if (rep.size() != other.size()) {
      return false;
    }

    for (int i = 0; i < (int) rep.size(); i++) {
      if (rep[i] != other[i]) {
        return false;
      }
    }

    return true;
  }

  void deDuplicate(CellDefinition& def) {
    cout << "Starting to remove duplicate circuit elements, num cells = " << def.numCells() << endl;

    vector<CellId> cellList = def.getCellList();
    set<CellId> deleted;
    for (int i = 0; i < (int) cellList.size(); i++) {
      const CellId replacerId = cellList[i];

      if (elem(replacerId, deleted)) {
        continue;
      }

      const Cell& replacer = def.getCellRefConst(replacerId);

      for (int j = i + 1; j < (int) cellList.size(); j++) {
        const CellId toReplaceId = cellList[j];

        if (elem(toReplaceId, deleted)) {
          continue;
        }

        const Cell& toReplace = def.getCellRefConst(toReplaceId);

        assert(toReplaceId != replacerId);
        
        if ((replacer.getCellType() == toReplace.getCellType()) &&
            isBinop(replacer.getCellType())) {

          auto otherIn0Drivers = replacer.getDrivers(PORT_ID_IN0);
          auto otherIn1Drivers = replacer.getDrivers(PORT_ID_IN1);

          auto repIn0Drivers = toReplace.getDrivers(PORT_ID_IN0);
          auto repIn1Drivers = toReplace.getDrivers(PORT_ID_IN1);

          bool in0Same = driversMatch(repIn0Drivers, otherIn0Drivers);
          bool in1Same = driversMatch(repIn1Drivers, otherIn1Drivers);

          if (in0Same && in1Same) {
            //            cout << "Can replace\n\t" << def.getCellName(toReplaceId) << "\n\twith\n\t" << def.getCellName(replacerId) << endl;

            vector<set<SignalBit> > receivers =
              toReplace.getPortReceivers(PORT_ID_OUT);

            for (int i = 0; i < (int) receivers.size(); i++) {
              SignalBit freshDriver{replacerId, PORT_ID_OUT, i};
              for (auto rcv : receivers[i]) {
                def.setDriver(rcv, freshDriver);
              }
            }

            //            cout << "Deleting cell" << endl;
            def.deleteCell(toReplaceId);
            deleted.insert(toReplaceId);

            //            cout << "Done deleting" << endl;
            
          } else {
            //cout << "CANT REPLACE" << endl;
          }
          
        }
      }
    }

    cout << "Done removing duplicates, num cells = " << def.numCells() << endl;
  }

  enum CellConstType {
    CELL_IS_CONST,
    CELL_IS_NON_CONST,
    CELL_IS_CONST_SEL_MUX
  };

  bool drivenByConstants(const CellId cid,
                         const PortId pid,
                         const CellDefinition& def,
                         map<CellId, CellConstType>& cellClassification) {

    bool allInputsConst = true;

    const Cell& cell = def.getCellRefConst(cid);
    for (auto driverBit : cell.getDrivers(pid).signals) {
      if (notEmpty(driverBit)) {
        CellId driverId = driverBit.cell;
        if (map_find(driverId, cellClassification) != CELL_IS_CONST) {
          allInputsConst = false;
          break;
        }
      }
    }

    return allInputsConst;
  }
  
  std::map<CellId, CellConstType>
  labelConstantCells(CellDefinition& def, const ValueStore& valueStore) {
    map<CellId, CellConstType> cellClassification;

    // Initialize classification
    std::set<CellId> cellsLeft;
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      cellsLeft.insert(cid);

      const Cell& cell = def.getCellRefConst(cid);
      if (cell.getCellType() == CELL_TYPE_CONST) {
        cellClassification[cid] = CELL_IS_CONST;
      } else {
        cellClassification[cid] = CELL_IS_NON_CONST;
      }
    }

    // Do DF analysis
    while (cellsLeft.size() > 0) {
      //cout << "Cells left = " << cellsLeft.size() << endl;
      CellId cid = *begin(cellsLeft);
      cellsLeft.erase(cid);

      const Cell& cell = def.getCellRefConst(cid);

      CellConstType oldVal = map_find(cid, cellClassification);

      CellConstType newVal = oldVal;

      // TODO: Add handling for registers and memory
      if (cell.getCellType() == CELL_TYPE_MUX) {
        bool selIsConstant =
          drivenByConstants(cid, PORT_ID_SEL, def, cellClassification);

        if (selIsConstant) {
          newVal = CELL_IS_CONST_SEL_MUX;
          BitVector bitVec = valueStore.materializeInput({cid, PORT_ID_SEL});
          if (bitVec == BitVector(1, 0) &&
              drivenByConstants(cid, PORT_ID_IN0, def, cellClassification)) {
            newVal = CELL_IS_CONST;
          } else if (bitVec == BitVector(1, 1) &&
                     drivenByConstants(cid, PORT_ID_IN1, def, cellClassification)) {
            newVal = CELL_IS_CONST;
          }

        } else {
          // DO nothing. We cannot change the mux if select is not constant.
          // TODO: Actuall if both inputs are the same we can fold, but
          // that case does not matter during configuration.
        }
      } else if (cell.getCellType() == CELL_TYPE_REG_ARST) {

        // Problem: Dataflow based constant folding runs into problems with registers
        // The graph structure does not change, so checking for backwards loops is
        // out of the question.
        cout << "Checking register" << endl;

        if (drivenByConstants(cid, PORT_ID_ARST, def, cellClassification)) {

          cout << "Register is driven by constants" << endl;

          bool inputIsOutput = true;
          auto drivers = cell.getDrivers(PORT_ID_IN);
          for (int offset = 0; offset < (int) drivers.signals.size(); offset++) {
            SignalBit driverBit = drivers.signals[offset];

            SignalBit outputValue{cid, PORT_ID_OUT, offset};

            if (driverBit != outputValue) {
              inputIsOutput = false;
              break;
            }
          }

          if (inputIsOutput) {
            cout << "Register is const" << endl;
            newVal = CELL_IS_CONST;
          }
        }
        
      } else if (!cell.isInputPortCell()) {

        bool allInputsConst = true;

        for (auto pid : cell.inputPorts()) {
          for (auto driverBit : cell.getDrivers(pid).signals) {
            if (notEmpty(driverBit)) {
              CellId driverId = driverBit.cell;
              if (map_find(driverId, cellClassification) != CELL_IS_CONST) {
                allInputsConst = false;
                break;
              }
            }
          }

          if (!allInputsConst) {
            break;
          }
        }

        newVal = allInputsConst ? CELL_IS_CONST : CELL_IS_NON_CONST;
      } else {
        // Do not change port cell designations
      }

      // if (newVal == CELL_IS_CONST) {
      //   //cout << "Cell " << def.getCellName(cid) << " is const" << endl;
      // }

      if (newVal != oldVal) {
        cellClassification[cid] = newVal;
        
        for (auto outPort : cell.outputPorts()) {
          auto receivers = cell.getPortReceivers(outPort);
          for (auto rcList : receivers) {
            for (auto rcBit : rcList) {
              if (notEmpty(rcBit)) {
                cellsLeft.insert(rcBit.cell);
              }
            }
          }
        }
      }
    }

    return cellClassification;
  }

  bool isBoundaryCell(const Cell& cell,
                      CellDefinition& def,
                      std::map<CellId, CellConstType>& cellClassification) {

    bool replaceOutputs = false;
          
    for (auto outPort : cell.outputPorts()) {
      auto receivers = cell.getPortReceivers(outPort);
      for (auto rcList : receivers) {
        for (auto rcBit : rcList) {
          if (notEmpty(rcBit) &&
              ((map_find(rcBit.cell, cellClassification) == CELL_IS_NON_CONST)
               ||
               def.isPortCell(rcBit.cell))) {
            replaceOutputs = true;
            //cout << "Cell " << def.getCellName(cid) << " is connected to non-const cell" << endl;
            break;
          }
        }
        if (replaceOutputs) {
          break;
        }
      }
      if (replaceOutputs) {
        break;
      }
    }

    return replaceOutputs;
  }

  // This is a monotonic dataflow analysis
  // Start time: 1:03
  void foldConstantsWRTState(CellDefinition& def,
                             const ValueStore& valueStore) {

    cout << "Starting dataflow" << endl;

    std::map<CellId, CellConstType> cellClassification =
      labelConstantCells(def, valueStore);
    
    cout << "Done with dataflow, now removing constants" << endl;

    std::set<CellId> toDelete;
    // Walk over cells replacing ports with constants as appropriate
    for (auto ctp : cellClassification) {
      CellConstType ct = ctp.second;
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      
      if (ct == CELL_IS_CONST) {
        //cout << "Processing constant cell " << def.getCellName(cid) << endl;
        
        if (!def.isPortCell(cid)) {
          toDelete.insert(cid);

          if (isBoundaryCell(cell, def, cellClassification)) {
            CellType tp = cell.getCellType();

            if (tp == CELL_TYPE_MUX) {
              BitVector sval = valueStore.getPortValue(cid, PORT_ID_SEL);
              BitVector value = valueStore.getPortValue(cid, PORT_ID_IN1);

              if (sval == BitVector(1, 0)) {
                value = valueStore.getPortValue(cid, PORT_ID_IN0);
              }

              def.replaceCellPortWithConstant(cid, PORT_ID_OUT, value);
            } else if (isRegister(tp)) {
              assert(false);
            } else if (tp == CELL_TYPE_MEM) {
              assert(false);
            } else {
              for (auto pid : cell.outputPorts()) {
                
                BitVector value = valueStore.getPortValue(cid, pid);
                def.replaceCellPortWithConstant(cid, pid, value);
              }
            }
          }
        }
      } else if (ct == CELL_IS_CONST_SEL_MUX) {
        cout << "Getting mux sel" << endl;
        BitVector bitVec = valueStore.materializeInput({cid, PORT_ID_SEL});
        cout << "Got mux sel" << endl;

        bool selIn1 = 0;
        if (bitVec.get(0).is_binary()) {
          selIn1 = bitVec.get(0).binary_value() == 1;
        }

        PortId inPort = selIn1 ? PORT_ID_IN1 : PORT_ID_IN0;
        int width = cell.getPortWidth(inPort);

        vector<SignalBit> inDrivers = cell.getDrivers(inPort).signals;
        auto& outReceivers = cell.getPortReceivers(PORT_ID_OUT);

        // if (outReceivers.size() != inDrivers.size()) {
        //   cout << "Error while folding " << def.getCellName(next) << ": outReceivers.size() = " << outReceivers.size() << ", but inDrivers = " << inDrivers.size() << endl;

              
        // }
        assert(outReceivers.size() == inDrivers.size());
            
        for (int offset = 0; offset < width; offset++) {
          SignalBit newDriver = inDrivers[offset];
          auto offsetReceivers = outReceivers[offset];

          for (auto receiverBit : offsetReceivers) {
            def.setDriver(receiverBit, newDriver);
          }
        }

        def.deleteCell(cid);
      }
    }

    cout << "Done with constant port folding, deleting useless cells" << endl;

    def.bulkDelete(toDelete);

    cout << "Done deleting useless cells" << endl;
  }

}
