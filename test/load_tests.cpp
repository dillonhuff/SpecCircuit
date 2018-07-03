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

    writeCSVLine({"PORTS"}, out);
    for (auto pn : def.getPortNames()) {
      CellId cid = def.getPortCellId(pn);
      const Cell& pcell = def.getCellRefConst(cid);
      writeCSVLine({pn,
            to_string(pcell.getParameterValue(PARAM_OUT_WIDTH).to_type<int>()),
            to_string(bvToInt(pcell.getParameterValue(PARAM_PORT_TYPE)))},
        out);
    }

    writeCSVLine({"CELLS"}, out);
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      writeCSVLine({def.getCellName(cid), to_string(cell.getCellType())}, out);

      // Write ports
      vector<string> ports;
      for (auto pt : cell.getPorts()) {
        ports.push_back(to_string(pt.first));
        ports.push_back(to_string(pt.second.width));
        ports.push_back(to_string(pt.second.type));
      }
      writeCSVLine(ports, out);

      // Write parameters
      vector<string> parameters;
      for (auto pm : cell.getParameters()) {
        parameters.push_back(to_string(pm.first));
        parameters.push_back(pm.second.binary_string());
      }
      writeCSVLine(parameters, out);

      // Write drivers
      for (auto inPort : cell.inputPorts()) {
        auto drivers = cell.getDrivers(inPort);
        vector<string> driverLines;
        driverLines.push_back("D");
        driverLines.push_back(to_string(inPort));
        for (int i = 0; i < (int) drivers.size(); i++) {
          SignalBit dt = drivers.signals[i];
          string driverName = def.getCellName(dt.cell);
          driverLines.push_back(driverName);
          driverLines.push_back(to_string(dt.port));
          driverLines.push_back(to_string(dt.offset));
        }
        writeCSVLine(driverLines, out);
      }

      if (cell.inputPorts().size() == 0) {
        writeCSVLine({"NO_INPUTS"}, out);
      }
    }

    writeCSVLine({"END"}, out);
    out.close();
  }

  struct CellLines {
    vector<string> decl;
    vector<string> ports;
    vector<string> parameters;
    vector<string> drivers;
  };

  void loadFromFile(Env& e,
                    const std::string& fileName) {
    ifstream in(fileName);

    // parse name
    vector<string> nameLine = readCSVLine(in);
    assert(nameLine.size() == 2);
    assert(nameLine[0] == "NAME");

    std::string name = nameLine[1];

    CellType modType = e.addCellType(name);
    CellDefinition& def = e.getDef(modType);

    auto portDeclLine = readCSVLine(in);
    assert(portDeclLine.size() == 1);
    assert(portDeclLine[0] == "PORTS");

    vector<string> nextLine = readCSVLine(in);
    while (nextLine.size() == 3) {
      auto portName = nextLine[0];
      auto portWidthStr = nextLine[1];
      auto portTypeStr = nextLine[2];
      assert((portTypeStr == "0") || (portTypeStr == "1"));

      PortType portType = portTypeStr == "0" ? PORT_TYPE_IN : PORT_TYPE_OUT;
      int portWidth = stoi(portWidthStr);
      def.addPort(portName, portWidth, portType);
      nextLine = readCSVLine(in);
    }

    //cout << "nextline = " << nextLine[0] << endl;
    assert(nextLine.size() == 1);
    assert(nextLine[0] == "CELLS");

    vector<CellLines> cells;
    nextLine = readCSVLine(in);
    while (nextLine.size() == 2) {
      auto declLine = nextLine;
      string cellName = nextLine[0];
      CellType ctp = stoi(nextLine[1]);
      // Read ports
      auto portsLine = readCSVLine(in);

      // Read parameters
      auto paramsLine = readCSVLine(in);
      assert((paramsLine.size() % 2) == 0);

      // TODO: Add real parameter loading
      map<Parameter, BitVector> parameters;

      // Port cells have already been created
      if (ctp != CELL_TYPE_PORT) {
        assert(false);
        def.addCell(cellName, ctp, parameters);
      }


      // Read drivers
      auto driversLine = readCSVLine(in);

      cells.push_back({declLine, portsLine, paramsLine, driversLine});

      // Read next cell
      nextLine = readCSVLine(in);
    }

    for (auto cellLine : cells) {
      string cellName = cellLine.decl[0];
      CellId cid = def.getCellId(cellName);
      auto driversLine = cellLine.drivers;

      if (driversLine[0] != "NO_INPUTS") {
        int i = 0;
        while (i < driversLine.size()) {
          assert(driversLine[i] == "D");
          i++;

          PortId pid = stoi(driversLine[i]);
          i++;

          int receiverOffset = 0;
          while (i < driversLine.size() && (driversLine[i] != "D")) {
            string driverCellName = driversLine[i];
            cout << "Looking for name " << driverCellName << endl;
            CellId driverCellId = def.getCellId(driverCellName);
            PortId driverCellPort = stoi(driversLine[i + 1]);
            int driverCellOffset = stoi(driversLine[i + 2]);
            def.setDriver({cid, pid, receiverOffset},
                          {driverCellId, driverCellPort, driverCellOffset});
            i += 3;
            receiverOffset++;
          }

        }
      }

    }

    assert(nextLine.size() == 1);
    assert(nextLine[0] == "END");
    
    // parse end of file
    in.close();

  }

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

      saveToFile(e, def, "passthrough.csv");

      e.deleteCellType(modType);

      REQUIRE(!e.hasCellType(modType));

      loadFromFile(e, "passthrough.csv");

      REQUIRE(e.hasCellType("negate"));

      CellDefinition& loadedDef = e.getDef(e.getCellType("negate"));

      REQUIRE(loadedDef.numCells() == 2);

      Simulator simLoaded(e, loadedDef);
      simLoaded.setFreshValue("in", BitVec(16, 156));
      simLoaded.update();

      REQUIRE(simLoaded.getBitVec("out") == ~BitVec(16, 156));
    }
    
  }
  
}
