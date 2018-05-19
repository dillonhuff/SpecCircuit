#include "output_verilog.h"

#include <fstream>

using namespace std;

namespace FlatCircuit {

  std::string verilogName(const CellType tp) {
    return toString(tp);
  }


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

  class VerilogInstance {
  public:
    std::string modName;
    std::string instName;

    std::map<string, string> parameters;
    std::map<string, string> portsToWires;

    VerilogInstance(const std::string& modName_,
                    const std::string& instName_) :
      modName(modName_), instName(instName_) {}

    std::string toString() const {
      string str = "\t" + modName;

      if (parameters.size() > 0) {
        str += " #(";

        int i = 0;
        for (auto pm : parameters) {
          str += "." + pm.first + "(" + pm.second + ")";
          if (i != (int) (parameters.size() - 1)) {
            str += ",\n";
          }
          i++;
        }

        str += ")";
      }

      str += + " " + instName;
      str += "(";

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

  class VerilogAssign {
  public:
    VerilogWire receiverWire;
    int receiverBit;
    VerilogWire driverWire;
    int driverBit;

    VerilogAssign(const VerilogWire receiverWire_,
                  const int receiverBit_,
                  const VerilogWire driverWire_,
                  const int driverBit_) : receiverWire(receiverWire_),
                                          receiverBit(receiverBit_),
                                          driverWire(driverWire_),
                                          driverBit(driverBit_) {}

  };
  
  class VerilogModule {
    unsigned long long nextInt;
  public:

    std::string name;

    std::map<std::string, VerilogInstance> instances;
    std::vector<VerilogConnection> connections;

    std::vector<VerilogWire> wires;
    std::vector<VerilogAssign> assigns;

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

    void addBitAssign(const VerilogWire receiverWire,
                      const int receiverBit,
                      const VerilogWire driverWire,
                      const int driverBit) {
      assigns.push_back(VerilogAssign(receiverWire, receiverBit,
                                      driverWire, driverBit));
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

      for (auto assign : assigns) {
        str += "\tassign " + assign.receiverWire.name +
          "[ " + to_string(assign.receiverBit) + " ] = " +
          assign.driverWire.name + "[ " + to_string(assign.driverBit) + " ];";
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

      VerilogInstance& m = vm.getInstance(def.getCellName(cid));
      for (auto pm : cell.getParameters()) {
        m.parameters.insert({parameterToString(pm.first),
              to_string(bvToInt(pm.second))});
      }
      
      instancePortWires[cellName] = portWires;
    }

    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      for (auto pid : cell.inputPorts()) {
        auto drivers = cell.getDrivers(pid);

        for (int i = 0; i < (int) drivers.size(); i++) {
          SignalBit driverBit = drivers.signals[i];
          SignalBit receiverBit = {cid, pid, i};

          VerilogWire& driverWire = instancePortWires.at(def.getCellName(driverBit.cell)).at(driverBit.port);
          VerilogWire& receiverWire = instancePortWires.at(def.getCellName(receiverBit.cell)).at(receiverBit.port);

          vm.addBitAssign(receiverWire, receiverBit.offset, driverWire, driverBit.offset);
        }
      }
    }

    return vm;
  }

  std::string verilogPrefix() {
    string str = "";

    str += "module CELL_TYPE_CONST #(parameter PARAM_WIDTH=1, parameter PARAM_INIT_VALUE=0) (output [PARAM_WIDTH - 1 : 0] PORT_ID_OUT); assign PORT_ID_OUT = PARAM_INIT_VALUE;\nendmodule\n\n";

    str += "module CELL_TYPE_MUL #(parameter PARAM_WIDTH=1) (input [PARAM_WIDTH - 1 : 0] PORT_ID_IN0, input [PARAM_WIDTH - 1 : 0] PORT_ID_IN1, output [PARAM_WIDTH - 1 : 0] PORT_ID_OUT); endmodule\n\n";

    str += "module CELL_TYPE_NOT #(parameter PARAM_WIDTH=1) (input [PARAM_WIDTH - 1 : 0] PORT_ID_IN, output [PARAM_WIDTH - 1 : 0] PORT_ID_OUT); endmodule\n\n";

    str += "module CELL_TYPE_ORR #(parameter PARAM_WIDTH=1) (input [PARAM_WIDTH - 1 : 0] PORT_ID_IN, output [PARAM_WIDTH - 1 : 0] PORT_ID_OUT); endmodule\n\n";

    str += "module CELL_TYPE_PORT #(parameter PARAM_PORT_TYPE=0, parameter PARAM_OUT_WIDTH=1) (input [PARAM_OUT_WIDTH - 1 : 0] PORT_ID_IN, output [PARAM_OUT_WIDTH - 1 : 0] PORT_ID_OUT); endmodule\n\n";

    str += "module CELL_TYPE_ZEXT #(parameter PARAM_IN_WIDTH=1, parameter PARAM_OUT_WIDTH=1) (input [PARAM_IN_WIDTH - 1 : 0] PORT_ID_IN, output [PARAM_OUT_WIDTH - 1 : 0] PORT_ID_OUT); endmodule\n\n";
    return str;

  }
  void outputVerilog(const CellDefinition& def,
                     const std::string& verilogFile) {

    VerilogModule mod = toVerilog(def);

    string verilogString = verilogPrefix();
    std::ofstream t(verilogFile);
    t << verilogString << endl;
    t << mod.toString() << endl;
    t.close();
    
  }
  
}
