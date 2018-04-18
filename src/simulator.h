#pragma once

#include "circuit.h"

namespace FlatCircuit {

  class SigPort {
  public:
    CellId cell;
    PortId port;
  };

  static inline std::string sigPortString(const CellDefinition& def,
                                          const SigPort port) {
    return "( " + def.cellName(port.cell) + ", " + portIdString(port.port) + ", " + toString(def.getCellRefConst(port.cell).getCellType()) + " )";
  }

  static inline bool operator<(const SigPort a, const SigPort b) {
    if (a.cell > b.cell) {
      return false;
    }

    if (a.cell == b.cell) {
      return a.port < b.port;
    }

    return true;
  }

  class Simulator {

  public:

    CellDefinition& def;

    std::map<SigPort, BitVector> portValues;
    std::set<SigPort> userInputs;
    std::set<SigPort> combChanges;
    std::set<SigPort> seqChanges;


    // Use this to save clock ports
    std::map<SigPort, BitVector> pastValues;
    
    
    Simulator(Env& e_, CellDefinition& def_) : def(def_) {

      std::cout << "Start init" << std::endl;
      for (auto c : def.getCellMap()) {
        auto tp = c.second.getCellType();

        CellId cid = c.first;
        Cell cl = c.second;

        std::cout << "Initializing " << def.cellName(cid) << std::endl;

        if (tp == CELL_TYPE_CONST) {
          BitVector initVal = cl.getParameterValue(PARAM_INIT_VALUE);
          portValues[{cid, PORT_ID_OUT}] = initVal;

          const Cell& c = def.getCellRef(cid);
          for (auto& receiverBus : c.getPortReceivers(PORT_ID_OUT)) {
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
        } else if (tp == CELL_TYPE_ZEXT) {
          int width = cl.getParameterValue(PARAM_OUT_WIDTH).to_type<int>();
          BitVector initVal = bsim::unknown_bv(width);

          combinationalSignalChange({cid, PORT_ID_OUT}, initVal);
        } else if (isBinop(tp) || isUnop(tp) ||
                   (tp == CELL_TYPE_MUX) || (tp == CELL_TYPE_REG_ARST) ||
                   (tp == CELL_TYPE_REG)) {

          int width = cl.getParameterValue(PARAM_WIDTH).to_type<int>();
          BitVector initVal = bsim::unknown_bv(width);
          portValues[{cid, PORT_ID_OUT}] = initVal;

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
            std::cout << "making reg" << std::endl;
            BitVector initVal(1, 0);
            SigPort clkPort = {cid, PORT_ID_CLK};
            pastValues[clkPort] = initVal;
            std::cout << "done reg" << std::endl;
          }
          
        } else {
          std::cout << "No initialization for cell type " << toString(tp) << std::endl;
          assert(false);
        }
      }

      std::cout << "End init" << std::endl;
    }

    void update() {
      //std::cout << "Starting with update" << std::endl;
      
      // Add user inputs to combChanges
      for (auto in : userInputs) {
        const Cell& c = def.getCellRef(in.cell);
        for (auto& receiverBus : c.getPortReceivers(in.port)) {
          for (auto& sigBit : receiverBus) {
            combChanges.insert({sigBit.cell, sigBit.port});
          }
        }

        //combChanges.insert(in);
      }

      userInputs = {};

      //std::cout << "Comb values than changed" << std::endl;
      // for (auto val : combChanges) {
      //   std::cout << "\t" << def.getCellName(val.cell) << ", " << portIdString(val.port) << std::endl;
      // }

      // std::cout << "Done with values" << std::endl;

      // do while updates
      do {

        // Comb updates
        while (combChanges.size() > 0) {
          SigPort nextComb = *std::begin(combChanges);
          combChanges.erase(nextComb);

          updatePort(nextComb);

        }

        // TODO: Add sequential updates
        //std::cout << "Sequential updates" << std::endl;
        for (auto s : seqChanges) {
          //std::cout << "\tUpdating " << sigPortString(def, s) << std::endl;

          // Note: This should reall delay updates to values?
          updatePort(s);
        }

      } while (combChanges.size() > 0);

      //std::cout << "Done with update" << std::endl;

      assert(combChanges.size() == 0);
    }

