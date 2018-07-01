#include "catch.hpp"

#include "analysis.h"
#include "convert_coreir.h"
#include "output_verilog.h"
#include "transformations.h"
#include "utils.h"

#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace CoreIR;

namespace FlatCircuit {
  TEST_CASE("CGRA convolution 3 x 3") {
    auto convConfigValues = loadBitStream("./test/conv_bw_only_config_lines.bsa");
    Env circuitEnv =
      loadFromCoreIR("global.top",
                     "./test/top.json");

    CellDefinition& def = circuitEnv.getDef("top");

    BitVector input(16, 0);
    BitVector correctOutput(16, 2*0);

    Simulator sim(circuitEnv, def);
    setCGRAInput(0, BitVector("16'h0"), sim);
    setCGRAInput(1, BitVector("16'h0"), sim);
    setCGRAInput(2, BitVector("16'h0"), sim);
    setCGRAInput(3, BitVector("16'h0"), sim);

    loadCGRAConfig(convConfigValues, sim);

    setCGRAInput(2, input, sim);
    sim.update();

    cout << "Inputs" << endl;
    printCGRAInputs(sim);

    int nCycles = 50;
    cout << "Computing " << nCycles << " cycles of data in interpreted mode" << endl;
    setCGRAInput(2, input, sim);

    for (int i = 0; i < nCycles; i++) {

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

      BitVector outputS0 = getCGRAOutput(0, sim);
      cout << "input    = " << input << ", " << input.to_type<int>() << endl;
      cout << "outputS0 = " << outputS0 << ", " << outputS0.to_type<int>() << endl;
    }

    BitVector interpOutputS0 = getCGRAOutput(0, sim);
    cout << "interpOutputS0 = " << interpOutputS0 << endl;
    
    cout << "Outputs" << endl;
    printCGRAOutputs(sim);
    
    BitVector outputS0 = getCGRAOutput(0, sim);
    cout << "outputS0 = " << outputS0 << endl;

    // SPECIALIZE
    sim.specializePort("reset_in", BitVec(1, 0));
    sim.specializePort("config_addr_in", BitVec(32, 0));
    sim.specializePort("config_data_in", BitVec(32, 0));

    sim.specializePort("tck", BitVec(1, 0));
    sim.specializePort("tdi", BitVec(1, 0));
    sim.specializePort("tms", BitVec(1, 0));
    sim.specializePort("trst_n", BitVec(1, 0));

    setCGRAInput(0, BitVector(16, 0), sim);
    setCGRAInput(1, BitVector(16, 0), sim);
    setCGRAInput(3, BitVector(16, 0), sim);

    specializeCircuit(sim);

    REQUIRE(definitionIsConsistent(def));

    input = BitVector(16, 18);
    setCGRAInput(2, input, sim);
    sim.update();

    cout << "Inputs after specializing" << endl;
    printCGRAInputs(sim);

    cout << "Outputs" << endl;
    printCGRAOutputs(sim);

    outputS0 = getCGRAOutput(0, sim);
    cout << "outputS0 = " << outputS0 << endl;

    //    REQUIRE(outputS0 == mul_general_width_bv(input, BitVec(16, 2)));
    REQUIRE(sim.compileCircuit());
    REQUIRE(sim.hasSimulateFunction());

    input = BitVector(16, 23);
    setCGRAInput(input, sim);
    sim.update();

    cout << "Inputs" << endl;
    printCGRAInputs(sim);

    cout << "Outputs after compiling" << endl;
    printCGRAOutputs(sim);

    outputS0 = getCGRAOutput(0, sim);    
    
    cout << "outputS0 = " << outputS0 << endl;;

    cout << "Clearing linebuffer" << endl;
    for (int i = 0; i < 100; i++) {
      cout << "Clearing cycle " << i << endl;
      setCGRAInput(2, BitVector(16, 0), sim);

      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

      BitVector outputS0 = getCGRAOutput(0, sim);
      cout << "input    = " << input << ", " << input.to_type<int>() << endl;
      cout << "outputS0 = " << outputS0 << ", " << outputS0.to_type<int>() << endl;
    }
    
    cout << "Computing " << nCycles << " cycles of data in compiled mode" << endl;
    setCGRAInput(2, input, sim);
    for (int i = 0; i < nCycles; i++) {

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

      BitVector outputS0 = getCGRAOutput(0, sim);
      cout << "input    = " << input << ", " << input.to_type<int>() << endl;
      cout << "outputS0 = " << outputS0 << ", " << outputS0.to_type<int>() << endl;
    }

    cout << "Outputs" << endl;
    printCGRAOutputs(sim);

    //REQUIRE(getCGRAOutput(0, sim) == interpOutputS0);

    outputVerilog(sim.def, "conv_bw_cgra.v");

    // PERFORMANCE TEST
    nCycles = 200000;
    cout << "Running cgra for " << nCycles << endl;

    auto start = high_resolution_clock::now();

    input = BitVector(16, 0);
    for (int i = 0; i < nCycles; i++) {
      if ((i % 100) == 0) {
        cout << "i = " << i << endl;
      }

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
    }

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Time taken for " << nCycles << " of conv_bw: "
         << duration.count() << " milliseconds" << endl;

    outputS0 = getCGRAOutput(0, sim);
    cout << "Input  = " << input << endl;
    cout << "Output = " << outputS0 << endl;
    cout << "Done" << endl;
    
  }
  
