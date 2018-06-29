#include "analysis.h"
#include "simulator.h"
#include "transformations.h"

#include <fstream>

#include <dlfcn.h>
#include <unistd.h>

using namespace std;

namespace FlatCircuit {

  // Supports creating a unique collection of items in an
  // arbitrary order
  template<typename T>
  class UniqueVector {
    std::vector<T> vec;
    std::set<T> alreadyAdded;
  public:

    bool elem(const T& t) const {
      return dbhc::elem(t, alreadyAdded);
    }

    size_t size() const {
      assert(alreadyAdded.size() == vec.size());

      return vec.size();
    }

    void push_back(const T& t) {
      if (!dbhc::elem(t, alreadyAdded)) {
        vec.push_back(t);
        alreadyAdded.insert(t);
      } else {
        assert(false);
      }
    }

    std::vector<T> getVec() const {
      return vec;
    }
  };

  std::vector<SigPort> Simulator::getDataPorts(const CellId sp) {
    UniqueVector<SigPort> dataPorts;

    const Cell& c = def.getCellRefConst(sp);

    CellType tp = c.getCellType();
    if (isUnop(tp) || (tp == CELL_TYPE_REG) || (tp == CELL_TYPE_REG_ARST)) {

      return {{sp, PORT_ID_IN}};

    } else if ((c.getCellType() == CELL_TYPE_MUX) || (isBinop(c.getCellType()))) {

      return {{sp, PORT_ID_IN0}, {sp, PORT_ID_IN1}};

    } else if (tp == CELL_TYPE_PORT) {
      if (c.isInputPortCell()) {
        return {};
      } else {
        return {{sp, PORT_ID_IN}};
      }
    } else if (tp == CELL_TYPE_CONST) {
      return {};
    } else if (tp == CELL_TYPE_SLICE) {
      return {{sp, PORT_ID_IN}};
    } else {
      cout << "Unsupported sig port " << sigPortString(def, {sp, PORT_ID_OUT}) << endl;
      assert(false);
    }

    return dataPorts.getVec();
  }

  std::vector<SigPort> Simulator::dataSources(const SigPort sp) {
    UniqueVector<SigPort> srcPorts;

    cout << "Getting data sources for " << sigPortString(def, sp) << endl;

    const Cell& c = def.getCellRefConst(sp.cell);

    cout << "Got data sources for " << sigPortString(def, sp) << endl;

    CellType tp = c.getCellType();

    cout << "Got cell type" << endl;

    if (tp == CELL_TYPE_PORT) {
      if (!c.isInputPortCell()) {

        cout << "Is output cell port" << endl;

        for (auto sigBit : c.getDrivers(PORT_ID_IN).signals) {
          if (notEmpty(sigBit)) {
            SigPort dp = {sigBit.cell, sigBit.port};
            srcPorts.push_back(dp);
          }
        }

      } else {
        cout << "Is input cell port" << endl;
      }
    } else if (tp == CELL_TYPE_CONST) {
      return {};
    } else if (isUnop(c.getCellType()) ||
        (tp == CELL_TYPE_REG_ARST) ||
        (tp == CELL_TYPE_REG)) {

      assert(sp.port == PORT_ID_OUT);

      for (auto sigBit : c.getDrivers(PORT_ID_IN).signals) {
        if (notEmpty(sigBit)) {
          SigPort dp = {sigBit.cell, sigBit.port};
          srcPorts.push_back(dp);
        }
      }
    } else if ((c.getCellType() == CELL_TYPE_MUX) || (isBinop(c.getCellType()))) {

      assert(sp.port == PORT_ID_OUT);

      for (auto sigBit : c.getDrivers(PORT_ID_IN0).signals) {
        if (notEmpty(sigBit)) {
          SigPort dp = {sigBit.cell, sigBit.port};
          srcPorts.push_back(dp);
        }
      }

      for (auto sigBit : c.getDrivers(PORT_ID_IN1).signals) {
        if (notEmpty(sigBit)) {
          SigPort dp = {sigBit.cell, sigBit.port};
          srcPorts.push_back(dp);
        }
      }

    } else if (tp == CELL_TYPE_SLICE) {
      for (auto sigBit : c.getDrivers(PORT_ID_IN).signals) {
        if (notEmpty(sigBit)) {
          SigPort dp = {sigBit.cell, sigBit.port};
          srcPorts.push_back(dp);
        }
      }

    } else {
      cout << "Unsupported sig port in data sources" << sigPortString(def, sp) << endl;
      assert(false);
    }

    return srcPorts.getVec();
  }

