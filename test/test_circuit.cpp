#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include "convert_coreir.h"
#include "simulator.h"
#include "transformations.h"

#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace CoreIR;

namespace FlatCircuit {

  std::vector<std::string> splitString(const std::string& str,
                                       const std::string& delimiter) {
    string s = str;
    size_t pos = 0;
    vector<string> strs;
    while ((pos = s.find(delimiter)) != std::string::npos) {
      std::string token;
      token = s.substr(0, pos);
      strs.push_back(token);
      s.erase(0, pos + delimiter.length());
    }

    return strs;
  }

  TEST_CASE("Specialization of inputs") {
    Env e;
    CellType modType = e.addCellType("one_mux");
    CellDefinition def = e.getDef(modType);
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
      def.replacePortWithConstant("sel", BitVec(1, 1));

      foldConstants(def, {});
      deleteDeadInstances(def);

      REQUIRE(definitionIsConsistent(def));


      Simulator sim(e, def);

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
      def.replacePortWithConstant("sel", BitVec(1, 0));

      Simulator sim(e, def);
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

    // cout << "Memory contents" << endl;
    // for (auto mem : state.memories) {
    //   cout << "\tMemory for " << state.def.cellName(mem.first) << endl;
    //   for (auto mm : mem.second.mem) {
    //     cout << "\t\t" << mm << endl;
    //   }
    // }
    
    REQUIRE(state.getBitVec("read_data") == BitVec(width, 0));

    state.setFreshValue("clk", BitVec(1, 0));
    state.setFreshValue("write_en", BitVec(1, 1));
    state.update();

    state.setFreshValue("clk", BitVec(1, 1));
    state.update();

    // cout << "Memory contents" << endl;
    // for (auto mem : state.memories) {
    //   cout << "\tMemory for " << state.def.cellName(mem.first) << endl;
    //   for (auto mm : mem.second.mem) {
    //     cout << "\t\t" << mm << endl;
    //   }
    // }

    REQUIRE(state.getBitVec("read_data") == BitVec(width, 23));

    state.setFreshValue("read_addr", BitVec(index, 2));
    state.update();

    REQUIRE(state.getBitVec("read_data") == BitVec(width, 0));
    
    deleteContext(c);
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

      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
      sim.update();
    
      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 1));
      sim.update();

      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
      sim.update();
    
      sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 0));
      sim.setFreshValue("in_3", PORT_ID_OUT, BitVec(16, 239));
      sim.update();

      REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 239));

      sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 1));
      sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 6));
      sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));

      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
      sim.update();
    
      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 1));
      sim.update();

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
      sim.update();

      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
      sim.update();
    
      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 1));
      sim.update();

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

      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
      sim.update();
    
      sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 1));
      sim.update();

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

      // cout << "Port values" << endl;
      // for (auto pv : sim.portValues) {
      //   if (def.hasCell(pv.first.cell)) {
      //     cout << "\t" << sigPortString(def, pv.first) << " --> " <<
      //       pv.second << endl;
      //   }
      // }

      REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 9));

      sim.setFreshValue("in_3", PORT_ID_OUT, BitVec(16, 12));
      sim.update();

      REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 12));
    }
    
  }

  TEST_CASE("CGRA PE tile") {
    Context* c = newContext();
    Namespace* g = c->getGlobal();

    CoreIRLoadLibrary_rtlil(c);

    Module* top;
    if (!loadFromFile(c,"./test/pe_tile_new_unq1.json", &top)) {
      cout << "Could not Load from json!!" << endl;
      c->die();
    }

    top = c->getModule("global.pe_tile_new_unq1");

    assert(top != nullptr);

    c->runPasses({"rungenerators", "split-inouts","delete-unused-inouts","deletedeadinstances","add-dummy-inputs", "packconnections", "removeconstduplicates", "flatten", "cullzexts", "removeconstduplicates"});

    if (!saveToFile(g, "./test/flat_pe_tile_new_unq1.json")) {
      cout << "Could not Load from json!!" << endl;
      c->die();
    }

    
    Env circuitEnv = convertFromCoreIR(c, top);

    REQUIRE(circuitEnv.getCellDefs().size() == 1);

    CellDefinition& def = circuitEnv.getDef(top->getName());

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

    sim.setFreshValue("reset", BitVector("1'h0"));
    sim.update();
    sim.setFreshValue("reset", BitVector("1'h1"));
    sim.update();
    sim.setFreshValue("reset", BitVector("1'h0"));
    sim.update();

    cout << "Reset chip" << endl;
    for (int i = 0; i < configValues.size(); i++) {

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      cout << "Evaluating " << i << endl;

      unsigned int configAddr = configValues[i].first;
      unsigned int configData = configValues[i].second;

      sim.setFreshValue("config_addr", BitVec(32, configAddr));
      sim.setFreshValue("config_data", BitVec(32, configData));

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
      
    }

    cout << "Done configuring PE tile" << endl;

    sim.setFreshValue("config_addr", BitVec(32, 0));
    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();

    int top_val = 5;

    sim.setFreshValue("in_BUS16_S2_T0", BitVec(16, top_val));

    sim.setFreshValue("in_BUS16_S0_T0", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S0_T1", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S0_T2", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S0_T3", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S0_T4", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T0", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T1", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T2", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T3", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T4", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T0", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T1", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T2", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T3", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T4", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T0", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T1", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T2", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T3", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T4", BitVec(16, top_val));

    cout << "Done setting inputs" << endl;

    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();
    
    cout << "Outputs" << endl;
    cout << sim.getBitVec("out_BUS16_S0_T0") << endl;
    cout << sim.getBitVec("out_BUS16_S0_T1") << endl;
    cout << sim.getBitVec("out_BUS16_S0_T2") << endl;
    cout << sim.getBitVec("out_BUS16_S0_T3") << endl;
    cout << sim.getBitVec("out_BUS16_S0_T4") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T0") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T1") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T2") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T3") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T4") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T0") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T1") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T2") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T3") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T4") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T0") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T1") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T2") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T3") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T4") << endl;

    REQUIRE(sim.getBitVec("out_BUS16_S0_T0", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T1", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T2", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) == BitVec(16, top_val*2));

    sim.def.replacePortWithConstant("reset", BitVec(1, 0));
    sim.def.replacePortWithConstant("config_addr", BitVec(32, 0));
    sim.def.replacePortWithConstant("config_data", BitVec(32, 0));
    sim.def.replacePortWithConstant("tile_id", BitVec("16'h15"));

    for (int s = 0; s < 4; s++) {
      for (int t = 0; t < 5; t++) {

        if ((s != 2) && (t != 0)) {
          string name16 = "in_BUS16_S" + to_string(s) + "_T" + to_string(t);
          sim.def.replacePortWithConstant(name16, BitVec(16, 0));
        }

        string name1 = "in_BUS1_S" + to_string(s) + "_T" + to_string(t);
        sim.def.replacePortWithConstant(name1, BitVec(1, 0));
        
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

    topVal = 56;
    sim.setFreshValue("in_BUS16_S2_T0", BitVector(16, topVal));
    sim.update();

    REQUIRE(sim.getBitVec("out_BUS16_S0_T0", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T1", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T2", PORT_ID_IN) == BitVec(16, topVal*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) == BitVec(16, topVal*2));

    deleteContext(c);
  }

  TEST_CASE("CGRA multiply by 2") {
    auto configValues = loadBitStream("./test/pw2_16x16_only_config_lines.bsa");
    Env circuitEnv =
      loadFromCoreIR("global.top",
                     "/Users/dillon/CoreIRWorkspace/CGRA_coreir/top.json");

    CellDefinition& def = circuitEnv.getDef("top");

    //BitVector input("16'hf0ff");
    BitVector input(16, 23);
    BitVector correctOutput(16, 2*23);

    Simulator sim(circuitEnv, def);
    sim.setFreshValue("reset_in", BitVector("1'h0"));
    sim.update();
    sim.setFreshValue("reset_in", BitVector("1'h1"));
    sim.update();
    sim.setFreshValue("reset_in", BitVector("1'h0"));
    sim.update();

    cout << "Reset chip" << endl;
    for (int i = 0; i < configValues.size(); i++) {

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      cout << "Evaluating " << i << endl;

      unsigned int configAddr = configValues[i].first;
      unsigned int configData = configValues[i].second;

      sim.setFreshValue("config_addr_in", BitVec(32, configAddr));
      sim.setFreshValue("config_data_in", BitVec(32, configData));

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
      
    }

    cout << "Done configuring PE tile" << endl;

    sim.setFreshValue("config_addr_in", BitVec(32, 0));
    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();

    cout << "Done setting inputs" << endl;

    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();

    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
        sim.setFreshValue(inName, BitVec(1, input.get(15 - track).binary_value()));
      }
    }
    
    sim.update();

    cout << "Inputs" << endl;
    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
        cout << "\t" << inName << " = " << sim.getBitVec(inName, PORT_ID_OUT) << endl;
      }
    }

    int nCycles = 4;
    cout << "Computing " << nCycles << " cycles of data" << endl;
    for (int i = 0; i < nCycles; i++) {
      cout << "Cycle " << i << endl;

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
    }

    cout << "Outputs" << endl;

    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string outName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_out";
        cout << "\t" << outName << " = " << sim.getBitVec(outName) << endl;
      }
    }

    BitVector outputS0(16, 0);
    for (int i = 0; i < 16; i++) {
      outputS0.set(i, sim.getBitVec("pad_S0_T" + to_string(15 - i) + "_out").get(0));
    }

    cout << "outputS0 = " << outputS0 << endl;

    REQUIRE(outputS0 == correctOutput);

    sim.def.replacePortWithConstant("reset_in", BitVec(1, 0));
    sim.def.replacePortWithConstant("config_addr_in", BitVec(32, 0));
    sim.def.replacePortWithConstant("config_data_in", BitVec(32, 0));

    //sim.def.replacePortWithConstant("clk_in", BitVec(1, 0));

    sim.def.replacePortWithConstant("tck", BitVec(1, 0));
    sim.def.replacePortWithConstant("tdi", BitVec(1, 0));
    sim.def.replacePortWithConstant("tms", BitVec(1, 0));
    sim.def.replacePortWithConstant("trst_n", BitVec(1, 0));

    for (int side = 0; side < 4; side++) {

      if (side != 2) {
        for (int track = 0; track < 16; track++) {
          string outName =
            "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
          cout << "Setting " << outName << " to a constant" << endl;
          sim.def.replacePortWithConstant(outName, BitVec(1, 0));
        }
      }
    }
    
    cout << "# of cells before constant folding = " << def.numCells() << endl;
    
    foldConstants(def, sim.allRegisterValues());
    cout << "# of cells after constant deleting instances = " << def.numCells() << endl;

    set<CellId> memCells;
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      string name = def.cellName(cid);

      if (name.substr(0, 3) == "mem") {
        memCells.insert(cid);
      }
    }

    def.bulkDelete(memCells);

    deleteDeadInstances(def);

    cout << "# of cells after constant folding = " << def.numCells() << endl;

    sim.refreshConstants();

    cout << "Cell list" << endl;
    for (auto& ctp : def.getCellMap()) {
      cout << "\t" << def.cellName(ctp.first) << endl;
      const Cell& cell = def.getCellRefConst(ctp.first);
      for (auto portId : cell.outputPorts()) {
        cout << "\tRecievers for " << portIdString(portId) << endl;
        for (auto sigBus : cell.getPortReceivers(portId)) {
          for (auto sigBit : sigBus) {
            cout << "\t\t" << toString(def, sigBit) << endl;
          }
        }
      }
    }

    REQUIRE(definitionIsConsistent(def));

    input = BitVector(16, 18);

    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
        sim.setFreshValue(inName, BitVec(1, input.get(15 - track).binary_value()));
      }
    }
    
    sim.update();

    cout << "Inputs" << endl;
    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
        cout << "\t" << inName << " = " << sim.getBitVec(inName, PORT_ID_OUT) << endl;
      }
    }

    cout << "Outputs" << endl;

    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string outName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_out";
        cout << "\t" << outName << " = " << sim.getBitVec(outName) << endl;
      }
    }
    
    for (int i = 0; i < 16; i++) {
      outputS0.set(i, sim.getBitVec("pad_S0_T" + to_string(15 - i) + "_out").get(0));
    }
    
    cout << "outputS0 = " << outputS0 << endl;;

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
    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
        cout << "\t" << inName << " = " << sim.getBitVec(inName, PORT_ID_OUT) << endl;
      }
    }

    cout << "Outputs after compiling" << endl;

    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string outName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_out";
        cout << "\t" << outName << " = " << sim.getBitVec(outName) << endl;
      }
    }
    
    for (int i = 0; i < 16; i++) {
      outputS0.set(i, sim.getBitVec("pad_S0_T" + to_string(15 - i) + "_out").get(0));
    }
    
    cout << "outputS0 = " << outputS0 << endl;;

    REQUIRE(outputS0 == mul_general_width_bv(input, BitVec(16, 2)));

    nCycles = 10000;
    cout << "Running cgra for " << nCycles << endl;

    auto start = high_resolution_clock::now();
 
    for (int i = 0; i < nCycles; i++) {
      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
    }

   // Get ending timepoint
    auto stop = high_resolution_clock::now();
 
    // Get duration. Substart timepoints to 
    // get durarion. To cast it to proper unit
    // use duration cast method
    auto duration = duration_cast<microseconds>(stop - start);

    cout << "Time taken for " << nCycles << ": "
         << duration.count() << " microseconds" << endl;
    cout << "Done" << endl;
  }

}
