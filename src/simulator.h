#pragma once

#include "circuit.h"

namespace FlatCircuit {

  class SimMemory {
  public:
    int depth;
    int width;
    std::vector<BitVector> mem;

    SimMemory() {
      depth = 0;
      width = 0;
    }

    SimMemory(const int depth, const int width) {
      mem.resize(depth);
      for (int i = 0; i < depth; i++) {
        mem[i] = BitVector(width, 0);
      }
    }
  };

  class Simulator {

  public:

    CellDefinition& def;

    std::map<SigPort, BitVector> portValues;
    std::map<CellId, BitVector> registerValues;

    std::set<std::pair<SigPort, BitVector> > userInputs;
    std::set<SigPort> combChanges;
    std::set<SigPort> seqChanges;

    // Use this to save clock / reset port past values to detect edges
    std::map<SigPort, BitVector> pastValues;

    std::map<CellId, SimMemory> memories;
    
    Simulator(Env& e_, CellDefinition& def_) : def(def_) {

      std::cout << "Start init" << std::endl;
      for (auto c : def.getCellMap()) {
        auto tp = c.second.getCellType();

        CellId cid = c.first;
        Cell cl = c.second;

        //std::cout << "Initializing " << def.cellName(cid) << std::endl;

        if (tp == CELL_TYPE_CONST) {
          BitVector initVal = cl.getParameterValue(PARAM_INIT_VALUE);
          portValues[{cid, PORT_ID_OUT}] = initVal;

          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }
        } else if (tp == CELL_TYPE_MEM) {
          int memWidth = cl.getMemWidth();
          int memDepth = cl.getMemDepth();

          SimMemory defaultMem(memDepth, memWidth);

          memories[cid] = defaultMem;

          BitVector initVal(memWidth, 0);
          portValues[{cid, PORT_ID_RDATA}] = initVal;

          BitVector clkVal(1, 0);
          SigPort clkPort = {cid, PORT_ID_CLK};
          pastValues[clkPort] = clkVal;
          
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
            portValues[{cid, PORT_ID_OUT}] = initVal;

            const Cell& c = def.getCellRef(cid);
            for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
              for (auto& sigBit : receiverBus) {
                combChanges.insert({sigBit.cell, sigBit.port});
              }
            }

          }
        } else if (tp == CELL_TYPE_SLICE) {

          int hi = cl.getParameterValue(PARAM_HIGH).to_type<int>();
          int lo = cl.getParameterValue(PARAM_LOW).to_type<int>();
          BitVector initVal = bsim::unknown_bv(hi - lo);
          portValues[{cid, PORT_ID_OUT}] = initVal;

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
          portValues[{cid, PORT_ID_OUT}] = initVal;

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
            //std::cout << "Register init value = " << initVal << std::endl;
            portValues[{cid, PORT_ID_OUT}] = initVal;
            registerValues[cid] = initVal;
          } else {
            BitVector initVal = bsim::unknown_bv(width);
            portValues[{cid, PORT_ID_OUT}] = initVal;
          }

          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
            for (auto& sigBit : receiverBus) {
              combChanges.insert({sigBit.cell, sigBit.port});
            }
          }

          if (tp == CELL_TYPE_REG_ARST) {
            BitVector initVal(1, 0); // = //bsim::unknown_bv(1);
            SigPort clkPort = {cid, PORT_ID_CLK};
            SigPort rstPort = {cid, PORT_ID_ARST};
            pastValues[clkPort] = initVal;
            pastValues[rstPort] = initVal;
          }

          if (tp == CELL_TYPE_REG) {
            //std::cout << "making reg" << std::endl;
            BitVector initVal(1, 0);
            SigPort clkPort = {cid, PORT_ID_CLK};
            pastValues[clkPort] = initVal;
            //std::cout << "done reg" << std::endl;
          }
          
        } else {
          std::cout << "No initialization for cell type " << toString(tp) <<
            std::endl;
          assert(false);
        }
      }

      std::cout << "End init" << std::endl;
    }

    void update() {
      //std::cout << "Starting with update" << std::endl;
      
      // Add user inputs to combChanges
      for (auto in : userInputs) {

        combinationalSignalChange({in.first.cell, in.first.port}, in.second);

      }

      userInputs = {};

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
                                    map_find(cid, registerValues));
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

      BitVector oldVal = portValues.at(sigPort);
      //portValues[{sigPort.cell, PORT_ID_OUT}] = bv;
      portValues[{sigPort.cell, sigPort.port}] = bv;

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
                pastValues[sigPort] = oldVal;
                seqChanges.insert({sigBit.cell, sigBit.port});
              }
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
      BitVector oldVal = map_find(cid, memories).mem.at(addr);

      if (same_representation(oldVal, writeData)) {
        return false;
      }

      assert(contains_key(cid, memories));

      auto& mem = memories[cid].mem;
      mem[addr] = writeData;

      assert(same_representation(map_find(cid, memories).mem[addr], writeData));

      return true;
    }

    bool registerStateChange(const CellId id,
                             const BitVector& newVal) {
      BitVector oldVal = map_find(id, registerValues);
      if (same_representation(oldVal, newVal)) {
        return false;
      }

      registerValues[id] = newVal;

      return true;
    }

    bool updateSequentialPort(const SigPort sigPort) {

      //std::cout << "Updating sequential port " << sigPortString(def, sigPort) << std::endl;

      Cell& c = def.getCellRef(sigPort.cell);
      CellType tp = c.getCellType();

      if (tp == CELL_TYPE_REG_ARST) {

        BitVector newClk = materializeInput({sigPort.cell, PORT_ID_CLK});
        BitVector newRst = materializeInput({sigPort.cell, PORT_ID_ARST});

        BitVector oldOut = map_find(sigPort.cell, registerValues);

        BitVector oldClk = pastValues.at({sigPort.cell, PORT_ID_CLK});
        BitVector oldRst = pastValues.at({sigPort.cell, PORT_ID_ARST});

        bool clkPos = c.clkPosedge();
        bool rstPos = c.rstPosedge();

        BitVector newOut = oldOut;

        //std::cout << "Updating reg arst " << def.cellName(sigPort.cell) << ", currently input = " << materializeInput({sigPort.cell, PORT_ID_IN}) << std::endl;
        //std::cout << "\told clock = " << oldClk << std::endl;
        //std::cout << "\tnew clock = " << newClk << std::endl;

        if (clkPos &&
            newClk.is_binary() &&
            oldClk.is_binary() &&
            (bvToInt(oldClk) == 0) && (bvToInt(newClk) == 1)) {

          //std::cout << "\tSet reg arst " << def.cellName(sigPort.cell) << ", to input = " << materializeInput({sigPort.cell, PORT_ID_IN}) << std::endl;

          newOut = materializeInput({sigPort.cell, PORT_ID_IN});
        }

        if (!clkPos &&
            newClk.is_binary() &&
            oldClk.is_binary() &&
            (bvToInt(oldClk) == 1) && (bvToInt(newClk) == 0)) {

          //std::cout << "\tSet reg arst " << def.cellName(sigPort.cell) << ", to input = " << materializeInput({sigPort.cell, PORT_ID_IN}) << std::endl;
          newOut = materializeInput({sigPort.cell, PORT_ID_IN});
        }

        if (rstPos &&
            newRst.is_binary() &&
            oldRst.is_binary() &&
            (bvToInt(oldRst) == 0) && (bvToInt(newRst) == 1)) {

          //std::cout << "\tSet reg arst " << def.cellName(sigPort.cell) << ", to input = " << materializeInput({sigPort.cell, PORT_ID_IN}) << std::endl;
          newOut = c.initValue();
        }

        if (!rstPos &&
            newRst.is_binary() &&
            oldRst.is_binary() &&
            (bvToInt(oldRst) == 1) && (bvToInt(newRst) == 0)) {

          //std::cout << "\tSet reg arst " << def.cellName(sigPort.cell) << ", to input = " << materializeInput({sigPort.cell, PORT_ID_IN}) << std::endl;
          newOut = c.initValue();
        }

        return registerStateChange(sigPort.cell, newOut);

      } else if (tp == CELL_TYPE_REG) {

        BitVector newClk = materializeInput({sigPort.cell, PORT_ID_CLK});

        BitVector oldOut = map_find(sigPort.cell, registerValues);
        BitVector oldClk = pastValues.at({sigPort.cell, PORT_ID_CLK});

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
        BitVector oldClk = map_find({sigPort.cell, PORT_ID_CLK}, pastValues);

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

            portValues[sigPort] = materializeInput(sigPort);

          }
        }

        return false;
      } else if (tp == CELL_TYPE_ZEXT) {
        BitVector in = materializeInput({sigPort.cell, PORT_ID_IN});

        int outWidth = c.getPortWidth(PORT_ID_OUT);
        BitVector res(outWidth, 0);
        for (uint i = 0; i < in.bitLength(); i++) {
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
          rdata = map_find(sigPort.cell, memories).mem[raddr.to_type<int>()];
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

    BitVector materializeInput(const SigPort sigPort) {
      int width = def.getCellRef(sigPort.cell).getPortWidth(sigPort.port);

      BitVector val(width, 0);

      auto& sigBus = def.getCellRef(sigPort.cell).getDrivers(sigPort.port);

      assert(sigBus.signals.size() == width);

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

    BitVector getBitVec(const CellId cid,
                        const PortId pid) {

      if (!contains_key({cid, pid}, portValues)) {
        std::cout << "No value for " << def.getCellName(cid) << ", " << portIdString(pid) << std::endl;
      }

      assert(contains_key({cid, pid}, portValues));
      return portValues.at({cid, pid});
    }    

    BitVector getBitVec(const std::string& cellName,
                        const PortId id) {
      CellId cid = def.getPortCellId(cellName);
      return getBitVec(cid, id);
    }

    BitVector getBitVec(const std::string& cellName) {
      return getBitVec(cellName, PORT_ID_IN);
    }

    std::vector<SigPort> traceValue(const std::string& cellName,
                                    const PortId portId) {
      return traceValue(def.getPortCellId(cellName), portId);
    }

    std::vector<SigPort> traceValue(const CellId id,
                                    const PortId portId);

    std::vector<SigPort> dataSources(const SigPort sp);
    std::vector<SigPort> getDataPorts(const CellId sp);
    
  };

}
