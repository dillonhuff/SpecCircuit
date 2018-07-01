#include "catch.hpp"

#include "convert_coreir.h"
#include "simulator.h"
#include "utils.h"

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

  class RegressionSimulator {
    Simulator interpSim;
    Simulator compileSim;

  public:
    RegressionSimulator(Env& e, CellDefinition& def) :
      interpSim(e, def), compileSim(e, def) {
      compileSim.compileCircuit();
    }

    void setFreshValue(const std::string& val, const BitVector& bv) {
      interpSim.setFreshValue(val, bv);
      compileSim.setFreshValue(val, bv);
    }

    void refreshConstants() {
      interpSim.refreshConstants();
      compileSim.refreshConstants();
    }

    void update() {
      interpSim.update();
      compileSim.update();
    }

    void debugPrintPorts() {
      interpSim.debugPrintPorts();
      compileSim.debugPrintPorts();
    }

    void debugPrintTableValues() {
      interpSim.debugPrintTableValues();
      compileSim.debugPrintTableValues();
    }

    BitVector getBitVec(const std::string& cellName) {
      auto interpBv = interpSim.getBitVec(cellName);
      auto compileBv = compileSim.getBitVec(cellName);

      if (!same_representation(interpBv, compileBv)) {
        cout << "ERROR: " << interpBv << " != " << compileBv << endl;
        assert(same_representation(interpBv, compileBv));
      }

      return interpBv;
    }

    bool internalStatesConsistent() {

      bool allConsistent = true;
      for (auto ctp : interpSim.def.getCellMap()) {
        CellId cid = ctp.first;
        const Cell& cell = interpSim.def.getCellRefConst(cid);
        for (auto outPort : cell.outputPorts()) {
          auto interpBv = interpSim.getBitVec(cid, outPort);
          auto compileBv = compileSim.getBitVec(cid, outPort);

          if (!same_representation(interpBv, compileBv)) {
            cout << "ERROR: Simulators disagree on " <<
              sigPortString(interpSim.def, {cid, outPort}) << ": " <<
              interpBv << " != " << compileBv << endl;
            allConsistent = false;

            for (auto inPort : cell.inputPorts()) {
              auto interpInput = interpSim.materializeInput({cid, inPort});
              auto compileInput = compileSim.materializeInput({cid, inPort});

              cout << "\t" << interpInput << endl;
              cout << "\t" << compileInput << endl;
            }
          }
          
        }
      }

      return allConsistent;
    }

  };

  TEST_CASE("Regression test mem unq internals") {
    Env circuitEnv = loadFromCoreIR("global.mem_unq1",
                                    "./test/memory_tile_unq1.json");
    CellDefinition& def = circuitEnv.getDef("mem_unq1");

    RegressionSimulator sim(circuitEnv, def);
    sim.refreshConstants();

    cout << "Compiling mem unq" << endl;
    //sim.compileCircuit();
    cout << "Done compiling mem unq" << endl;

    sim.setFreshValue("addr", BitVector(9, 13));
    sim.setFreshValue("cen", BitVector(1, 1));
    sim.setFreshValue("wen", BitVector(1, 1));
    sim.setFreshValue("data_in", BitVector(16, 562));

    posedge("clk", sim);

    sim.debugPrintPorts();

    sim.setFreshValue("wen", BitVector(1, 0));
    posedge("clk", sim);

    posedge("clk", sim);
    posedge("clk", sim);
    posedge("clk", sim);

    assert(sim.internalStatesConsistent());
    //REQUIRE(sim.getBitVec("data_out") == BitVector(16, 562));

    sim.setFreshValue("addr", BitVector(9, 0));
    sim.setFreshValue("data_in", BitVector(16, 4965));
    posedge("clk", sim);

    cout << "data_out first = " << sim.getBitVec("data_out") << endl;
    cout << "Table values" << endl;
    sim.debugPrintTableValues();
    REQUIRE(same_representation(sim.getBitVec("data_out"), BitVector("16'hxxxx")));
    //REQUIRE(sim.getBitVec("data_out") == BitVector(16, 0));

    sim.setFreshValue("wen", BitVector(1, 1));
    posedge("clk", sim);

    cout << "data_out = " << sim.getBitVec("data_out") << endl;
    REQUIRE(same_representation(sim.getBitVec("data_out"), BitVector("16'hxxxx")));
    //REQUIRE(sim.getBitVec("data_out") == BitVector(16, 0));

    posedge("clk", sim);

    REQUIRE(sim.getBitVec("data_out") == BitVector(16, 4965));

  }

  TEST_CASE("Register x propagation") {
    Env e;

    CellType opType = CELL_TYPE_REG;
    CellType notChainType = e.addCellType(toString(opType) + "_test_cell");
    CellDefinition& def = e.getDef(notChainType);
    def.addPort("in", 16, PORT_TYPE_IN);
    def.addPort("clk", 1, PORT_TYPE_IN);
    def.addPort("out", 16, PORT_TYPE_OUT);

    CellId binop =
      def.addCell(toString(opType) + "_test_inst",
                  opType,
                  {{PARAM_WIDTH, BitVector(32, 16)},
                      {PARAM_INIT_VALUE, BitVector(16, 0)},
                        {PARAM_CLK_POSEDGE, BitVector(1, 1)}});

    CellId inCell = def.getPortCellId("in");
    CellId clkCell = def.getPortCellId("clk");
    CellId outCell = def.getPortCellId("out");
    def.connect(inCell, PORT_ID_OUT, binop, PORT_ID_IN);
    def.connect(clkCell, PORT_ID_OUT, binop, PORT_ID_CLK);
    def.connect(binop, PORT_ID_OUT, outCell, PORT_ID_IN);

    Simulator interpSim(e, def);

    Simulator compileSim(e, def);
    compileSim.compileCircuit();

    interpSim.setFreshValue("in", BitVector("16'hxxxx"));
    posedge("clk", interpSim);
    
    compileSim.setFreshValue("in", BitVector("16'hxxxx"));
    posedge("clk", compileSim);

    REQUIRE(same_representation(interpSim.getBitVec("out"),
                                compileSim.getBitVec("out")));

    REQUIRE(same_representation(compileSim.getBitVec("out"),
                                BitVector("16'hxxxx")));

  }

  TEST_CASE("Unary op x propagation") {
    Env e;

    vector<CellType> unops{CELL_TYPE_PASSTHROUGH, CELL_TYPE_NOT};
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

      REQUIRE(same_representation(compileSim.getBitVec("out"),
                                  BitVector("16'hxxxx")));
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
