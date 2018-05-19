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

    VerilogInstance(const std::string& modName_,
                    const std::string& instName_) :
      modName(modName_), instName(instName_) {}

    std::string toString() const {
      return "\t" + modName + " " + instName + "();\n";
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

    std::vector<VerilogInstance> instances;
    std::vector<VerilogConnection> connections;

    std::vector<VerilogWire> wires;

    VerilogModule() : nextInt(0) {}

    VerilogWire freshWire(const int width) {
      std::string name = "fresh_wire_" + to_string(nextInt);
      nextInt++;

      VerilogWire w(name, width);
      wires.push_back(w);

      return w;
    }

    void addOutput(const std::string& name, const int width) {
      wires.push_back(VerilogWire(name, width));
    }

    void addInput(const std::string& name, const int width) {
      wires.push_back(VerilogWire(name, width));
    }

    std::string toString() const {
      string str = "module " + name + "();\n";

      for (auto w : wires) {
        str += "\twire [" + to_string(w.width - 1) + " : 0] " + w.name + ";\n";
      }
      for (auto inst : instances) {
        str += inst.toString();
        str += "\n";
      }
      str += "endmodule\n";
      return str;
    }

    void addUnop(const CellId cid,
                 const CellDefinition& def) {
      CellType tp = def.getCellRefConst(cid).getCellType();
      instances.push_back(VerilogInstance(verilogName(tp), def.getCellName(cid)));
    }

    void addBinop(const CellId cid,
                  const CellDefinition& def) {
      CellType tp = def.getCellRefConst(cid).getCellType();
      instances.push_back(VerilogInstance(verilogName(tp), def.getCellName(cid)));
    }

  };

  void cellToVerilogInstance(const CellId cid,
                             VerilogModule& vm,
                             const CellDefinition& def) {
    const Cell& cell = def.getCellRefConst(cid);
    CellType tp = cell.getCellType();

    if (def.isPortCell(cid) && cell.isInputPortCell()) {
      vm.addInput(def.getCellName(cid), cell.getPortWidth(PORT_ID_OUT));
    } else if (def.isPortCell(cid) && !cell.isInputPortCell()) {
      vm.addOutput(def.getCellName(cid), cell.getPortWidth(PORT_ID_IN));
    } else if (isUnop(tp)) {
      vm.addUnop(cid, def);
    } else if (isBinop(tp)) {
      vm.addBinop(cid, def);
    } else if (tp == CELL_TYPE_CONST) {
      
    } else {
      cout << "Unsupported cell type for cell " << def.getCellName(cid) << endl;
      assert(false);
    }

    
  }

  VerilogModule toVerilog(const CellDefinition& def) {
    VerilogModule vm;
    vm.name = "flat_module";

    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      cellToVerilogInstance(cid, vm, def);
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
