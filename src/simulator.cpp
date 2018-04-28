#include "simulator.h"

using namespace std;

namespace FlatCircuit {

  // Supports creating a unique collection of items in an
  // arbitrary order
  template<typename T>
  class UniqueVector {
    std::vector<T> vec;
    std::set<T> alreadyAdded;
  public:

    void push_back(const T& t) {
      if (!elem(t, alreadyAdded)) {
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

  maybe<std::vector<CellId> > levelizeCircuit(const CellDefinition& def) {
    std::vector<CellId> cells;
    return cells;
  }

  void compileCppLib(const std::string& cppName,
                     const std::string& targetBinary) {
    int ret =
      system(("clang++ -std=c++11 -fPIC -dynamiclib " + cppName + " -o " + targetBinary).c_str());

    assert(ret == 0);
  }

  // JITInfo buildSimLib(CoreIR::Module* m,
  //                     CoreIR::NGraph& gr,
  //                     const std::string& libName) {
  //   MemLayout layout = buildLayout(gr);

  //   string cppCode = libCode(m, gr, layout);

  //   string targetBinary = "./lib" + libName + ".dylib";
  //   string cppName = "./" + libName + ".cpp";
  //   ofstream out(cppName);
  //   out << cppCode << endl;
  //   compileCppLib(cppName, targetBinary);

  //   DylibInfo dlib = loadLibWithFunc(targetBinary);

  //   return {layout, dlib};
  // }
  
  bool Simulator::compileCircuit() {
    maybe<std::vector<CellId> > levelized =
      levelizeCircuit(def);

    
    return levelized.has_value();
  }

}
