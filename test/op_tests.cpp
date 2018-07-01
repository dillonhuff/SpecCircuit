#include "catch.hpp"

#include "simulator.h"

namespace FlatCircuit {

  CellDefinition& buildReduceBinopCellDef(Env& e, CellType opType) {
    CellType notChainType = e.addCellType(toString(opType) + "_test_cell");
    CellDefinition& def = e.getDef(notChainType);
    def.addPort("in0", 16, PORT_TYPE_IN);
    def.addPort("in1", 16, PORT_TYPE_IN);
    def.addPort("out", 1, PORT_TYPE_OUT);

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

  void compareSimulators(Simulator& interpSim, Simulator& compileSim) {
    int max = 1 << 15;
    int min = 0;

    for (int i = 0; i < 200; i++) {

      int in0V = min + (rand() % static_cast<int>(max - min + 1));
      int in1V = min + (rand() % static_cast<int>(max - min + 1));
      BitVector in0 = BitVector(16, in0V);
      BitVector in1 = BitVector(16, in1V);

      cout << "in0 = " << in0 << ", " << in0.to_type<int>() << endl;
      cout << "in1 = " << in1 << ", " << in1.to_type<int>() << endl;

      interpSim.setFreshValue("in0", in0);
      interpSim.setFreshValue("in1", in1);
      interpSim.update();

      compileSim.setFreshValue("in0", in0);
      compileSim.setFreshValue("in1", in1);
      compileSim.update();

      auto interpOut = interpSim.getBitVec("out");
      auto compiledOut = compileSim.getBitVec("out");
        
      cout << "interp out    = " << interpOut << ", " << interpOut.to_type<int>() << endl;
      cout << "compiled out  = " << compiledOut << ", " << compiledOut.to_type<int>() << endl;
        
      REQUIRE(interpSim.getBitVec("out") == compileSim.getBitVec("out"));
    }
  }

  TEST_CASE("Unary op x propagation") {
    Env e;

    vector<CellType> unops{CELL_TYPE_PASSTHROUGH};
    for (auto opType : unops) {
      //CellType opType = CELL_TYPE_PASSTHROUGH;

      CellType notChainType = e.addCellType(toString(opType) + "_test_cell");
      CellDefinition& def = e.getDef(notChainType);
      def.addPort("in", 16, PORT_TYPE_IN);
      def.addPort("out", 16, PORT_TYPE_OUT);

      CellId binop =
        def.addCell(toString(opType) + "_test_inst",
                    opType,
                    {{PARAM_WIDTH, BitVector(32, 16)}});

      CellId inCell = def.getPortCellId("in");
      CellId outCell = def.getPortCellId("out");
      def.connect(inCell, PORT_ID_OUT, binop, PORT_ID_IN);
      def.connect(binop, PORT_ID_OUT, outCell, PORT_ID_IN);

      Simulator interpSim(e, def);

      Simulator compileSim(e, def);
      compileSim.compileCircuit();

      interpSim.setFreshValue("in", BitVector("16'hxxxx"));
      interpSim.update();
      compileSim.setFreshValue("in", BitVector("16'hxxxx"));
      compileSim.update();

      REQUIRE(same_representation(interpSim.getBitVec("out"),
                                  compileSim.getBitVec("out")));
    }
  }

  TEST_CASE("Comparing binary ops in simulation and interpretation") {
    Env e;
    vector<BitVector> interpResults;
    srand(23419);

    vector<CellType> binops{CELL_TYPE_SUB, CELL_TYPE_MUL, CELL_TYPE_AND, CELL_TYPE_OR, CELL_TYPE_XOR};
    for (auto binop : binops) {

      CellDefinition& def = buildBinopCellDef(e, binop);

      Simulator interpSim(e, def);

      Simulator compileSim(e, def);
      compileSim.compileCircuit();

      compareSimulators(interpSim, compileSim);

      cout << "Comparing raw to interpreted" << endl;
      compileSim.simulateRaw();
      compareSimulators(interpSim, compileSim);

    }

    vector<CellType> reduceOps{CELL_TYPE_UGT, CELL_TYPE_ULT, CELL_TYPE_UGE, CELL_TYPE_ULE, CELL_TYPE_EQ, CELL_TYPE_NEQ};
    for (auto binop : reduceOps) {

      cout << "Testing operation " << toString(binop) << endl;
      
      CellDefinition& def = buildReduceBinopCellDef(e, binop);

      Simulator interpSim(e, def);

      Simulator compileSim(e, def);
      compileSim.compileCircuit();

      compareSimulators(interpSim, compileSim);

      cout << "Comparing raw to interpreted" << endl;
      compileSim.simulateRaw();
      compareSimulators(interpSim, compileSim);
    }
    
  }
}
