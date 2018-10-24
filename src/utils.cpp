#include "utils.h"

using namespace std;

namespace FlatCircuit {

  void loadCGRAConfig(const CGRABitStream& configValues,
                      Simulator& sim) {
    reset("reset_in", sim);

    cout << "Reset chip" << endl;
    for (int i = 0; i < (int) configValues.size(); i++) {

      cout << "Evaluating " << i << endl;

      unsigned int configAddr = configValues[i].first;
      unsigned int configData = configValues[i].second;

      sim.setFreshValue("config_addr_in", BitVec(32, configAddr));
      sim.setFreshValue("config_data_in", BitVec(32, configData));

      posedge("clk_in", sim);
    }

    cout << "Done configuring CGRA tile" << endl;

    sim.setFreshValue("config_addr_in", BitVec(32, 0));
    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();
  }

  void loadMemoryTileConfig(const CGRABitStream& configValues,
                            Simulator& sim) {
    reset("reset", sim);

    sim.setFreshValue("config_write", BitVec(1, 1));
    sim.setFreshValue("tile_id", BitVec("16'h18"));
    posedge("clk_in", sim);
    
    cout << "Reset chip" << endl;
    for (int i = 0; i < (int) configValues.size(); i++) {

      cout << "Evaluating " << i << endl;

      unsigned int configAddr = configValues[i].first;
      unsigned int configData = configValues[i].second;

      sim.setFreshValue("config_addr", BitVec(32, configAddr));
      sim.setFreshValue("config_data", BitVec(32, configData));

      posedge("clk_in", sim);
    }

    cout << "Done configuring Memory tile" << endl;

    sim.setFreshValue("config_addr", BitVec(32, 0));
    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.setFreshValue("config_write", BitVec(1, 0));
    sim.update();

    
    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();

  }

  void reset(const std::string& rstName, Simulator& sim) {
    sim.setFreshValue(rstName, BitVec(1, 0));
    sim.update();
    sim.setFreshValue(rstName, BitVec(1, 1));
    sim.update();
    sim.setFreshValue(rstName, BitVec(1, 0));
    sim.update();
  }
  
  void negedge(const std::string& clkName, Simulator& sim) {
    sim.setFreshValue(clkName, BitVec(1, 1));
    sim.update();
    sim.setFreshValue(clkName, BitVec(1, 0));
    sim.update();
  }
  
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

  void setCGRAInput(const int side, const BitVector& input, Simulator& sim) {
    for (int track = 0; track < 16; track++) {
      string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
      auto bit = input.get(15 - track);
      if (bit.is_binary()) {
        sim.setFreshValue(inName, BitVec(1, bit.binary_value()));
      } else if (bit.is_unknown()) {
        sim.setFreshValue(inName, BitVec(1, "x"));
      } else {
        assert(false);
      }
    }
  }

  void setCGRAInputTwoState(const int side,
                            const int width,
                            const unsigned long value,
                            Simulator& sim) {

    for (int track = 0; track < 16; track++) {
      string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
      sim.setFreshValueTwoState(inName, 1, (value >> (15 - track)) & 0x1);
    }
    
  }
  
  void setCGRAInput(const BitVector& input, Simulator& sim) {
    for (int side = 0; side < 4; side++) {

      for (int track = 0; track < 16; track++) {
        string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
        sim.setFreshValue(inName, BitVec(1, input.get(15 - track).binary_value()));
      }
    }
  }

  void printCGRAInputs(Simulator& sim) {
    cout << "Inputs" << endl;
    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string inName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_in";
        cout << "\t" << inName << " = " << sim.getBitVec(inName, PORT_ID_OUT) << endl;
      }
    }
  }

  void printCGRAOutputs(Simulator& sim) {

    for (int side = 0; side < 4; side++) {
      cout << "Side " << side << endl;
      for (int track = 0; track < 16; track++) {
        string outName = "pad_S" + to_string(side) + "_T" + to_string(track) + "_out";
        cout << "\t" << outName << " = " << sim.getBitVec(outName) << endl;
      }
    }
  }

  BitVector getCGRAOutput(const int sideNo, Simulator& sim) {
    BitVector outputS0(16, 0);
    for (int i = 0; i < 16; i++) {
      outputS0.set(i, sim.getBitVec("pad_S" + to_string(sideNo) + "_T" + to_string(15 - i) + "_out").get(0));
    }
    return outputS0;
  }

  
}
