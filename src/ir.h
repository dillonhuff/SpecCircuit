#pragma once

#include <string>

#include "circuit.h"
#include "cpp_utils.h"
#include "value_store.h"

using namespace std;

namespace FlatCircuit {

  class IRInstruction {
  public:

    virtual std::string toString(ValueStore& valueStore) const {
      assert(false);
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      assert(false);
    }

    virtual ~IRInstruction() {}
  };

  class IRComment : public IRInstruction {
  public:
    std::string text;

    IRComment() : text("") {}
    IRComment(const std::string& text_) : text(text_) {}

    virtual std::string toString(ValueStore& valueStore) const {
      return ln("// " + text);
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return ln("// " + text);
    }

    virtual ~IRComment() {}
  };

  class IRMemoryTest : public IRInstruction {
  public:
    std::string wenName;
    std::string lastClkVar;
    std::string clkVar;
    std::string label;

    IRMemoryTest(const std::string& wenName_,
                 const std::string& lastClkVar_,
                 const std::string& clkVar_,
                 const std::string& label_) : wenName(wenName_),
                                              lastClkVar(lastClkVar_),
                                              clkVar(clkVar_),
                                              label(label_) {}

    virtual std::string toString(ValueStore& valueStore) const {
      return "\tif (!((" + wenName + " == BitVector(1, 1)) && posedge(" +
        lastClkVar + ", " + clkVar + "))) { goto " + label + "; }\n";

    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return "\t// if (!((" + wenName + " == BitVector(1, 1)) && posedge(" +
        lastClkVar + ", " + clkVar + "))) { goto " + label + "; }\n";

    }
    
  };

  class IREdgeTest : public IRInstruction {
  public:
    EdgeType edgeType;
    std::string prev;
    std::string curr;
    std::string label;
    
    IREdgeTest(const EdgeType edgeType_,
               const std::string& prev_,
               const std::string& curr_,
               const std::string& label_) : edgeType(edgeType_),
                                            prev(prev_),
                                            curr(curr_),
                                            label(label_) {}

    virtual std::string toString(ValueStore& valueStore) const {
      std::string edgeName =
        (edgeType == EDGE_TYPE_POSEDGE) ? "!posedge" : "!negedge";

      return ln("if (" + edgeName + "(" + prev + ", " + curr + ")) { goto " +
                label + "; }");
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      std::string edgeName =
        (edgeType == EDGE_TYPE_POSEDGE) ? "!two_state_posedge" : "!two_state_negedge";

      return ln("if (" + edgeName + "(" + prev + ", " + curr + ")) { goto " +
                label + "; }");
    }

    virtual ~IREdgeTest() {}
  };
  
  class IRLabel : public IRInstruction {

  public:
    std::string name;

    IRLabel(const std::string& name_) : name(name_) {}

    virtual std::string toString(ValueStore& valueStore) const {
      return ln(name + ":");
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return ln(name + ":");
    }

  };

  class IRMemoryLoad : public IRInstruction {
  public:

    std::string receiver;
    CellId cid;
    std::string waddrName;

    IRMemoryLoad(const std::string& receiver_,
                 const CellId cid_,
                 const std::string& waddrName_) :
      receiver(receiver_), cid(cid_), waddrName(waddrName_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      // return ln(receiv"values[" +
      //           std::to_string(valueStore.getMemoryOffset(cid)) + " + " +
      //           waddrName + "] = " + wdataName);
      return ln("// memory load");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      return ln(receiver + " = values[" +
                std::to_string(valueStore.getMemoryOffset(cid)) + " + " +
                "(" + waddrName + ".is_binary() ? " + waddrName +
                ".to_type<int>() : 0)]");
    }
  };
  
  class IRMemoryStore : public IRInstruction {
  public:
    CellId cid;
    std::string waddrName;
    std::string wdataName;

    IRMemoryStore(const CellId cid_,
                  const std::string& waddrName_,
                  const std::string& wdataName_) :
      cid(cid_), waddrName(waddrName_), wdataName(wdataName_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return ln("values[" +
                std::to_string(valueStore.getMemoryOffset(cid)) + " + " +
                waddrName + "] = " + wdataName);
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      return ln("values[" +
                std::to_string(valueStore.getMemoryOffset(cid)) + " + " +
                "(" + waddrName + ".is_binary() ? " + waddrName +
                ".to_type<int>() : 0)] = " + wdataName);

    }

  };

  class IRRegisterStore : public IRInstruction {
  public:
    CellId cid;
    std::string result;

    IRRegisterStore(const CellId cid_,
                    const std::string& result_) : cid(cid_), result(result_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {

      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);
      unsigned long offset = valueStore.getRawRegisterOffset(cid);

      std::string accessStr = accessString("values", offset, bitWidth);

      return ln(accessStr + " = " + result);
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      return ln("values[" + to_string(valueStore.getRegisterOffset(cid)) + "] = " +
                result);
    }

  };