  TEST_CASE("CGRA convolution 3 x 1") {
    auto convConfigValues = loadBitStream("./test/conv_3_1_only_config_lines.bsa");
    Env circuitEnv =
      loadFromCoreIR("global.top",
                     "./test/top.json");

    CellDefinition& def = circuitEnv.getDef("top");

    BitVector input(16, 0);
    BitVector correctOutput(16, 2*0);

    Simulator sim(circuitEnv, def);
    setCGRAInput(0, BitVector("16'h0"), sim);
    setCGRAInput(1, BitVector("16'h0"), sim);
    setCGRAInput(2, BitVector("16'h0"), sim);
    setCGRAInput(3, BitVector("16'h0"), sim);

    loadCGRAConfig(convConfigValues, sim);

    setCGRAInput(2, input, sim);
    sim.update();

    cout << "Inputs" << endl;
    printCGRAInputs(sim);

    int nCycles = 50;
    cout << "Computing " << nCycles << " cycles of data in interpreted mode" << endl;
    setCGRAInput(2, input, sim);

    for (int i = 0; i < nCycles; i++) {

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

      BitVector outputS0 = getCGRAOutput(0, sim);
      cout << "input    = " << input << ", " << input.to_type<int>() << endl;
      cout << "outputS0 = " << outputS0 << ", " << outputS0.to_type<int>() << endl;
    }

    BitVector interpOutputS0 = getCGRAOutput(0, sim);
    cout << "interpOutputS0 = " << interpOutputS0 << endl;
    
    cout << "Outputs" << endl;
    printCGRAOutputs(sim);
    
    BitVector outputS0 = getCGRAOutput(0, sim);
    cout << "outputS0 = " << outputS0 << endl;

    // SPECIALIZE
    sim.specializePort("reset_in", BitVec(1, 0));
    sim.specializePort("config_addr_in", BitVec(32, 0));
    sim.specializePort("config_data_in", BitVec(32, 0));

    sim.specializePort("tck", BitVec(1, 0));
    sim.specializePort("tdi", BitVec(1, 0));
    sim.specializePort("tms", BitVec(1, 0));
    sim.specializePort("trst_n", BitVec(1, 0));

    setCGRAInput(0, BitVector(16, 0), sim);
    setCGRAInput(1, BitVector(16, 0), sim);
    setCGRAInput(3, BitVector(16, 0), sim);

    specializeCircuit(sim);

    REQUIRE(definitionIsConsistent(def));

    input = BitVector(16, 18);
    setCGRAInput(2, input, sim);
    sim.update();

    cout << "Inputs after specializing" << endl;
    printCGRAInputs(sim);

    cout << "Outputs" << endl;
    printCGRAOutputs(sim);

    outputS0 = getCGRAOutput(0, sim);
    cout << "outputS0 = " << outputS0 << endl;

    //    REQUIRE(outputS0 == mul_general_width_bv(input, BitVec(16, 2)));
    REQUIRE(sim.compileCircuit());
    REQUIRE(sim.hasSimulateFunction());

    input = BitVector(16, 23);
    setCGRAInput(input, sim);
    sim.update();

    cout << "Inputs" << endl;
    printCGRAInputs(sim);

    cout << "Outputs after compiling" << endl;
    printCGRAOutputs(sim);

    outputS0 = getCGRAOutput(0, sim);    
    
    cout << "outputS0 = " << outputS0 << endl;;

    cout << "Clearing linebuffer" << endl;
    for (int i = 0; i < 100; i++) {
      cout << "Clearing cycle " << i << endl;
      setCGRAInput(2, BitVector(16, 0), sim);

      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

      BitVector outputS0 = getCGRAOutput(0, sim);
      cout << "input    = " << input << ", " << input.to_type<int>() << endl;
      cout << "outputS0 = " << outputS0 << ", " << outputS0.to_type<int>() << endl;
    }
    
    cout << "Computing " << nCycles << " cycles of data in compiled mode" << endl;
    setCGRAInput(2, input, sim);
    for (int i = 0; i < nCycles; i++) {

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

      BitVector outputS0 = getCGRAOutput(0, sim);
      cout << "input    = " << input << ", " << input.to_type<int>() << endl;
      cout << "outputS0 = " << outputS0 << ", " << outputS0.to_type<int>() << endl;
    }

    cout << "Outputs" << endl;
    printCGRAOutputs(sim);

    //REQUIRE(getCGRAOutput(0, sim) == interpOutputS0);

    outputVerilog(sim.def, "conv_3_1_cgra.v");

    // PERFORMANCE TEST
    nCycles = 200000;
    cout << "Running cgra for " << nCycles << endl;

    auto start = high_resolution_clock::now();

    input = BitVector(16, 0);
    CellId cid = def.getPortCellId("clk_in");

    CellId s2t0 = def.getPortCellId("pad_S2_T0_in");
    CellId s2t1 = def.getPortCellId("pad_S2_T1_in");
    CellId s2t2 = def.getPortCellId("pad_S2_T2_in");
    CellId s2t3 = def.getPortCellId("pad_S2_T3_in");
    CellId s2t4 = def.getPortCellId("pad_S2_T4_in");
    CellId s2t5 = def.getPortCellId("pad_S2_T5_in");
    CellId s2t6 = def.getPortCellId("pad_S2_T6_in");
    CellId s2t7 = def.getPortCellId("pad_S2_T7_in");
    CellId s2t8 = def.getPortCellId("pad_S2_T8_in");
    CellId s2t9 = def.getPortCellId("pad_S2_T9_in");
    CellId s2t10 = def.getPortCellId("pad_S2_T10_in");
    CellId s2t11 = def.getPortCellId("pad_S2_T11_in");
    CellId s2t12 = def.getPortCellId("pad_S2_T12_in");
    CellId s2t13 = def.getPortCellId("pad_S2_T13_in");
    CellId s2t14 = def.getPortCellId("pad_S2_T14_in");
    CellId s2t15 = def.getPortCellId("pad_S2_T15_in");
    
    for (int i = 0; i < nCycles; i++) {
      if ((i % 100) == 0) {
        cout << "i = " << i << endl;
      }

      //sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.setFreshValue(cid, PORT_ID_OUT, BitVec(1, 0));
      //sim.setFreshValue(s0t0, BitVec(
      sim.update();

      input = BitVector(16, i);
      
      //setCGRAInput(2, input, sim);

      //sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.setFreshValue(cid, PORT_ID_OUT, BitVec(1, 1));
      sim.update();
    }

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Time taken for " << nCycles << " of conv_3_1: "
         << duration.count() << " milliseconds" << endl;

    outputS0 = getCGRAOutput(0, sim);
    cout << "Input  = " << input << endl;
    cout << "Output = " << outputS0 << endl;
    cout << "Done" << endl;
    
  }

