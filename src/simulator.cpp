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

  std::vector<std::vector<SigPort> >
  staticSimulationEvents(const CellDefinition& def) {
    vector<vector<SigPort> > staticEvents;
    UniqueVector<SigPort> combChanges;
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
        for (auto sigPort : cell.receiverSigPorts(PORT_ID_OUT)) {
          if ((sigPort.port != PORT_ID_ARST) &&
              (sigPort.port != PORT_ID_CLK)) {
            freshChanges.insert(sigPort);
            combChanges.push_back(sigPort);
          } else {
            seqChanges.insert(sigPort);
          }
        }
      }
    }

    cout << "Starting to collect events" << endl;
    do {

      while (freshChanges.size() > 0) {
        SigPort sigPort = *std::begin(freshChanges);
        freshChanges.erase(sigPort);

        const Cell& c = def.getCellRefConst(sigPort.cell);

        for (auto outPort : c.outputPorts()) {
          for (auto rPort : c.receiverSigPorts(outPort)) {
            if ((rPort.port != PORT_ID_ARST) &&
                (rPort.port != PORT_ID_CLK)) {
              freshChanges.insert(rPort);
              combChanges.push_back(rPort);
            } else {
              seqChanges.insert(rPort);
            }
          }
        }
        
      }

      staticEvents.push_back(combChanges.getVec());
      combChanges = {};

      vector<SigPort> seqChangesVec(begin(seqChanges), end(seqChanges));
      staticEvents.push_back(seqChangesVec);

      for (auto seqPort : seqChanges) {
        for (auto outPort : def.getCellRefConst(seqPort.cell).outputPorts()) {
          for (auto rPort : def.getCellRefConst(seqPort.cell).receiverSigPorts(outPort)) {
            if ((rPort.port != PORT_ID_ARST) &&
                (rPort.port != PORT_ID_CLK)) {
              freshChanges.insert(rPort);
              combChanges.push_back(rPort);
            } else {
              // TODO: Improve this update
              seqChanges.insert(rPort);
            }
          }
        }
      }

      // // NOTE: This needs to change in order for simulation semantics to be correct
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
      system(("clang++ -std=c++11 -fPIC -dynamiclib -I/Users/dillon/CppWorkspace/bsim/src/ " + cppName + " -o " + targetBinary).c_str());

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
                                     const std::map<SigPort, unsigned long>& offsets) {
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

    if (canDirectCopy) {
      cppCode += ln(argName + " = " + "values[" + to_string(map_find({singleDriverCell, singleDriverPort}, offsets)) + "]");
    } else {

      for (int offset = 0; offset < drivers.signals.size(); offset++) {
        SignalBit driverBit = drivers.signals[offset];
        string valString = "values[" + to_string(map_find({driverBit.cell, driverBit.port}, offsets)) + "].get(" + to_string(driverBit.offset) + ")";
        cppCode += ln(argName + ".set(" + to_string(offset) + ", " + valString + ")");
      }
    }

    return cppCode;
    
  }
  
  std::string Simulator::codeToMaterialize(const CellId cid,
                                           const PortId pid,
                                           const std::string& argName) {
    return codeToMaterializeOffset(cid, pid, argName, portOffsets);
    // const Cell& cell = def.getCellRefConst(cid);
    // auto drivers = cell.getDrivers(pid);
    // string cppCode = "";
    // cppCode += ln("bsim::quad_value_bit_vector " + argName + "(" +
    //               to_string(drivers.signals.size()) + ", 0)");

    // bool canDirectCopy = true;
    // CellId singleDriverCell;
    // PortId singleDriverPort;
    // set<CellId> driverCells;

    // for (int offset = 0; offset < drivers.signals.size(); offset++) {
    //   SignalBit driverBit = drivers.signals[offset];
    //   driverCells.insert(driverBit.cell);

    //   singleDriverCell = driverBit.cell;
    //   singleDriverPort = driverBit.port;

    //   if (driverCells.size() > 1) {
    //     canDirectCopy = false;
    //     break;
    //   }

    //   if (driverBit.offset != offset) {
    //     canDirectCopy = false;
    //     break;
    //   }
    // }

    // if (def.getCellRefConst(singleDriverCell).getPortWidth(singleDriverPort) !=
    //     cell.getPortWidth(pid)) {
    //   canDirectCopy = false;
    // }

    // if (canDirectCopy) {
    //   cppCode += ln(argName + " = " + "values[" + to_string(map_find({singleDriverCell, singleDriverPort}, portOffsets)) + "]");
    // } else {

    //   for (int offset = 0; offset < drivers.signals.size(); offset++) {
    //     SignalBit driverBit = drivers.signals[offset];
    //     string valString = "values[" + to_string(map_find({driverBit.cell, driverBit.port}, portOffsets)) + "].get(" + to_string(driverBit.offset) + ")";
    //     cppCode += ln(argName + ".set(" + to_string(offset) + ", " + valString + ")");
    //   }
    // }

    // return cppCode;
  }

  std::string
  Simulator::sequentialBlockCode(const std::vector<SigPort>& levelized) {
    string cppCode = "";
    
    for (auto sigPort : levelized) {
      CellId cid = sigPort.cell;
      const Cell& cell = def.getCellRefConst(cid);

      cppCode += ln("// ----- Sequential update for cell " + def.cellName(cid));

      CellType tp = cell.getCellType();
      if (tp == CELL_TYPE_REG_ARST) {
        cppCode += "\t{\n";
        string inVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, inVar);

        string clkVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_CLK);
        cppCode += codeToMaterialize(cid, PORT_ID_CLK, clkVar);
        string lastClkVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_CLK) + "_last";
        cppCode += codeToMaterializeOffset(cid, PORT_ID_CLK, lastClkVar, pastValueOffsets);

        string rstVar = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_ARST);
        cppCode += codeToMaterialize(cid, PORT_ID_ARST, rstVar);

        cppCode += "\t}\n";
        
      } else {
        cout << "Unsupported cell " << def.cellName(cid) << " in sequentialBlockCode" << endl;
        assert(false);
      }
    }

    return cppCode;
  }
  
  std::string
  Simulator::combinationalBlockCode(const std::vector<SigPort>& levelized) {

    string cppCode = "";
    
    for (auto sigPort : levelized) {
      CellId cid = sigPort.cell;
      const Cell& cell = def.getCellRefConst(cid);
      cppCode += ln("// ----- Code for cell " + def.cellName(cid));

      cppCode += "\t{\n";
      if ((cell.getCellType() == CELL_TYPE_PORT) && !cell.isInputPortCell()) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_IN}, portOffsets)) + "] = " + argName + ";\n"; //"bsim::quad_value_bit_vector(16, 0);\n"; //argName + ";\n";

      } else if (cell.isInputPortCell()) {
        cppCode += ln("// No code for input port " + def.cellName(cid));
      } else if (cell.getCellType() == CELL_TYPE_CONST) {
        cppCode += ln("// No code for const port " + def.cellName(cid));
      } else if (cell.getCellType() == CELL_TYPE_ZEXT) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        int outWidth = cell.getPortWidth(PORT_ID_OUT);

        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = zero_extend(" + to_string(outWidth) + ", " + argName + ");\n";

      } else if (cell.getCellType() == CELL_TYPE_SUB) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = sub_general_width_bv(" + argName0 + ", " + argName1 + ");\n";

      } else if (cell.getCellType() == CELL_TYPE_ADD) {
        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = add_general_width_bv(" + argName0 + ", " + argName1 + ");\n";

      } else if (cell.getCellType() == CELL_TYPE_MUL) {

        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = mul_general_width_bv(" + argName0 + ", " + argName1 + ");\n";

      } else if (cell.getCellType() == CELL_TYPE_ORR) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = orr(" + argName + ");\n";

      } else if (cell.getCellType() == CELL_TYPE_NOT) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = ~(" + argName + ");\n";

      } else if (cell.getCellType() == CELL_TYPE_REG_ARST) {
        
      } else {
        cout << "Insert code for unsupported node " + def.cellName(cid) << endl;
        assert(false);
      }

      cppCode += "\t}\n";
    }

    return cppCode;
  }

  void
  Simulator::compileLevelizedCircuit(const std::vector<std::vector<SigPort> >& updates) {
    string cppCode = "#include <vector>\n#include \"quad_value_bit_vector.h\"\nvoid simulate(std::vector<bsim::quad_value_bit_vector>& values) {\n";

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
  
  bool Simulator::compileCircuit() {
    // maybe<std::vector<CellId> > levelized =
    //   levelizeCircuit(def);

    vector<vector<SigPort> > updates = staticSimulationEvents(def);

    assert(updates.size() > 1);

    // vector<CellId> combUpdates;
    // for (auto sigPort : updates[0]) {
    //   combUpdates.push_back(sigPort.cell);
    // }
    compileLevelizedCircuit(updates); //staticSimulationEvents); //combUpdates);
    return updates.size() > 1;
    //return levelized.has_value();
  }

  Simulator::~Simulator() {
    if (libHandle != nullptr) {
      dlclose(libHandle);
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
  
}
