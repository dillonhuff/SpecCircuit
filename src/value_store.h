#pragma once

#include "circuit.h"
#include "ir.h"

namespace FlatCircuit {

  class ValueStore {
    std::vector<BitVector> simValueTable;

    std::map<SigPort, unsigned long> portOffsets;
    std::map<CellId, unsigned long> memoryOffsets;
    std::map<CellId, unsigned long> registerOffsets;    
    std::map<SigPort, unsigned long> pastValueOffsets;

    // Raw (2 state) table entries
    //    unsigned char* rawSimValueTable;
    unsigned long rawTableSize;

    std::map<SigPort, unsigned long> rawPortOffsets;
    std::map<CellId, unsigned long> rawMemoryOffsets;
    std::map<CellId, unsigned long> rawRegisterOffsets;
    std::map<SigPort, unsigned long> rawPastValueOffsets;
    
  public:
    CellDefinition& def;

    ValueStore(CellDefinition& def_) : def(def_) {}

    void buildRawValueTable() {
      unsigned long rawOffset = 0;
      map<unsigned long, unsigned long> quadOffsetsToRawOffsets;
      for (unsigned long i = 0; i < simValueTable.size(); i++) {
        quadOffsetsToRawOffsets[i] = rawOffset;
        rawOffset += storedByteLength(simValueTable[i].bitLength());
      }

      rawTableSize = rawOffset;

      for (auto sp : portOffsets) {
        rawPortOffsets[sp.first] = map_find(sp.second, quadOffsetsToRawOffsets);
      }

      for (auto sp : pastValueOffsets) {
        rawPastValueOffsets[sp.first] = map_find(sp.second, quadOffsetsToRawOffsets);
      }

      for (auto sp : memoryOffsets) {
        rawMemoryOffsets[sp.first] = map_find(sp.second, quadOffsetsToRawOffsets);
      }

      for (auto sp : registerOffsets) {
        rawRegisterOffsets[sp.first] = map_find(sp.second, quadOffsetsToRawOffsets);
      }
      
    }

    void debugPrintTableValues() const;

    std::vector<BitVector>& getValueTable() { return simValueTable; }

    unsigned long getMemoryOffset(const CellId cid) const {
      return map_find(cid, memoryOffsets);
    }

    unsigned long getRegisterOffset(const CellId cid) const {
      return map_find(cid, registerOffsets);
    }

    unsigned long rawPortValueOffset(const CellId cid,
                                     const PortId pid) {
      return map_find({cid, pid}, rawPortOffsets);
    }
    
    unsigned long portValueOffset(const CellId cid,
                                  const PortId pid) {
      if (!contains_key({cid, pid}, portOffsets)) {
        unsigned long nextInd = simValueTable.size();
        simValueTable.push_back(BitVector(1, 0));
        portOffsets[{cid, pid}] = nextInd;

        return nextInd;
      }

      return map_find({cid, pid}, portOffsets);
    }

    void initMemory(const CellId cid) {
      assert(!contains_key(cid, memoryOffsets));

      const Cell& cl = def.getCellRefConst(cid);
      int memWidth = cl.getMemWidth();
      int memDepth = cl.getMemDepth();

      BitVector defaultValue(memWidth, 0);

      unsigned long nextInd = simValueTable.size();
      memoryOffsets[cid] = nextInd;
      for (unsigned long i = 0; i < (unsigned long) memDepth; i++) {
        simValueTable.push_back(defaultValue);
      }
    }

    BitVector getMemoryValue(const CellId cid,
                             const int offset) const {
      assert(offset >= 0);

      return simValueTable[map_find(cid, memoryOffsets) + ((unsigned long) offset)];
    }

    void setMemoryValue(const CellId cid,
                        const int addr,
                        const BitVector& writeData) {

      assert(contains_key(cid, memoryOffsets));
      assert(addr >= 0);

      simValueTable[map_find(cid, memoryOffsets) + ((unsigned long) addr)] =
        writeData;

    }

