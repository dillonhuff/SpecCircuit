[![Build Status](https://travis-ci.org/dillonhuff/FlatCircuit.svg?branch=master)](https://travis-ci.org/dillonhuff/FlatCircuit)

# Circuit Specialization

This is an experimental RTL simulator that can perform runtime circuit
specialization to accelerate simulations of reconfigurable architectures.

# Build Instructions

```bash
cmake .
make -j
```

# Dependencies

The simulator currently reads in coreir or its own simplified file format.
To read in verilog you will need to download the VerilogToCoreIR tool.