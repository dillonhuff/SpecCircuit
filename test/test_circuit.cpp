#define CATCH_CONFIG_MAIN

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

  TEST_CASE("Prefix matching") {

    REQUIRE(matchesAnyPrefix("mem_0x18$memory_core$mem_inst1$mem_inst$data_array$mem",
                             {"mem_0x18"}));
  }

  TEST_CASE("Compiling 2 state values") {

    SECTION("Directly from inputs to outputs") {
      Env e;
      CellType modType = e.addCellType("in_to_out");
      CellDefinition& def = e.getDef(modType);
      def.addPort("in", 8, PORT_TYPE_IN);
      def.addPort("out", 8, PORT_TYPE_OUT);

      CellId in = def.getPortCellId("in");
      CellId out = def.getPortCellId("out");

      def.connect(in, PORT_ID_OUT,
                  out, PORT_ID_IN);

      Simulator sim(e, def);
      sim.setFreshValue("in", BitVec(8, 0));
      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(8, 0));

      sim.compileCircuit();
      sim.simulateRaw();

      sim.setFreshValue("in", BitVec(8, 12));
      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(8, 12));
      
    }

    SECTION("Directly from inputs to outputs, longer than 8 bits") {
      Env e;
      CellType modType = e.addCellType("in_to_out");
      CellDefinition& def = e.getDef(modType);
      def.addPort("in", 16, PORT_TYPE_IN);
      def.addPort("out", 16, PORT_TYPE_OUT);

      CellId in = def.getPortCellId("in");
      CellId out = def.getPortCellId("out");

      def.connect(in, PORT_ID_OUT,
                  out, PORT_ID_IN);

      Simulator sim(e, def);
      sim.setFreshValue("in", BitVec(16, 0));
      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(16, 0));
      
      sim.compileCircuit();
      sim.simulateRaw();

      sim.setFreshValue("in", BitVec(16, 871));
      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(16, 871));
      
    }

    SECTION("One register") {
      Env e;
      CellType modType = e.addCellType("reg_circ");
      CellDefinition& def = e.getDef(modType);
      def.addPort("in", 8, PORT_TYPE_IN);
      def.addPort("clk", 1, PORT_TYPE_IN);
      def.addPort("arst", 1, PORT_TYPE_IN);
      def.addPort("out", 8, PORT_TYPE_OUT);

      CellId reg = def.addCell("reg",
                               CELL_TYPE_REG_ARST,
                               {{PARAM_WIDTH, BitVector(32, 8)},
                                   {PARAM_INIT_VALUE, BitVector(8, 12)},
                                     {PARAM_CLK_POSEDGE, BitVector(1, 1)},
                                       {PARAM_ARST_POSEDGE, BitVector(1, 0)}});

      CellId in = def.getPortCellId("in");
      CellId clk = def.getPortCellId("clk");
      CellId arst = def.getPortCellId("arst");
      CellId out = def.getPortCellId("out");

      def.connect(in, PORT_ID_OUT,
                  reg, PORT_ID_IN);

      def.connect(clk, PORT_ID_OUT,
                  reg, PORT_ID_CLK);

      def.connect(arst, PORT_ID_OUT,
                  reg, PORT_ID_ARST);

      def.connect(reg, PORT_ID_OUT,
                  out, PORT_ID_IN);

      Simulator sim(e, def);
      sim.compileCircuit();
      sim.simulateRaw();

      sim.setFreshValue("arst", BitVec(1, 1));
      sim.update();
      sim.setFreshValue("arst", BitVec(1, 0));
      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(8, 12));

      sim.setFreshValue("in", BitVec(8, 29));
      posedge("clk", sim);

      REQUIRE(sim.getBitVec("out") == BitVec(8, 29));

      posedge("arst", sim);
      negedge("arst", sim);

      REQUIRE(sim.getBitVec("out") == BitVec(8, 12));

    }
  }

  TEST_CASE("Compiled code generation for circuit with register") {

    SECTION("One register") {
      Env e;
      CellType modType = e.addCellType("reg_circ");
      CellDefinition& def = e.getDef(modType);
      def.addPort("in", 8, PORT_TYPE_IN);
      def.addPort("clk", 1, PORT_TYPE_IN);
      def.addPort("arst", 1, PORT_TYPE_IN);
      def.addPort("out", 8, PORT_TYPE_OUT);

      CellId reg = def.addCell("reg",
                               CELL_TYPE_REG_ARST,
                               {{PARAM_WIDTH, BitVector(32, 8)},
                                   {PARAM_INIT_VALUE, BitVector(8, 12)},
                                     {PARAM_CLK_POSEDGE, BitVector(1, 1)},
                                       {PARAM_ARST_POSEDGE, BitVector(1, 0)}});

      CellId in = def.getPortCellId("in");
      CellId clk = def.getPortCellId("clk");
      CellId arst = def.getPortCellId("arst");
      CellId out = def.getPortCellId("out");

      def.connect(in, PORT_ID_OUT,
                  reg, PORT_ID_IN);

      def.connect(clk, PORT_ID_OUT,
                  reg, PORT_ID_CLK);

      def.connect(arst, PORT_ID_OUT,
                  reg, PORT_ID_ARST);

      def.connect(reg, PORT_ID_OUT,
                  out, PORT_ID_IN);

      Simulator sim(e, def);
      sim.compileCircuit();

      sim.setFreshValue("arst", BitVec(1, 1));
      sim.update();
      sim.setFreshValue("arst", BitVec(1, 0));
      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(8, 12));

      sim.setFreshValue("in", BitVec(8, 29));
      posedge("clk", sim);

      REQUIRE(sim.getBitVec("out") == BitVec(8, 29));

      posedge("arst", sim);
      negedge("arst", sim);

      REQUIRE(sim.getBitVec("out") == BitVec(8, 12));

    }

    SECTION("One register with clock gating") {
      Env e;
      CellType modType = e.addCellType("reg_circ");
      CellDefinition& def = e.getDef(modType);
      def.addPort("in", 8, PORT_TYPE_IN);
      def.addPort("clk", 1, PORT_TYPE_IN);
      def.addPort("clk_en", 1, PORT_TYPE_IN);
      def.addPort("arst", 1, PORT_TYPE_IN);
      def.addPort("out", 8, PORT_TYPE_OUT);

      CellId reg = def.addCell("reg",
                               CELL_TYPE_REG_ARST,
                               {{PARAM_WIDTH, BitVector(32, 8)},
                                   {PARAM_INIT_VALUE, BitVector(8, 12)},
                                     {PARAM_CLK_POSEDGE, BitVector(1, 1)},
                                       {PARAM_ARST_POSEDGE, BitVector(1, 0)}});

      CellId clkAnd = def.addCell("and",
                                  CELL_TYPE_AND,
                                  {{PARAM_WIDTH, BitVector(32, 1)}});

      CellId in = def.getPortCellId("in");
      CellId clk = def.getPortCellId("clk");
      CellId arst = def.getPortCellId("arst");
      CellId out = def.getPortCellId("out");
      CellId clk_en = def.getPortCellId("clk_en");

      def.connect(in, PORT_ID_OUT,
                  reg, PORT_ID_IN);

      def.connect(clk, PORT_ID_OUT,
                  clkAnd, PORT_ID_IN0);

      def.connect(clk_en, PORT_ID_OUT,
                  clkAnd, PORT_ID_IN1);

      def.connect(clkAnd, PORT_ID_OUT,
                  reg, PORT_ID_CLK);

      def.connect(arst, PORT_ID_OUT,
                  reg, PORT_ID_ARST);

      def.connect(reg, PORT_ID_OUT,
                  out, PORT_ID_IN);

      Simulator sim(e, def);
      sim.compileCircuit();

      sim.setFreshValue("arst", BitVec(1, 1));
      sim.update();
      sim.setFreshValue("arst", BitVec(1, 0));
      sim.update();

      SECTION("First reset") {
        REQUIRE(sim.getBitVec("out") == BitVec(8, 12));
      }

      sim.setFreshValue("clk_en", BitVec(1, 1));
      sim.setFreshValue("in", BitVec(8, 29));
      posedge("clk", sim);

      SECTION("Setting value after one clock cycle and clk_en = 1") {
        REQUIRE(sim.getBitVec("out") == BitVec(8, 29));
      }

      posedge("arst", sim);
      negedge("arst", sim);

      SECTION("Resetting again") {
        REQUIRE(sim.getBitVec("out") == BitVec(8, 12));
      }

      cout << "Single register values" << endl;
      sim.debugPrintRegisters();

      REQUIRE(sim.getRegisterValue(reg) == sim.getBitVec("out"));
      
      sim.setFreshValue("in", BitVec(8, 57));
      posedge("clk", sim);
      negedge("clk", sim);

      SECTION("Running the clock again") {
        REQUIRE(sim.getBitVec("out") == BitVec(8, 57));
      }

      sim.setFreshValue("clk_en", BitVec(1, 0));
      sim.setFreshValue("in", BitVec(8, 18));
      posedge("clk", sim);

      SECTION("clk rises, but clk_en = 0") {
        REQUIRE(sim.getBitVec("out") == BitVec(8, 57));
      }

      cout << "Single register values" << endl;
      sim.debugPrintRegisters();

      REQUIRE(sim.getRegisterValue(reg) == sim.getBitVec("out"));

      sim.setFreshValue("clk_en", BitVec(1, 1));
      sim.update();

      SECTION("Generate a new rising clock edge as and.out goes 0 -> 1") {
        REQUIRE(sim.getBitVec("out") == BitVec(8, 18));
      }

      negedge("clk", sim);

      sim.setFreshValue("clk", BitVec(1, 1));
      sim.update();

      sim.setFreshValue("clk_en", BitVec(1, 1));
      sim.setFreshValue("in", BitVec(8, 33));
      sim.update();

      SECTION("Clock enable on but no rising edge") {
        REQUIRE(sim.getBitVec("out") == BitVec(8, 18));
      }
    }

  }

  TEST_CASE("Specialization of inputs") {
    Env e;
    CellType modType = e.addCellType("one_mux");
    CellDefinition& def = e.getDef(modType);
    def.addPort("in0", 4, PORT_TYPE_IN);
    def.addPort("in1", 4, PORT_TYPE_IN);
    def.addPort("sel", 1, PORT_TYPE_IN);
    def.addPort("out", 4, PORT_TYPE_OUT);

    CellId muxId = def.addCell("mux0",
                               CELL_TYPE_MUX,
                               {{PARAM_WIDTH, BitVector(32, 4)}});

    CellId in0Cell = def.getPortCellId("in0");
    CellId in1Cell = def.getPortCellId("in1");
    CellId selCell = def.getPortCellId("sel");
    CellId outSel = def.getPortCellId("out");

    def.connect(in0Cell, PORT_ID_OUT,
                muxId, PORT_ID_IN0);

    def.connect(in1Cell, PORT_ID_OUT,
                muxId, PORT_ID_IN1);

    def.connect(selCell, PORT_ID_OUT,
                muxId, PORT_ID_SEL);

    def.connect(selCell, PORT_ID_OUT,
                muxId, PORT_ID_SEL);

    def.connect(muxId, PORT_ID_OUT,
                outSel, PORT_ID_IN);

    SECTION("Unspecialized simulation") {
      Simulator sim(e, def);
      sim.setFreshValue("in0", BitVector(4, 2));
      sim.setFreshValue("in1", BitVector(4, 13));
      sim.setFreshValue("sel", BitVector(1, 0));
      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(4, 2));

      sim.setFreshValue("sel", BitVector(1, 1));
      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(4, 13));

    }

    SECTION("Specializing wrt select == 1") {
      Simulator sim(e, def);
      sim.specializePort("sel", BitVec(1, 1));

      foldConstants(def, {});
      deleteDeadInstances(def);
      sim.refreshConstants();

      REQUIRE(definitionIsConsistent(def));

      cout << "Setting in0" << endl;

      sim.setFreshValue("in0", BitVector(4, 2));

      cout << "Setting in1" << endl;

      sim.setFreshValue("in1", BitVector(4, 13));

      cout << "Done setting in1" << endl;

      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(4, 13));

      SECTION("Compile code") {

        REQUIRE(sim.compileCircuit());

        REQUIRE(sim.hasSimulateFunction());

        sim.setFreshValue("in1", BitVector(4, 12));

        sim.update();

        REQUIRE(sim.getBitVec("out") == BitVec(4, 12));
      }
    }

    SECTION("Specializing wrt select == 0") {
      Simulator sim(e, def);
      sim.specializePort("sel", BitVec(1, 0));
      sim.refreshConstants();
      
      sim.setFreshValue("in0", BitVector(4, 2));
      sim.setFreshValue("in1", BitVector(4, 13));
      sim.update();

      REQUIRE(sim.getBitVec("out") == BitVec(4, 2));
    }
    
  }
  
  TEST_CASE("Memory") {
    Context* c = newContext();
    Namespace* g = c->getGlobal();

    uint width = 20;
    uint depth = 4;
    uint index = 2;

    Type* memoryType = c->Record({
        {"clk", c->Named("coreir.clkIn")},
          {"write_data", c->BitIn()->Arr(width)},
            {"write_addr", c->BitIn()->Arr(index)},
              {"write_en", c->BitIn()},
                {"read_data", c->Bit()->Arr(width)},
                  {"read_addr", c->BitIn()->Arr(index)}
      });

      
    Module* memory = c->getGlobal()->newModuleDecl("memory0", memoryType);
    ModuleDef* def = memory->newModuleDef();

    def->addInstance("m0",
                     "coreir.mem",
                     {{"width", Const::make(c,width)},{"depth", Const::make(c,depth)}});

    def->connect("self.clk", "m0.clk");
    def->connect("self.write_en", "m0.wen");
    def->connect("self.write_data", "m0.wdata");
    def->connect("self.write_addr", "m0.waddr");
    def->connect("self.read_data", "m0.rdata");
    def->connect("self.read_addr", "m0.raddr");

    memory->setDef(def);

    c->runPasses({"rungenerators","flattentypes","flatten"});      

    SECTION("Simulating memory") {
      Env circuitEnv = convertFromCoreIR(c, memory);
      CellDefinition cDef = circuitEnv.getDef("memory0");

      Simulator state(circuitEnv, cDef);

      state.setFreshValue("clk", BitVec(1, 0));
      state.update();

      // Do not write when write_en == 0
      state.setFreshValue("clk", BitVec(1, 1));
      state.setFreshValue("write_en", BitVec(1, 0));
      state.setFreshValue("write_addr", BitVec(index, 0));
      state.setFreshValue("write_data", BitVec(width, 23));
      state.setFreshValue("read_addr", BitVec(index, 0));
      state.update();

      REQUIRE(state.getBitVec("read_data") == BitVec(width, 0));

      state.setFreshValue("clk", BitVec(1, 0));
      state.setFreshValue("write_en", BitVec(1, 1));
      state.update();

      state.setFreshValue("clk", BitVec(1, 1));
      state.update();

      REQUIRE(state.getBitVec("read_data") == BitVec(width, 23));

      state.setFreshValue("read_addr", BitVec(index, 2));
      state.update();

      REQUIRE(state.getBitVec("read_data") == BitVec(width, 0));
    }

    SECTION("Simulating with compiled code") {
      Env circuitEnv = convertFromCoreIR(c, memory);
      CellDefinition cDef = circuitEnv.getDef("memory0");

      Simulator state(circuitEnv, cDef);
      state.compileCircuit();

      state.setFreshValue("clk", BitVec(1, 0));
      state.update();

      // Do not write when write_en == 0
      state.setFreshValue("clk", BitVec(1, 1));
      state.setFreshValue("write_en", BitVec(1, 0));
      state.setFreshValue("write_addr", BitVec(index, 0));
      state.setFreshValue("write_data", BitVec(width, 23));
      state.setFreshValue("read_addr", BitVec(index, 0));
      state.update();

      REQUIRE(state.getBitVec("read_data") == BitVec(width, 0));

      state.setFreshValue("clk", BitVec(1, 0));
      state.setFreshValue("write_en", BitVec(1, 1));
      state.update();

      state.setFreshValue("clk", BitVec(1, 1));
      state.update();

      REQUIRE(state.getBitVec("read_data") == BitVec(width, 23));

      state.setFreshValue("read_addr", BitVec(index, 2));
      state.update();

      REQUIRE(state.getBitVec("read_data") == BitVec(width, 0));
    }

    SECTION("Simulating with two state compiled code") {
      Env circuitEnv = convertFromCoreIR(c, memory);
      CellDefinition cDef = circuitEnv.getDef("memory0");

      Simulator state(circuitEnv, cDef);
      state.compileCircuit();
      state.simulateRaw();

      state.setFreshValue("clk", BitVec(1, 0));
      state.update();

      // Do not write when write_en == 0
      state.setFreshValue("clk", BitVec(1, 1));
      state.setFreshValue("write_en", BitVec(1, 0));
      state.setFreshValue("write_addr", BitVec(index, 0));

      cout << "Before setting 23 in write_data" << endl;
      state.debugPrintRawValueTable();

      state.setFreshValue("write_data", BitVec(width, 23));

      cout << "After setting 23 in write_data" << endl;
      state.debugPrintRawValueTable();

      state.setFreshValue("read_addr", BitVec(index, 0));
      cout << "After setting read_addr" << endl;
      state.debugPrintRawValueTable();

      state.update();

      cout << "After setting read_addr and updating" << endl;
      state.debugPrintRawValueTable();
      
      REQUIRE(state.getBitVec("read_data") == BitVec(width, 0));

      state.setFreshValue("clk", BitVec(1, 0));

      cout << "After setting clk" << endl;
      state.debugPrintRawValueTable();
      
      state.setFreshValue("write_en", BitVec(1, 1));

      cout << "After setting write_en" << endl;
      state.debugPrintRawValueTable();
      
      state.update();

      cout << "After first update call" << endl;
      state.debugPrintRawValueTable();
      
      state.setFreshValue("clk", BitVec(1, 1));
      state.update();

      cout << "Before testing for 23 in read_data" << endl;
      state.debugPrintRawValueTable();

      REQUIRE(state.getBitVec("read_data") == BitVec(width, 23));

      state.setFreshValue("read_addr", BitVec(index, 2));
      state.update();

      REQUIRE(state.getBitVec("read_data") == BitVec(width, 0));
    }
    
    deleteContext(c);
  }

  TEST_CASE("Mem unq1") {
    Env circuitEnv = loadFromCoreIR("global.mem_unq1",
                                    "./test/memory_tile_unq1.json");
    CellDefinition& def = circuitEnv.getDef("mem_unq1");

    SECTION("Interpreted simulation") {
      Simulator sim(circuitEnv, def);
      sim.setFreshValue("addr", BitVector(9, 13));
      sim.setFreshValue("cen", BitVector(1, 1));
      sim.setFreshValue("wen", BitVector(1, 1));
      sim.setFreshValue("data_in", BitVector(16, 562));

      posedge("clk", sim);

      sim.setFreshValue("wen", BitVector(1, 0));
      posedge("clk", sim);

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 562));

      sim.setFreshValue("addr", BitVector(9, 0));
      sim.setFreshValue("data_in", BitVector(16, 4965));
      posedge("clk", sim);

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 0));

      sim.setFreshValue("wen", BitVector(1, 1));
      posedge("clk", sim);

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 0));

      posedge("clk", sim);

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 4965));
    }

    SECTION("Compiled simulation") {
      Simulator sim(circuitEnv, def);
      sim.refreshConstants();

      cout << "Compiling mem unq" << endl;
      sim.compileCircuit();
      cout << "Done compiling mem unq" << endl;

      sim.setFreshValue("addr", BitVector(9, 13));
      sim.setFreshValue("cen", BitVector(1, 1));
      sim.setFreshValue("wen", BitVector(1, 1));
      sim.setFreshValue("data_in", BitVector(16, 562));

      posedge("clk", sim);

      sim.debugPrintPorts();

      // cout << "All values in mem unq" << endl;
      // sim.debugPrintTableValues();
    
      // cout << "WEN should be 1" << endl;
      // sim.debugPrintMemories();
      
      sim.setFreshValue("wen", BitVector(1, 0));
      posedge("clk", sim);

      //      cout << "WEN should be 0" << endl;
      //      sim.debugPrintMemories();

      posedge("clk", sim);
      posedge("clk", sim);
      posedge("clk", sim);

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 562));

      sim.setFreshValue("addr", BitVector(9, 0));
      sim.setFreshValue("data_in", BitVector(16, 4965));
      posedge("clk", sim);

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 0));

      sim.setFreshValue("wen", BitVector(1, 1));
      posedge("clk", sim);

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 0));

      posedge("clk", sim);

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 4965));
    }

    // Q: What are the problems in this test case?
    // A: The test logic itself is complicated, so Im not sure I trust it
    //
    //    It involves low level code generation (Q: Why is this a problem?)
    //
    //    Flattening makes it harder to recreate the structure we are simulating
    //
    //    The whole test bench takes too long to run often, so I dont know if what Im
    //    doing to fix this bug is breaking other things

    // Possible problem: Unary comparators should have width 1 on the output but
    // they actually have widths equal to the lengths of their arguments. But if
    // this is the problem why arent they also adjusted to the correct size by
    // the offsetting process when constructing two state code?
    SECTION("Two state compiled simulation") {
      Simulator sim(circuitEnv, def);
      sim.refreshConstants();

      cout << "Compiling mem unq" << endl;
      sim.compileCircuit();
      sim.simulateRaw();
      cout << "Done compiling mem unq" << endl;

      sim.setFreshValue("addr", BitVector(9, 13));
      sim.setFreshValue("cen", BitVector(1, 1));
      sim.setFreshValue("wen", BitVector(1, 1));
      sim.setFreshValue("data_in", BitVector(16, 562));

      posedge("clk", sim);

      sim.debugPrintPorts();

      // cout << "All values in mem unq" << endl;
      // sim.debugPrintTableValues();
    
      // cout << "WEN should be 1" << endl;
      // sim.debugPrintMemories();
      
      sim.setFreshValue("wen", BitVector(1, 0));
      posedge("clk", sim);

      //      cout << "WEN should be 0" << endl;
      //      sim.debugPrintMemories();

      posedge("clk", sim);
      posedge("clk", sim);
      posedge("clk", sim);

      // cout << "Value table before check for 562 output" << endl;
      // sim.debugPrintRawValueTable();

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 562));

      sim.setFreshValue("addr", BitVector(9, 0));
      sim.setFreshValue("data_in", BitVector(16, 4965));
      posedge("clk", sim);

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 0));

      sim.setFreshValue("wen", BitVector(1, 1));
      posedge("clk", sim);

      // cout << "Value table before setting wen" << endl;
      // sim.debugPrintRawValueTable();
      
      sim.setFreshValue("wen", BitVector(1, 0));

      // cout << "Value table after setting wen" << endl;
      // sim.debugPrintRawValueTable();
      
      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 0));

      // cout << "Value table before clk for 4965 output" << endl;
      // sim.debugPrintRawValueTable();
      
      //posedge("clk", sim);
      sim.setFreshValue("clk", BitVec(1, 0));
      sim.update();

      // cout << "After updating clk low" << endl;
      // sim.debugPrintRawValueTable();
      
      sim.setFreshValue("clk", BitVec(1, 1));
      sim.update();
      
      // cout << "Value table before check for 4965 output" << endl;
      // sim.debugPrintRawValueTable();
      
      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 4965));
    }
    
  }

  TEST_CASE("Testing memory core") {
    Env circuitEnv =
      loadFromCoreIR("global.memory_core_unq1",
                     "./test/memory_tile_unq1.json");

    CellDefinition& def = circuitEnv.getDef("memory_core_unq1");
    
    REQUIRE(circuitEnv.getCellDefs().size() == 1);

    bool foundMem = false;
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      if (def.getCellRefConst(cid).getCellType() == CELL_TYPE_MEM) {
        foundMem = true;
      }
    }

    REQUIRE(foundMem);

    SECTION("without compilation") {
      Simulator sim(circuitEnv, def);
      reset("reset", sim);

      sim.setFreshValue("clk_en", BitVec(1, 1));
      sim.setFreshValue("config_en", BitVec(1, 1));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.setFreshValue("config_en_sram", BitVec(4, 0));
      sim.update();

      BitVector configAddr(32, 0);
      sim.setFreshValue("config_addr", configAddr);
      sim.update();

      uint32_t configDataInt = 0;
      configDataInt |= ((uint32_t) 2) << 0; // SRAM mode
      configDataInt |= ((uint32_t) 1) << 2; // Tile enabled
      configDataInt |= ((uint32_t) 8) << 3; // Depth 8
    
      BitVector configData(32, configDataInt);

      cout << "Memory tile config data = " << configData << endl;
      sim.setFreshValue("config_data", configData);

      posedge("clk_in", sim);

      sim.setFreshValue("config_en", BitVec(1, 0));
      sim.update();

      sim.setFreshValue("wen_in", BitVec(1, 1));
      sim.setFreshValue("addr_in", BitVec(16, 4));
      sim.setFreshValue("data_in", BitVec(16, 72));
      posedge("clk_in", sim);

      // cout << "Debug immediately after write call " << endl;
      // sim.debugPrintMemories();

      sim.setFreshValue("wen_in", BitVec(1, 1));
      sim.setFreshValue("addr_in", BitVec(16, 2));
      sim.setFreshValue("data_in", BitVec(16, 45));
      posedge("clk_in", sim);
    
      sim.setFreshValue("wen_in", BitVec(1, 0));
      sim.setFreshValue("addr_in", BitVec(16, 4));
      sim.setFreshValue("ren_in", BitVec(1, 1));
      posedge("clk_in", sim);

      posedge("clk_in", sim);
      posedge("clk_in", sim);

      // sim.debugPrintMemories();

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 72));

    }

    SECTION("Memory tile using compiled code") {
      Simulator sim(circuitEnv, def);
      reset("reset", sim);

      sim.setFreshValue("clk_en", BitVec(1, 1));
      sim.setFreshValue("config_en", BitVec(1, 1));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.setFreshValue("config_en_sram", BitVec(4, 0));
      sim.update();

      BitVector configAddr(32, 0);
      sim.setFreshValue("config_addr", configAddr);
      sim.update();

      uint32_t configDataInt = 0;
      configDataInt |= ((uint32_t) 2) << 0; // SRAM mode
      configDataInt |= ((uint32_t) 1) << 2; // Tile enabled
      configDataInt |= ((uint32_t) 8) << 3; // Depth 8
    
      BitVector configData(32, configDataInt);

      cout << "Memory tile config data = " << configData << endl;
      sim.setFreshValue("config_data", configData);

      posedge("clk_in", sim);

      sim.setFreshValue("config_en", BitVec(1, 0));
      sim.update();

      sim.specializePort("config_data", BitVec(32, 0));
      sim.specializePort("config_addr", BitVec(32, 0));
      sim.specializePort("config_en_sram", BitVec(4, 0));
      sim.specializePort("clk_en", BitVec(1, 1));
      sim.specializePort("config_en_linebuf", BitVec(1, 0));
      sim.specializePort("config_read", BitVec(1, 0));
      sim.specializePort("config_write", BitVec(1, 0));
      sim.specializePort("config_en", BitVec(1, 0));
      sim.specializePort("reset", BitVec(1, 0));
      sim.specializePort("chain_wen_in", BitVec(1, 0));
      sim.specializePort("chain_in", BitVec(16, 0));

      foldConstants(sim.def, sim.allRegisterValues());
      deleteDeadInstances(sim.def);

      dbhc::maybe<PortId> clkPort = getTrueClockPort(sim.def);
      REQUIRE(clkPort.has_value());
      REQUIRE(def.getPortName(clkPort.get_value()) == "clk_in");

      REQUIRE(allPosedge(def));

      sim.refreshConstants();

      REQUIRE(sim.compileCircuit());

      sim.setFreshValue("wen_in", BitVec(1, 1));
      sim.setFreshValue("addr_in", BitVec(16, 4));
      sim.setFreshValue("data_in", BitVec(16, 72));
      posedge("clk_in", sim);

      // cout << "Debug immediately after write call " << endl;
      // sim.debugPrintMemories();

      sim.setFreshValue("wen_in", BitVec(1, 1));
      sim.setFreshValue("addr_in", BitVec(16, 2));
      sim.setFreshValue("data_in", BitVec(16, 45));
      posedge("clk_in", sim);
    
      sim.setFreshValue("wen_in", BitVec(1, 0));
      sim.setFreshValue("addr_in", BitVec(16, 4));
      sim.setFreshValue("ren_in", BitVec(1, 1));
      posedge("clk_in", sim);

      posedge("clk_in", sim);
      posedge("clk_in", sim);

      // sim.debugPrintMemories();

      REQUIRE(sim.getBitVec("data_out") == BitVector(16, 72));

      int nCycles = 10000;
      cout << "Running memory core for " << nCycles << endl;

      auto start = high_resolution_clock::now();

      BitVector input = BitVector(16, 0);
      for (int i = 0; i < nCycles; i++) {
        // if ((i % 100) == 0) {
        //   cout << "i = " << i << endl;
        // }

        sim.setFreshValue("wen_in", BitVec(1, i % 2));
        sim.setFreshValue("clk_in", BitVec(1, 0));
        sim.update();

        input = BitVector(16, i);
        sim.setFreshValue("data_in", input);

        sim.setFreshValue("clk_in", BitVec(1, 1));
        sim.update();
      }

      auto stop = high_resolution_clock::now();

      auto duration = duration_cast<milliseconds>(stop - start);

      cout << "Time taken for " << nCycles << ": "
           << duration.count() << " milliseconds" << endl;
      
    }

  }    

  TEST_CASE("Memory core linebuffer") {

    // Testing Linebuffer mode

    Env circuitEnv =
      loadFromCoreIR("global.memory_core_unq1",
                     "./test/memory_tile_unq1.json");

    CellDefinition& def = circuitEnv.getDef("memory_core_unq1");
    
    REQUIRE(circuitEnv.getCellDefs().size() == 1);
    
    cout << "Testing linebuffer mode" << endl;

    Simulator sim(circuitEnv, def);
    reset("reset", sim);

    sim.setFreshValue("flush", BitVec(1, 0));
    sim.setFreshValue("clk_en", BitVec(1, 1));
    sim.setFreshValue("config_en", BitVec(1, 1));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.setFreshValue("config_en_sram", BitVec(4, 0));
    sim.update();

    BitVector configAddr(32, 0);
    sim.setFreshValue("config_addr", configAddr);
    sim.update();

    uint32_t configDataInt = 0;
    configDataInt |= ((uint32_t) 0) << 0; // Linebuffer mode
    configDataInt |= ((uint32_t) 1) << 2; // Tile enabled
    configDataInt |= ((uint32_t) 10) << 3; // Depth 10
    
    BitVector configData(32, configDataInt);

    cout << "Memory tile config data = " << configData << endl;
    sim.setFreshValue("config_data", configData);

    posedge("clk_in", sim);

    BitVector lbIn(16, 0);
    int dataCycles = 99;
    for (int i = 0; i < dataCycles; i++) {
      lbIn = BitVector(16, i);
      cout << "Cycle " << i << " in linebuffer mode" << endl;
      sim.setFreshValue("wen_in", BitVec(1, 1));
      sim.setFreshValue("data_in", lbIn);
      posedge("clk_in", sim);

      cout << sim.getBitVec("almost_empty") << endl;
      cout << sim.getBitVec("almost_full") << endl;
      cout << sim.getBitVec("chain_out") << endl;
      cout << sim.getBitVec("chain_valid_out") << endl;
      cout << "data_out = " << sim.getBitVec("data_out") << endl;
      cout << "read_data = " << sim.getBitVec("read_data") << endl;
      cout << "read_data_linebuf = " << sim.getBitVec("read_data_linebuf") << endl;
      cout << "read_data_sram = " << sim.getBitVec("read_data_sram") << endl;
      cout << "valid_out = " << sim.getBitVec("valid_out") << endl;
    }

    for (int i = 0; i < dataCycles; i++) {
      cout << "Read cycle " << i << " in linebuffer mode" << endl;

      sim.setFreshValue("wen_in", BitVec(1, 0));
      posedge("clk_in", sim);

      cout << sim.getBitVec("almost_empty") << endl;
      cout << sim.getBitVec("almost_full") << endl;
      cout << sim.getBitVec("chain_out") << endl;
      cout << sim.getBitVec("chain_valid_out") << endl;
      cout << "data_out = " << sim.getBitVec("data_out") << endl;
      cout << "read_data = " << sim.getBitVec("read_data") << endl;
      cout << "read_data_linebuf = " << sim.getBitVec("read_data_linebuf") << endl;
      cout << "read_data_sram = " << sim.getBitVec("read_data_sram") << endl;
      cout << "valid_out = " << sim.getBitVec("valid_out") << endl;
    }
    
    //sim.debugPrintMemories();

    sim.setFreshValue("wen_in", BitVec(1, 1));
    sim.setFreshValue("addr_in", BitVec(16, 2));
    sim.setFreshValue("data_in", BitVec(16, 45));
    posedge("clk_in", sim);
    
    sim.setFreshValue("wen_in", BitVec(1, 0));
    sim.setFreshValue("addr_in", BitVec(16, 4));
    sim.setFreshValue("ren_in", BitVec(1, 1));
    posedge("clk_in", sim);

    posedge("clk_in", sim);
    posedge("clk_in", sim);

    // sim.debugPrintMemories();

    //REQUIRE(sim.getBitVec("data_out") == BitVector(16, 72));
      
      
  }
    
  TEST_CASE("Simulating a mux loop") {

    Context* c = newContext();
    Namespace* g = c->getGlobal();

    uint width = 2;

    Type* twoMuxType =
      c->Record({
          {"in", c->BitIn()->Arr(width)},
            {"sel", c->BitIn()},
              {"out", c->Bit()->Arr(width)}
        });

    Module* twoMux = c->getGlobal()->newModuleDecl("twoMux", twoMuxType);
    ModuleDef* def = twoMux->newModuleDef();

    def->addInstance("mux0",
                     "coreir.mux",
                     {{"width", Const::make(c, width)}});

    def->connect("self.sel", "mux0.sel");
    def->connect("self.in", "mux0.in0");
    def->connect("mux0.out", "mux0.in1");
    def->connect("mux0.out", "self.out");

    twoMux->setDef(def);

    cout << "Creating twoMux simulation" << endl;

    c->runPasses({"rungenerators", "split-inouts","delete-unused-inouts","deletedeadinstances","add-dummy-inputs", "packconnections", "removeconstduplicates", "flatten", "cullzexts", "removeconstduplicates"});

    Env circuitEnv = convertFromCoreIR(c, twoMux);

    REQUIRE(circuitEnv.getCellDefs().size() == 1);

    CellDefinition& cDef = circuitEnv.getDef(twoMux->getName());
    
    Simulator state(circuitEnv, cDef);

    state.setFreshValue("sel", BitVector(1, 0));
    state.setFreshValue("in", BitVector(width, "11"));
    state.update();

    REQUIRE(state.getBitVec("out") == BitVector(width, "11"));
      
    deleteContext(c);
    
  }
  
  TEST_CASE("Loading Connect box") {
    Env circuitEnv = loadFromCoreIR("global.cb_unq1", "./test/cb_unq1.json");
    CellDefinition& def = circuitEnv.getDef("cb_unq1"); //top->getName());

    SECTION("Simulating circuit as created") {
      const Cell& clkPort = def.getPortCell("clk");

      REQUIRE(clkPort.getCellType() == CELL_TYPE_PORT);
      REQUIRE(clkPort.getParameterValue(PARAM_PORT_TYPE).to_type<int>() == PORT_CELL_FOR_INPUT);
      REQUIRE(clkPort.hasPort(PORT_ID_OUT));

      const Cell& resPort = def.getPortCell("out");

      REQUIRE(resPort.getCellType() == CELL_TYPE_PORT);
      REQUIRE(resPort.getParameterValue(PARAM_PORT_TYPE).to_type<int>() == PORT_CELL_FOR_OUTPUT);
      REQUIRE(resPort.hasPort(PORT_ID_IN));

      // Check connections

      bool allOutBitsDriven;
      int outWidth = resPort.getPortWidth(PORT_ID_IN);
    
      REQUIRE(outWidth == 16);

      const SignalBus& drivers = resPort.getDrivers(PORT_ID_IN);

      for (auto sigBit : drivers.signals) {
        cout << toString(sigBit) << endl;
      }

      assert(drivers.signals.size() == 16);
      for (int i = 0; i < outWidth; i++) {

        REQUIRE(notEmpty(drivers.signals[i]));

      }

      // Simulate the connect box
      Simulator sim(circuitEnv, def);
      sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 0));
      sim.update();
      sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 1));
      sim.update();
      sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 0));
      sim.update();

      sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 1));
      sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 3));
      sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));

      posedge("clk", sim);

      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
      sim.update();
    
      sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 0));
      sim.setFreshValue("in_3", PORT_ID_OUT, BitVec(16, 239));
      sim.update();

      REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 239));

      sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 1));
      sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 6));
      sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));

      posedge("clk", sim);

      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
      sim.update();
    
      sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 0));
      sim.setFreshValue("in_6", PORT_ID_OUT, BitVec(16, 7));
      sim.update();

      REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 7));

      sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 2));
      sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));
      sim.setFreshValue("in_2", PORT_ID_OUT, BitVec(16, 0));
      sim.setFreshValue("in_6", PORT_ID_OUT, BitVec(16, 9));

      posedge("clk", sim);

      REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 9));
    }

    SECTION("Evaluating after setting ports to constants") {

      Simulator sim(circuitEnv, def);
      sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 0));
      sim.update();
      sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 1));
      sim.update();
      sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 0));
      sim.update();

      sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 1));
      sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 3));
      sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));

      posedge("clk", sim);
      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
      sim.update();

      sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 0));
      sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 0));
      sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));
      sim.setFreshValue("in_0", PORT_ID_OUT, BitVec(16, 1));
      sim.setFreshValue("in_1", PORT_ID_OUT, BitVec(16, 1));
      sim.setFreshValue("in_2", PORT_ID_OUT, BitVec(16, 1));
      sim.setFreshValue("in_3", PORT_ID_OUT, BitVec(16, 1));
      sim.update();
      
      def.replacePortWithConstant("config_en", BitVec(1, 0));
      def.replacePortWithConstant("reset", BitVec(1, 0));
      def.replacePortWithConstant("config_addr", BitVec(32, 0));
      def.replacePortWithConstant("config_data", BitVec(32, 0));

      foldConstants(def, sim.allRegisterValues());
      deleteDeadInstances(def);

      REQUIRE(def.numCells() == 16);

      sim.refreshConstants();

      REQUIRE(definitionIsConsistent(def));

      cout << "Folded connect box def" << endl;
      for (auto cell : def.getCellMap()) {
        cout << "\t" << def.cellName(cell.first) << endl;
        if (cell.second.hasPort(PORT_ID_OUT)) {
          cout << "\t\tReceivers" << endl;
          for (auto sigBus : cell.second.getPortReceivers(PORT_ID_OUT)) {
            for (auto sigBit : sigBus) {
              cout << "\t\t" << toString(def, sigBit) << endl;
            }
          }
        }
      }

      cout << "Done printing def" << endl;

      sim.setFreshValue("clk", BitVec(1, 0));
      sim.setFreshValue("in_3", PORT_ID_OUT, BitVec(16, 9));
      sim.update();

      REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 9));

      sim.setFreshValue("in_3", PORT_ID_OUT, BitVec(16, 12));
      sim.update();

      REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 12));
    }
    
  }

  TEST_CASE("CGRA PE tile") {
    Env circuitEnv =
      loadFromCoreIR("global.pe_tile_new_unq1",
                     "./test/pe_tile_new_unq1.json");

    CellDefinition& def = circuitEnv.getDef("pe_tile_new_unq1");
    
    REQUIRE(circuitEnv.getCellDefs().size() == 1);

    auto configValues = loadBitStream("./test/hwmaster_pw2_sixteen.bsa");

    // NOTE: Unknown value on cg_en causes problems?
    Simulator sim(circuitEnv, def);

    sim.setFreshValue("tile_id", BitVector("16'h15"));

    sim.setFreshValue("in_BUS1_S1_T0", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T1", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T2", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T3", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T4", BitVector("1'h1"));

    cout << "Set tile_id" << endl;

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
    
    cout << "Outputs" << endl;
    for (int s = 0; s < 4; s++) {
      for (int t = 0; t < 5; t++) {
        cout << sim.getBitVec("out_BUS16_S" + to_string(s) + "_T" + to_string(t))
             << endl;
      }
    }

    REQUIRE(sim.getBitVec("out_BUS16_S0_T0", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T1", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T2", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) == BitVec(16, top_val*2));

    sim.specializePort("reset", BitVec(1, 0));
    sim.specializePort("config_addr", BitVec(32, 0));
    sim.specializePort("config_data", BitVec(32, 0));
    sim.specializePort("tile_id", BitVec("16'h15"));

    for (int s = 0; s < 4; s++) {
      for (int t = 0; t < 5; t++) {

        if ((s != 2) && (t != 0)) {
          string name16 = "in_BUS16_S" + to_string(s) + "_T" + to_string(t);
          sim.specializePort(name16, BitVec(16, 0));
        }

        string name1 = "in_BUS1_S" + to_string(s) + "_T" + to_string(t);
        sim.specializePort(name1, BitVec(1, 0));
        
      }
    }

    cout << "# of cells before constant folding = " << def.numCells() << endl;
    
    foldConstants(def, sim.allRegisterValues());
    deleteDeadInstances(def);

    cout << "# of cells after constant deleting instances = " << def.numCells() << endl;

    sim.refreshConstants();

    cout << "# of cells after constant folding = " << def.numCells() << endl;
    for (auto& ctp : def.getCellMap()) {
      cout << "\t" << def.cellName(ctp.first) << endl;
    }

    REQUIRE(sim.getBitVec("out_BUS16_S0_T0", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T1", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T2", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) == BitVec(16, top_val*2));

    cout << "out_BUS16_S0_T0 = " << sim.getBitVec("out_BUS16_S0_T0") << endl;;

    int topVal = 954;
    sim.setFreshValue("in_BUS16_S2_T0", BitVec(16, topVal));
    sim.update();

    cout << "out_BUS16_S0_T0 = " << sim.getBitVec("out_BUS16_S0_T0") << endl;;

    REQUIRE(sim.getBitVec("out_BUS16_S0_T0", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T1", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T2", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) == BitVec(16, topVal*2));

    // Compiling specialized code
    REQUIRE(sim.compileCircuit());

    REQUIRE(sim.hasSimulateFunction());

    cout << "Done compiling" << endl;

    topVal = 56;
    sim.setFreshValue("in_BUS16_S2_T0", BitVector(16, topVal));
    sim.update();

    REQUIRE(sim.getBitVec("out_BUS16_S0_T0", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T1", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T2", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) == BitVec(16, topVal*2));

    outputVerilog(sim.def, "mul_2_pe.v");

    cout << "Simulating raw" << endl;

    sim.simulateRaw();

    int nCycles = 10000;
    cout << "Running pe tile x2 raw for " << nCycles << endl;

    auto start = high_resolution_clock::now();

    BitVector input = BitVector(16, 0);
    for (int i = 0; i < nCycles; i++) {
      sim.update();

      input = BitVector(16, i);
      sim.setFreshValue("in_BUS16_S2_T0", input);

      sim.update();
    }

    cout << sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) << endl;
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Time taken for " << nCycles << ": "
         << duration.count() << " milliseconds" << endl;
    
    topVal = 239;
    sim.setFreshValue("in_BUS16_S2_T0", BitVector(16, topVal));
    sim.update();

    REQUIRE(sim.getBitVec("out_BUS16_S0_T0", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T1", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T2", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) == BitVec(16, topVal*2));

  }

  TEST_CASE("Memory tile convolution") {
    auto convConfigValues = loadBitStream("./test/conv_2_1_only_config_lines.bsa");
    Env circuitEnv =
      loadFromCoreIR("global.memory_tile_unq1",
                     "./test/memory_tile_unq1.json");

    CellDefinition& def = circuitEnv.getDef("memory_tile_unq1");

    Simulator sim(circuitEnv, def);

    // One of these will be the write enable!
    for (int s = 0; s < 4; s++) {
      for (int t = 0; t < 5; t++) {
        if (s != 1) {
          sim.setFreshValue("in_0_BUS1_" + to_string(s) + "_" + to_string(t),
                            BitVector(1, 1));

          sim.setFreshValue("in_0_BUS16_" + to_string(s) + "_" + to_string(t),
                            BitVector(16, 0));

        }

        if (s != 3) {
          sim.setFreshValue("in_1_BUS1_" + to_string(s) + "_" + to_string(t),
                            BitVector(1, 1));

          sim.setFreshValue("in_1_BUS16_" + to_string(s) + "_" + to_string(t),
                            BitVector(16, 0));
          
        }
      }
    }

    loadMemoryTileConfig(convConfigValues, sim);
    

    sim.debugPrintRegisters();


    // One of these will be the write enable!
    for (int s = 0; s < 4; s++) {
      for (int t = 0; t < 5; t++) {
        if (s != 1) {
          sim.setFreshValue("in_0_BUS1_" + to_string(s) + "_" + to_string(t),
                            BitVector(1, 1));
        }

        if (s != 3) {
          sim.setFreshValue("in_1_BUS1_" + to_string(s) + "_" + to_string(t),
                            BitVector(1, 1));

        }
      }
    }

    sim.setFreshValue("gin_0", BitVec(1, 0));
    sim.setFreshValue("gin_1", BitVec(1, 0));
    sim.setFreshValue("gin_2", BitVec(1, 0));

    // This is the clock gate signal!
    sim.setFreshValue("gin_3", BitVec(1, 0));
    
    posedge("clk_in", sim);

    
    for (int i = 0; i < 50; i++) {
      BitVector input(16, i);
      for (int s = 0; s < 4; s++) {
        for (int t = 0; t < 5; t++) {
          if (s != 1) {
            sim.setFreshValue("in_0_BUS16_" + to_string(s) + "_" + to_string(t),
                              input);
          }

          if (s != 3) {
            sim.setFreshValue("in_1_BUS16_" + to_string(s) + "_" + to_string(t),
                              input);
          }
          
        }
      }

      posedge("clk_in", sim);

      //sim.debugPrintMemories();
    }

    sim.debugPrintPorts();

    cout << "Cell 1266 = " << sim.def.getCellName(1266) << endl;
    allInputsToConstants(sim.def);
    
    foldConstants(def, sim.allRegisterValues());
    deleteDeadInstances(def);

    cout << "Refreshing constants" << endl;

    sim.refreshConstants();

    cout << "Compiling circuit" << endl;

    sim.compileCircuit();

    cout << "Constant offsets" << endl;
    for (auto ctp : sim.def.getCellMap()) {
      CellId cid = ctp.first;

      const Cell& cell = sim.def.getCellRefConst(cid);
      if (cell.getCellType() == CELL_TYPE_CONST) {
        cout << "\tConstant " << sim.def.getCellName(cid) << " offset = "
             << sim.portValueOffset(cid, PORT_ID_OUT) << endl;
      }
    }

    int cellsInFifoController = 0;
    for (auto ctp : sim.def.getCellMap()) {
      CellId cid = ctp.first;
      string name = sim.def.getCellName(cid);

      if (matchesPrefix(name, "memory_core$fifo_control")) {
        cellsInFifoController++;
      }
    }

    cout << "# of cells from the fifo controller = "
         << cellsInFifoController << endl;

    REQUIRE(sim.def.numCells() < (int) 2*sim.def.getPortCells().size());

  }

  TEST_CASE("Pre specialized CGRA multiply by 2") {
    Env circuitEnv =
      loadFromCoreIR("global.flat_module",
                     "./mul_2_cgra.json");

    CellDefinition& def = circuitEnv.getDef("flat_module");
    Simulator sim(circuitEnv, def);

    BitVector input(16, 5485);
    setCGRAInput(2, input, sim);
    sim.update();

    REQUIRE(mul_general_width_bv(BitVector(16, 2), input) == getCGRAOutput(0, sim));

    sim.compileCircuit();

    input = BitVector(16, 23);
    setCGRAInput(2, input, sim);
    sim.update();

    REQUIRE(mul_general_width_bv(BitVector(16, 2), input) == getCGRAOutput(0, sim));

    sim.simulateRaw();

    input = BitVector(16, 876);
    setCGRAInput(2, input, sim);
    sim.update();

    REQUIRE(mul_general_width_bv(BitVector(16, 2), input) == getCGRAOutput(0, sim));

    int nCycles = 1000000;
    cout << "Running CGRA x2 raw for " << nCycles << endl;

    auto start = high_resolution_clock::now();

    input = BitVector(16, 0);
    int lastVal = 0;

    CellId pad_S2_T0 = def.getPortCellId("pad_S2_T0_in");
    CellId pad_S2_T1 = def.getPortCellId("pad_S2_T1_in");
    CellId pad_S2_T2 = def.getPortCellId("pad_S2_T2_in");
    CellId pad_S2_T3 = def.getPortCellId("pad_S2_T3_in");
    CellId pad_S2_T4 = def.getPortCellId("pad_S2_T4_in");
    CellId pad_S2_T5 = def.getPortCellId("pad_S2_T5_in");
    CellId pad_S2_T6 = def.getPortCellId("pad_S2_T6_in");
    CellId pad_S2_T7 = def.getPortCellId("pad_S2_T7_in");
    CellId pad_S2_T8 = def.getPortCellId("pad_S2_T8_in");
    CellId pad_S2_T9 = def.getPortCellId("pad_S2_T9_in");
    CellId pad_S2_T10 = def.getPortCellId("pad_S2_T10_in");
    CellId pad_S2_T11 = def.getPortCellId("pad_S2_T11_in");
    CellId pad_S2_T12 = def.getPortCellId("pad_S2_T12_in");
    CellId pad_S2_T13 = def.getPortCellId("pad_S2_T13_in");
    CellId pad_S2_T14 = def.getPortCellId("pad_S2_T14_in");
    CellId pad_S2_T15 = def.getPortCellId("pad_S2_T15_in");

    CellId clkIn = def.getPortCellId("clk_in");

    for (int i = 0; i < nCycles; i++) {

      //sim.setFreshValueTwoState("clk_in", 1, 0);
      sim.setFreshValueTwoState(clkIn, PORT_ID_OUT, 1, 0);
      sim.update();

      //input = BitVector(16, i);
      //setCGRAInputTwoState(2, 16, i, sim);
      //setCGRAInput(2, input, sim);

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

      //sim.setFreshValueTwoState("clk_in", 1, 0);
      sim.setFreshValueTwoState(clkIn, PORT_ID_OUT, 1, 1);
      sim.update();

      lastVal = i;
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Time taken for " << nCycles << ": "
         << duration.count() << " milliseconds" << endl;

    REQUIRE(mul_general_width_bv(BitVector(16, 2), BitVector(16, lastVal)) ==
            getCGRAOutput(0, sim));

    input = BitVector(16, 5923);
    setCGRAInput(2, input, sim);
    sim.update();
    
    REQUIRE(mul_general_width_bv(BitVector(16, 2), input) == getCGRAOutput(0, sim));
    
  }

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

    REQUIRE(getCGRAOutput(0, sim) == interpOutputS0);

    outputVerilog(sim.def, "conv_2_1_cgra.v");

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