  class IRRegisterReset : public IRInstruction {
  public:
    CellId cid;
    BitVector result;
    std::string resString;

    IRRegisterReset(const CellId cid_,
                    const BitVector& result_) : cid(cid_), result(result_) {
      resString = result.hex_string();
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {

      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);
      unsigned long offset = valueStore.getRawRegisterOffset(cid);

      std::string accessStr = accessString("values", offset, bitWidth);

      return ln(accessStr + " = " + std::to_string(result.to_type<uint64_t>()));
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      return ln("values[" + to_string(valueStore.getRegisterOffset(cid)) +
                "] = BitVector(\"" + resString + "\")");
    }

  };
  
  class IRTableStore : public IRInstruction {
  public:
    CellId cid;
    PortId pid;
    std::string value;

    IRTableStore(const CellId cid_, const PortId pid_, const std::string& value_) :
      cid(cid_), pid(pid_), value(value_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(pid);

      unsigned long offset = valueStore.rawPortValueOffset(cid, pid);

      std::string accessStr = accessString("values", offset, bitWidth);

      return ln(accessStr + " = " + value + "; // table store");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      unsigned long offset = valueStore.portValueOffset(cid, pid);
      return ln("values[" + std::to_string(offset) + "] = " + value);
    }
    
  };

  class IRDeclareTemp : public IRInstruction {
  public:
    unsigned long bitWidth;
    std::string name;

    IRDeclareTemp(const unsigned long bitWidth_, const std::string& name_) :
      bitWidth(bitWidth_), name(name_) {
      assert(bitWidth < 64);
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return ln(containerPrimitive(bitWidth) + " " + name + " = 0");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      return ln("BitVector " + name + "(" + std::to_string(bitWidth) + ", 0)");
    }
    
  };

  class IRBinop : public IRInstruction {
  public:
    std::string receiver;
    std::string arg0;
    std::string arg1;
    const Cell& cell;
    
    IRBinop(const std::string& receiver_,
            const std::string& arg0_,
            const std::string& arg1_,
            const Cell& cell_) :
      receiver(receiver_), arg0(arg0_), arg1(arg1_), cell(cell_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return "// Binop " + receiver + "\n";
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      CellType tp = cell.getCellType();
      switch (tp) {
      case CELL_TYPE_AND:
        return ln(receiver + " = (" + arg0 + " & " + arg1 + ")");

      case CELL_TYPE_UGE:
        return ln(receiver + " = BitVector(1, (" +
                  arg0 + " > " + arg1 + ") || (" +
                  arg0 + " == " + arg1 + "))");

      case CELL_TYPE_ULE:
        return ln(receiver + " = BitVector(1, (" +
                  arg0 + " < " + arg1 + ") || (" +
                  arg0 + " == " + arg1 + "))");

      case CELL_TYPE_UGT:
        return ln(receiver + " = BitVector(1, (" + arg0 + " > " + arg1 + "))");

      case CELL_TYPE_ULT:
        return ln(receiver + " = BitVector(1, (" + arg0 + " < " + arg1 + "))");
        
      case CELL_TYPE_OR:
        return ln(receiver + " = (" + arg0 + " | " + arg1 + ")");

      case CELL_TYPE_XOR:
        return ln(receiver + " = (" + arg0 + " ^ " + arg1 + ")");

      case CELL_TYPE_ADD:
        return ln(receiver + " = add_general_width_bv(" + arg0 + ", " + arg1 + ")");

      case CELL_TYPE_MUL:
        return ln(receiver + " = mul_general_width_bv(" + arg0 + ", " + arg1 + ")");

      case CELL_TYPE_SUB:
        return ln(receiver + " = sub_general_width_bv(" + arg0 + ", " + arg1 + ")");

      case CELL_TYPE_LSHR:
        return ln(receiver + " = lshr(" + arg0 + ", " + arg1 + ")");
        
      case CELL_TYPE_ASHR:
        return ln(receiver + " = ashr(" + arg0 + ", " + arg1 + ")");
        
      case CELL_TYPE_SHL:
        return ln(receiver + " = shl(" + arg0 + ", " + arg1 + ")");
        
      case CELL_TYPE_EQ:
        return ln(receiver + " = BitVector(1, " + arg0 + " == " + arg1 + ")");

      case CELL_TYPE_NEQ:
        return ln(receiver + " = BitVector(1, " + arg0 + " != " + arg1 + ")");
        
      default:
        std::cout << "Error: Unsupported binop " << FlatCircuit::toString(tp) << std::endl;
        assert(false);
      }

    }

  };

  class IRPortLoad : public IRInstruction {
  public:
    std::string receiver;
    const CellId cid;
    const PortId pid;
    bool isPast;

