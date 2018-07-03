#include "catch.hpp"

#include "simulator.h"
#include <fstream>

using namespace CoreIR;
using namespace std;

namespace FlatCircuit {

  std::vector<std::string> readCSVLine(std::istream& in) {
    std::string line;
    getline(in, line);

    cout << "Read line = " << line << endl;
    
    vector<string> tokens;
    string currentToken = "";
    int i = 0;
    while (i < (int) line.size()) {
      if (line[i] != ',') {
        currentToken += line[i];
      } else {
        tokens.push_back(currentToken);
        currentToken = "";
      }
      
      i++;
    }

    cout << "Parsed into tokens" << endl;
    for (auto tok : tokens) {
      cout << "\t" << tok << endl;
    }
    
    return tokens;
  }
  
  void writeCSVLine(const std::vector<std::string>& tokens,
                    std::ostream& out) {
    for (auto token : tokens) {
      out << token << ",";
    }

    out << std::endl;
  }

  void saveToFile(const Env& e,
                  const CellDefinition& def,
                  const std::string& fileName) {
    ofstream out(fileName);
    writeCSVLine({"NAME", e.getCellTypeName(def)}, out);
    writeCSVLine({"PORT_NAMES"}, out);
    for (auto pn : def.getPortNames()) {
      CellId cid = def.getPortCellId(pn);
      const Cell& pcell = def.getCellRefConst(cid);
      writeCSVLine({pn,
            pcell.getParameterValue(PARAM_OUT_WIDTH).binary_string(),
            to_string(bvToInt(pcell.getParameterValue(PARAM_PORT_TYPE)))},
        out);
    }
    writeCSVLine({"END"}, out);
    out.close();
  }

  void loadFromFile(Env& e,
                    const std::string& fileName) {
    ifstream in(fileName);

    // parse name
    vector<string> nameLine = readCSVLine(in);
    assert(nameLine.size() == 2);
    assert(nameLine[0] == "NAME");

    std::string name = nameLine[1];

    CellType modType = e.addCellType(name);
    //CellDefinition& def = e.getDef(modType);
    
    vector<string> endLine = readCSVLine(in);
    assert(endLine.size() == 1);
    assert(endLine[0] == "END");

    // parse end of file
    in.close();

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

    saveToFile(e, def, "passthrough.csv");

    e.deleteCellType(modType);

    REQUIRE(!e.hasCellType(modType));

    loadFromFile(e, "passthrough.csv");

    REQUIRE(e.hasCellType("passthrough"));

    Simulator simLoaded(e, e.getDef(e.getCellType("passthrough")));
    simLoaded.setFreshValue("in", BitVec(8, 156));
    simLoaded.update();

    REQUIRE(simLoaded.getBitVec("out") == BitVec(8, 156));

  }
  
}
