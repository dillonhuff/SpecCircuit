#pragma once

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

  
}
