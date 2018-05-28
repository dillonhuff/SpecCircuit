#include <chrono>
#include <iostream>

using namespace std;

#include "analysis.h"
#include "convert_coreir.h"
#include "output_verilog.h"
#include "transformations.h"
#include "utils.h"

using namespace std::chrono;

namespace FlatCircuit {

  void runCGRA(const int nCycles, Simulator& sim) {
    BitVector input = BitVector(16, 0);
    for (int i = 0; i < nCycles; i++) {
      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
    }

  }

  void runCGRATwoState(const int nCycles, Simulator& sim) {

    CellId pad_S2_T0 = sim.def.getPortCellId("pad_S2_T0_in");
    CellId pad_S2_T1 = sim.def.getPortCellId("pad_S2_T1_in");
    CellId pad_S2_T2 = sim.def.getPortCellId("pad_S2_T2_in");
    CellId pad_S2_T3 = sim.def.getPortCellId("pad_S2_T3_in");
    CellId pad_S2_T4 = sim.def.getPortCellId("pad_S2_T4_in");
    CellId pad_S2_T5 = sim.def.getPortCellId("pad_S2_T5_in");
    CellId pad_S2_T6 = sim.def.getPortCellId("pad_S2_T6_in");
    CellId pad_S2_T7 = sim.def.getPortCellId("pad_S2_T7_in");
    CellId pad_S2_T8 = sim.def.getPortCellId("pad_S2_T8_in");
    CellId pad_S2_T9 = sim.def.getPortCellId("pad_S2_T9_in");
    CellId pad_S2_T10 = sim.def.getPortCellId("pad_S2_T10_in");
    CellId pad_S2_T11 = sim.def.getPortCellId("pad_S2_T11_in");
    CellId pad_S2_T12 = sim.def.getPortCellId("pad_S2_T12_in");
    CellId pad_S2_T13 = sim.def.getPortCellId("pad_S2_T13_in");
    CellId pad_S2_T14 = sim.def.getPortCellId("pad_S2_T14_in");
    CellId pad_S2_T15 = sim.def.getPortCellId("pad_S2_T15_in");

    CellId clkIn = sim.def.getPortCellId("clk_in");

    int lastVal = 0;
    
    //BitVector input = BitVector(16, 0);

    for (int i = 0; i < nCycles; i++) {
      sim.setFreshValueTwoState(clkIn, PORT_ID_OUT, 1, 0);
      sim.update();

      // sim.setFreshValue("clk_in", BitVec(1, 0));
      // sim.update();

      unsigned long value = i;

      sim.setFreshValueTwoState(pad_S2_T0, PORT_ID_OUT, 1, (value >> (15 - 0)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T1, PORT_ID_OUT, 1, (value >> (15 - 1)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T2, PORT_ID_OUT, 1, (value >> (15 - 2)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T3, PORT_ID_OUT, 1, (value >> (15 - 3)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T4, PORT_ID_OUT, 1, (value >> (15 - 4)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T5, PORT_ID_OUT, 1, (value >> (15 - 5)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T6, PORT_ID_OUT, 1, (value >> (15 - 6)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T7, PORT_ID_OUT, 1, (value >> (15 - 7)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T8, PORT_ID_OUT, 1, (value >> (15 - 8)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T9, PORT_ID_OUT, 1, (value >> (15 - 9)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T10, PORT_ID_OUT, 1, (value >> (15 - 10)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T11, PORT_ID_OUT, 1, (value >> (15 - 11)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T12, PORT_ID_OUT, 1, (value >> (15 - 12)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T13, PORT_ID_OUT, 1, (value >> (15 - 13)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T14, PORT_ID_OUT, 1, (value >> (15 - 14)) & 0x1);
      sim.setFreshValueTwoState(pad_S2_T15, PORT_ID_OUT, 1, (value >> (15 - 15)) & 0x1);
      
      // input = BitVector(16, i);
      // setCGRAInput(2, input, sim);

      sim.setFreshValueTwoState(clkIn, PORT_ID_OUT, 1, 1);
      sim.update();
      
      // sim.setFreshValue("clk_in", BitVec(1, 1));
      // sim.update();

      lastVal = i;
    }

    assert(mul_general_width_bv(BitVector(16, 2), BitVector(16, lastVal)) ==
           getCGRAOutput(0, sim));
    
  }

  void runTimesTwo(Env& circuitEnv) {
    cout << "================== Starting Times Two ==================" << endl;
    auto configValues = loadBitStream("./test/pw2_16x16_only_config_lines.bsa");

    CellDefinition& def = circuitEnv.getDef("top");

    BitVector input(16, 23);
    BitVector correctOutput(16, 2*23);

    Simulator sim(circuitEnv, def);
    loadCGRAConfig(configValues, sim);

    setCGRAInput(2, input, sim);
    sim.update();

    cout << "Inputs" << endl;    
    printCGRAInputs(sim);

    int nCycles = 4;
    cout << "Computing " << nCycles << " cycles of data" << endl;
    for (int i = 0; i < nCycles; i++) {
      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

    }

    cout << "Outputs" << endl;
    printCGRAOutputs(sim);
    
    BitVector outputS0 = getCGRAOutput(0, sim);
    cout << "outputS0 = " << outputS0 << endl;

    assert(outputS0 == correctOutput);

    sim.def.replacePortWithConstant("reset_in", BitVec(1, 0));
    sim.def.replacePortWithConstant("config_addr_in", BitVec(32, 0));
    sim.def.replacePortWithConstant("config_data_in", BitVec(32, 0));

    sim.def.replacePortWithConstant("tck", BitVec(1, 0));
    sim.def.replacePortWithConstant("tdi", BitVec(1, 0));
    sim.def.replacePortWithConstant("tms", BitVec(1, 0));
    sim.def.replacePortWithConstant("trst_n", BitVec(1, 0));

    for (int side = 0; side < 4; side++) {

      if (side != 2) {
        for (int track = 0; track < 16; track++) {
          string outName =
            "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
          cout << "Setting " << outName << " to a constant" << endl;
          sim.def.replacePortWithConstant(outName, BitVec(1, 0));
        }
      }
    }

    cout << "# of cells before constant folding = " << def.numCells() << endl;

    specializeCircuit(sim);

    assert(definitionIsConsistent(def));

    input = BitVector(16, 18);
    setCGRAInput(2, input, sim);
    sim.update();

    cout << "Inputs" << endl;
    printCGRAInputs(sim);

    cout << "Outputs" << endl;
    printCGRAOutputs(sim);

    outputS0 = getCGRAOutput(0, sim);
    cout << "outputS0 = " << outputS0 << endl;

    assert(outputS0 == mul_general_width_bv(input, BitVec(16, 2)));
    assert(sim.compileCircuit());
    assert(sim.hasSimulateFunction());

    input = BitVector(16, 23);

    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
        sim.setFreshValue(inName, BitVec(1, input.get(15 - track).binary_value()));
      }
    }
    
    sim.update();

    cout << "Inputs" << endl;
    printCGRAInputs(sim);

    cout << "Outputs after compiling" << endl;
    printCGRAOutputs(sim);

    outputS0 = getCGRAOutput(0, sim);    

    cout << "outputS0 = " << outputS0 << endl;;

    assert(outputS0 == mul_general_width_bv(input, BitVec(16, 2)));

    outputVerilog(sim.def, "mul_2_cgra.v");

    nCycles = 10000;
    cout << "Running cgra for " << nCycles << endl;

    auto start = high_resolution_clock::now();

    runCGRA(nCycles, sim);

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Specialized 4 state mode. Time taken for " << nCycles << ": "
         << duration.count() << " milliseconds" << endl;

    outputS0 = getCGRAOutput(0, sim);
    cout << "Input  = " << input << endl;
    cout << "Output = " << outputS0 << endl;
    cout << "Done" << endl;

    cout << "Starting to simulate raw" << endl;
    sim.simulateRaw();

    cout << "Running CGRA x2 raw for " << nCycles << endl;

    start = high_resolution_clock::now();

    runCGRATwoState(nCycles, sim);

    stop = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(stop - start);
    cout << "Specialized 2 state mode. Time taken for " << nCycles << ": "
         << duration.count() << " milliseconds" << endl;

    cout << "Input  = " << input << endl;
    cout << "Output = " << outputS0 << endl;
    cout << "Done" << endl;
    
    input = BitVector(16, 5923);
    setCGRAInput(2, input, sim);
    sim.update();
    
    assert(mul_general_width_bv(BitVector(16, 2), input) == getCGRAOutput(0, sim));

    cout << "================== Done Times Two ==================" << endl;
  }

  void flatCircuitBenchmarks() {
    cout << "================== Starting Flat ==================" << endl;

    Env circuitEnv =
      loadFromCoreIR("global.top",
                     "./test/top.json");

    runTimesTwo(circuitEnv);

    cout << "================== Done Flat     ==================" << endl;
  }
}

using namespace FlatCircuit;

void iverilogBenchmarks() {
  cout << "================== Starting iverilog ==================" << endl;
  // Test unspecialized

  // Test specialized

  cout << "================== Done iverilog     ==================" << endl;
}

int main() {
  cout << "================== Starting Benchmarks ==================" << endl;

  flatCircuitBenchmarks();
  iverilogBenchmarks();
  
  cout << "================== Done Benchmarks     ==================" << endl;
}
