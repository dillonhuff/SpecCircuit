#include "output_verilog.h"

#include <fstream>

using namespace std;

namespace FlatCircuit {

  std::string verilogName(const CellType tp) {
    return toString(tp);
  }

  class VerilogInstance {
  public:
    std::string modName;
    std::string instName;

    std::map<string, string> portsToWires;

    VerilogInstance(const std::string& modName_,
                    const std::string& instName_) :
      modName(modName_), instName(instName_) {}

    std::string toString() const {
      string str = "\t" + modName + " " + instName + "(";

      int i = 0;
      for (auto pw : portsToWires) {
        str += "." + pw.first + "(" + pw.second + ")";
        if (i != (int) (portsToWires.size() - 1)) {
          str += ",\n";
        }
        i++;
      }

      str += ");\n";

      return str;
    }

    void connect(const std::string& port, const std::string& wire) {
      portsToWires.insert({port, wire});
    }
  };

  class VerilogConnection {
  };
  

  class VerilogWire {
  public:

    std::string name;
    int width;
    bool isInput;
    bool isOutput;
    bool isRegister;

    VerilogWire(const std::string& name_, const int width_) :
      name(name_), width(width_) {}

    
  };

  class VerilogModule {
    unsigned long long nextInt;
  public:

    std::string name;

    std::map<std::string, VerilogInstance> instances;
    std::vector<VerilogConnection> connections;

    std::vector<VerilogWire> wires;

    VerilogModule() : nextInt(0) {}

    VerilogInstance& getInstance(const std::string& str) {
      if (!contains_key(str, instances)) {
        cout << "Verilog module does not contain " << str << endl;
      }
      assert(contains_key(str, instances));
      
      return instances.at(str);
    }

    VerilogWire freshWire(const int width) {
      std::string name = "fresh_wire_" + to_string(nextInt);
      nextInt++;

      VerilogWire w(name, width);
      wires.push_back(w);

      return w;
    }

    void addOutput(const std::string& name, const int width, const CellId cid, const CellDefinition& def) {
      wires.push_back(VerilogWire(name, width));

      CellType tp = def.getCellRefConst(cid).getCellType();
      instances.insert({def.getCellName(cid),
            VerilogInstance(verilogName(tp), def.getCellName(cid))});
    }

    void addInput(const std::string& name, const int width, const CellId cid, const CellDefinition& def) {
      wires.push_back(VerilogWire(name, width));

      CellType tp = def.getCellRefConst(cid).getCellType();
      instances.insert({def.getCellName(cid),
            VerilogInstance(verilogName(tp), def.getCellName(cid))});
      
    }

    std::string toString() const {
      string str = "module " + name + "();\n";

      for (auto w : wires) {
        str += "\twire [" + to_string(w.width - 1) + " : 0] " + w.name + ";\n";
      }
      for (auto inst : instances) {
        str += inst.second.toString();
        str += "\n";
      }
      str += "endmodule\n";
      return str;
    }

    void addUnop(const CellId cid,
                 const CellDefinition& def) {
      CellType tp = def.getCellRefConst(cid).getCellType();
      instances.insert({def.getCellName(cid),
            VerilogInstance(verilogName(tp), def.getCellName(cid))});
    }

    void addBinop(const CellId cid,
                  const CellDefinition& def) {
      CellType tp = def.getCellRefConst(cid).getCellType();
      instances.insert({def.getCellName(cid),
            VerilogInstance(verilogName(tp), def.getCellName(cid))});
    }

  };

  void cellToVerilogInstance(const CellId cid,
                             VerilogModule& vm,
                             const CellDefinition& def,
                             map<PortId, VerilogWire>& portMap) {
    const Cell& cell = def.getCellRefConst(cid);
    CellType tp = cell.getCellType();

    if (def.isPortCell(cid) && cell.isInputPortCell()) {
      vm.addInput(def.getCellName(cid), cell.getPortWidth(PORT_ID_OUT), cid, def);
    } else if (def.isPortCell(cid) && !cell.isInputPortCell()) {
      vm.addOutput(def.getCellName(cid), cell.getPortWidth(PORT_ID_IN), cid, def);
    } else if (isUnop(tp)) {
      vm.addUnop(cid, def);
    } else if (isBinop(tp)) {
      vm.addBinop(cid, def);
    } else if (tp == CELL_TYPE_CONST) {
      CellType tp = def.getCellRefConst(cid).getCellType();
      vm.instances.insert({def.getCellName(cid),
            VerilogInstance(verilogName(tp), def.getCellName(cid))});
    } else {
      cout << "Unsupported cell type for cell " << def.getCellName(cid) << endl;
      assert(false);
    }

    VerilogInstance& inst = vm.getInstance(def.getCellName(cid));
    for (auto pm : portMap) {
      inst.connect(portIdString(pm.first), pm.second.name);
    }

  }

  VerilogModule toVerilog(const CellDefinition& def) {
    VerilogModule vm;
    vm.name = "flat_module";

    map<string, map<PortId, VerilogWire> > instancePortWires;

    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      string cellName = def.getCellName(cid);

      map<PortId, VerilogWire> portWires;

      const Cell& cell = def.getCellRefConst(cid);

      for (auto ptp : cell.getPorts()) {
        PortId pid = ptp.first;
        VerilogWire w = vm.freshWire(cell.getPortWidth(pid));
        portWires.insert({pid, w});
      }

      cellToVerilogInstance(cid, vm, def, portWires);
      
      instancePortWires[cellName] = portWires;
    }

    return vm;
  }

  void outputVerilog(const CellDefinition& def,
                     const std::string& verilogFile) {

    VerilogModule mod = toVerilog(def);

    string verilogString = "";
    std::ofstream t(verilogFile);
    t << mod.toString() << endl;
    t.close();
    
  }
  
}
