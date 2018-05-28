#pragma once

#include <cassert>
#include <cmath>
#include <string>

namespace FlatCircuit {

  static inline std::string ln(const std::string& s) {
    return "\t" + s + ";\n";
  }

  static inline unsigned long storedByteLength(const unsigned long numBits) {
    if (numBits < 8) {
      return 1;
    }

    return ceil(numBits / 8);
  }

  static inline std::string containerPrimitive(const unsigned long bitWidth) {
    if (bitWidth <= 8) {
      return "uint8_t";
    }

    if (bitWidth <= 16) {
      return "uint16_t";
    }

    if (bitWidth <= 32) {
      return "uint32_t";
    }

    if (bitWidth <= 64) {
      return "uint64_t";
    }

    assert(false);
  }

  enum EdgeType {
    EDGE_TYPE_POSEDGE,
    EDGE_TYPE_NEGEDGE
  };

  static inline
  std::string
  accessString(const std::string& arrayName,
               const int byteOffset,
               const int bitWidth) {
    auto cBvType = containerPrimitive(bitWidth);
    return "*((" + cBvType + "*)(values + "  + std::to_string(byteOffset) + "))";
  }

  void compileCppLib(const std::string& cppName,
                     const std::string& targetBinary);

  std::string cppLibName(const std::string& baseName);
}
