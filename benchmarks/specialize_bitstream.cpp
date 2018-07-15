#include "analysis.h"
#include "convert_coreir.h"
#include "output_verilog.h"
#include "serialize.h"
#include "transformations.h"
#include "utils.h"

#include <chrono>

using namespace std::chrono;
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
  // Env circuitEnv;
  // loadFromFile(circuitEnv, "top.csv");

  CellDefinition& def = circuitEnv.getDef("top");

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

  CellId clkCell = def.getPortCellId("clk_in");

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

  nCycles = 1000000;
  cout << "Computing " << nCycles << " cycles of data in compiled mode" << endl;

  setCGRAInput(2, input, sim);

  auto start = high_resolution_clock::now();
  auto stop = high_resolution_clock::now();

  auto duration = duration_cast<milliseconds>(stop - start);

  FILE* out = fopen("specialize_bs_log.txt", "w");
  
  start = high_resolution_clock::now();

  for (int i = 0; i < nCycles; i++) {

    sim.setFreshValue(clkCell, PORT_ID_OUT, BitVec(1, 0));
    sim.update();

    input = BitVector(16, i);

    sim.setFreshValue(s2t0, PORT_ID_OUT, BitVec(1, input.get(15 ).binary_value()));
    sim.setFreshValue(s2t1, PORT_ID_OUT, BitVec(1, input.get(14 ).binary_value()));
    sim.setFreshValue(s2t2, PORT_ID_OUT, BitVec(1, input.get(13 ).binary_value()));
    sim.setFreshValue(s2t3, PORT_ID_OUT, BitVec(1, input.get(12 ).binary_value()));
    sim.setFreshValue(s2t4, PORT_ID_OUT, BitVec(1, input.get(11 ).binary_value()));
    sim.setFreshValue(s2t5, PORT_ID_OUT, BitVec(1, input.get(10 ).binary_value()));
    sim.setFreshValue(s2t6, PORT_ID_OUT, BitVec(1, input.get(9 ).binary_value()));
    sim.setFreshValue(s2t7, PORT_ID_OUT, BitVec(1, input.get(8 ).binary_value()));
    sim.setFreshValue(s2t8, PORT_ID_OUT, BitVec(1, input.get(7 ).binary_value()));
    sim.setFreshValue(s2t9, PORT_ID_OUT, BitVec(1, input.get(6 ).binary_value()));
    sim.setFreshValue(s2t10, PORT_ID_OUT, BitVec(1, input.get(5 ).binary_value()));
    sim.setFreshValue(s2t11, PORT_ID_OUT, BitVec(1, input.get(4 ).binary_value()));
    sim.setFreshValue(s2t12, PORT_ID_OUT, BitVec(1, input.get(3 ).binary_value()));
    sim.setFreshValue(s2t13, PORT_ID_OUT, BitVec(1, input.get(2 ).binary_value()));
    sim.setFreshValue(s2t14, PORT_ID_OUT, BitVec(1, input.get(1 ).binary_value()));
    sim.setFreshValue(s2t15, PORT_ID_OUT, BitVec(1, input.get(0 ).binary_value()));
    
    sim.setFreshValue(clkCell, PORT_ID_OUT, BitVec(1, 1));
    sim.update();

    auto output = getCGRAOutput(0, sim);

    fprintf(out, "%s\n", output.binary_string().c_str());
  }

  stop = high_resolution_clock::now();
  duration = duration_cast<milliseconds>(stop - start);
  cout << "Time taken for " << nCycles << ": "
       << duration.count() << " milliseconds" << endl;
  
  outputS0 = getCGRAOutput(0, sim);
  
  cout << "Outputs" << endl;
  printCGRAOutputs(sim);

  cout << "input    = " << input << endl;
  cout << "outputS0 = " << outputS0 << endl;

  outputVerilog(sim.def, outputName);

  fclose(out);
}
