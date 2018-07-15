#pragma once

#include <string>

#include "circuit.h"
#include "cpp_utils.h"
#include "value_store.h"

using namespace std;

namespace FlatCircuit {

  static inline
  std::string
  xMask(const std::string& name) {
    return name + "_x_mask";
  }

  static inline
  std::string
  zMask(const std::string& name) {
    return name + "_z_mask";
  }
  
  static inline
  std::string
  setTableBitString(const std::string& offset, const std::string& bitString) {
    return ln("values[ " + offset + " ] = " + bitString);
  }

  static inline std::string
  storeTableString(const std::string& offset,
                   const std::string& value) {
    return ln("storeToTable(values, " + offset + ", " + value + ")");
  }

  static inline std::string
  storeRegisterStateString(const std::string& wireOffset,
                           const std::string& stateOffset,
                           const std::string& width) {
    return ln("storeRegisterState(values, " + wireOffset + ", " + stateOffset + ", " + width + ")");
  }

  static inline std::string
  loadTableString(const std::string& value, const std::string& offset, const std::string& width) {
    return ln("loadFromTable(" + value + ", values, " + offset + ", " + width + ")");
  }

  enum IRInstrType {
    IR_INSTR_DECLARE_TEMP,
    IR_INSTR_OTHER
  };
  
  class IRInstruction {
  public:

    virtual IRInstrType getType() const {
      return IR_INSTR_OTHER;
    }

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

  class IRTestJNE : public IRInstruction {
  public:
    std::string wenName;
    std::string label;

    IRTestJNE(const std::string& wenName_,
              const std::string& label_) : wenName(wenName_),
                                              label(label_) {}

    virtual std::string toString(ValueStore& valueStore) const {
      //return "\tif (!((" + wenName + " == BitVector(1)))) { goto " + label + "; }\n";
      return "\tif (!((" + wenName + ".get(0) == quad_value(1)))) { goto " + label + "; }\n";
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return "\tif (!((" + wenName + "))) { goto " + label + "; }\n";
    }

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
      // return "\tif (!((" + wenName + ".get(0) == quad_value(1)) && two_state_posedge(" +
      //   lastClkVar + ", " + clkVar + "))) { goto " + label + "; }\n";
      return "\tif (!((" + wenName + ") && two_state_posedge(" +
        lastClkVar + ", " + clkVar + "))) { goto " + label + "; }\n";

      
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return "\tif (!((" + wenName + ") && two_state_posedge(" +
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
        (edgeType == EDGE_TYPE_POSEDGE) ? "!two_state_posedge" : "!two_state_negedge";

      return ln("if (" + edgeName + "(" + prev + ", " + curr + ")) { goto " +
                label + "; }");
    }

    // TODO: Replace edge tests!
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
      int memWidth =
        valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_RDATA);
      unsigned long offset = valueStore.getRawMemoryOffset(cid);
      std::string offsetStr = "(" + std::to_string(offset) + " + " + waddrName + " * " + std::to_string(storedByteLength(memWidth)) + ")";
      std::string accessStr = accessString("values", offsetStr, memWidth);

