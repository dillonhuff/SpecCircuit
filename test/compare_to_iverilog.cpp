#include "catch.hpp"

#include "convert_coreir.h"
#include "output_verilog.h"
#include "simulator.h"
#include "utils.h"

#include <fstream>

namespace FlatCircuit {

  CellDefinition& buildBinopCellDef(Env& e, CellType opType) {
    CellType notChainType = e.addCellType(toString(opType) + "_test_cell");
    CellDefinition& def = e.getDef(notChainType);
    def.addPort("in0", 16, PORT_TYPE_IN);
    def.addPort("in1", 16, PORT_TYPE_IN);
    def.addPort("out", 16, PORT_TYPE_OUT);

    CellId binop =
      def.addCell(toString(opType) + "_test_inst",
                  opType,
                  {{PARAM_WIDTH, BitVector(32, 16)}});

    CellId in0Cell = def.getPortCellId("in0");
    CellId in1Cell = def.getPortCellId("in1");
    CellId outCell = def.getPortCellId("out");
    def.connect(in0Cell, PORT_ID_OUT, binop, PORT_ID_IN0);
    def.connect(in1Cell, PORT_ID_OUT, binop, PORT_ID_IN1);
    def.connect(binop, PORT_ID_OUT, outCell, PORT_ID_IN);

    return def;
  }
  
  TEST_CASE("Compare an and to iverilog") {
    Env e;
    CellDefinition& def = buildBinopCellDef(e, CELL_TYPE_AND);

    Simulator sim(e, def);
    sim.setFreshValue("in0", BitVector(16, "0010101000111000"));
    sim.setFreshValue("in1", BitVector(16, "1101101000010101"));
    sim.update();

    cout << "flat sim out = " << sim.getBitVec("out") << endl;

    outputVerilog(def, "adder_test.v");

    int iverilogCompile = system("iverilog -o adder_tb binop_tb.v adder_test.v");
    assert(iverilogCompile == 0);
    int iverilogRun = system("./adder_tb");
    assert(iverilogRun == 0);

    ifstream in("tb_output.txt");
    string line;
    getline(in, line);

    cout << "Line = " << line << endl;

    BitVector iverilogRes = BitVector(16, line);

    REQUIRE(sim.getBitVec("out") == iverilogRes);
  }

  TEST_CASE("Comparing the PE tile") {
    Env circuitEnv =
      loadFromCoreIR("global.pe_tile_new_unq1",
                     "./test/pe_tile_new_unq1.json");

    CellDefinition& def = circuitEnv.getDef("pe_tile_new_unq1");
    auto configValues = loadBitStream("./test/hwmaster_pw2_sixteen.bsa");

    Simulator sim(circuitEnv, def);

    sim.setFreshValue("tile_id", BitVector("16'h15"));

    sim.setFreshValue("in_BUS1_S1_T0", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T1", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T2", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T3", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T4", BitVector("1'h1"));

    cout << "Set tile_id" << endl;
    //loadCGRAConfig(configValues, sim);

    reset("reset", sim);

    cout << "Reset chip" << endl;
    for (int i = 0; i < (int) configValues.size(); i++) {

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      cout << "Evaluating " << i << endl;

      unsigned int configAddr = configValues[i].first;
      unsigned int configData = configValues[i].second;

      sim.setFreshValue("config_addr", BitVec(32, configAddr));
      sim.setFreshValue("config_data", BitVec(32, configData));

      posedge("clk_in", sim);
      
    }

    cout << "Done configuring PE tile" << endl;

    sim.setFreshValue("config_addr", BitVec(32, 0));

    posedge("clk_in", sim);

    int top_val = 5;

    for (int s = 0; s < 4; s++) {
      for (int t = 0; t < 5; t++) {
        sim.setFreshValue("in_BUS16_S" + to_string(s) + "_T" + to_string(t),
                          BitVec(16, top_val));
      }
    }

    cout << "Done setting inputs" << endl;

    posedge("clk_in", sim);
    posedge("clk_in", sim);
    
    REQUIRE(sim.getBitVec("out_BUS16_S0_T0", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T1", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T2", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) == BitVec(16, top_val*2));

    outputVerilog(def, "pe_tile_x2_test.v");

    int iverilogCompile = system("iverilog -o pe_x2_tb pe_tb_x2.v pe_tile_x2_test.v");
    assert(iverilogCompile == 0);
    int iverilogRun = system("./pe_x2_tb");
    assert(iverilogRun == 0);
    
  }

}
