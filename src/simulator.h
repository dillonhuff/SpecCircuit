#pragma once

#include "ir.h"

namespace FlatCircuit {

  class CodeGenState {
    unsigned long long uniqueNum;
    std::vector<IRInstruction*> codeLines;
    CellDefinition& def;

  public:

    CodeGenState(CellDefinition& def_) : uniqueNum(0), def(def_) {}

    ~CodeGenState() {
      for (auto i : codeLines) {
        delete i;
      }
    }

    void addAssign(const CellId cid,
                   const PortId pid,
                   const std::string& assignCode,
                   ValueStore& valueStore) {
      codeLines.push_back(new IRTableStore(cid, pid, assignCode));
    }

    void addInstruction(IRInstruction* instr) {
      codeLines.push_back(instr);
    }
    
    void addBinop(const std::string& receiver,
                  const Cell& cell,
                  const std::string& arg0,
                  const std::string& arg1) {
      codeLines.push_back(new IRBinop(receiver, arg0, arg1, cell));
    }

    void addUnop(const std::string& receiver,
                 const Cell& cell,
                 const std::string& org) {
      codeLines.push_back(new IRUnop(receiver, org, cell));
    }
    
    void addLabel(const std::string& labelName) {
      codeLines.push_back(new IRLabel(labelName));
    }

    std::string getNewLabel(const std::string& prefix) {
      unsigned long long next = uniqueNum;
      uniqueNum++;
      return prefix + "_label_" + std::to_string(next);
    }
    
    std::string getPortTemp(const CellId cid, const PortId pid) {
      return getPortTemp(cid, pid, "");
    }

    std::string getPortTemp(const CellId cid,
                            const PortId pid,
                            const std::string& suffix) {
      std::string argName =
        "cell_" + std::to_string(cid) + "_" +
        portIdString(pid) + "_" +
        suffix + "_" +
        std::to_string(uniqueNum);

      uniqueNum++;

      addQVBVDecl(argName,
                  def.getCellRefConst(cid).getPortWidth(pid));
      
      return argName;
    }

    std::string getVariableName(const CellId cid,
                                const PortId pid,
                                ValueStore& store) {
      std::string argName = getPortTemp(cid, pid);
      for (auto instr : store.codeToMaterialize(cid, pid, argName)) {
        codeLines.push_back(instr);
      }

      return argName;
    }

    void addEdgeTestJNE(const EdgeType edgeType,
                        const std::string& prev,
                        const std::string& curr,
                        const std::string& label) {
      codeLines.push_back(new IREdgeTest(edgeType, prev, curr, label));
    }

    void addQVBVDecl(const std::string& name,
                     const int length) {
      codeLines.push_back(new IRDeclareTemp(length, name));
    }

    void addUpdateMemoryCode(const CellId cid,
                             const std::string& waddrName,
                             const std::string& wdataName,
                             ValueStore& valueStore) {

      codeLines.push_back(new IRMemoryStore(cid, waddrName, wdataName));
      
      // codeLines.push_back(new IRAssign("values[" +
      //                                  std::to_string(valueStore.getMemoryOffset(cid)) + " + " +
      //                                  "(" + waddrName + ".is_binary() ? " + waddrName +
      //                                  ".to_type<int>() : 0)]", wdataName));
    }

    void addMemoryTestJNE(const std::string& wenName,
                          const std::string& lastClkVar,
                          const std::string& clkVar,
                          const std::string& label) {
      codeLines.push_back(new IRMemoryTest(wenName, lastClkVar, clkVar, label));
    }
    
    void addRegisterAssign(const CellId cid,
                           const std::string& value,
                           ValueStore& valueStore) {
      codeLines.push_back(valueStore.codeToAssignRegister(cid, value));
    }
    
    std::string getLastValueVariableName(const CellId cid,
                                         const PortId pid,
                                         ValueStore& store) {
      std::string argName = getPortTemp(cid, pid, "last");
      for (auto instr : store.codeToMaterializePastValue(cid, pid, argName)) {
        codeLines.push_back(instr);
      }

      return argName;
    }
    
    void addComment(const std::string& str) {
      codeLines.push_back(new IRComment(str));
    }
    
    void addAssign(const std::string& receiver, const std::string& value) {
      codeLines.push_back(new IRAssign(receiver, value));
    }