      return ln(receiver + " = " + accessStr);
    }
    
    virtual std::string toString(ValueStore& valueStore) const {

      int memWidth =
        valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_RDATA);
      unsigned long offset = valueStore.getRawMemoryOffset(cid);
      std::string offsetStr = "(" + std::to_string(offset) + " + " + waddrName + " * " + std::to_string(storedByteLength(memWidth)) + ")";
      std::string accessStr = accessString("values", offsetStr, memWidth);
      std::string accessStrX = accessString("x_mask", offsetStr, memWidth);
      std::string accessStrZ = accessString("z_mask", offsetStr, memWidth);

      return ln(receiver + " = " + accessStr) +
        ln(xMask(receiver) + " = " + accessStrX) +
        ln(zMask(receiver) + " = " + accessStrZ);
      
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
      int memWidth =
        valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_RDATA);
      unsigned long offset = valueStore.getRawMemoryOffset(cid);
      std::string offsetStr = "(" + std::to_string(offset) + " + " +
        waddrName + " * " + std::to_string(storedByteLength(memWidth)) + ")";
      std::string accessStr = accessString("values", offsetStr, memWidth);

      return ln(accessStr + " = " + wdataName);
    }
    
    virtual std::string toString(ValueStore& valueStore) const {

      int memWidth =
        valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_RDATA);
      unsigned long offset = valueStore.getRawMemoryOffset(cid);
      std::string offsetStr = "(" + std::to_string(offset) + " + " +
        waddrName + " * " + std::to_string(storedByteLength(memWidth)) + ")";
      std::string accessStr = accessString("values", offsetStr, memWidth);
      std::string accessStrX = accessString("x_mask", offsetStr, memWidth);
      std::string accessStrZ = accessString("z_mask", offsetStr, memWidth);

      return ln(accessStr + " = " + wdataName) +
        ln(accessStrX + " = " + xMask(wdataName)) +
        ln(accessStrZ + " = " + zMask(wdataName));

      // return storeTableString(std::to_string(valueStore.getMemoryOffset(cid)) + " + " +
      //                         to_string(valueStore.def.getCellRefConst(cid).getMemWidth()) + "* (" + waddrName + ".is_binary() ? " + waddrName +
      //                         ".to_type<int>() : 0)",
      //                         wdataName);
      //return ln("storeToTable(values, " + ")");
      // return ln("values[" +
      //           std::to_string(valueStore.getMemoryOffset(cid)) + " + " +
      //           "(" + waddrName + ".is_binary() ? " + waddrName +
      //           ".to_type<int>() : 0)] = " + wdataName);

    }

  };

  class IRRegisterStateCopy : public IRInstruction {
  public:
    CellId cid;

    IRRegisterStateCopy(const CellId cid_) : cid(cid_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);

      unsigned long offset = valueStore.rawPortValueOffset(cid, PORT_ID_OUT);
      unsigned long regOffset = valueStore.getRawRegisterOffset(cid);

      std::string accessStr = accessString("values", offset, bitWidth);
      std::string registerStr = accessString("values", regOffset, bitWidth);

      return ln(accessStr + " = " + registerStr + "; // register state copy") +
        ln("std::cout << \"After storing value = \" << " + accessStr + " << std::endl");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {

      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);

      unsigned long offset = valueStore.rawPortValueOffset(cid, PORT_ID_OUT);
      unsigned long regOffset = valueStore.getRawRegisterOffset(cid);

      std::string accessStr = accessString("values", offset, bitWidth);
      std::string registerStr = accessString("values", regOffset, bitWidth);

      std::string accessStrX = accessString("x_mask", offset, bitWidth);
      std::string registerStrX = accessString("x_mask", regOffset, bitWidth);
      
      return ln(accessStr + " = " + registerStr + "; // register state copy") +
        ln(accessStrX + " = " + registerStrX + "; // register state copy");
        //ln("std::cout << \"After storing value = \" << " + accessStr + " << std::endl");

      // std::string state =
      //   "values[" + to_string(valueStore.getRegisterOffset(cid)) + "]";

      // std::string wire =
      //   "values[" +
      //   to_string(valueStore.portValueOffset(cid, PORT_ID_OUT)) +
      //   "]";

      // Changing this causes an error?
      // return storeRegisterStateString(to_string(valueStore.portValueOffset(cid, PORT_ID_OUT)),
      //                                 to_string(valueStore.getRegisterOffset(cid)),
      //                                 to_string(valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT)));

      // return storeTableString(to_string(valueStore.portValueOffset(cid, PORT_ID_OUT)),
      //                         state);
      //      return ln(wire + " = " + state);
    }

  };

  // Is this class even used anywhere?
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

      return ln(accessStr + " = " + result + "; // IRRegisterStore") +
        ln("std::cout << \"After storing value = " + result + " \" << " + result + " << \" to location the result is \" << " + accessStr + " << std::endl");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);
      unsigned long offset = valueStore.getRawRegisterOffset(cid);

      std::string accessStr = accessString("values", offset, bitWidth);
      std::string accessStrX = accessString("x_mask", offset, bitWidth);
      std::string accessStrZ = accessString("z_mask", offset, bitWidth);

      return ln(accessStr + " = " + result + "; // IRRegisterStore") +
        ln(accessStrX + " = " + xMask(result) + "; // IRRegisterStore") +
        ln(accessStrZ + " = " + zMask(result) + "; // IRRegisterStore");
    }

  };

  class IRRegisterReset : public IRInstruction {
  public:
    CellId cid;
    BitVector result;
    //std::string resString;

    IRRegisterReset(const CellId cid_,
                    const BitVector& result_) : cid(cid_), result(result_) {
      //resString = result.hex_string();
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {

      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);
      unsigned long offset = valueStore.getRawRegisterOffset(cid);

      std::string accessStr = accessString("values", offset, bitWidth);

      return ln(accessStr + " = " + std::to_string(result.to_type<uint64_t>()));
    }
    
    virtual std::string toString(ValueStore& valueStore) const {

      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);
      unsigned long offset = valueStore.getRawRegisterOffset(cid);

      std::string accessStr = accessString("values", offset, bitWidth);
      std::string accessStrX = accessString("x_mask", offset, bitWidth);

      return ln(accessStr + " = " + std::to_string(result.to_type<uint64_t>())) +
        ln(accessStrX + " = " + std::to_string(0));
        //ln(accessStrX + " = " + std::to_string(result.to_type<uint64_t>()));

      // int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);
      // std::string widthStr = std::to_string(bitWidth);
      // int regOffset = valueStore.getRegisterOffset(cid);

      // std::string code = "";
      // for (int i = 0; i < bitWidth; i++) {
      //   std::string bitString = "quad_value(" + result.get(i).binary_string() + ")";
      //   code += setTableBitString(std::to_string(regOffset + i), bitString);
      // }

      // return code;

      // return storeTableString(to_string(valueStore.getRegisterOffset(cid)),
      //                         "bsim::static_quad_value_bit_vector<" + widthStr + ">(\"" + resString + "\")");

    }

  };
  
  class IRTableStore : public IRInstruction {
  public:
    CellId cid;
    PortId pid;
    std::string value;
    bool isPast;

    IRTableStore(const CellId cid_, const PortId pid_, const std::string& value_) :
      cid(cid_), pid(pid_), value(value_), isPast(false) {}

    IRTableStore(const CellId cid_, const PortId pid_, const std::string& value_, const bool isPast_) :

      cid(cid_), pid(pid_), value(value_), isPast(isPast_) {}
    
    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(pid);

      unsigned long offset = valueStore.rawPortValueOffset(cid, pid);
      if (isPast) {
        offset = valueStore.rawPortPastValueOffset(cid, pid);
      }

      std::string accessStr = accessString("values", offset, bitWidth);

      return ln(accessStr + " = " + value + "; // table store");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      unsigned long offset = valueStore.portValueOffset(cid, pid);
      if (isPast) {
        offset = valueStore.pastValueOffset(cid, pid);
      }

      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(pid);
      std::string accessStr = accessString("values", offset, bitWidth);
      std::string accessStrX = accessString("x_mask", offset, bitWidth);
      std::string accessStrZ = accessString("z_mask", offset, bitWidth);

      return ln(accessStr + " = " + value + "; // table store") +
        ln(accessStrX + " = " + xMask(value) + "; // table store") +
        ln(accessStrZ + " = " + zMask(value) + "; // table store");
    }
    
  };

  class IRDeclareTemp : public IRInstruction {
  public:
    unsigned long bitWidth;
    std::string name;

    virtual IRInstrType getType() const {
      return IR_INSTR_DECLARE_TEMP;
    }

    IRDeclareTemp(const unsigned long bitWidth_, const std::string& name_) :
      bitWidth(bitWidth_), name(name_) {
      //assert(bitWidth < 64);
    }

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      return ln(containerPrimitive(bitWidth) + " " + name + " = 0");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      return ln(containerPrimitive(bitWidth) + " " + name + " = 0") +
        ln(containerPrimitive(bitWidth) + " " + xMask(name) + " = 0") +
        ln(containerPrimitive(bitWidth) + " " + zMask(name) + " = 0");
    }
    
  };

  class IRBinop : public IRInstruction {
  public:
    std::string receiver;
    std::string arg0;
    std::string arg1;
    const Cell& cell;
    std::vector<std::string> temps;
    
    IRBinop(const std::string& receiver_,
            const std::string& arg0_,
            const std::string& arg1_,
            const Cell& cell_) :
      receiver(receiver_), arg0(arg0_), arg1(arg1_), cell(cell_) {}

    IRBinop(const std::string& receiver_,
            const std::string& arg0_,
            const std::string& arg1_,
            const Cell& cell_,
            const std::vector<std::string>& temps_) :
      receiver(receiver_), arg0(arg0_), arg1(arg1_), cell(cell_), temps(temps_) {}
    
    virtual std::string twoStateCppCode(ValueStore& valueStore) const;
    
    virtual std::string toString(ValueStore& valueStore) const;

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

      //      std::cout << "Getting offset for " << sigPortString(valueStore.def, {cid, pid}) << std::endl;
      unsigned long offset;
      if (!isPast) {
        offset = valueStore.rawPortValueOffset(cid, pid);
      } else {
        offset = valueStore.rawPortPastValueOffset(cid, pid);
      }

      //std::cout << "Got offset" << std::endl;

      std::string accessStr = accessString("values", offset, bitWidth);

      return ln(receiver + " = " + accessStr + "; // IRPortLoad");
    }
    
    virtual std::string toString(ValueStore& valueStore) const {
      int bitWidth = valueStore.def.getCellRefConst(cid).getPortWidth(pid);
      unsigned long offset;
      if (!isPast) {
        offset = valueStore.portValueOffset(cid, pid);
      } else {
        offset = valueStore.pastValueOffset(cid, pid);
      }

      std::string accessStr = accessString("values", offset, bitWidth);
      std::string accessStrX = accessString("x_mask", offset, bitWidth);
      std::string accessStrZ = accessString("z_mask", offset, bitWidth);

      return ln(receiver + " = " + accessStr + "; // IRPortLoad") +
        ln(xMask(receiver) + " = " + accessStrX + "; // IRPortLoad") +
        ln(zMask(receiver) + " = " + accessStrZ + "; // IRPortLoad");
      
      //unsigned long offset = valueStore.portValueOffset(cid, pid);
      //return ln(receiver + " = values[" + std::to_string(offset) + "]");
      //return loadTableString(receiver, std::to_string(offset), std::to_string(valueStore.def.getCellRefConst(cid).getPortWidth(pid)));
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
    //unsigned long offset;
    CellId cid;
    PortId pid;
    unsigned long setOffset;
    SignalBit driverBit;
    bool isPastValue;
    //std::string source;

    IRSetBit(const std::string& receiver_,
             //const unsigned long offset_,
             const CellId cid_,
             const PortId pid_,
             const unsigned long setOffset_,
             const SignalBit driverBit_,
             const bool isPastValue_) :
      receiver(receiver_),
      cid(cid_),
      pid(pid_),
      setOffset(setOffset_),
      driverBit(driverBit_),
      isPastValue(isPastValue_) {}

    virtual std::string twoStateCppCode(ValueStore& valueStore) const {
      int bitWidth =
        valueStore.def.getCellRefConst(driverBit.cell).getPortWidth(driverBit.port);

      unsigned long offset;
      if (!isPastValue) {
        offset = valueStore.rawPortValueOffset(driverBit.cell, driverBit.port);
      } else {
        offset = valueStore.rawPortPastValueOffset(driverBit.cell, driverBit.port);
      }

      //std::cout << "Got offset" << std::endl;

      std::string valString = accessString("values", offset, bitWidth);

      std::string receiverBV =
        containerPrimitive(valueStore.def.getCellRefConst(cid).getPortWidth(pid));
      return ln(receiver + " |= ((" + receiverBV+ ")((" + valString + " >> " +
                std::to_string(driverBit.offset) + " ) & 0x1))<< " +
                std::to_string(setOffset));
    }
    
    virtual std::string toString(ValueStore& valueStore) const {

      int bitWidth =
        valueStore.def.getCellRefConst(driverBit.cell).getPortWidth(driverBit.port);

      unsigned long offset;
      if (!isPastValue) {
        offset = valueStore.rawPortValueOffset(driverBit.cell, driverBit.port);
      } else {
        offset = valueStore.rawPortPastValueOffset(driverBit.cell, driverBit.port);
      }

      //std::cout << "Got offset" << std::endl;

      std::string valString = accessString("values", offset, bitWidth);
      std::string valStringX = accessString("x_mask", offset, bitWidth);

      std::string receiverBV =
        containerPrimitive(valueStore.def.getCellRefConst(cid).getPortWidth(pid));
      return ln(receiver + " |= ((" + receiverBV+ ")((" + valString + " >> " +
                std::to_string(driverBit.offset) + " ) & 0x1))<< " +
                std::to_string(setOffset)) +
        ln(xMask(receiver) + " |= ((" + receiverBV+ ")((" + valStringX + " >> " +
           std::to_string(driverBit.offset) + " ) & 0x1))<< " +
           std::to_string(setOffset));
        ln(zMask(receiver) + " |= ((" + receiverBV+ ")((" + valStringX + " >> " +
           std::to_string(driverBit.offset) + " ) & 0x1))<< " +
           std::to_string(setOffset));
      
      // string valString = "";
      // unsigned long offset = 0;

      // if (isPastValue) {

      //   offset = valueStore.pastValueOffset(driverBit.cell, driverBit.port);

      // } else {

      //   offset = valueStore.portValueOffset(driverBit.cell, driverBit.port);

      // }

      // return ln("loadBitFromTable(values," + receiver +
      //           ", " + std::to_string(setOffset) +
      //           ", " + std::to_string(offset) +
      //           ", " + std::to_string(driverBit.offset) + ")");
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

      return ln(receiver + " = (" + sel + " ? " + arg1 + " : " + arg0 + ")") +
        ln(xMask(receiver) + " = (" + sel + " ? " + xMask(arg1) + " : " + xMask(arg0) + ")") +
        ln(zMask(receiver) + " = (" + sel + " ? " + zMask(arg1) + " : " + zMask(arg0) + ")");
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

    virtual std::string twoStateCppCode(ValueStore& valueStore) const;

    virtual std::string toString(ValueStore& valueStore) const;

  };

}