    bool combinationalSignalChange(const SigPort sigPort,
                                   const BitVector& bv) {
      BitVector oldVal = portValues.at(sigPort);
      portValues[{sigPort.cell, PORT_ID_OUT}] = bv;

      const Cell& c = def.getCellRef(sigPort.cell);

      bool changed = !same_representation(oldVal, bv);

      if (changed) {
        for (auto& receiverBus : c.getPortReceivers(sigPort.port)) {
          for (auto& sigBit : receiverBus) {

            if (notEmpty(sigBit)) {
              if ((sigBit.port != PORT_ID_ARST) &&
                  (sigBit.port != PORT_ID_CLK)) {
                combChanges.insert({sigBit.cell, sigBit.port});
              } else {
                seqChanges.insert({sigBit.cell, sigBit.port});
              }
            }
          }
        }
      }

      return changed;
    }

    bool updatePort(const SigPort sigPort) {

      //std::cout << "Updating port " << sigPortString(def, sigPort) << std::endl; //def.getCellName(sigPort.cell) << ", " << portIdString(sigPort.port) << std::endl;

      Cell& c = def.getCellRef(sigPort.cell);
      CellType tp = c.getCellType();

      if ((tp == CELL_TYPE_PORT) || (tp == CELL_TYPE_CONST)) {
        if (tp == CELL_TYPE_PORT) {
          //          std::cout << "Updating port" << std::endl;

          // This is an odd value because in general the simulator does not
          // store values of input ports in portValues
          BitVector ptp = c.getParameterValue(PARAM_PORT_TYPE);
          int ptpInt = ptp.to_type<int>();
          if (ptpInt == PORT_CELL_FOR_OUTPUT) {
            //            std::cout << "Updating output port" << std::endl;

            portValues[sigPort] = materializeInput(sigPort);

            //            std::cout << "Done updating output port" << std::endl;

          }
        }

        return false;
      } else if (tp == CELL_TYPE_PASSTHROUGH) {
        BitVector in = materializeInput({sigPort.cell, PORT_ID_IN});

        BitVector oldOut = getBitVec(sigPort.cell, PORT_ID_OUT);

        BitVector newOut = in;

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

      } else if (tp == CELL_TYPE_REG_ARST) {
        BitVector newClk = materializeInput({sigPort.cell, PORT_ID_CLK});
        BitVector newRst = materializeInput({sigPort.cell, PORT_ID_ARST});

        // assert(newClk.is_binary());
        // assert(newRst.is_binary());

        BitVector oldOut = getBitVec(sigPort.cell, PORT_ID_OUT);

        BitVector oldClk = pastValues.at({sigPort.cell, PORT_ID_CLK});
        BitVector oldRst = pastValues.at({sigPort.cell, PORT_ID_ARST});

        // assert(oldClk.is_binary());
        // assert(oldRst.is_binary());
        
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

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);        
        // portValues[{sigPort.cell, PORT_ID_OUT}] = newOut;
        
        // return !same_representation(oldOut, newOut);
      } else if (tp == CELL_TYPE_MUX) {
        
        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});
        BitVector sel = materializeInput({sigPort.cell, PORT_ID_SEL});

        BitVector oldOut = getBitVec(sigPort.cell, PORT_ID_OUT);

        assert(sel.bitLength() == 1);

        BitVector newOut = in0;

        if (sel.is_binary()) {
          int i = bvToInt(sel);
          newOut = i == 1 ? in1 : in0; 
        }

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);                
        // portValues[{sigPort.cell, PORT_ID_OUT}] = newOut;

        // return !same_representation(oldOut, newOut);
      } else if (tp == CELL_TYPE_OR) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector oldOut = getBitVec(sigPort.cell, PORT_ID_OUT);

        BitVector newOut = in0 | in1;

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);        

        // portValues[{sigPort.cell, PORT_ID_OUT}] = newOut;
        // return !same_representation(oldOut, newOut);
        
      } else if (tp == CELL_TYPE_ORR) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN});

        BitVector oldOut = getBitVec(sigPort.cell, PORT_ID_OUT);

        BitVector newOut = orr(in0);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

        // portValues[{sigPort.cell, PORT_ID_OUT}] = newOut;
        // return !same_representation(oldOut, newOut);

      } else if (tp == CELL_TYPE_EQ) {

        BitVector in0 = materializeInput({sigPort.cell, PORT_ID_IN0});
        BitVector in1 = materializeInput({sigPort.cell, PORT_ID_IN1});

        BitVector oldOut = getBitVec(sigPort.cell, PORT_ID_OUT);

        BitVector newOut = BitVector(1, in0 == in1);

        return combinationalSignalChange({sigPort.cell, PORT_ID_OUT}, newOut);

        // portValues[{sigPort.cell, PORT_ID_OUT}] = newOut;
        // return !same_representation(oldOut, newOut);
        
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
      portValues[{cid, pid}] = bv;
      userInputs.insert({cid, pid});
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

  };
}