    std::string getCode(ValueStore& valueStore) const {
      std::string cppCode = "";
      for (auto instr : codeLines) {
        cppCode += instr->toString(valueStore);
      }

      return cppCode;
    }

    std::string getTwoStateCode(ValueStore& valueStore) const {
      std::string cppCode = "";
      for (auto instr : codeLines) {
        cppCode += instr->twoStateCppCode(valueStore);
      }

      return cppCode;
    }

  };

  std::vector<SigPort>
  sequentialDependencies(const Cell& cell,
                         const PortId pid);

  std::vector<SigPort>
  combinationalDependencies(const Cell& cell,
                            const PortId pid,
                            const CellDefinition& def);
  
  class Simulator {

    ValueStore valueStore;

    void* libHandle;
    void* simulateFuncHandle;
    void* rawSimulateFuncHandle;

    bool simRaw;

    std::set<SigPort> sequentialPorts;

  public:

    CellDefinition& def;

    std::set<std::pair<SigPort, BitVector> > userInputs;
    std::set<SigPort> combChanges;
    std::set<SigPort> seqChanges;

    Simulator(Env& e_, CellDefinition& def_) :
      valueStore(def_),
      libHandle(nullptr),
      simulateFuncHandle(nullptr),
      rawSimulateFuncHandle(nullptr),
      simRaw(false),
      def(def_) {

      std::cout << "Start init" << std::endl;
      for (auto c : def.getCellMap()) {
        auto tp = c.second.getCellType();

        CellId cid = c.first;
        Cell cl = c.second;

        //std::cout << "Initializing " << def.cellName(cid) << std::endl;

        if (tp == CELL_TYPE_CONST) {
          BitVector initVal = cl.getParameterValue(PARAM_INIT_VALUE);
          setPortValue(cid, PORT_ID_OUT, initVal);

          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }
        } else if (tp == CELL_TYPE_MEM) {
          initMemory(cid);
          int memWidth = cl.getMemWidth();

          BitVector initVal(memWidth, 0);
          setPortValue(cid, PORT_ID_RDATA, initVal);

          BitVector clkVal(1, 0);
          SigPort clkPort = {cid, PORT_ID_CLK};
          setPastValue(clkPort.cell, clkPort.port, clkVal);
          
          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_RDATA)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }

        } else if (tp == CELL_TYPE_PORT) {
          if (cl.hasPort(PORT_ID_OUT)) {
            int width = cl.getParameterValue(PARAM_OUT_WIDTH).to_type<int>();
            BitVector initVal = bsim::unknown_bv(width);
            setPortValue(cid, PORT_ID_OUT, initVal);

            const Cell& c = def.getCellRef(cid);
            for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
              for (auto& sigBit : receiverBus) {
                combChanges.insert({sigBit.cell, sigBit.port});
              }
            }

          } else {

            int width = cl.getParameterValue(PARAM_OUT_WIDTH).to_type<int>();
            BitVector initVal = bsim::unknown_bv(width);
            setPortValue(cid, PORT_ID_IN, initVal);

          }
        } else if (tp == CELL_TYPE_SLICE) {

          int hi = cl.getParameterValue(PARAM_HIGH).to_type<int>();
          int lo = cl.getParameterValue(PARAM_LOW).to_type<int>();
          BitVector initVal = bsim::unknown_bv(hi - lo);
          setPortValue(cid, PORT_ID_OUT, initVal);

          // TODO: Ignore empty sigbits
          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }
          
        } else if (tp == CELL_TYPE_ZEXT) {
          int width = cl.getParameterValue(PARAM_OUT_WIDTH).to_type<int>();
          BitVector initVal = bsim::unknown_bv(width);
          setPortValue(cid, PORT_ID_OUT, initVal);

          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }
          
        } else if (isBinop(tp) || isUnop(tp) ||
                   (tp == CELL_TYPE_MUX) || (tp == CELL_TYPE_REG_ARST) ||
                   (tp == CELL_TYPE_REG)) {

          int width = cl.getParameterValue(PARAM_WIDTH).to_type<int>(); 
          if ((tp == CELL_TYPE_REG) ||
              (tp == CELL_TYPE_REG_ARST)) {
            BitVector initVal = cl.getParameterValue(PARAM_INIT_VALUE);

            setPortValue(cid, PORT_ID_OUT, initVal);

            setRegisterValue(cid, initVal);
          } else {
            BitVector initVal = bsim::unknown_bv(width);
            setPortValue(cid, PORT_ID_OUT, initVal);
          }

          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }

          if (tp == CELL_TYPE_REG_ARST) {
            BitVector initVal(1, 0);
            SigPort clkPort = {cid, PORT_ID_CLK};
            SigPort rstPort = {cid, PORT_ID_ARST};

            setPastValue(clkPort.cell, clkPort.port, initVal);
            setPastValue(rstPort.cell, rstPort.port, initVal);
          }

          if (tp == CELL_TYPE_REG) {
            BitVector initVal(1, 0);
            SigPort clkPort = {cid, PORT_ID_CLK};
            setPastValue(clkPort.cell, clkPort.port, initVal);
          }
          
        } else {
          std::cout << "No initialization for cell type " << toString(tp) <<
            std::endl;
          assert(false);
        }
      }

      for (auto portCell : def.getPortCells()) {
        const Cell& cell = def.getCellRefConst(portCell);
        if (cell.isInputPortCell()) {
          if (sequentialDependencies(cell, PORT_ID_OUT).size() > 0) {
            sequentialPorts.insert({portCell, PORT_ID_OUT});
          }
        }
      }
      
      std::cout << "End init" << std::endl;
    }

    void simulateRaw() {
      simRaw = true;
      valueStore.setCompiledRaw();
    }

    void updateCompiledTwoState() {
      // Set past values for inputs connected to clocks
      for (auto sp : sequentialPorts) {
        setPastValue(sp.cell, sp.port,
                     getPortValue(sp.cell, sp.port));
      }

      for (auto in : userInputs) {
        setPortValue(in.first.cell, in.first.port, in.second);
      }
      userInputs = {};

      assert(hasSimulateFunction());

      void (*simFunc)(unsigned char*) =
        reinterpret_cast<void (*)(unsigned char*)>(rawSimulateFuncHandle);
        
      simFunc(valueStore.getRawValueTable());
    }

    void update() {

      //std::cout << "Starting with update" << std::endl;

      if (simRaw) {
        updateCompiledTwoState();
        return;
      }


      // Add user inputs to combChanges
      for (auto portCell : def.getPortCells()) {
        if (def.getCellRefConst(portCell).isInputPortCell()) {
          combinationalSignalChange({portCell, PORT_ID_OUT},
                                    getPortValue(portCell, PORT_ID_OUT));
        }
      }

      // Add user inputs 
      for (auto in : userInputs) {
        combinationalSignalChange({in.first.cell, in.first.port}, in.second);
      }
      userInputs = {};

      // Otherwise run x value simulation
      if (hasSimulateFunction()) {

        void (*simFunc)(BitVector*) =
          reinterpret_cast<void (*)(BitVector*)>(simulateFuncHandle);
        
        //simFunc(valueStore.simValueTable);
        simFunc(&(valueStore.getValueTable()[0]));
        return;
      }

      // If there is no simulate function use the interpreter
      do {

        while (combChanges.size() > 0) {
          SigPort nextComb = *std::begin(combChanges);
          combChanges.erase(nextComb);

          updatePort(nextComb);
        }


        std::vector<CellId> registersToUpdate;
        std::vector<CellId> memoriesToUpdate;
        for (auto s : seqChanges) {

          bool stateChanged = updateSequentialPort(s);
          CellType tp = def.getCellRefConst(s.cell).getCellType();
          if (stateChanged) {
            if ((tp == CELL_TYPE_REG) || (tp == CELL_TYPE_REG_ARST)) {
              registersToUpdate.push_back(s.cell);
            } else {
              assert(tp == CELL_TYPE_MEM);
              memoriesToUpdate.push_back(s.cell);
            }
          }
        }

        seqChanges = {};

        //std::cout << "# of memory updates = " << memoriesToUpdate.size() << std::endl;

        // Change to udpate port?
        for (auto cid : registersToUpdate) {
          combinationalSignalChange({cid, PORT_ID_OUT},
                                    getRegisterValue(cid));
        }

        for (auto cid : memoriesToUpdate) {
          updatePort({cid, PORT_ID_RDATA});
        }
        
      } while (combChanges.size() > 0);

      assert(combChanges.size() == 0);
      assert(seqChanges.size() == 0);
    }

    bool combinationalSignalChange(const SigPort sigPort,
                                   const BitVector& bv) {
      //std::cout << "Combinational signal change for " << sigPortString(def, sigPort) << " to " << bv << std::endl;

      BitVector oldVal = getPortValue(sigPort.cell, sigPort.port);

      setPortValue(sigPort.cell, sigPort.port, bv);

      const Cell& c = def.getCellRef(sigPort.cell);

      bool changed = !same_representation(oldVal, bv);

      if (changed) {

        //std::cout << "Value changed" << std::endl;

        for (auto& receiverBus : c.getPortReceivers(sigPort.port)) {
          for (auto& sigBit : receiverBus) {

            if (notEmpty(sigBit)) {
              //std::cout << "Updating receiver " << toString(def, sigBit) << std::endl;
              if ((sigBit.port != PORT_ID_ARST) &&
                  (sigBit.port != PORT_ID_CLK)) {
                combChanges.insert({sigBit.cell, sigBit.port});
              } else {
                setPastValue(sigPort.cell, sigPort.port, oldVal);
                seqChanges.insert({sigBit.cell, sigBit.port});
              }
            }

          }
        }
      }

      for (auto& receiverBus : c.getPortReceivers(sigPort.port)) {
        for (auto& sigBit : receiverBus) {

          if (notEmpty(sigBit)) {
            if ((sigBit.port != PORT_ID_ARST) &&
                (sigBit.port != PORT_ID_CLK)) {
            } else {
              setPastValue(sigPort.cell, sigPort.port, oldVal);
            }
          }

        }
      }
      
      return changed;
    }

    bool memoryStateChange(const CellId cid,
                           const BitVector& writeAddr,
                           const BitVector& writeData) {
      int addr = writeAddr.to_type<int>();
      BitVector oldVal = getMemoryValue(cid, addr);

      if (same_representation(oldVal, writeData)) {
        return false;
      }

      setMemoryValue(cid, addr, writeData);

      assert(same_representation(getMemoryValue(cid, addr), writeData));

      return true;
    }

    bool registerStateChange(const CellId id,
                             const BitVector& newVal) {
      BitVector oldVal = getRegisterValue(id);
      if (same_representation(oldVal, newVal)) {
        return false;
      }

      setRegisterValue(id, newVal);

      return true;
    }

    bool updateSequentialPort(const SigPort sigPort) {

      //std::cout << "Updating sequential port " << sigPortString(def, sigPort) << std::endl;

      Cell& c = def.getCellRef(sigPort.cell);
      CellType tp = c.getCellType();

      if (tp == CELL_TYPE_REG_ARST) {

        BitVector newClk = materializeInput({sigPort.cell, PORT_ID_CLK});
        BitVector newRst = materializeInput({sigPort.cell, PORT_ID_ARST});

        BitVector oldOut = getRegisterValue(sigPort.cell);

        BitVector oldClk = getPastValue(sigPort.cell, PORT_ID_CLK);
        BitVector oldRst = getPastValue(sigPort.cell, PORT_ID_ARST);

        bool clkPos = c.clkPosedge();
        bool rstPos = c.rstPosedge();

        BitVector newOut = oldOut;

        if (clkPos &&
            newClk.is_binary() &&
            oldClk.is_binary() &&
            (bvToInt(oldClk) == 0) && (bvToInt(newClk) == 1)) {

          newOut = materializeInput({sigPort.cell, PORT_ID_IN});
        }

        if (!clkPos &&
            newClk.is_binary() &&
            oldClk.is_binary() &&
            (bvToInt(oldClk) == 1) && (bvToInt(newClk) == 0)) {

          newOut = materializeInput({sigPort.cell, PORT_ID_IN});
        }

        if (rstPos &&
            newRst.is_binary() &&
            oldRst.is_binary() &&
            (bvToInt(oldRst) == 0) && (bvToInt(newRst) == 1)) {

          newOut = c.initValue();
        }

        if (!rstPos &&
            newRst.is_binary() &&
            oldRst.is_binary() &&
            (bvToInt(oldRst) == 1) && (bvToInt(newRst) == 0)) {

          newOut = c.initValue();
        }

        return registerStateChange(sigPort.cell, newOut);

      } else if (tp == CELL_TYPE_REG) {

        BitVector newClk = materializeInput({sigPort.cell, PORT_ID_CLK});

        BitVector oldOut = getRegisterValue(sigPort.cell);
        BitVector oldClk = getPastValue(sigPort.cell, PORT_ID_CLK);

        bool clkPos = c.clkPosedge();

        BitVector newOut = oldOut;

        if (clkPos &&
            newClk.is_binary() &&
            oldClk.is_binary() &&
            (bvToInt(oldClk) == 0) && (bvToInt(newClk) == 1)) {
          newOut = materializeInput({sigPort.cell, PORT_ID_IN});
        }

        if (!clkPos &&
            newClk.is_binary() &&
            oldClk.is_binary() &&
            (bvToInt(oldClk) == 1) && (bvToInt(newClk) == 0)) {
          newOut = materializeInput({sigPort.cell, PORT_ID_IN});
        }

        return registerStateChange(sigPort.cell, newOut);

      } else if (tp == CELL_TYPE_MEM) {

        BitVector newClk = materializeInput({sigPort.cell, PORT_ID_CLK});
        BitVector oldClk = getPastValue(sigPort.cell, PORT_ID_CLK);

        BitVector writeEnable = materializeInput({sigPort.cell, PORT_ID_WEN});

        if (newClk.is_binary() &&
            oldClk.is_binary() &&
            writeEnable.is_binary() &&
            (writeEnable.get(0) == 1) &&
            (bvToInt(oldClk) == 0) && (bvToInt(newClk) == 1)) {

          //std::cout << "Getting inputs on high clock" << std::endl;

          BitVector writeAddr = materializeInput({sigPort.cell, PORT_ID_WADDR});
          BitVector writeData = materializeInput({sigPort.cell, PORT_ID_WDATA});

          //std::cout << "Got inputs" << std::endl;

          return memoryStateChange(sigPort.cell, writeAddr, writeData);
        }

        return false;
        
      } else {
        assert(false);
      }
      
    }

    bool updatePort(const SigPort sigPort) {

      //std::cout << "Updating port " << sigPortString(def, sigPort) << std::endl; //def.getCellName(sigPort.cell) << ", " << portIdString(sigPort.port) << std::endl;

      Cell& c = def.getCellRef(sigPort.cell);
      CellType tp = c.getCellType();

      if ((tp == CELL_TYPE_PORT) || (tp == CELL_TYPE_CONST)) {
        if (tp == CELL_TYPE_PORT) {
          // This is an odd value because in general the simulator does not
          // store values of input ports in portValues
          BitVector ptp = c.getParameterValue(PARAM_PORT_TYPE);
          int ptpInt = ptp.to_type<int>();
          if (ptpInt == PORT_CELL_FOR_OUTPUT) {

            setPortValue(sigPort.cell, sigPort.port, materializeInput(sigPort));

          }
        }

        return false;
      } else if (tp == CELL_TYPE_ZEXT) {
        BitVector in = materializeInput({sigPort.cell, PORT_ID_IN});

        int outWidth = c.getPortWidth(PORT_ID_OUT);
        BitVector res(outWidth, 0);
        for (uint i = 0; i < (uint) in.bitLength(); i++) {
          res.set(i, in.get(i));
        }
        
        BitVector newOut = res;

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);
        
      } else if (tp == CELL_TYPE_PASSTHROUGH) {
        BitVector in = materializeInput({sigPort.cell, PORT_ID_IN});

        BitVector newOut = in;

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_MEM) {

        // Reads are combinational, writes are sequential
        BitVector raddr = materializeInput({sigPort.cell, PORT_ID_RADDR});
        BitVector rdata(def.getCellRefConst(sigPort.cell).getMemWidth(), 21);

        if (raddr.is_binary()) {
          rdata = getMemoryValue(sigPort.cell, raddr.to_type<int>());
        }

        //std::cout << "Updating memory rdata port, raddr = " << raddr << ", rdata = " << rdata << std::endl;

        return combinationalSignalChange({sigPort.cell, PORT_ID_RDATA}, rdata);
      } else if (tp == CELL_TYPE_REG_ARST) {

        return false;

      } else if (tp == CELL_TYPE_REG) {

        return false;

      } else if (tp == CELL_TYPE_MUX) {
        
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});
        BitVector sel = materializeInput({sigPort.cell, PORT_ID_SEL});

        assert(sel.bitLength() == 1);

        BitVector newOut = in0;

        if (sel.is_binary()) {
          int i = bvToInt(sel);
          newOut = (i == 1) ? in1 : in0; 
        }

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_UGT) {
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = BitVector(1, in0 > in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_ULT) {
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = BitVector(1, in0 < in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);
        
      } else if (tp == CELL_TYPE_UGE) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = BitVector(1, (in0 > in1) || (in0 == in1));

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);
        
      } else if (tp == CELL_TYPE_ULE) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = BitVector(1, (in0 < in1) || (in0 == in1));

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);
        
      } else if (tp == CELL_TYPE_SLICE) {

        BitVector in = materializeInput({sigPort.cell, PORT_ID_IN});

        uint lo = c.getParameterValue(PARAM_LOW).to_type<int>();
        uint hi = c.getParameterValue(PARAM_HIGH).to_type<int>();

        assert((hi - lo) > 0);

        BitVector res(hi - lo, 0);
        for (uint i = lo; i < hi; i++) {
          res.set(i - lo, in.get(i));
        }
        BitVector newOut = res;
        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);
        
      } else if (tp == CELL_TYPE_SHL) {
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = shl(in0, in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);       

      } else if (tp == CELL_TYPE_ASHR) {
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = ashr(in0, in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);       

      } else if (tp == CELL_TYPE_LSHR) {
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = lshr(in0, in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);       

      } else if (tp == CELL_TYPE_XOR) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = in0 ^ in1;

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);       

      } else if (tp == CELL_TYPE_SUB) {
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = sub_general_width_bv(in0, in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);       

      } else if (tp == CELL_TYPE_MUL) {
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = mul_general_width_bv(in0, in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_ADD) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = add_general_width_bv(in0, in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_AND) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = in0 & in1;

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);       

      } else if (tp == CELL_TYPE_OR) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = in0 | in1;

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);
        
      } else if (tp == CELL_TYPE_ORR) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN});
        BitVector newOut = orr(in0);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_ANDR) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN});
        BitVector newOut = andr(in0);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_NOT) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN});

        BitVector newOut = ~in0;

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_NEQ) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = BitVector(1, in0 != in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_EQ) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector newOut = BitVector(1, in0 == in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);
        
      } else {
        std::cout << "No update for cell type " << toString(tp) << std::endl;
        assert(false);
      }

    }

    BitVector materializeInput(const SigPort sigPort) const {
      int width = def.getCellRef(sigPort.cell).getPortWidth(sigPort.port);

      BitVector val(width, 0);

      auto& sigBus = def.getCellRef(sigPort.cell).getDrivers(sigPort.port);

      assert(((int) sigBus.signals.size()) == width);

      for (int i = 0; i < (int) sigBus.signals.size(); i++) {
        SignalBit b = sigBus.signals.at(i);

        assert(notEmpty(b));

        val.set(i, getBitVec(b.cell, b.port).get(b.offset));
      }
        
      return val;
    }

    void setFreshValue(const std::string& cellName,
                       const PortId id,
                       const BitVector& bv) {
      CellId cid = def.getPortCellId(cellName);
      setFreshValue(cid, id, bv);
    }

    void setFreshValue(const std::string& cellName,
                       const BitVector& bv) {
      setFreshValue(cellName, PORT_ID_OUT, bv);
    }
    
    void setFreshValue(const CellId cid,
                       const PortId pid,
                       const BitVector& bv) {
      assert(def.hasCell(cid));
      assert(def.getCellRef(cid).getPortWidth(pid) == bv.bitLength());

      userInputs.insert({{cid, pid}, bv});
    }

    void initMemory(const CellId cid) {
      valueStore.initMemory(cid);
    }

    unsigned long portValueOffset(const CellId cid,
                                  const PortId pid) {
      return valueStore.portValueOffset(cid, pid);
    }

    void setPortValue(const CellId cid,
                      const PortId pid,
                      const BitVector& bv) {
      valueStore.setPortValue(cid, pid, bv);
    }

    BitVector getPortValue(const CellId cid,
                           const PortId pid) const {
      return valueStore.getPortValue(cid, pid);
    }

    void setRegisterValue(const CellId cid,
                          const BitVector& bv) {
      valueStore.setRegisterValue(cid, bv);
    }

    BitVector getRegisterValue(const CellId cid) const {
      return valueStore.getRegisterValue(cid);
    }

    unsigned long pastValueOffset(const CellId cid,
                                  const PortId pid) {
      return valueStore.pastValueOffset(cid, pid);
    }
    
    void setPastValue(const CellId cid,
                      const PortId pid,
                      const BitVector& bv) {
      return valueStore.setPastValue(cid, pid, bv);
    }

    BitVector getPastValue(const CellId cid,
                           const PortId pid) {
      return valueStore.getPastValue(cid, pid);
    }
    
    std::map<CellId, BitVector> allRegisterValues() const {
      std::map<CellId, BitVector> regValues;
      for (auto roff : def.getCellMap()) {
        if (isRegister(def.getCellRefConst(roff.first).getCellType())) {
          regValues.insert({roff.first, getRegisterValue(roff.first)});
        }
      }
      return regValues;
    }
    
    // This is the user facing funtion. getPortValue is for internal use
    BitVector getBitVec(const CellId cid,
                        const PortId pid) const {

      return getPortValue(cid, pid);
    }    

    BitVector getBitVec(const std::string& cellName,
                        const PortId id) {
      CellId cid = def.getPortCellId(cellName);
      return getBitVec(cid, id);
    }

    BitVector getBitVec(const std::string& cellName) {
      return getBitVec(cellName, PORT_ID_IN);
    }

    BitVector getMemoryValue(const CellId cid,
                             const int offset) const {
      return valueStore.getMemoryValue(cid, offset);
    }

    void setMemoryValue(const CellId cid,
                        const int addr,
                        const BitVector& writeData) {
      valueStore.setMemoryValue(cid, addr, writeData);
    }
    
    std::vector<SigPort> traceValue(const std::string& cellName,
                                    const PortId portId) {
      return traceValue(def.getPortCellId(cellName), portId);
    }

    void refreshConstants() {
      for (auto cp : def.getCellMap()) {
        CellId cid = cp.first;

        if (def.getCellRefConst(cid).getCellType() == CELL_TYPE_CONST) {

          const Cell& c = def.getCellRef(cid);
          BitVector initVal = c.getParameterValue(PARAM_INIT_VALUE);

          setPortValue(cid, PORT_ID_OUT, initVal);

          if (sequentialDependencies(c, PORT_ID_OUT).size() > 0) {
            setPastValue(cid, PORT_ID_OUT, initVal);
          }

          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }
        }

      }

      update();
    }

    std::vector<SigPort> traceValue(const CellId id,
                                    const PortId portId);

    std::vector<SigPort> dataSources(const SigPort sp);
    std::vector<SigPort> getDataPorts(const CellId sp);

    bool compileCircuit();

    bool hasSimulateFunction() const {
      return (libHandle != nullptr) && (simulateFuncHandle != nullptr);
    }

    void
    compileLevelizedCircuit(const std::vector<std::vector<SigPort> >& updates);
    
    void debugPrintTableValues() const;
    void debugPrintMemories() const;
    void debugPrintMemories(const std::vector<std::string>& prefixes) const;
    void debugPrintRegisters() const;
    void debugPrintPorts() const;

    std::string
    sequentialBlockCode(const std::vector<SigPort>& levelized,
                        CodeGenState& state);
    
    std::string
    combinationalBlockCode(const std::vector<SigPort>& levelized,
                           CodeGenState& state);

    void
    binopCode(CodeGenState& codeState,
              const CellId cid) {

      std::string argName0 = codeState.getVariableName(cid, PORT_ID_IN0, valueStore);
      std::string argName1 = codeState.getVariableName(cid, PORT_ID_IN1, valueStore);

      std::string outName = codeState.getPortTemp(cid, PORT_ID_OUT);
      codeState.addBinop(outName, def.getCellRefConst(cid), argName0, argName1);
      codeState.addAssign(cid, PORT_ID_OUT, outName, valueStore);
    }

    void
    unopCode(CodeGenState& codeState,
             const CellId cid) {

      std::string argName = codeState.getVariableName(cid, PORT_ID_IN, valueStore);
      std::string outName = codeState.getPortTemp(cid, PORT_ID_OUT);
      codeState.addUnop(outName, def.getCellRefConst(cid), argName);
      codeState.addAssign(cid, PORT_ID_OUT, outName, valueStore);
    }
    
    
    ~Simulator();
    
  };

  bool matchesPrefix(const std::string& str,
                     const std::string& prefix);
  
  bool matchesAnyPrefix(const std::string& str,
                        const std::vector<std::string>& prefixes);
  
}
