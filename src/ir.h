#pragma once

#include <string>

#include "circuit.h"
#include "cpp_utils.h"

using namespace std;

namespace FlatCircuit {

  class IRType {
  public:
    virtual ~IRType() {}
  };

  class TwoStateBitType : public IRType {
  };

  class FourStateBitType : public IRType {
  };

  class ArrayType : public IRType {
  public:
    IRType* elemType;
    unsigned long long length;
  };

  class PointerType : public IRType {
  public:
    IRType* refType;
  };

  class IRInstruction {
  public:
    std::string text;

    IRInstruction() : text("") {}
    IRInstruction(const std::string& text_) : text(text_) {}

    virtual std::string toString() const {
      return text;
    }

    virtual std::string twoStateCppCode() const {
      return "//No code\n";
    }

    virtual ~IRInstruction() {}
  };

  class IRComment : public IRInstruction {
  public:
    std::string text;

    IRComment() : text("") {}
    IRComment(const std::string& text_) : text(text_) {}

    virtual std::string toString() const {
      return ln("// " + text);
    }

    virtual std::string twoStateCppCode() const {
      return ln("// " + text);
    }

    virtual ~IRComment() {}
  };
  
  class IRLabel : public IRInstruction {

  public:
    std::string name;

    IRLabel(const std::string& name_) : name(name_) {}

    virtual std::string toString() const {
      return ln(name + ":");
    }

    virtual std::string twoStateCppCode() const {
      return ln(name + ":");
    }

  };

  class IRTableStore : public IRInstruction {
  public:
    unsigned long offset;
    std::string value;

    IRTableStore(const unsigned long offset_, const std::string& value_) :
      offset(offset_), value(value_) {}

    virtual std::string twoStateCppCode() const {
      return "// Table store " + value + "\n";
    }
    
    virtual std::string toString() const {
      return ln("values[" + std::to_string(offset) + "] = " + value);
    }
    
  };

  class IRDeclareTemp : public IRInstruction {
  public:
    unsigned long bitWidth;
    std::string name;

    IRDeclareTemp(const unsigned long bitWidth_, const std::string& name_) :
      bitWidth(bitWidth_), name(name_) {}

    virtual std::string twoStateCppCode() const {
      return ln("//char " + name + "[" + std::to_string(storedByteLength(bitWidth)) + "]");
    }
    
    virtual std::string toString() const {
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

    virtual std::string twoStateCppCode() const {
      return "// Binop " + receiver + "\n";
    }
    
    virtual std::string toString() const {
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

  class IRAssign : public IRInstruction {
  public:
    std::string receiver;
    std::string source;

    IRAssign(const std::string& receiver_,
             const std::string& source_) : receiver(receiver_), source(source_) {}

    virtual std::string twoStateCppCode() const {
      return ln("//" + receiver + " = " + source);
    }
    
    virtual std::string toString() const {
      return ln(receiver + " = " + source);
    }
  };

  class IRSetBit : public IRInstruction {
  public:
    std::string receiver;
    unsigned long offset;
    std::string source;

    IRSetBit(const std::string& receiver_,
             const unsigned long offset_,
             const std::string& source_) :
      receiver(receiver_), offset(offset_), source(source_) {}

    virtual std::string twoStateCppCode() const {
      return "// set bit";
    }
    
    virtual std::string toString() const {
      return ln(receiver + ".set(" + std::to_string(offset) + ", " + source + ")");
        //ln(receiver + " = " + source);
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

    virtual std::string twoStateCppCode() const {
      return "// Unop " + receiver + "\n";
    }
    
    virtual std::string toString() const {
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

  // Need:
  // binop, unop, multiplex, jump from test, load slice?
  // offset?
  // dereference :: Ptr(Tp) -> Tp
  // store array :: Ptr(Array[Tp]) -> offset -> Tp -> ()
  // load array :: Ptr(Array[n]) -> offset -> k -> Array[k]
  // load array element :: Ptr(Array[n]) -> offset -> type stored in array
  // binop :: (Array[n] -> Array[i] -> Array[p]) -> Array[n] -> Array[i] -> Array[p]

  // Its annoying that quad value bit vectors are a single data structure. Ideally
  // the quad value data structure would itself be a giant array of quad value bits

  // Load array  :: Ptr(Array[Tp][n]) -> offset -> width -> Array[Tp][n]
  // Store array :: Ptr(Array[Tp][n]) -> Array[Tp][n] -> offset -> ()
  // Binop       :: Array[Tp][n] -> Array[Tp][m] -> Array[Tp][k]

  // How do I model the input array? It is a pointer to an array of bits?
  // The normal arrays of bits will be stored as immediates

  // Copy segment of input array to temp would be:
  // set(temp[0], (*array)[0]);
  // OR
  // set(temp[0], *(array[0]));

  // I guess the array indexing will be in stages:
  // 1. Get the bit vector in the array
  // 2. Get the bit within the bit vector that we want

  // Really the issue is that the "array" isnt really a C-style array
  // because the entries (bit vectors) have variable size.

  // One solution is to treat it as an array of sum[0, i, n](size[i]) bits
  // Then load the bits completely

  // Also: Need to support loading from a vector of bit values as well
  
}
