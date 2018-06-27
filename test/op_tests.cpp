#include "catch.hpp"

#include "simulator.h"

namespace FlatCircuit {

  TEST_CASE("Comparing binary ops in simulation and interpretation") {
    Env e;
    CellType notChainType = e.addCellType("binop_test_cell");
    CellDefinition& def = e.getDef(notChainType);
    def.addPort("in0", 16, PORT_TYPE_IN);
    def.addPort("in1", 16, PORT_TYPE_IN);
    def.addPort("out", 16, PORT_TYPE_OUT);

    CellId binop =
      def.addCell("sub",
                  CELL_TYPE_SUB,
                  {{PARAM_WIDTH, BitVector(32, 16)}});

    CellId in0Cell = def.getPortCellId("in0");
    CellId in1Cell = def.getPortCellId("in1");
    CellId outCell = def.getPortCellId("out");
    def.connect(in0Cell, PORT_ID_OUT, binop, PORT_ID_IN0);
    def.connect(in1Cell, PORT_ID_OUT, binop, PORT_ID_IN1);
    def.connect(binop, PORT_ID_OUT, outCell, PORT_ID_IN);

    Simulator interpSim(e, def);

    Simulator compileSim(e, def);
    compileSim.compileCircuit();

    vector<BitVector> interpResults;
    for (int i = 0; i < 200; i++) {
      int j = (i % 7) + 23;
      BitVector in0 = BitVector(16, i);
      BitVector in1 = BitVector(16, j);

      cout << "in0 = " << in0 << endl;
      cout << "in1 = " << in1 << endl;

      interpSim.setFreshValue("in0", in0);
      interpSim.setFreshValue("in1", in1);
      interpSim.update();

      compileSim.setFreshValue("in0", in0);
      compileSim.setFreshValue("in1", in1);
      compileSim.update();

      REQUIRE(interpSim.getBitVec("out") == compileSim.getBitVec("out"));
      //interpResults.push_back(interpSim.getBitVec("out"));
    }

    // vector<BitVector> compileResults;
    // for (int i = 0; i < 200; i++) {
    //   int j = (i % 7) + 23;
    //   BitVector in0 = BitVector(16, i);
    //   BitVector in1 = BitVector(16, j);

    //   compileSim.setFreshValue("in0", in0);
    //   compileSim.setFreshValue("in1", in1);

    //   compileSim.update();

    //   compileResults.push_back(compileSim.getBitVec("out"));
    // }

    // REQUIRE(compileResults.size() == interpResults.size());

    // for (int i = 0; i < (int) compileResults.size(); i++) {
    //   REQUIRE(compileResults[i] == interpResults[i]);
    // }
  }
}