  std::vector<SigPort> Simulator::traceValue(const CellId cellId,
                                             const PortId portId) {
    std::vector<SigPort> receiversToTrace{{cellId, portId}};
    std::vector<SigPort> trace;
    std::set<SigPort> alreadyAdded;

    while (receiversToTrace.size() > 0) {
      SigPort nextReceiver = receiversToTrace.back();
      receiversToTrace.pop_back();

      cout << "Tracing " << sigPortString(def, nextReceiver) << endl;

      CellId cid = nextReceiver.cell;
      PortId pid = nextReceiver.port;

      for (auto sigBit : def.getCellRefConst(cid).getDrivers(pid).signals) {

        SigPort next = {sigBit.cell, sigBit.port};
        if (!elem(next, alreadyAdded)) {
          trace.push_back(next);
          alreadyAdded.insert(next);

          vector<SigPort> drivers = dataSources(next);

          cout << "Adding data ports for drivers to receivers" << endl;
          for (auto d : drivers) {
            for (auto rPort : getDataPorts(d.cell)) {
              if (!elem(rPort, alreadyAdded)) {
                receiversToTrace.push_back(rPort);
              }
            }
          }
        }
      }

      cout << "Done tracing " << sigPortString(def, nextReceiver) << endl;
    }

    return trace;
  }