    void setPortValue(const CellId cid,
                      const PortId pid,
                      const BitVector& bv) {
      if (!contains_key({cid, pid}, portOffsets)) {
        unsigned long nextInd = simValueTable.size();
        portOffsets[{cid, pid}] = nextInd;
        simValueTable.push_back(bv);
      }

      simValueTable[map_find({cid, pid}, portOffsets)] = bv;
    }

    BitVector getPortValue(const CellId cid,
                           const PortId pid) const {
      return simValueTable[map_find({cid, pid}, portOffsets)];
    }

    void setRegisterValue(const CellId cid,
                          const BitVector& bv) {
      if (!contains_key(cid, registerOffsets)) {
        unsigned long nextInd = simValueTable.size();
        registerOffsets[cid] = nextInd;
        simValueTable.push_back(bv);
      }

      simValueTable[map_find(cid, registerOffsets)] = bv;
    }

    BitVector getRegisterValue(const CellId cid) const {
      return simValueTable[map_find(cid, registerOffsets)];
    }

    void setPastValue(const CellId cid,
                      const PortId pid,
                      const BitVector& bv) {
      unsigned long offset = pastValueOffset(cid, pid);
      simValueTable[offset] = bv;
    }

    BitVector getPastValue(const CellId cid,
                           const PortId pid) {
      return simValueTable[pastValueOffset(cid, pid)];
    }

    unsigned long pastValueOffset(const CellId cid,
                                  const PortId pid) {
      const Cell& cell = def.getCellRefConst(cid);

      if (cell.getPortType(pid) == PORT_TYPE_OUT) {

        if (!contains_key({cid, pid}, pastValueOffsets)) {
          unsigned long nextInd = simValueTable.size();
          simValueTable.push_back(BitVector(1, 0));
          
          pastValueOffsets[{cid, pid}] = nextInd;

          return nextInd;
        }

        return map_find({cid, pid}, pastValueOffsets);

      } else {
        assert(cell.getPortType(pid) == PORT_TYPE_IN);
        
        auto drivers = cell.getDrivers(pid);
        assert(drivers.signals.size() == 1);

        SignalBit driverBit = drivers.signals[0];

        assert(notEmpty(driverBit));
        assert(driverBit.offset == 0);

        return pastValueOffset(driverBit.cell, driverBit.port);
      }
    }

    std::string codeToAssign(const CellId cid,
                             const PortId pid,
                             const std::string& assignCode) {
      return ln("values[" + std::to_string(portValueOffset(cid, pid)) + "] = " +
                assignCode);
    }

    IRInstruction* codeToAssignRegister(const CellId cid,
                                        const std::string& assignCode) {
      // return ln("values[" + std::to_string(map_find(cid, registerOffsets)) + "] = " +
      //           assignCode);

      return
        new IRAssign("values[" + std::to_string(map_find(cid, registerOffsets)) + "]",
                     assignCode);
    }
    
    std::string codeToAssignPastValue(const CellId cid,
                                      const PortId pid,
                                      const std::string& assignCode) {
      return ln("values[" + std::to_string(map_find({cid, pid}, pastValueOffsets)) + "] = " +
                assignCode);
    }
    
    //std::string
    std::vector<IRInstruction*>
    codeToMaterializeOffset(const CellId cid,
                            const PortId pid,
                            const std::string& argName,
                            const std::map<SigPort, unsigned long>& offsets) const;

    //    std::string
    std::vector<IRInstruction*>
    codeToMaterialize(const CellId cid,
                      const PortId pid,
                      const std::string& argName) const;

    // std::string
    std::vector<IRInstruction*>
    codeToMaterializePastValue(const CellId cid,
                               const PortId pid,
                               const std::string& argName) const {
      return codeToMaterializeOffset(cid, pid, argName, pastValueOffsets);
    }
    
  };


}
