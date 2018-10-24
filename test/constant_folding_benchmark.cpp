#include "catch.hpp"

#include "analysis.h"
#include "convert_coreir.h"
#include "output_verilog.h"
#include "serialize.h"
#include "transformations.h"
#include "utils.h"

#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace CoreIR;

// NOTE: This is not portable since top.csv is too large to put on github
namespace FlatCircuit {

  TEST_CASE("Constant fold CGRA x2") {
    auto configValues = loadBitStream("./test/pw2_16x16_only_config_lines.bsa");

    Env circuitEnv;
    loadFromFile(circuitEnv, "top.csv");
      
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
