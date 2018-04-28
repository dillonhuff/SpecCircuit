#include "simulator.h"

#include <fstream>

#include <dlfcn.h>

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

  std::string Simulator::codeToMaterialize(const CellId cid,
                                           const PortId pid,
                                           const std::string& argName) {
    const Cell& cell = def.getCellRefConst(cid);
    auto drivers = cell.getDrivers(pid);
    string cppCode = "";
    cppCode += ln("bsim::quad_value_bit_vector " + argName + "(" +
                  to_string(drivers.signals.size()) + ", 0)");

    for (int offset = 0; offset < drivers.signals.size(); offset++) {
      SignalBit driverBit = drivers.signals[offset];
      string valString = "values[" + to_string(map_find({driverBit.cell, driverBit.port}, portOffsets)) + "].get(" + to_string(driverBit.offset) + ")";
      cppCode += ln(argName + ".set(" + to_string(offset) + ", " + valString + ")");
    }

    return cppCode;
  }

  void Simulator::compileLevelizedCircuit(const std::vector<CellId>& levelized) {
    string cppCode = "#include <vector>\n#include \"quad_value_bit_vector.h\"\nvoid simulate(std::vector<bsim::quad_value_bit_vector>& values) {\n";

    for (auto cid : levelized) {
      const Cell& cell = def.getCellRefConst(cid);
      cppCode += ln("// ----- Code for cell " + def.cellName(cid));
      if ((cell.getCellType() == CELL_TYPE_PORT) && !cell.isInputPortCell()) {

        //auto drivers = cell.getDrivers(PORT_ID_IN);

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_IN}, portOffsets)) + "] = " + argName + ";\n";

      } else if (cell.isInputPortCell()) {
        cppCode += ln("// No code for input port " + def.cellName(cid));
      } else if (cell.getCellType() == CELL_TYPE_CONST) {
        cppCode += ln("// No code for const port " + def.cellName(cid));
      } else if (cell.getCellType() == CELL_TYPE_ZEXT) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        int outWidth = cell.getPortWidth(PORT_ID_OUT);

        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = zero_extend(" + to_string(outWidth) + ", " + argName + ");\n";

      } else if (cell.getCellType() == CELL_TYPE_MUL) {

        string argName0 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN0);
        cppCode += codeToMaterialize(cid, PORT_ID_IN0, argName0);

        string argName1 = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN1);
        cppCode += codeToMaterialize(cid, PORT_ID_IN1, argName1);

        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = bsim::quad_value_bit_vector(16, 0);\n"; //mul_general_width_bv(" + argName0 + ", " + argName1 + ");\n";

      } else if (cell.getCellType() == CELL_TYPE_ORR) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = orr(" + argName + ");\n";

      } else if (cell.getCellType() == CELL_TYPE_NOT) {

        string argName = "cell_" + to_string(cid) + "_" + portIdString(PORT_ID_IN);
        cppCode += codeToMaterialize(cid, PORT_ID_IN, argName);
        cppCode += "\tvalues[" + to_string(map_find({cid, PORT_ID_OUT}, portOffsets)) + "] = ~(" + argName + ");\n";

      } else {
        cout << "Insert code for unsupported node " + def.cellName(cid) << endl;
        assert(false);
      }
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
    maybe<std::vector<CellId> > levelized =
      levelizeCircuit(def);

    compileLevelizedCircuit(levelized.get_value());
    
    return levelized.has_value();
  }

}
