#include "catch.hpp"

#include "convert_coreir.h"
#include "serialize.h"
#include "simulator.h"
#include "utils.h"

#include <cstdio>
#include <fstream>

using namespace CoreIR;
using namespace std;

namespace FlatCircuit {

  TEST_CASE("Storing and loading circuits") {

    SECTION("Passthrough") {
      Env e;

      CellType modType = e.addCellType("passthrough");
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

      saveToFile(e, def, "passthrough.csv");

      e.deleteCellType(modType);

      REQUIRE(!e.hasCellType(modType));

      loadFromFile(e, "passthrough.csv");

      REQUIRE(e.hasCellType("passthrough"));

      CellDefinition& loadedDef = e.getDef(e.getCellType("passthrough"));

      REQUIRE(loadedDef.numCells() == 2);

      Simulator simLoaded(e, loadedDef);
      simLoaded.setFreshValue("in", BitVec(8, 156));
      simLoaded.update();

      REQUIRE(simLoaded.getBitVec("out") == BitVec(8, 156));
    }

    SECTION("Negation") {
      Env e;

      CellType modType = e.addCellType("negate");
      CellDefinition& def = e.getDef(modType);
      def.addPort("in", 16, PORT_TYPE_IN);
      def.addPort("out", 16, PORT_TYPE_OUT);

      CellId in = def.getPortCellId("in");
      CellId out = def.getPortCellId("out");

      CellId neg = def.addCell("neg",
                               CELL_TYPE_NOT,
                               {{PARAM_WIDTH, BitVector(32, 16)}});

      def.connect(in, PORT_ID_OUT,
                  neg, PORT_ID_IN);

      def.connect(neg, PORT_ID_OUT,
                  out, PORT_ID_IN);
      
      Simulator sim(e, def);
      sim.setFreshValue("in", BitVec(16, 0));
      sim.update();

      REQUIRE(sim.getBitVec("out") == ~BitVec(16, 0));

      saveToFile(e, def, "negate.csv");

      e.deleteCellType(modType);

      REQUIRE(!e.hasCellType(modType));

      loadFromFile(e, "negate.csv");

      REQUIRE(e.hasCellType("negate"));

      CellDefinition& loadedDef = e.getDef(e.getCellType("negate"));

      REQUIRE(loadedDef.numCells() == 3);

      Simulator simLoaded(e, loadedDef);
      simLoaded.setFreshValue("in", BitVec(16, 156));
      simLoaded.update();

      REQUIRE(simLoaded.getBitVec("out") == ~BitVec(16, 156));
    }

    SECTION("Connect box") {
      Env circuitEnv = loadFromCoreIR("global.cb_unq1", "./test/cb_unq1.json");
      CellDefinition& def = circuitEnv.getDef("cb_unq1");
      {
        Simulator sim(circuitEnv, def);
        reset("reset", sim);

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
      }



      cout << "Starting to save" << endl;

      saveToFile(circuitEnv, def, "connect_box.csv");

      cout << "Done saving" << endl;

      CellType cbTp = circuitEnv.getCellType("cb_unq1");
      circuitEnv.deleteCellType(cbTp);

      cout << "Done deleting" << endl;
      
      REQUIRE(!circuitEnv.hasCellType(cbTp));

      cout << "Loading" << endl;
      loadFromFile(circuitEnv, "connect_box.csv");
      cout << "Done loading" << endl;

      REQUIRE(circuitEnv.hasCellType("cb_unq1"));

      {
        CellDefinition& loadedDef =
          circuitEnv.getDef(circuitEnv.getCellType("cb_unq1"));
        Simulator sim(circuitEnv, loadedDef);
        reset("reset", sim);

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
      }

    }

    // // original size   : du -s 1588152	top.csv
    // // with bulk conns : du -s 856816	top.csv
    // SECTION("Top") {
    //   Env circuitEnv = loadFromCoreIR("global.top", "./test/top.json");
    //   CellDefinition& def = circuitEnv.getDef("top");

    //   cout << "Starting to save" << endl;

    //   saveToFile(circuitEnv, def, "top.csv");

    //   cout << "Done saving" << endl;

    //   CellType cbTp = circuitEnv.getCellType("top");
    //   circuitEnv.deleteCellType(cbTp);

    //   cout << "Done deleting" << endl;
      
    //   REQUIRE(!circuitEnv.hasCellType(cbTp));

    //   cout << "Loading" << endl;
    //   loadFromFile(circuitEnv, "top.csv");
    //   cout << "Done loading" << endl;

    //   REQUIRE(circuitEnv.hasCellType("top"));

    // }
    
  }
  
}
