#include "analysis.h"
#include "convert_coreir.h"
#include "output_verilog.h"
#include "transformations.h"
#include "utils.h"

using namespace std;
using namespace CoreIR;
using namespace FlatCircuit;

int main(const int argc, const char** argv) {
  assert(argc == 3);
  string bitstreamFile = argv[1];
  string outputName = argv[2];

  cout << "Bitstream file = " << bitstreamFile << endl;
  auto convConfigValues = loadBitStream(bitstreamFile);
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

  int nCycles = 10;
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

  assert(definitionIsConsistent(def));

  input = BitVector(16, 18);
  setCGRAInput(2, input, sim);
  sim.update();

  cout << "Inputs after specializing" << endl;
  printCGRAInputs(sim);

  cout << "Outputs" << endl;
  printCGRAOutputs(sim);

  outputS0 = getCGRAOutput(0, sim);
  cout << "outputS0 = " << outputS0 << endl;

  assert(sim.compileCircuit());
  assert(sim.hasSimulateFunction());

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

  outputVerilog(sim.def, outputName);

}
