#include "catch.hpp"

#include "output_verilog.h"
#include "simulator.h"

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

}