    IRPortLoad(const std::string& receiver_,
               const CellId cid_,
               const PortId pid_,
               const bool isPast_) :
      receiver(receiver_), cid(cid_), pid(pid_), isPast(isPast_) {}


    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(pid);

      std::cout << "Getting offset for " << sigPortString(valueStore.def, {cid, pid}) << std::endl;
      unsigned long offset;
      if (!isPast) {
        offset = valueStore.rawPortValueOffset(cid, pid);
      } else {
        offset = valueStore.rawPortPastValueOffset(cid, pid);
      }

      std::cout << "Got offset" << std::endl;

      std::string accessStr = accessString("values", offset, bitWidth);

      return ln(receiver + " = " + accessStr + "; // IRPortLoad");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      unsigned long offset;
      if (!isPast) {
        offset = valueStore.portValueOffset(cid, pid);
      } else {
        offset = valueStore.pastValueOffset(cid, pid);
      }

      //unsigned long offset = valueStore.portValueOffset(cid, pid);
      return ln(receiver + " = values[" + std::to_string(offset) + "]");
    }

  };

  class IRAssign : public IRInstruction {
  public:
    std::string receiver;
    std::string source;

    IRAssign(const std::string& receiver_,
             const std::string& source_) : receiver(receiver_), source(source_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return ln(receiver + " = " + source + "; // IRAssign");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      return ln(receiver + " = " + source);
    }
  };

  class IRSetBit : public IRInstruction {
  public:
    std::string receiver;
    unsigned long offset;
    SignalBit driverBit;
    bool isPastValue;
    //std::string source;

    IRSetBit(const std::string& receiver_,
             const unsigned long offset_,
             const SignalBit driverBit_,
             const bool isPastValue_) :
      receiver(receiver_),
      offset(offset_),
      driverBit(driverBit_),
      isPastValue(isPastValue_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      string valString = "values[0]";
      if (isPastValue) {
      } else {
      }

      return ln(receiver + " |= ((" + valString + " >> " +
                std::to_string(driverBit.offset) + " ) & 0x1) << " +
                std::to_string(offset));
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      //return ln(receiver + ".set(" + std::to_string(offset) + ", " + source + ")");

      if (isPastValue) {
        string valString = "values[" +
          std::to_string(valueStore.pastValueOffset(driverBit.cell, driverBit.port)) +
                         "].get(" + std::to_string(driverBit.offset) + ")";

        return ln(receiver + ".set(" + valString);
      } else {
        string valString = "values[" +
          std::to_string(valueStore.portValueOffset(driverBit.cell, driverBit.port)) +
                         "].get(" + std::to_string(driverBit.offset) + ")";

        return ln(receiver + ".set(" + std::to_string(offset) + ", " +
                  valString + ")");

      }

    }
  };

  class IRMux : public IRInstruction {
  public:
    std::string receiver;
    std::string sel;
    std::string arg0;
    std::string arg1;
    CellId cid;

    IRMux(const std::string& receiver_,
          const std::string& sel_,
          const std::string& arg0_,
          const std::string& arg1_,
          const CellId cid_) :
      receiver(receiver_), sel(sel_), arg0(arg0_), arg1(arg1_), cid(cid_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return ln(receiver + " = (" + sel + " ? " + arg1 + " : " + arg0 + ")");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      return ln(receiver + " = (" + sel + " == BitVector(1, 1) ? " + arg1 + " : " + arg0 + ")");
    }
    
  };

  class IRUnop : public IRInstruction {
  public:
    std::string receiver;
    std::string arg;
    const Cell& cell;

    IRUnop(const std::string& receiver_,
           const std::string& arg_,
           const Cell& cell_) :
      receiver(receiver_), arg(arg_), cell(cell_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return "// Unop " + receiver + "\n";
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      CellType unop = cell.getCellType();

      switch (unop) {

      case CELL_TYPE_PASSTHROUGH:
        return ln(receiver + " = " + arg);

      case CELL_TYPE_NOT:
        return ln(receiver + " = ~(" + arg + ")");

      case CELL_TYPE_ORR:
        return ln(receiver + " = orr(" + arg + ")");

      case CELL_TYPE_SLICE:
        return ln(receiver + " = slice(" + arg + ", " +
                  to_string(bvToInt(cell.getParameterValue(PARAM_LOW))) + ", " +
                  to_string(bvToInt(cell.getParameterValue(PARAM_HIGH))) + ")");
        
      case CELL_TYPE_ZEXT:
        return ln(receiver + " = zero_extend(" +
                  to_string(cell.getPortWidth(PORT_ID_OUT)) + 
                  ", " + arg + ")");
      default:
        std::cout << "IRUnop error: " << FlatCircuit::toString(unop) << std::endl;
        assert(false);
      }
    }

  };

}
