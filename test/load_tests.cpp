#include "catch.hpp"

#include "simulator.h"

using namespace CoreIR;

namespace FlatCircuit {

  void saveToFile(const CellDefinition& def, const std::string& fileName) {
  }

  void loadFromFile(const Env& e, const std::string& fileName) {
  }
  
  TEST_CASE("Storing and then loading a passthrough circuit") {
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

    saveToFile(def, "passthrough.csv");

    e.deleteCellType(modType);

    REQUIRE(!e.hasCellType(modType));

    loadFromFile(e, "passthrough.csv");

    REQUIRE(e.hasCellType(modType));
  }
  
}
