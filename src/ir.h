#pragma once

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
    virtual ~IRInstruction() {}
  };

  // Need:
  // binop, unop, multiplex, jump from test, load slice?
  // offset?
  // dereference :: Ptr(Tp) -> Tp
  // store array :: Ptr(Array[Tp]) -> offset -> Tp -> ()
  // load array :: Ptr(Array[n]) -> offset -> k -> Array[k]
  // binop :: (Array[n] -> Array[i] -> Array[p]) -> Array[n] -> Array[i] -> Array[p]
  // 

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
