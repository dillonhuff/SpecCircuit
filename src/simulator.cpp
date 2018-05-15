#include "simulator.h"

#include <fstream>

#include <dlfcn.h>
#include <unistd.h>

using namespace std;

namespace FlatCircuit {

  // Supports creating a unique collection of items in an
  // arbitrary order
  template<typename T>
  class UniqueVector {
    std::vector<T> vec;
    std::set<T> alreadyAdded;
  public:

    bool elem(const T& t) const {
      return dbhc::elem(t, alreadyAdded);
    }

    size_t size() const {
      assert(alreadyAdded.size() == vec.size());

      return vec.size();
    }

    void push_back(const T& t) {
      if (!dbhc::elem(t, alreadyAdded)) {
        vec.push_back(t);
        alreadyAdded.insert(t);
      } else {
        assert(false);
      }
    }

    std::vector<T> getVec() const {
      return vec;
    }
  };

  std::vector<SigPort> Simulator::getDataPorts(const CellId sp) {
    UniqueVector<SigPort> dataPorts;

    const Cell& c = def.getCellRefConst(sp);

    CellType tp = c.getCellType();
    if (isUnop(tp) || (tp == CELL_TYPE_REG) || (tp == CELL_TYPE_REG_ARST)) {

      return {{sp, PORT_ID_IN}};

    } else if ((c.getCellType() == CELL_TYPE_MUX) || (isBinop(c.getCellType()))) {

      return {{sp, PORT_ID_IN0}, {sp, PORT_ID_IN1}};

    } else if (tp == CELL_TYPE_PORT) {
      if (c.isInputPortCell()) {
        return {};
      } else {
        return {{sp, PORT_ID_IN}};
      }
    } else if (tp == CELL_TYPE_CONST) {
      return {};
    } else if (tp == CELL_TYPE_SLICE) {
      return {{sp, PORT_ID_IN}};
    } else {
      cout << "Unsupported sig port " << sigPortString(def, {sp, PORT_ID_OUT}) << endl;
      assert(false);
    }

    return dataPorts.getVec();
  }

  std::vector<SigPort> Simulator::dataSources(const SigPort sp) {
    UniqueVector<SigPort> srcPorts;

    cout << "Getting data sources for " << sigPortString(def, sp) << endl;

    const Cell& c = def.getCellRefConst(sp.cell);

    cout << "Got data sources for " << sigPortString(def, sp) << endl;

    CellType tp = c.getCellType();

    cout << "Got cell type" << endl;

    if (tp == CELL_TYPE_PORT) {
      if (!c.isInputPortCell()) {

        cout << "Is output cell port" << endl;

        for (auto sigBit : c.getDrivers(PORT_ID_IN).signals) {
          if (notEmpty(sigBit)) {
            SigPort dp = {sigBit.cell, sigBit.port};
            srcPorts.push_back(dp);
          }
        }

      } else {
        cout << "Is input cell port" << endl;
      }
    } else if (tp == CELL_TYPE_CONST) {
      return {};
    } else if (isUnop(c.getCellType()) ||
        (tp == CELL_TYPE_REG_ARST) ||
        (tp == CELL_TYPE_REG)) {

      assert(sp.port == PORT_ID_OUT);

      for (auto sigBit : c.getDrivers(PORT_ID_IN).signals) {
        if (notEmpty(sigBit)) {
          SigPort dp = {sigBit.cell, sigBit.port};
          srcPorts.push_back(dp);
        }
      }
    } else if ((c.getCellType() == CELL_TYPE_MUX) || (isBinop(c.getCellType()))) {

      assert(sp.port == PORT_ID_OUT);

      for (auto sigBit : c.getDrivers(PORT_ID_IN0).signals) {
        if (notEmpty(sigBit)) {
          SigPort dp = {sigBit.cell, sigBit.port};
          srcPorts.push_back(dp);
        }
      }

      for (auto sigBit : c.getDrivers(PORT_ID_IN1).signals) {
        if (notEmpty(sigBit)) {
          SigPort dp = {sigBit.cell, sigBit.port};
          srcPorts.push_back(dp);
        }
      }

    } else if (tp == CELL_TYPE_SLICE) {
      for (auto sigBit : c.getDrivers(PORT_ID_IN).signals) {
        if (notEmpty(sigBit)) {
          SigPort dp = {sigBit.cell, sigBit.port};
          srcPorts.push_back(dp);
        }
      }

    } else {
      cout << "Unsupported sig port in data sources" << sigPortString(def, sp) << endl;
      assert(false);
    }

    return srcPorts.getVec();
  }