  TEST_CASE("CGRA convolution") {
    auto convConfigValues = loadBitStream("./test/conv_2_1_only_config_lines.bsa");
    Env circuitEnv =
      loadFromCoreIR("global.top",
                     "./test/top.json");
                     //"/Users/dillon/CoreIRWorkspace/CGRA_coreir/top.json");

    CellDefinition& def = circuitEnv.getDef("top");

    BitVector input(16, 0);
    BitVector correctOutput(16, 2*0);

    Simulator sim(circuitEnv, def);
    setCGRAInput(0, BitVector("16'h0"), sim);
    setCGRAInput(1, BitVector("16'h0"), sim);
    setCGRAInput(2, BitVector("16'h0"), sim);
    setCGRAInput(3, BitVector("16'h0"), sim);

    loadCGRAConfig(convConfigValues, sim);

    setCGRAInput(2, input, sim);
    sim.update();

    cout << "Inputs" << endl;
    printCGRAInputs(sim);

    int nCycles = 20;
    cout << "Computing " << nCycles << " cycles of data in interpreted mode" << endl;
    setCGRAInput(2, input, sim);

    for (int i = 0; i < nCycles; i++) {

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

      BitVector outputS0 = getCGRAOutput(0, sim);
      cout << "input    = " << input << ", " << input.to_type<int>() << endl;
      cout << "outputS0 = " << outputS0 << ", " << outputS0.to_type<int>() << endl;
    }

    BitVector interpOutputS0 = getCGRAOutput(0, sim);
    cout << "interpOutputS0 = " << interpOutputS0 << endl;
    
    cout << "Outputs" << endl;
    printCGRAOutputs(sim);
    
    BitVector outputS0 = getCGRAOutput(0, sim);
    cout << "outputS0 = " << outputS0 << endl;

    // SPECIALIZE
    sim.specializePort("reset_in", BitVec(1, 0));
    sim.specializePort("config_addr_in", BitVec(32, 0));
    sim.specializePort("config_data_in", BitVec(32, 0));

    sim.specializePort("tck", BitVec(1, 0));
    sim.specializePort("tdi", BitVec(1, 0));
    sim.specializePort("tms", BitVec(1, 0));
    sim.specializePort("trst_n", BitVec(1, 0));

    setCGRAInput(0, BitVector(16, 0), sim);
    setCGRAInput(1, BitVector(16, 0), sim);
    setCGRAInput(3, BitVector(16, 0), sim);

    specializeCircuit(sim);

    REQUIRE(definitionIsConsistent(def));

    input = BitVector(16, 18);
    setCGRAInput(2, input, sim);
    sim.update();

    cout << "Inputs after specializing" << endl;
    printCGRAInputs(sim);

    cout << "Outputs" << endl;
    printCGRAOutputs(sim);

    outputS0 = getCGRAOutput(0, sim);
    cout << "outputS0 = " << outputS0 << endl;

    //    REQUIRE(outputS0 == mul_general_width_bv(input, BitVec(16, 2)));
    REQUIRE(sim.compileCircuit());
    REQUIRE(sim.hasSimulateFunction());

    input = BitVector(16, 23);
    setCGRAInput(input, sim);
    sim.update();

    cout << "Inputs" << endl;
    printCGRAInputs(sim);

    cout << "Outputs after compiling" << endl;
    printCGRAOutputs(sim);

    outputS0 = getCGRAOutput(0, sim);    
    
    cout << "outputS0 = " << outputS0 << endl;;

    cout << "Clearing linebuffer" << endl;
    for (int i = 0; i < 100; i++) {
      cout << "Clearing cycle " << i << endl;
      setCGRAInput(2, BitVector(16, 0), sim);

      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

      BitVector outputS0 = getCGRAOutput(0, sim);
      cout << "input    = " << input << ", " << input.to_type<int>() << endl;
      cout << "outputS0 = " << outputS0 << ", " << outputS0.to_type<int>() << endl;
    }
    
    cout << "Computing " << nCycles << " cycles of data in compiled mode" << endl;
    setCGRAInput(2, input, sim);
    for (int i = 0; i < nCycles; i++) {

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      cout << "Cycle " << i << endl;

      posedge("clk_in", sim);

      BitVector outputS0 = getCGRAOutput(0, sim);
      cout << "input    = " << input << ", " << input.to_type<int>() << endl;
      cout << "outputS0 = " << outputS0 << ", " << outputS0.to_type<int>() << endl;
    }

    cout << "Outputs" << endl;
    printCGRAOutputs(sim);

    outputVerilog(sim.def, "conv_2_1_cgra.v");    

    REQUIRE(getCGRAOutput(0, sim) == interpOutputS0);

    // PERFORMANCE TEST
    nCycles = 10000;
    cout << "Running cgra for " << nCycles << endl;

    auto start = high_resolution_clock::now();

    input = BitVector(16, 0);
    for (int i = 0; i < nCycles; i++) {
      if ((i % 100) == 0) {
        cout << "i = " << i << endl;
      }

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
    }

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Time taken for " << nCycles << " of conv_2_1: "
         << duration.count() << " milliseconds" << endl;

    outputS0 = getCGRAOutput(0, sim);
    cout << "Input  = " << input << endl;
    cout << "Output = " << outputS0 << endl;
    cout << "Done" << endl;
  }