  std::set<CellId> levelZeroCells(const CellDefinition& def) {
    set<CellId> levelZero;
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.getCellType() == CELL_TYPE_PORT) {
        if (cell.isInputPortCell()) {
          levelZero.insert(cid);
        }
      } else if ((cell.getCellType() == CELL_TYPE_CONST) ||
                 isRegister(cell.getCellType())) {
        levelZero.insert(cid);
      }
    }

    return levelZero;
  }

  void updateDependencies(const Cell& cell,
                          const PortId pid,
                          set<SigPort>& freshChanges,
                          std::vector<SigPort>& combChanges,
                          std::set<SigPort>& seqChanges,
                          const CellDefinition& def) {
    for (auto sigPort : combinationalDependencies(cell, pid, def)) {
      freshChanges.insert(sigPort);
      combChanges.push_back(sigPort);
    }

    for (auto sigPort : sequentialDependencies(cell, pid)) {
      seqChanges.insert(sigPort);
    }
    
  }
  
  std::vector<std::vector<SigPort> >
  staticSimulationEvents(const CellDefinition& def) {

    vector<vector<SigPort> > staticEvents;
    std::vector<SigPort> combChanges;
    set<SigPort> seqChanges;

    set<SigPort> freshChanges;

    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);

      CellType tp = cell.getCellType();
      if ((tp == CELL_TYPE_REG) ||
          (tp == CELL_TYPE_CONST) ||
          (tp == CELL_TYPE_REG_ARST) ||
          cell.isInputPortCell()) {

        updateDependencies(cell, PORT_ID_OUT, freshChanges,
                           combChanges, seqChanges, def);

      } else if (tp == CELL_TYPE_MEM) {

        updateDependencies(cell, PORT_ID_RDATA,
                           freshChanges, combChanges, seqChanges,
                           def);
        
      }
    }

    cout << "Starting to collect events" << endl;
    do {

      cout << "In getting comb changes" << endl;
      while (freshChanges.size() > 0) {
        SigPort sigPort = *std::begin(freshChanges);
        freshChanges.erase(sigPort);

        const Cell& c = def.getCellRefConst(sigPort.cell);

        //cout << "\tUpdating cell " << sigPortString(def, sigPort) << endl;

        for (auto outPort : c.outputPorts()) {
          updateDependencies(c, outPort, freshChanges, combChanges, seqChanges, def);
        }
        
      }

      cout << "In got " << combChanges.size() << " comb changes" << endl;

      staticEvents.push_back(combChanges);
      combChanges = {};

      vector<SigPort> seqChangesVec(begin(seqChanges), end(seqChanges));
      staticEvents.push_back(seqChangesVec);

      for (auto seqPort : seqChanges) {
        const Cell& cell = def.getCellRefConst(seqPort.cell);

        if (cell.getCellType() == CELL_TYPE_MEM) {
          combChanges.push_back({seqPort.cell, PORT_ID_RADDR});
          freshChanges.insert({seqPort.cell, PORT_ID_RADDR});
        } else if (isRegister(cell.getCellType())) {
          combChanges.push_back({seqPort.cell, PORT_ID_OUT});
          freshChanges.insert({seqPort.cell, PORT_ID_OUT});
        }

        for (auto outPort : def.getCellRefConst(seqPort.cell).outputPorts()) {
          updateDependencies(def.getCellRefConst(seqPort.cell), outPort,
                             freshChanges, combChanges, seqChanges, def);
        }

      }

      // NOTE: This needs to change in order for simulation semantics to be correct
      seqChanges = {};
    } while (combChanges.size() > 0);

    cout << "Done collecting events" << endl;

    return staticEvents;
  }

  maybe<std::vector<CellId> >
  levelizeCircuit(const CellDefinition& def) {
    UniqueVector<CellId> levelized;
    for (auto cid : levelZeroCells(def)) {
      levelized.push_back(cid);
    }

    bool addedNew = true;
    while (addedNew) {
      addedNew = false;
      for (auto ctp : def.getCellMap()) {
        CellId cid = ctp.first;
        if (levelized.elem(cid)) {
          continue;
        }

        const Cell& cell = def.getCellRefConst(cid);

        bool allDriversInEarlierLevels = true;
        for (auto port : cell.inputPorts()) {
          for (auto driverBit : cell.getDrivers(port).signals) {
            if (notEmpty(driverBit) && !levelized.elem(driverBit.cell)) {
              allDriversInEarlierLevels = false;
              break;
            }
          }
        }

        if (allDriversInEarlierLevels) {
          addedNew = true;
          levelized.push_back(cid);
          break;
        }
      }
    }

    return levelized.getVec();
  }

  struct DylibInfo {
    void* libHandle;
    void* simFuncHandle;
    //void* rawSimFuncHandle;
  };

  DylibInfo loadLibWithFunc(const std::string& targetBinary,
                            const std::string& symbolName) {
    
    void* myLibHandle = dlopen(targetBinary.c_str(), RTLD_NOW);

    if (myLibHandle == nullptr) {
      printf("dlsym failed: %s\n", dlerror());
      assert(false);
    }

    cout << "lib handle = " << myLibHandle << endl;

    void* myFuncFunV;
    // Must remove the first underscore from the symbol name to find it?
    myFuncFunV = dlsym(myLibHandle, symbolName.c_str()); //"_Z8simulatePN4bsim10quad_valueE");
    if (myFuncFunV == nullptr) {
      printf("dlsym failed: %s\n", dlerror());
      assert(false);
    } else {
      printf("FOUND\n");
    }

    return {myLibHandle, myFuncFunV};

    // void* rawSimFunc = dlsym(myLibHandle, "_Z18simulate_two_statePh");
    // if (rawSimFunc == nullptr) {
    //   printf("dlsym failed: %s\n", dlerror());
    //   assert(false);
    // } else {
    //   printf("FOUND\n");
    // }
    
    // return {myLibHandle, myFuncFunV, rawSimFunc};
  }

  std::string
  Simulator::sequentialBlockCode(const std::vector<SigPort>& levelized,
                                 CodeGenState& codeState) {
    for (auto sigPort : levelized) {
      CellId cid = sigPort.cell;
      const Cell& cell = def.getCellRefConst(cid);

      codeState.addComment(ln("// ----- Sequential update for cell " + def.cellName(cid)));


      CellType tp = cell.getCellType();
      if (tp == CELL_TYPE_REG_ARST) {

        string inVar = codeState.getVariableName(cid, PORT_ID_IN, valueStore);
        string clkVar = codeState.getVariableName(cid, PORT_ID_CLK, valueStore);
        string lastClkVar =
          codeState.getLastValueVariableName(cid, PORT_ID_CLK, valueStore);
        string rstVar = codeState.getVariableName(cid, PORT_ID_ARST, valueStore);
        string lastRstVar =
          codeState.getLastValueVariableName(cid, PORT_ID_ARST, valueStore);

        string lbl = codeState.getNewLabel("reg_clk");
        EdgeType edgeType =
          cell.clkPosedge() ? EDGE_TYPE_POSEDGE : EDGE_TYPE_NEGEDGE;
        codeState.addEdgeTestJNE(edgeType, lastClkVar, clkVar, lbl);
        codeState.addRegisterAssign(cid, inVar, valueStore);
        codeState.addLabel(lbl);

        BitVector init = cell.getParameterValue(PARAM_INIT_VALUE);

        string rlbl = codeState.getNewLabel("reg_arst");

        EdgeType rstEdge =
          cell.rstPosedge() ? EDGE_TYPE_POSEDGE : EDGE_TYPE_NEGEDGE;
        codeState.addEdgeTestJNE(rstEdge, lastRstVar, rstVar, rlbl);
        
        codeState.addInstruction(new IRRegisterReset(cid, init));
        codeState.addLabel(rlbl);
        
      } else if (tp == CELL_TYPE_REG) {


        string inVar = codeState.getVariableName(cid, PORT_ID_IN, valueStore);
        string clkVar = codeState.getVariableName(cid, PORT_ID_CLK, valueStore);
        string lastClkVar =
          codeState.getLastValueVariableName(cid, PORT_ID_CLK, valueStore);
        
        string lbl = codeState.getNewLabel("reg_arst");

        EdgeType edgeType =
          cell.clkPosedge() ? EDGE_TYPE_POSEDGE : EDGE_TYPE_NEGEDGE;
        codeState.addEdgeTestJNE(edgeType, lastClkVar, clkVar, lbl);
        codeState.addRegisterAssign(cid, inVar, valueStore);
        codeState.addLabel(lbl);

        
      } else if (tp == CELL_TYPE_MEM) {

        string clkVar = codeState.getVariableName(cid, PORT_ID_CLK, valueStore);
        string lastClkVar =
          codeState.getLastValueVariableName(cid, PORT_ID_CLK, valueStore);
        
        string waddrName = codeState.getVariableName(cid, PORT_ID_WADDR, valueStore);
        string wdataName = codeState.getVariableName(cid, PORT_ID_WDATA, valueStore);
        string wenName = codeState.getVariableName(cid, PORT_ID_WEN, valueStore);

        string lbl = codeState.getNewLabel("mem_clk");
        codeState.addMemoryTestJNE(wenName, lastClkVar, clkVar, lbl);
        codeState.addUpdateMemoryCode(cid, waddrName, wdataName, valueStore);
        codeState.addLabel(lbl);

      } else {
        cout << "Unsupported cell " << def.cellName(cid) << " in sequentialBlockCode" << endl;
        assert(false);
      }

    }


    return "";
  }

  std::string
  Simulator::sequentialBlockUpdates(const std::vector<SigPort>& levelized,
                                    CodeGenState& codeState) {
    for (auto sigPort : levelized) {
      CellId cid = sigPort.cell;
      const Cell& cell = def.getCellRefConst(cid);

      codeState.addComment(ln("// ----- Sequential update for cell " + def.cellName(cid)));


      CellType tp = cell.getCellType();
      if (tp == CELL_TYPE_REG_ARST) {

        string inVar = codeState.getVariableName(cid, PORT_ID_IN, valueStore);
        string clkVar = codeState.getVariableName(cid, PORT_ID_CLK, valueStore);
        string lastClkVar =
          codeState.getLastValueVariableName(cid, PORT_ID_CLK, valueStore);
        string rstVar = codeState.getVariableName(cid, PORT_ID_ARST, valueStore);
        string lastRstVar =
          codeState.getLastValueVariableName(cid, PORT_ID_ARST, valueStore);

        codeState.addRegisterAssign(cid, inVar, valueStore);

        BitVector init = cell.getParameterValue(PARAM_INIT_VALUE);

        
      } else if (tp == CELL_TYPE_REG) {


        string inVar = codeState.getVariableName(cid, PORT_ID_IN, valueStore);
        string clkVar = codeState.getVariableName(cid, PORT_ID_CLK, valueStore);
        string lastClkVar =
          codeState.getLastValueVariableName(cid, PORT_ID_CLK, valueStore);
        
        codeState.addRegisterAssign(cid, inVar, valueStore);

        
      } else if (tp == CELL_TYPE_MEM) {

        string clkVar = codeState.getVariableName(cid, PORT_ID_CLK, valueStore);
        string lastClkVar =
          codeState.getLastValueVariableName(cid, PORT_ID_CLK, valueStore);
        
        string waddrName = codeState.getVariableName(cid, PORT_ID_WADDR, valueStore);
        string wdataName = codeState.getVariableName(cid, PORT_ID_WDATA, valueStore);
        string wenName = codeState.getVariableName(cid, PORT_ID_WEN, valueStore);

        string lbl = codeState.getNewLabel("mem_en");
        codeState.addInstruction(new IRTestJNE(wenName, lbl));
        codeState.addUpdateMemoryCode(cid, waddrName, wdataName, valueStore);
        codeState.addLabel(lbl);
        
        ///codeState.addUpdateMemoryCode(cid, waddrName, wdataName, valueStore);

      } else {
        cout << "Unsupported cell " << def.cellName(cid) << " in sequentialBlockCode" << endl;
        assert(false);
      }

    }


    return "";
  }
  
  std::string
  Simulator::combinationalBlockCode(const std::vector<SigPort>& levelized,
                                    CodeGenState& codeState) {

    string cppCode = "";

    for (auto sigPort : levelized) {
      CellId cid = sigPort.cell;
      PortId port = sigPort.port;
      
      const Cell& cell = def.getCellRefConst(cid);
      codeState.addComment(ln("// ----- Code for cell " + def.cellName(cid) + ", " + portIdString(port)));

      bool sentToSeqPort = false;
      for (auto outPort : cell.outputPorts()) {
        if (sequentialDependencies(cell, outPort).size() > 0) {
          sentToSeqPort = true;
          break;
        }
      }

      string pastValueTmp = "";
      if (sentToSeqPort) {

        pastValueTmp = codeState.getPortTemp(cid, PORT_ID_OUT);

        codeState.addInstruction(new IRPortLoad(pastValueTmp, cid, PORT_ID_OUT, false));
        // codeState.addAssign(pastValueTmp,
        //                     "values[" +
        //                     to_string(valueStore.portValueOffset(cid, PORT_ID_OUT)) +
        //                     "]");
      }

      if ((cell.getCellType() == CELL_TYPE_PORT) && !cell.isInputPortCell()) {

        string argName = codeState.getVariableName(cid, PORT_ID_IN, valueStore);

        codeState.addAssign(cid, PORT_ID_IN, argName, valueStore);

      } else if (cell.isInputPortCell()) {
        codeState.addComment(ln("// No code for input port " + def.cellName(cid)));
      } else if (cell.getCellType() == CELL_TYPE_CONST) {
        codeState.addComment(ln("// No code for const port " + def.cellName(cid)));
      } else if (cell.getCellType() == CELL_TYPE_SUB) {
        std::string argName0 =
          codeState.getVariableName(cid, PORT_ID_IN0, valueStore);
        std::string argName1 =
          codeState.getVariableName(cid, PORT_ID_IN1, valueStore);

        std::string outName = codeState.getPortTemp(cid, PORT_ID_OUT);

        string resTemp = argName0 + "_" + argName1 + "_cpy_temp";

        int tempWidth = def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);
        codeState.addInstruction(new IRDeclareTemp(tempWidth, resTemp));
        
        codeState.addInstruction(new IRBinop(outName, argName0, argName1, def.getCellRefConst(cid), {resTemp}));
        codeState.addAssign(cid, PORT_ID_OUT, outName, valueStore);

      } else if (cell.getCellType() == CELL_TYPE_MUL) {
        std::string argName0 =
          codeState.getVariableName(cid, PORT_ID_IN0, valueStore);
        std::string argName1 =
          codeState.getVariableName(cid, PORT_ID_IN1, valueStore);

        std::string outName = codeState.getPortTemp(cid, PORT_ID_OUT);

        string resTemp = argName0 + "_" + argName1 + "_mul_res_temp";
        string shiftTemp = argName0 + "_" + argName1 + "_mul_shift_temp";
        string accumTemp = argName0 + "_" + argName1 + "_mul_accum_temp";

        int tempWidth = 2*def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT);
        codeState.addInstruction(new IRDeclareTemp(tempWidth, resTemp));
        codeState.addInstruction(new IRDeclareTemp(tempWidth, shiftTemp));
        codeState.addInstruction(new IRDeclareTemp(tempWidth, accumTemp));
        
        codeState.addInstruction(new IRBinop(outName, argName0, argName1, def.getCellRefConst(cid), {resTemp, shiftTemp, accumTemp}));
        codeState.addAssign(cid, PORT_ID_OUT, outName, valueStore);

      } else if (isBinop(cell.getCellType())) {
        binopCode(codeState, cid);
      } else if (isUnop(cell.getCellType())) {
        unopCode(codeState, cid);        
      } else if (cell.getCellType() == CELL_TYPE_MEM) {

        string raddrName = codeState.getVariableName(cid, PORT_ID_RADDR, valueStore);

        string memValue = codeState.getPortTemp(cid, PORT_ID_RDATA);
        codeState.addInstruction(new IRMemoryLoad(memValue, cid, raddrName));
        codeState.addAssign(cid, PORT_ID_RDATA, memValue, valueStore);
        
      } else if (cell.getCellType() == CELL_TYPE_MUX) {

        string argName0 = codeState.getVariableName(cid, PORT_ID_IN0, valueStore);
        string argName1 = codeState.getVariableName(cid, PORT_ID_IN1, valueStore);
        string sel = codeState.getVariableName(cid, PORT_ID_SEL, valueStore);

        string out = codeState.getPortTemp(cid, PORT_ID_OUT);
        codeState.addInstruction(new IRMux(out, sel, argName0, argName1, cid));

        codeState.addAssign(cid,
                            PORT_ID_OUT,
                            out,
                            valueStore);
        
      } else if (cell.getCellType() == CELL_TYPE_REG_ARST) {

        codeState.addInstruction(new IRRegisterStateCopy(cid));

      } else if (cell.getCellType() == CELL_TYPE_REG) {
        codeState.addInstruction(new IRRegisterStateCopy(cid));

      } else {
        cout << "Signal Port " << toString(def, {cid, port, 0}) << endl;
        cout << "Insert code for unsupported node " + def.cellName(cid)
             << " : " << toString(def.getCellRefConst(cid).getCellType()) << endl;
        assert(false);
      }

      if (sentToSeqPort) {

        codeState.addInstruction(new IRTableStore(cid, PORT_ID_OUT, pastValueTmp, true));
        //codeState.addAssign("values[" + to_string(valueStore.pastValueOffset(cid, PORT_ID_OUT)) + "]", pastValueTmp);

      }

    }

    return "";
  }

  void Simulator::sequentialPortUpdateCode(CodeGenState& codeState) {
    for (auto port : sequentialPorts) {
      CellId cid = port.cell;
      PortId pid = port.port;

      // Copy the current value of the variable to the past value
      string pastValueTmp = codeState.getPortTemp(cid, pid);
      codeState.addInstruction(new IRPortLoad(pastValueTmp, cid, pid, false));
      codeState.addInstruction(new IRTableStore(cid, pid, pastValueTmp, true));
    }
  }

  bool
  canCompressSequentialTests(const std::vector<std::vector<SigPort> >& updates,
                             const CellDefinition& def) {

    if ((updates.size() == 4) && (updates[3].size() == 0)) {
      cout << "Found example of possible clock compressible app" << endl;
      std::set<SigPort> clockDrivers;
      std::set<SigPort> rstDrivers;
      auto seqUpdates = updates[1];

      for (auto sigPort : seqUpdates) {
        CellId cid = sigPort.cell;
        const Cell& cell = def.getCellRefConst(cid);
        PortId pid = sigPort.port;

        assert((pid == PORT_ID_CLK) ||
               (pid == PORT_ID_ARST));
        assert(cell.getPortWidth(pid) == 1);

        auto drivers = cell.getDrivers(pid).signals;
        
        if (pid == PORT_ID_CLK) {
          for (auto driverBit : drivers) {
            assert(notEmpty(driverBit));
            clockDrivers.insert({driverBit.cell, driverBit.port});
          }
        } else {
          assert(pid == PORT_ID_ARST);

          for (auto driverBit : drivers) {
            assert(notEmpty(driverBit));
            rstDrivers.insert({driverBit.cell, driverBit.port});
          }
        }
      }

      cout << "Clock drivers" << endl;
      for (auto sps : clockDrivers) {
        cout << "\t" << sigPortString(def, sps) << endl;
      }

      cout << "Reset drivers" << endl;
      for (auto sps : rstDrivers) {
        cout << "\t" << sigPortString(def, sps) << endl;
      }
      
      bool oneClockDriver = false;
      
      if ((clockDrivers.size() == 1) &&
          def.isPortCell((*begin(clockDrivers)).cell)) {
        cout << "Uniform clock drivers " << endl;
        oneClockDriver = true;
      }

      bool allRstConst = true;
      for (auto driverPort : rstDrivers) {
        CellId cid = driverPort.cell;
        if (!(def.getCellRefConst(cid).getCellType() == CELL_TYPE_CONST)) {
          allRstConst = false;
        }
      }

      if (allRstConst) {
        cout << "All resets are constants" << endl;
      }

      if (allRstConst && oneClockDriver) {
        cout << "Can do sequential test folding" << endl;
      }

      return allRstConst && oneClockDriver;
    }

    return false;
  }

  CodeGenState
  Simulator::buildCodeGenState(const std::vector<std::vector<SigPort> >& updates) {
    // TODO: Check edge types on registers!!
    //bool canCompress = canCompressSequentialTests(updates, def);
    bool canCompress = false;
    CodeGenState codeState(def);

    if (!canCompress) {

      for (int i = 0; i < (int) updates.size(); i += 2) {
        combinationalBlockCode(updates[i + 0], codeState);
        sequentialBlockCode(updates[i + 1], codeState);
      }

    } else {
      // PROBLEM: Jeffs designs are not registered, need some updates

      string lbl = codeState.getNewLabel("clk_is_high");

      SigPort sp = *begin(updates[1]);
      CellId cid = sp.cell;

      cout << "Cellid = " << cid << endl;
      
      const Cell& cell = def.getCellRefConst(cid);

      cout << "SigPort = " << sigPortString(def, sp) << endl;

      string clkVar = codeState.getVariableName(cid, PORT_ID_CLK, valueStore);
      string lastClkVar =
        codeState.getLastValueVariableName(cid, PORT_ID_CLK, valueStore);

      cout << "Got clock names" << endl;

      EdgeType edgeType = EDGE_TYPE_POSEDGE;
      if (isRegister(cell.getCellType())) {
        edgeType =
          cell.clkPosedge() ? EDGE_TYPE_POSEDGE : EDGE_TYPE_NEGEDGE;
      }

      codeState.addEdgeTestJNE(edgeType, lastClkVar, clkVar, lbl);

      combinationalBlockCode(updates[0], codeState);
      sequentialBlockUpdates(updates[1], codeState);

      codeState.addLabel(lbl);

      combinationalBlockCode(updates[2], codeState);
    }

    sequentialPortUpdateCode(codeState);

    return codeState;
  }

  void
  Simulator::compileLevelizedCircuit(const std::vector<std::vector<SigPort> >& updates) {
    CodeGenState codeState = buildCodeGenState(updates);

    string cppCode = "#include <vector>\n#include \"static_bv_wrapper.h\"\n"
      "using namespace bsim;\n\n";
    
    valueStore.buildRawValueTable();
    
    cppCode += ln("// Input layout");
    for (auto cid : def.getPortCells()) {
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.isInputPortCell()) {

        cppCode += ln("// " + def.getCellName(cid) + " ---> " +
                      to_string(valueStore.portValueOffset(cid, PORT_ID_OUT)));

      } else if (def.isPortCell(cid)) {

        // Output cell
        cppCode += ln("// " + def.getCellName(cid) + " ---> " +
                      to_string(valueStore.portValueOffset(cid, PORT_ID_IN)));

      }
    }

    cppCode +=
      "static inline void storeRegisterState(bsim::quad_value* values, const unsigned long wireOffset, const unsigned long stateOffset, const unsigned long width) {\n"
      "\tfor (unsigned long i = 0; i < width; i++) {\n"
      "\t\tvalues[wireOffset + i] = values[stateOffset + i];\n"
      "\t}\n"
      "}\n\n"

      "void simulate(bsim::quad_value* values) {\n";
    
    assert((updates.size() % 2) == 0);

    cppCode += codeState.getCode(valueStore);

    cppCode += "}";

    string libName = "circuit_jit";
    string targetBinary = cppLibName(libName);
    string cppName = "./" + libName + ".cpp";
    ofstream out(cppName);
    out << cppCode << endl;
    out.close();

    compileCppLib(cppName, targetBinary);

    DylibInfo dlib = loadLibWithFunc(targetBinary,
                                     "_Z8simulatePN4bsim10quad_valueE");
    libHandle = dlib.libHandle;
    simulateFuncHandle = dlib.simFuncHandle;

    // Generate and load 2 state code
    cppCode = "#include <vector>\n#include \"static_quad_value_bit_vector.h\"\n"
      "using namespace bsim;\n\n";
    
    cppCode += ln("// Input layout");
    for (auto cid : def.getPortCells()) {
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.isInputPortCell()) {
        cppCode += ln("// RAW Offset: " + def.getCellName(cid) + " ---> " +
                      to_string(valueStore.rawPortValueOffset(cid, PORT_ID_OUT)));
        if (sequentialDependencies(cell, PORT_ID_OUT).size() > 0) {
          cppCode += ln("// RAW Offset past value: " + def.getCellName(cid) + " ---> " +
                        to_string(valueStore.rawPortPastValueOffset(cid, PORT_ID_OUT)));
        }
      } else if (def.isPortCell(cid)) {
        cppCode += ln("// RAW Offset: " + def.getCellName(cid) + " ---> " +
                      to_string(valueStore.rawPortValueOffset(cid, PORT_ID_IN)));
      }
    }

    cppCode +=
      "bool two_state_posedge(const uint8_t a, const uint8_t b) { return !a && b; }\n\n"
      "bool two_state_negedge(const uint8_t a, const uint8_t b) { return a && !b; }\n\n"
      "void simulate_two_state(unsigned char* values) {\n";
    cppCode += codeState.getTwoStateCode(valueStore);
    cppCode += "}\n\n";

    libName = "circuit_jit_raw";
    targetBinary = cppLibName(libName);
    cppName = "./" + libName + ".cpp";
    ofstream outRaw(cppName);
    outRaw << cppCode << endl;
    outRaw.close();

    compileCppLib(cppName, targetBinary);
    
    DylibInfo dlibRaw = loadLibWithFunc(targetBinary,
                                        "_Z18simulate_two_statePh");
    rawLibHandle = dlibRaw.libHandle;
    rawSimulateFuncHandle = dlibRaw.simFuncHandle;

  }

  // An update to a node is dead if:
  // 1. There is another update to the same node later
  // 2. Nothing the node drives is updated between the current and next update
  std::vector<std::vector<SigPort> >
  deleteDeadUpdates(const std::vector<std::vector<SigPort> >& filteredUpdates,
                    const CellDefinition& def) {
    return filteredUpdates;
  }

  bool Simulator::compileCircuit() {

    vector<vector<SigPort> > updates = staticSimulationEvents(def);

    vector<vector<SigPort> > filteredUpdates;
    for (auto updateGroup : updates) {
      vector<SigPort> filtered;

      auto reversedUpdateGroup = updateGroup;
      reverse(reversedUpdateGroup);

      vector<SigPort> filteredReversed;
      set<CellId> usedCells;
      for (auto sigPort : reversedUpdateGroup) {
        CellId cell = sigPort.cell;

        if (!elem(cell, usedCells)) {
          usedCells.insert(cell);
          filteredReversed.push_back(sigPort);
        }
      }

      reverse(filteredReversed);

      filteredUpdates.push_back(filteredReversed);
    }

    compileLevelizedCircuit(deleteDeadUpdates(filteredUpdates, def));

    return updates.size() > 1;
  }

  Simulator::~Simulator() {
    if (libHandle != nullptr) {
      dlclose(libHandle);
    }

    if (rawLibHandle != nullptr) {
      dlclose(rawLibHandle);
    }

  }

  void Simulator::debugPrintTableValues() const {
    valueStore.debugPrintTableValues();
  }

  void Simulator::debugPrintRawValueTable() const {
    valueStore.debugPrintRawValueTable();
  }
  
  void Simulator::debugPrintRegisters() const {
    cout << "Register values" << endl;
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (isRegister(cell.getCellType())) {
        cout << "\t" << def.getCellName(cid) << " state = " <<
          getRegisterValue(cid) << endl;
      }
    }

  }

  void Simulator::debugPrintPorts() const {
    cout << "Port values" << endl;
    for (auto cid : def.getPortCells()) {
      if (def.getCellRefConst(cid).isInputPortCell()) {
        cout << "\t" << def.getCellName(cid) << " = " << getBitVec(cid, PORT_ID_OUT) << endl;
      } else {
        cout << "\t" << def.getCellName(cid) << " = " << getBitVec(cid, PORT_ID_IN) << endl;
      }
    }

  }
  
  void Simulator::debugPrintMemories() const {
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.getCellType() == CELL_TYPE_MEM) {
        int depth = cell.getMemDepth();
        cout << "Memory " << def.getCellName(cid) << " values" << endl;
        cout << "\tRADDR = " << materializeInput({cid, PORT_ID_RADDR}) << endl;
        cout << "\tRDATA = " << getPortValue(cid, PORT_ID_RDATA) << endl;
        cout << "\tWADDR = " << materializeInput({cid, PORT_ID_WADDR}) << endl;
        cout << "\tWDATA = " << materializeInput({cid, PORT_ID_WDATA}) << endl;
        cout << "\tWEN   = " << materializeInput({cid, PORT_ID_WEN}) << endl;
        cout << "\tCLK   = " << materializeInput({cid, PORT_ID_CLK}) << endl;
        for (int i = 0; i < depth; i++) {
          cout << "\t" << i << " --> " << getMemoryValue(cid, i) << ", " << getMemoryValue(cid, i).to_type<int>() << endl;
        }
      }
    }
  }

  bool matchesPrefix(const std::string& str,
                     const std::string& prefix) {
    if (str.size() < prefix.size()) {
      return false;
    }

    if (str.substr(0, prefix.size()) == prefix) {
      return true;
    }

    return false;
  }
  
  bool matchesAnyPrefix(const std::string& str,
                        const std::vector<std::string>& prefixes) {
    for (auto prefix : prefixes) {
      if (matchesPrefix(str, prefix)) {
        return true;
      }
    }
    return false;
  }

  void
  Simulator::debugPrintMemories(const std::vector<std::string>& prefixes) const {
    for (auto ctp : def.getCellMap()) {
      CellId cid = ctp.first;
      const Cell& cell = def.getCellRefConst(cid);
      if (cell.getCellType() == CELL_TYPE_MEM) {

        if (matchesAnyPrefix(def.getCellName(cid), prefixes)) {
          int depth = cell.getMemDepth();
          cout << "Memory " << def.getCellName(cid) << " values" << endl;
          cout << "\tRADDR = " << materializeInput({cid, PORT_ID_RADDR}) << endl;
          cout << "\tRDATA = " << getPortValue(cid, PORT_ID_RDATA) << endl;
          cout << "\tWADDR = " << materializeInput({cid, PORT_ID_WADDR}) << endl;
          cout << "\tWDATA = " << materializeInput({cid, PORT_ID_WDATA}) << endl;
          cout << "\tWEN   = " << materializeInput({cid, PORT_ID_WEN}) << endl;
          cout << "\tCLK   = " << materializeInput({cid, PORT_ID_CLK}) << endl;
          for (int i = 0; i < depth; i++) {
            cout << "\t" << i << " --> " << getMemoryValue(cid, i) << ", " << getMemoryValue(cid, i).to_type<int>() << endl;
          }
        }
      }
    }
  }

  void Simulator::specializePort(const std::string& port, const BitVector& value) {
    CellId cid = def.getPortCellId(port);

    assert(!elem(cid, specializedPorts));

    def.replacePortWithConstant(port, value);
    specializedPorts.insert(cid);
  }

  void specializeCircuit(const std::vector<std::string>& constantPorts,
                         Simulator& sim) {
    for (auto portName : constantPorts) {
      CellId cid = sim.def.getPortCellId(portName);
      BitVector currentValue = sim.getBitVec(cid, PORT_ID_OUT);
      sim.def.replacePortWithConstant(portName, currentValue);
    }
    specializeCircuit(sim);
  }


  void specializeCircuit(Simulator& sim) {
    cout << "# of cells before constant folding = " << sim.def.numCells() << endl;
    foldConstants(sim.def, sim.allRegisterValues());
    cout << "# of cells after constant deleting instances = " <<
      sim.def.numCells() << endl;

    deleteDeadInstances(sim.def);

    cout << "# of cells after constant folding = " << sim.def.numCells() << endl;

    deDuplicate(sim.def);

    sim.refreshConstants();
  }

}
