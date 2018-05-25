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
    virtual ~IRInstruction() {}
  };

  class IRLabel : public IRInstruction {

  public:
    std::string name;

    IRLabel(const std::string& name_) : name(name_) {}

    virtual std::string toString() const {
      return ln(name + ":");
    }
  };

  class IRBinop : public IRInstruction {
  public:
    std::string receiver;
    std::string arg;

    IRBinop(const std::string& receiver_, const std::string& arg_) :
      receiver(receiver_), arg(arg_) {}

    virtual std::string toString() const {
      return ln(receiver + " = " + arg);
    }

  };

  class IRUnop : public IRInstruction {
  public:
    std::string receiver;
    CellType unop;
    std::string arg;

    IRUnop(const std::string& receiver_,
           const CellType tp_,
           const std::string& arg_) :
      receiver(receiver_), unop(tp_), arg(arg_) {}

    virtual std::string toString() const {
      switch (unop) {

      case CELL_TYPE_PASSTHROUGH:
        return ln(receiver + " = " + arg);
      case CELL_TYPE_NOT:
        return ln(receiver + " = ~(" + arg + ")");

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