  TEST_CASE("CGRA multiply by 2") {
    auto configValues = loadBitStream("./test/pw2_16x16_only_config_lines.bsa");
    Env circuitEnv =
      loadFromCoreIR("global.top",
                     "./test/top.json");

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

    REQUIRE(outputS0 == correctOutput);

    sim.specializePort("reset_in", BitVec(1, 0));
    sim.specializePort("config_addr_in", BitVec(32, 0));
    sim.specializePort("config_data_in", BitVec(32, 0));

    sim.specializePort("tck", BitVec(1, 0));
    sim.specializePort("tdi", BitVec(1, 0));
    sim.specializePort("tms", BitVec(1, 0));
    sim.specializePort("trst_n", BitVec(1, 0));

    for (int side = 0; side < 4; side++) {

      if (side != 2) {
        for (int track = 0; track < 16; track++) {
          string outName =
            "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
          cout << "Setting " << outName << " to a constant" << endl;
          sim.specializePort(outName, BitVec(1, 0));
        }
      }
    }

    cout << "# of cells before constant folding = " << def.numCells() << endl;

    specializeCircuit(sim);

    REQUIRE(definitionIsConsistent(def));

    input = BitVector(16, 18);
    setCGRAInput(2, input, sim);
    sim.update();

    cout << "Inputs" << endl;
    printCGRAInputs(sim);

    cout << "Outputs" << endl;
    printCGRAOutputs(sim);

    outputS0 = getCGRAOutput(0, sim);
    cout << "outputS0 = " << outputS0 << endl;

    REQUIRE(outputS0 == mul_general_width_bv(input, BitVec(16, 2)));
    REQUIRE(sim.compileCircuit());
    REQUIRE(sim.hasSimulateFunction());

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

    REQUIRE(outputS0 == mul_general_width_bv(input, BitVec(16, 2)));

    outputVerilog(sim.def, "mul_2_cgra.v");

    nCycles = 10000;
    cout << "Running cgra for " << nCycles << endl;

    auto start = high_resolution_clock::now();

    input = BitVector(16, 0);
    for (int i = 0; i < nCycles; i++) {
      if ((i % 100) == 0) {
        cout << "i = " << i << endl;
      }

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
    }

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Time taken for " << nCycles << ": "
         << duration.count() << " milliseconds" << endl;

    outputS0 = getCGRAOutput(0, sim);
    cout << "Input  = " << input << endl;
    cout << "Output = " << outputS0 << endl;
    cout << "Done" << endl;

    cout << "Starting to simulate raw" << endl;
    sim.simulateRaw();

    cout << "Running CGRA x2 raw for " << nCycles << endl;

    start = high_resolution_clock::now();

    input = BitVector(16, 0);
    for (int i = 0; i < nCycles; i++) {
      // if ((i % 100) == 0) {
      //   cout << "i = " << i << endl;
      // }

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      input = BitVector(16, i);
      setCGRAInput(2, input, sim);

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
    }

    stop = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(stop - start);
    cout << "Time taken for " << nCycles << ": "
         << duration.count() << " milliseconds" << endl;

    input = BitVector(16, 5923);
    setCGRAInput(2, input, sim);
    sim.update();
    
    REQUIRE(mul_general_width_bv(BitVector(16, 2), input) == getCGRAOutput(0, sim));
  }

}