  std::vector<SigPort> Simulator::traceValue(const CellId cellId,
                                             const PortId portId) {
    std::vector<SigPort> receiversToTrace{{cellId, portId}};
    std::vector<SigPort> trace;
    std::set<SigPort> alreadyAdded;

    while (receiversToTrace.size() > 0) {
      SigPort nextReceiver = receiversToTrace.back();
      receiversToTrace.pop_back();

      cout << "Tracing " << sigPortString(def, nextReceiver) << endl;

      CellId cid = nextReceiver.cell;
      PortId pid = nextReceiver.port;

      for (auto sigBit : def.getCellRefConst(cid).getDrivers(pid).signals) {

        SigPort next = {sigBit.cell, sigBit.port};
        if (!elem(next, alreadyAdded)) {
          trace.push_back(next);
          alreadyAdded.insert(next);

          vector<SigPort> drivers = dataSources(next);

          cout << "Adding data ports for drivers to receivers" << endl;
          for (auto d : drivers) {
            for (auto rPort : getDataPorts(d.cell)) {
              if (!elem(rPort, alreadyAdded)) {
                receiversToTrace.push_back(rPort);
              }
            }
          }
        }
      }

      cout << "Done tracing " << sigPortString(def, nextReceiver) << endl;
    }

    return trace;
  }

