#include "serialize.h"

using namespace std;
using namespace CoreIR;

namespace FlatCircuit {

  Parameter intToParameter(const int i) {
    switch(i) {
    case 0:
      return PARAM_PORT_TYPE;
    case 1:
      return PARAM_WIDTH;
    case 2:
      return PARAM_HIGH;
    case 3:
      return PARAM_LOW;
    case 4:
      return PARAM_IN_WIDTH;
    case 5:
      return PARAM_IN0_WIDTH;
    case 6:
      return PARAM_IN1_WIDTH;
    case 7:
      return PARAM_OUT_WIDTH;
    case 8:
      return PARAM_SEL_WIDTH;
    case 9:
      return PARAM_CLK_POSEDGE;
    case 10:
      return PARAM_ARST_POSEDGE;
    case 11:
      return PARAM_INIT_VALUE;
    case 12:
      return PARAM_MEM_WIDTH;
    case 13:
      return PARAM_MEM_DEPTH;
    case 14:
      return PARAM_HAS_INIT;

    default:
      assert(false);
    }
  }

  std::vector<std::string> readCSVLine(FILE* in) { //std::istream& in) {
    //std::string line;

    char * linePtr = NULL;
    size_t len = 0;
    //ssize_t read;

    int res = getline(&linePtr, &len, in);
    assert(res != 0);

    std::string line(linePtr);
    //while ((read = getline(&line, &len, fp)) != -1) {

    //getline(in, line);

    //cout << "Read line = " << line << endl;


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

    // cout << "Parsed into tokens" << endl;
    // for (auto tok : tokens) {
    //   cout << "\t" << tok << endl;
    // }
    
    return tokens;
  }
  
  void writeCSVLine(const std::vector<std::string>& tokens,
                    FILE* out) {
    for (auto token : tokens) {
      fprintf(out, "%s,", token.c_str());
    }

    fprintf(out, "\n");

  }

  bool isBulkConnection(const CellId cid,
                        const PortId pid,
                        const CellDefinition& def) {
    const Cell& cell = def.getCellRefConst(cid);
    auto drivers = cell.getDrivers(pid);

    for (int i = 0; i < (int) drivers.size(); i++) {
      SignalBit driverBit = drivers.signals[i];
      if (driverBit.offset != i) {
        return false;
      }
    }

    CellId driverCell = drivers.signals[0].cell;
    PortId driverPort = drivers.signals[0].port;
    for (int i = 0; i < (int) drivers.size(); i++) {
      if (driverPort != drivers.signals[i].port) {
        return false;
      }

      if (driverCell != drivers.signals[i].cell) {
        return false;
      }

    }

    return true;
  }

  void saveToFile(const Env& e,
                  const CellDefinition& def,
                  const std::string& fileName) {
    //ofstream out(fileName);
    FILE* out = fopen(fileName.c_str(), "w");

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
      if (cell.getParameters().size() > 0) {
        vector<string> parameters;
        for (auto pm : cell.getParameters()) {
          parameters.push_back(to_string(pm.first));
          parameters.push_back(pm.second.binary_string());
        }
        writeCSVLine(parameters, out);
      } else {
        writeCSVLine({"NO_PARAMETERS"}, out);
      }

      // Write drivers
      vector<string> driverLines;      
      for (auto inPort : cell.inputPorts()) {
        auto drivers = cell.getDrivers(inPort);

        driverLines.push_back("D");
        driverLines.push_back(to_string(inPort));
        if (isBulkConnection(cid, inPort, def) && cell.getPortWidth(inPort) > 1) {
            SignalBit dt = drivers.signals[0];
            string driverName = def.getCellName(dt.cell);
            driverLines.push_back(driverName);
            driverLines.push_back(to_string(dt.port));
            driverLines.push_back("B");
        } else {
          for (int i = 0; i < (int) drivers.size(); i++) {
            SignalBit dt = drivers.signals[i];
            string driverName = def.getCellName(dt.cell);
            driverLines.push_back(driverName);
            driverLines.push_back(to_string(dt.port));
            driverLines.push_back(to_string(dt.offset));
          }
        }
      }

      if (driverLines.size() > 0) {
        writeCSVLine(driverLines, out);
      } else {
        writeCSVLine({"NO_INPUTS"}, out);
      }
    }

    writeCSVLine({"END"}, out);
    fclose(out);
  }

  struct CellLines {
    vector<string> decl;
    vector<string> ports;
    vector<string> parameters;
    vector<string> drivers;
  };

  void loadFromFile(Env& e,
                    const std::string& fileName) {
    //ifstream in(fileName);
    FILE* in = fopen(fileName.c_str(), "r");

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

      map<Parameter, BitVector> parameters;
      for (int i = 0; i < (int) paramsLine.size(); i += 2) {
        Parameter p = intToParameter(stoi(paramsLine[i]));
        //cout << "Creating bit vector from " << paramsLine[i + 1] << endl;
        BitVector value(paramsLine[i + 1].size(), paramsLine[i + 1]);
        parameters[p] = value;
      }

      // Port cells have already been created
      if (ctp != CELL_TYPE_PORT) {
        //cout << "adding cell " << cellName << endl;
        def.addCell(cellName, ctp, parameters);
      } else {
        //cout << "cell " << cellName << " is a port" << endl;
      }

      // Read drivers
      auto driversLine = readCSVLine(in);

      cells.push_back({declLine, portsLine, paramsLine, driversLine});
      // cout << "Parsed Cell" << endl;
      // cout << "\t";
      //writeCSVLine(declLine, cout);
      //cout << endl;
      // Read next cell
      nextLine = readCSVLine(in);
    }

    cout << "Nextline = " << nextLine[0] << endl;
    cout << "# of cells = " << cells.size() << endl;

    assert(nextLine.size() == 1);
    assert(nextLine[0] == "END");

    // Wire up the circuit
    for (auto cellLine : cells) {
      string cellName = cellLine.decl[0];
      CellId cid = def.getCellId(cellName);
      const Cell& cell = def.getCellRefConst(cid);
      auto driversLine = cellLine.drivers;

      if (driversLine[0] != "NO_INPUTS") {
        int i = 0;
        while (i < (int) driversLine.size()) {
          assert(driversLine[i] == "D");
          i++;

          PortId pid = stoi(driversLine[i]);
          i++;

          int receiverOffset = 0;
          while ((i < (int) driversLine.size()) && (driversLine[i] != "D")) {
            string driverCellName = driversLine[i];
            //cout << "Looking for name " << driverCellName << endl;
            CellId driverCellId = def.getCellId(driverCellName);
            PortId driverCellPort = stoi(driversLine[i + 1]);
            string driverOffsetVal = driversLine[i + 2];
            if (driverOffsetVal != "B") {
              int driverCellOffset = stoi(driversLine[i + 2]);
              def.setDriver({cid, pid, receiverOffset},
                            {driverCellId, driverCellPort, driverCellOffset});
            } else {
              assert(receiverOffset == 0);

              for (int k = 0; k < (int) cell.getPortWidth(pid); k++) {
                def.setDriver({cid, pid, k},
                              {driverCellId, driverCellPort, k});
              }
              i += 3;
              break;
            }
            i += 3;
            receiverOffset++;
          }

        }
      }

    }

    // parse end of file
    //in.close();
    fclose(in);

  }
  
}