  std::set<CellId> levelZeroCells(const CellDefinition& def) {
    set<CellId> levelZero;
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.getCellType() == CELL_TYPE_PORT) {
        if (cell.isInputPortCell()) {
          levelZero.insert(cid);
        }
      } else if ((cell.getCellType() == CELL_TYPE_CONST) ||
                 isRegister(cell.getCellType())) {
        levelZero.insert(cid);
      }
    }

    return levelZero;
  }

  std::vector<SigPort>
  combinationalDependencies(const Cell& cell,
                            const PortId pid,
                            const CellDefinition& def) {
    vector<SigPort> deps;
    
    for (auto sigPort : cell.receiverSigPorts(pid)) {
      const Cell& receiverCell = def.getCellRefConst(sigPort.cell);

      if ((receiverCell.getCellType() == CELL_TYPE_MEM) &&
          (sigPort.port != PORT_ID_RADDR)) {
        continue;
      }

      if (!isRegister(receiverCell.getCellType()) &&
          !((receiverCell.getCellType() == CELL_TYPE_MEM) &&
            (sigPort.port == PORT_ID_RADDR)) &&
          (sigPort.port != PORT_ID_ARST) &&
          (sigPort.port != PORT_ID_CLK)) {

        deps.push_back(sigPort);

      }
    }

    return deps;
  }

  std::vector<SigPort>
  sequentialDependencies(const Cell& cell,
                         const PortId pid) {
    vector<SigPort> deps;
    
    for (auto sigPort : cell.receiverSigPorts(pid)) {
      if ((sigPort.port == PORT_ID_ARST) ||
          (sigPort.port == PORT_ID_CLK)) {

        deps.push_back(sigPort);

      }
    }

    return deps;
  }

  void updateDependencies(const Cell& cell,
                          const PortId pid,
                          set<SigPort>& freshChanges,
                          //                          UniqueVector<SigPort>& combChanges,
                          std::vector<SigPort>& combChanges,
                          std::set<SigPort>& seqChanges,
                          const CellDefinition& def) {
    for (auto sigPort : combinationalDependencies(cell, pid, def)) {
      //      if (!combChanges.elem(sigPort)) {
      freshChanges.insert(sigPort);
      combChanges.push_back(sigPort);
      //      } else {
      //        assert(false);
      //      }
    }

    for (auto sigPort : sequentialDependencies(cell, pid)) {
      seqChanges.insert(sigPort);
    }
    
  }
  
  std::vector<std::vector<SigPort> >
  staticSimulationEvents(const CellDefinition& def) {

    vector<vector<SigPort> > staticEvents;
    //    UniqueVector<SigPort> combChanges;
    std::vector<SigPort> combChanges;
    set<SigPort> seqChanges;

    set<SigPort> freshChanges;

    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);

      CellType tp = cell.getCellType();
      if ((tp == CELL_TYPE_REG) ||
          (tp == CELL_TYPE_CONST) ||
          (tp == CELL_TYPE_REG_ARST) ||
          cell.isInputPortCell()) {

        updateDependencies(cell, PORT_ID_OUT, freshChanges,
                           combChanges, seqChanges, def);

      } else if (tp == CELL_TYPE_MEM) {

        updateDependencies(cell, PORT_ID_RDATA,
                           freshChanges, combChanges, seqChanges,
                           def);
        
      }
    }

    cout << "Starting to collect events" << endl;
    do {

      cout << "In getting comb changes" << endl;
      while (freshChanges.size() > 0) {
        SigPort sigPort = *std::begin(freshChanges);
        freshChanges.erase(sigPort);

        const Cell& c = def.getCellRefConst(sigPort.cell);

        //cout << "\tUpdating cell " << sigPortString(def, sigPort) << endl;

        for (auto outPort : c.outputPorts()) {
          updateDependencies(c, outPort, freshChanges, combChanges, seqChanges, def);
        }
        
      }

      cout << "In got " << combChanges.size() << " comb changes" << endl;

      //staticEvents.push_back(combChanges.getVec());
      staticEvents.push_back(combChanges);
      combChanges = {};

      vector<SigPort> seqChangesVec(begin(seqChanges), end(seqChanges));
      staticEvents.push_back(seqChangesVec);

      for (auto seqPort : seqChanges) {
        const Cell& cell = def.getCellRefConst(seqPort.cell);

        if (cell.getCellType() == CELL_TYPE_MEM) {
          combChanges.push_back({seqPort.cell, PORT_ID_RADDR});
          freshChanges.insert({seqPort.cell, PORT_ID_RADDR});
        } else if (isRegister(cell.getCellType())) {
          combChanges.push_back({seqPort.cell, PORT_ID_OUT});
          freshChanges.insert({seqPort.cell, PORT_ID_OUT});
        }

        for (auto outPort : def.getCellRefConst(seqPort.cell).outputPorts()) {
          updateDependencies(def.getCellRefConst(seqPort.cell), outPort,
                             freshChanges, combChanges, seqChanges, def);
        }

      }

      // NOTE: This needs to change in order for simulation semantics to be correct
      seqChanges = {};
    } while (combChanges.size() > 0);

    cout << "Done collecting events" << endl;

    return staticEvents;
  }

  maybe<std::vector<CellId> >
  levelizeCircuit(const CellDefinition& def) {
    UniqueVector<CellId> levelized;
    for (auto cid : levelZeroCells(def)) {
      levelized.push_back(cid);
    }

    bool addedNew = true;
    while (addedNew) {
      addedNew = false;
      for (auto ctp : def.getCellMap()) {
        CellId cid = ctp.first;
        if (levelized.elem(cid)) {
          continue;
        }

        const Cell& cell = def.getCellRefConst(cid);

        bool allDriversInEarlierLevels = true;
        for (auto port : cell.inputPorts()) {
          for (auto driverBit : cell.getDrivers(port).signals) {
            if (notEmpty(driverBit) && !levelized.elem(driverBit.cell)) {
              allDriversInEarlierLevels = false;
              break;
            }
          }
        }

        if (allDriversInEarlierLevels) {
          addedNew = true;
          levelized.push_back(cid);
          break;
        }
      }
    }

    return levelized.getVec();
  }

  void compileCppLib(const std::string& cppName,
                     const std::string& targetBinary) {
    int ret =
      system(("clang++ -std=c++11 -O3 -fPIC -dynamiclib -I/Users/dillon/CppWorkspace/bsim/src/ " + cppName + " -o " + targetBinary).c_str());

    //    sleep(2);

    assert(ret == 0);
  }

  struct DylibInfo {
    void* libHandle;
    void* simFuncHandle;
  };
  
  DylibInfo loadLibWithFunc(const std::string& targetBinary) {
    void* myLibHandle = dlopen(targetBinary.c_str(), RTLD_LOCAL);

    if (myLibHandle == nullptr) {
      printf("dlsym failed: %s\n", dlerror());
      assert(false);
    }

    cout << "lib handle = " << myLibHandle << endl;

    void* myFuncFunV;
    // Must remove the first underscore from the symbol name to find it?
    myFuncFunV = dlsym(myLibHandle, "_Z8simulateRNSt3__16vectorIN4bsim21quad_value_bit_vectorENS_9allocatorIS2_EEEE");
    if (myFuncFunV == nullptr) {
      printf("dlsym failed: %s\n", dlerror());
      assert(false);
    } else {
      printf("FOUND\n");
    }

    return {myLibHandle, myFuncFunV};
  }

  std::string ln(const std::string& s) {
    return "\t" + s + ";\n";
  }
  
  std::string
  Simulator::codeToMaterializeOffset(const CellId cid,
                                     const PortId pid,
                                     const std::string& argName,
                                     const std::map<SigPort, unsigned long>& offsets) const {
    const Cell& cell = def.getCellRefConst(cid);
    auto drivers = cell.getDrivers(pid);
    string cppCode = "";
    cppCode += ln("bsim::quad_value_bit_vector " + argName + "(" +
                  to_string(drivers.signals.size()) + ", 0)");

    bool canDirectCopy = true;
    CellId singleDriverCell;
    PortId singleDriverPort;
    set<CellId> driverCells;

    for (int offset = 0; offset < drivers.signals.size(); offset++) {
      SignalBit driverBit = drivers.signals[offset];
      driverCells.insert(driverBit.cell);

      singleDriverCell = driverBit.cell;
      singleDriverPort = driverBit.port;

      if (driverCells.size() > 1) {
        canDirectCopy = false;
        break;
      }

      if (driverBit.offset != offset) {
        canDirectCopy = false;
        break;
      }
    }

    if (def.getCellRefConst(singleDriverCell).getPortWidth(singleDriverPort) !=
        cell.getPortWidth(pid)) {
      canDirectCopy = false;
    }

    SigPort sp = {singleDriverCell, singleDriverPort};
    if (canDirectCopy && !contains_key(sp, offsets)) {
      cout << "Error: " << sigPortString(def, sp) << endl;
      assert(contains_key(sp, offsets));
    }

    if (canDirectCopy) {
      cppCode += ln(argName + " = " + "values[" + to_string(map_find(sp, offsets)) + "]");
    } else {

      for (int offset = 0; offset < drivers.signals.size(); offset++) {
        SignalBit driverBit = drivers.signals[offset];
        string valString = "values[" + to_string(map_find({driverBit.cell, driverBit.port}, offsets)) + "].get(" + to_string(driverBit.offset) + ")";
        // string valString = "values[" + to_string(map_find(sp, offsets)) +
        //   "].get(" + to_string(driverBit.offset) + ")";
        cppCode += ln(argName + ".set(" + to_string(offset) + ", " + valString + ")");
      }
    }

    return cppCode;
    
  }
  
  std::string Simulator::codeToMaterialize(const CellId cid,
                                           const PortId pid,
                                           const std::string& argName) const {
    return codeToMaterializeOffset(cid, pid, argName, portOffsets);
  }

  std::string
  Simulator::sequentialBlockCode(const std::vector<SigPort>& levelized) {
    string cppCode = "";
    
    for (auto sigPort : levelized) {
      CellId cid = sigPort.cell;
      const Cell& cell = def.getCellRefConst(cid);

      cppCode += ln("// ----- Sequential update for cell " + def.cellName(cid));

      cppCode += "\t{\n";      

      CellType tp = cell.getCellType();
      if (tp == CELL_TYPE_REG_ARST) {

        string inVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, inVar);

        string clkVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_CLK);
        cppCode += codeToMaterialize(cid, PORT_ID_CLK, clkVar);
        string lastClkVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_CLK) + "_last";
        cppCode += codeToMaterializeOffset(cid, PORT_ID_CLK, lastClkVar, pastValueOffsets);

        string rstVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_ARST);
        cppCode += codeToMaterialize(cid, PORT_ID_ARST, rstVar);
        string lastRstVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_ARST) + "_last";
        cppCode += codeToMaterializeOffset(cid, PORT_ID_ARST, lastRstVar, pastValueOffsets);

        string updateValueClk = "values[" + to_string(map_find(cid, registerOffsets)) + "] = " + inVar + ";";
        if (cell.clkPosedge()) {
          cppCode += "\tif (posedge(" + lastClkVar + ", " + clkVar + ")) { " + updateValueClk + " }\n";
        } else {
          cppCode += "\tif (negedge(" + lastClkVar + ", " + clkVar + ")) { " + updateValueClk + " }\n";
        }

        BitVector init = cell.getParameterValue(PARAM_INIT_VALUE);
        string updateValueRst = "values[" + to_string(map_find(cid, registerOffsets)) + "] = BitVector(\"" + init.hex_string() + "\")" + ";";
        if (cell.rstPosedge()) {
          cppCode += "\tif (posedge(" + lastRstVar + ", " + rstVar + ")) { " + updateValueRst + " }\n";
        } else {
          cppCode += "\tif (negedge(" + lastRstVar + ", " + rstVar + ")) { " + updateValueRst + " }\n";
        }
        
      } else if (tp == CELL_TYPE_REG) {

        string inVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, inVar);

        string clkVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_CLK);
        cppCode += codeToMaterialize(cid, PORT_ID_CLK, clkVar);
        string lastClkVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_CLK) + "_last";
        cppCode += codeToMaterializeOffset(cid, PORT_ID_CLK, lastClkVar, pastValueOffsets);

        string updateValueClk = "values[" + to_string(map_find(cid, registerOffsets)) + "] = " + inVar + ";";
        if (cell.clkPosedge()) {
          cppCode += "\tif (posedge(" + lastClkVar + ", " + clkVar + ")) { " + updateValueClk + " }\n";
        } else {
          cppCode += "\tif (negedge(" + lastClkVar + ", " + clkVar + ")) { " + updateValueClk + " }\n";
        }

      } else if (tp == CELL_TYPE_MEM) {

        string clkVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_CLK);
        cppCode += codeToMaterialize(cid, PORT_ID_CLK, clkVar);

        string lastClkVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_CLK) + "_last";
        cppCode += codeToMaterializeOffset(cid, PORT_ID_CLK, lastClkVar, pastValueOffsets);

        string waddrName =
          "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_WADDR);
        cppCode += codeToMaterialize(cid, PORT_ID_WADDR, waddrName);

        string wdataName =
          "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_WDATA);
        cppCode += codeToMaterialize(cid, PORT_ID_WDATA, wdataName);

        string wenName =
          "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_WEN);
        cppCode += codeToMaterialize(cid, PORT_ID_WEN, wenName);
        
        string updateValueClk = "values[" +
          to_string(map_find(cid, memoryOffsets)) + " + " +
          "(" + waddrName + ".is_binary() ? " + waddrName + ".to_type<int>() : 0)] = " + wdataName + ";";

        cppCode += "\tif ((" + wenName + " == BitVector(1, 1))";
        cppCode += " && posedge(" + lastClkVar + ", " + clkVar + ")) { " + updateValueClk + " }\n";

      } else {
        cout << "Unsupported cell " << def.cellName(cid) << " in sequentialBlockCode" << endl;
        assert(false);
      }

      cppCode += "\t}\n";

    }


    return cppCode;
  }

  BinopCode Simulator::addBinop(const std::string& allCode,
                                const CellId cid) const {
    string cppCode = "";
    string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
    cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

    string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
    cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

    cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = BitVector(1, (" + argName0 + " < " + argName1 + ") || (" +
                  argName0 + " == " + argName1 + "))");

    return {argName0, argName1, allCode + cppCode};
  }
  
  std::string
  Simulator::combinationalBlockCode(const std::vector<SigPort>& levelized) {

    string cppCode = "";
    
    for (auto sigPort : levelized) {
      CellId cid = sigPort.cell;
      PortId port = sigPort.port;
      
      const Cell& cell = def.getCellRefConst(cid);
      cppCode += ln("// ----- Code for cell " + def.cellName(cid) + ", " + portIdString(port));

      cppCode += "\t{\n";

      bool sentToSeqPort = false;
      PortId outPort = PORT_ID_OUT;
      if (elem(outPort, cell.outputPorts())) {
        for (auto& receiverBus : cell.getPortReceivers(PORT_ID_OUT)) {
          for (auto& sigBit : receiverBus) {

            if (notEmpty(sigBit)) {
              if ((sigBit.port != PORT_ID_ARST) &&
                  (sigBit.port != PORT_ID_CLK)) {
              } else {
                sentToSeqPort = true;
                break;
              }
            }

          }
        }

        if (sentToSeqPort) {
          string oldOutName = "cell_" + to_string(cid) + "_" +
            portIdString(PORT_ID_OUT) + "_old_value";
          cppCode += ln("BitVector " + oldOutName + " = values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "]");
        }
      }
      
      if ((cell.getCellType() == CELL_TYPE_PORT) && !cell.isInputPortCell()) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        cppCode += ln("values[" + to_string(portValueOffset(cid, PORT_ID_IN)) + "] = " + argName);

      } else if (cell.isInputPortCell()) {
        cppCode += ln("// No code for input port " + def.cellName(cid));
      } else if (cell.getCellType() == CELL_TYPE_CONST) {
        cppCode += ln("// No code for const port " + def.cellName(cid));
      } else if (cell.getCellType() == CELL_TYPE_ZEXT) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        int outWidth = cell.getPortWidth(PORT_ID_OUT);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = zero_extend(" + to_string(outWidth) + ", " + argName + ")");

      } else if (cell.getCellType() == CELL_TYPE_UGE) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = BitVector(1, (" + argName0 + " > " + argName1 + ") || (" +
                      argName0 + " == " + argName1 + "))");

      } else if (cell.getCellType() == CELL_TYPE_ULE) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = BitVector(1, (" + argName0 + " < " + argName1 + ") || (" +
                      argName0 + " == " + argName1 + "))");

      } else if (cell.getCellType() == CELL_TYPE_UGT) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = BitVector(1, (" + argName0 + " > " + argName1 + "))");

      } else if (cell.getCellType() == CELL_TYPE_ULT) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = BitVector(1, (" + argName0 + " < " + argName1 + "))");

      } else if (cell.getCellType() == CELL_TYPE_LSHR) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = lshr(" + argName0 + ", " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_ASHR) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = ashr(" + argName0 + ", " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_SHL) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = shl(" + argName0 + ", " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_SLICE) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName0);

        int start = bvToInt(cell.getParameterValue(PARAM_LOW));
        int end = bvToInt(cell.getParameterValue(PARAM_HIGH));

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = slice(" + argName0 + ", " + to_string(start) + ", " + to_string(end) + ")");
        
      } else if (cell.getCellType() == CELL_TYPE_SUB) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = sub_general_width_bv(" + argName0 + ", " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_ADD) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = add_general_width_bv(" + argName0 + ", " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_MUL) {

        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = mul_general_width_bv(" + argName0 + ", " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_AND) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = (" + argName0 + " & " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_OR) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = (" + argName0 + " | " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_EQ) {

        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = BitVector(1, " + argName0 + " == " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_NEQ) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = BitVector(1, " + argName0 + " != " + argName1 + ")");

      } else if (cell.getCellType() == CELL_TYPE_ORR) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = orr(" + argName + ")");

      } else if (cell.getCellType() == CELL_TYPE_NOT) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = ~(" + argName + ")");

      } else if (cell.getCellType() == CELL_TYPE_MEM) {
        string raddrName =
          "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_RADDR);
        cppCode += codeToMaterialize(cid, PORT_ID_RADDR, raddrName);

        
        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_RDATA}, portOffsets)) + "] = values[" + to_string(map_find(cid, memoryOffsets)) + " + " +
                      "(" + raddrName + ".is_binary() ? " + raddrName + ".to_type<int>() : 0)]");
        
      } else if (cell.getCellType() == CELL_TYPE_MUX) {

        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        string sel = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_SEL);
        cppCode += codeToMaterialize(cid, PORT_ID_SEL, sel);
        
        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = (" + sel + " == BitVector(1, 1) ? " + argName1 + " : " + argName0 + ")");
        
      } else if (cell.getCellType() == CELL_TYPE_REG_ARST) {

        string state = "values[" + to_string(map_find(cid, registerOffsets)) + "]";

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = " + state);
        
      } else if (cell.getCellType() == CELL_TYPE_REG) {
        string state = "values[" + to_string(map_find(cid, registerOffsets)) + "]";

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = " + state);
        
      } else if (cell.getCellType() == CELL_TYPE_PASSTHROUGH) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = " + argName);
        
      } else {
        cout << "Signal Port " << toString(def, {cid, port, 0}) << endl;
        cout << "Insert code for unsupported node " + def.cellName(cid)
             << " : " << toString(def.getCellRefConst(cid).getCellType()) << endl;
        assert(false);
      }

      if (sentToSeqPort) {
        string oldOutName = "cell_" + to_string(cid) + "_" +
          portIdString(PORT_ID_OUT) + "_old_value";

        cppCode += ln("values[" + to_string(map_find({cid, PORT_ID_OUT}, pastValueOffsets)) + "] = " + oldOutName);

      }
      //cout << "Done" << endl;

      cppCode += "\t}\n";
    }



    return cppCode;
  }

  void
  Simulator::compileLevelizedCircuit(const std::vector<std::vector<SigPort> >& updates) {
    string cppCode = "#include <vector>\n#include \"quad_value_bit_vector.h\"\n"
      "using namespace bsim;\n\n";

    cppCode += ln("// Input layout");
    for (auto cid : def.getPortCells()) {
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.isInputPortCell()) {
        cppCode += ln("// " + def.getCellName(cid) + " ---> " +
                      to_string(map_find({cid, PORT_ID_OUT}, portOffsets)));
      }
    }

    cppCode +=
      "typedef bsim::quad_value_bit_vector BitVector;\n\n"
      "bool posedge(const bsim::quad_value_bit_vector& a, const bsim::quad_value_bit_vector& b) { return (a == BitVector(1, 0)) && (b == BitVector(1, 1)); }\n\n"
      "bool negedge(const bsim::quad_value_bit_vector& a, const bsim::quad_value_bit_vector& b) { return (a == BitVector(1, 1)) && (b == BitVector(1, 0)); }\n\n"
      "void simulate(std::vector<bsim::quad_value_bit_vector>& values) {\n";

    assert((updates.size() % 2) == 0);
    
    for (int i = 0; i < updates.size(); i += 2) {
      cppCode += combinationalBlockCode(updates[i + 0]);
      cppCode += sequentialBlockCode(updates[i + 1]);
    }

    cppCode += "}";

    string libName = "circuit_jit";
    string targetBinary = "./lib" + libName + ".dylib";
    string cppName = "./" + libName + ".cpp";
    ofstream out(cppName);
    out << cppCode << endl;

    compileCppLib(cppName, targetBinary);

    DylibInfo dlib = loadLibWithFunc(targetBinary);
    libHandle = dlib.libHandle;
    simulateFuncHandle = dlib.simFuncHandle;
  }

  // An update to a node is dead if:
  // 1. There is another update to the same node later
  // 2. Nothing the node drives is updated between the current and next update
  std::vector<std::vector<SigPort> >
  deleteDeadUpdates(const std::vector<std::vector<SigPort> >& filteredUpdates,
                    const CellDefinition& def) {
    assert((filteredUpdates.size() % 2) == 0);

    std::vector<std::vector<SigPort> > updates;
    for (int i = 0; i < (int) filteredUpdates.size(); i += 2) {
      vector<SigPort> combUpdates = updates[i];
      bool updateLater = false;
      bool anyUseBeforeUpdate = false;
      
      for (int j = i + 1; j < (int) filteredUpdates.size(); j++) {
        
      }
    }
    
    return updates;
  }

  bool Simulator::compileCircuit() {

    vector<vector<SigPort> > updates = staticSimulationEvents(def);

    vector<vector<SigPort> > filteredUpdates;
    for (auto updateGroup : updates) {
      vector<SigPort> filtered;

      auto reversedUpdateGroup = updateGroup;
      reverse(reversedUpdateGroup);

      vector<SigPort> filteredReversed;
      set<CellId> usedCells;
      for (auto sigPort : reversedUpdateGroup) {
        CellId cell = sigPort.cell;

        if (!elem(cell, usedCells)) {
          usedCells.insert(cell);
          filteredReversed.push_back(sigPort);
        }
      }

      reverse(filteredReversed);

      filteredUpdates.push_back(filteredReversed);
    }

    compileLevelizedCircuit(deleteDeadUpdates(filteredUpdates, def));

    return updates.size() > 1;
  }

  Simulator::~Simulator() {
    if (libHandle != nullptr) {
      dlclose(libHandle);
    }
  }

  void Simulator::debugPrintTableValues() const {
    cout << "Table values" << endl;
    for (int i = 0; i < (int) simValueTable.size(); i++) {
      cout << "\t" << i << " = " << simValueTable.at(i) << endl;
    }
  }

  void Simulator::debugPrintRegisters() const {
    cout << "Register values" << endl;
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (isRegister(cell.getCellType())) {
        cout << "\t" << def.getCellName(cid) << " state = " <<
          getRegisterValue(cid) << endl;
      }
    }

  }

  void Simulator::debugPrintPorts() const {
    cout << "Port values" << endl;
    for (auto cid : def.getPortCells()) {
      if (def.getCellRefConst(cid).isInputPortCell()) {
        cout << "\t" << def.getCellName(cid) << " = " << getBitVec(cid, PORT_ID_OUT) << endl;
      } else {
        cout << "\t" << def.getCellName(cid) << " = " << getBitVec(cid, PORT_ID_IN) << endl;
      }
    }

  }
  
  void Simulator::debugPrintMemories() const {
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.getCellType() == CELL_TYPE_MEM) {
        int depth = cell.getMemDepth();
        cout << "Memory " << def.getCellName(cid) << " values" << endl;
        cout << "\tRADDR = " << materializeInput({cid, PORT_ID_RADDR}) << endl;
        cout << "\tRDATA = " << getPortValue(cid, PORT_ID_RDATA) << endl;
        cout << "\tWADDR = " << materializeInput({cid, PORT_ID_WADDR}) << endl;
        cout << "\tWDATA = " << materializeInput({cid, PORT_ID_WDATA}) << endl;
        cout << "\tWEN   = " << materializeInput({cid, PORT_ID_WEN}) << endl;
        cout << "\tCLK   = " << materializeInput({cid, PORT_ID_CLK}) << endl;
        for (int i = 0; i < depth; i++) {
          cout << "\t" << i << " --> " << getMemoryValue(cid, i) << ", " << getMemoryValue(cid, i).to_type<int>() << endl;
        }
      }
    }
  }

  bool matchesPrefix(const std::string& str,
                     const std::string& prefix) {
    if (str.size() < prefix.size()) {
      return false;
    }

    if (str.substr(0, prefix.size()) == prefix) {
      return true;
    }

    return false;
  }
  
  bool matchesAnyPrefix(const std::string& str,
                        const std::vector<std::string>& prefixes) {
    for (auto prefix : prefixes) {
      if (matchesPrefix(str, prefix)) {
        return true;
      }
    }
    return false;
  }

  void
  Simulator::debugPrintMemories(const std::vector<std::string>& prefixes) const {
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.getCellType() == CELL_TYPE_MEM) {

        if (matchesAnyPrefix(def.getCellName(cid), prefixes)) {
          int depth = cell.getMemDepth();
          cout << "Memory " << def.getCellName(cid) << " values" << endl;
          cout << "\tRADDR = " << materializeInput({cid, PORT_ID_RADDR}) << endl;
          cout << "\tRDATA = " << getPortValue(cid, PORT_ID_RDATA) << endl;
          cout << "\tWADDR = " << materializeInput({cid, PORT_ID_WADDR}) << endl;
          cout << "\tWDATA = " << materializeInput({cid, PORT_ID_WDATA}) << endl;
          cout << "\tWEN   = " << materializeInput({cid, PORT_ID_WEN}) << endl;
          cout << "\tCLK   = " << materializeInput({cid, PORT_ID_CLK}) << endl;
          for (int i = 0; i < depth; i++) {
            cout << "\t" << i << " --> " << getMemoryValue(cid, i) << ", " << getMemoryValue(cid, i).to_type<int>() << endl;
          }
        }
      }
    }
  }
  
}
