#pragma once

#include <map>

#include "circuit.h"
#include "cpp_utils.h"

namespace FlatCircuit {

  class IRInstruction;

  class ValueStore {
    std::vector<BitVector> simValueTable;

    std::map<SigPort, unsigned long> portOffsets;
    std::map<CellId, unsigned long> memoryOffsets;
    std::map<CellId, unsigned long> registerOffsets;    
    std::map<SigPort, unsigned long> pastValueOffsets;

    // Raw (2 state) table entries
    unsigned char* rawSimValueTable;
    unsigned long rawTableSize;

    std::map<SigPort, unsigned long> rawPortOffsets;
    std::map<CellId, unsigned long> rawMemoryOffsets;
    std::map<CellId, unsigned long> rawRegisterOffsets;
    std::map<SigPort, unsigned long> rawPastValueOffsets;

    bool compiledRaw;
    
  public:
    CellDefinition& def;

    ValueStore(CellDefinition& def_) : compiledRaw(false), def(def_) {}

    void buildRawValueTable() {
      unsigned long rawOffset = 0;
      std::map<unsigned long, unsigned long> quadOffsetsToRawOffsets;
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

      compiledRaw = true;
      rawSimValueTable =
        static_cast<unsigned char*>(malloc(rawOffset));
      //rawSimValueTable[0] = 23;
      
    }

    void debugPrintTableValues() const;

    std::vector<BitVector>& getValueTable() { return simValueTable; }
    unsigned char* getRawValueTable() { return rawSimValueTable; }

    unsigned long getMemoryOffset(const CellId cid) const {
      return map_find(cid, memoryOffsets);
    }

    unsigned long getRegisterOffset(const CellId cid) const {
      return map_find(cid, registerOffsets);
    }

    unsigned long rawPortValueOffset(const CellId cid,
                                     const PortId pid) {
      assert(contains_key({cid, pid}, portOffsets));
      assert(contains_key({cid, pid}, rawPortOffsets));

      return map_find({cid, pid}, rawPortOffsets);
    }

    unsigned long rawPortPastValueOffset(const CellId cid,
                                         const PortId pid) {
      assert(contains_key({cid, pid}, pastValueOffsets));
      assert(contains_key({cid, pid}, rawPastValueOffsets));

      return map_find({cid, pid}, pastValueOffsets);
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

      if (compiledRaw) {
        int pWidth = def.getCellRefConst(cid).getPortWidth(pid);

        assert(pWidth <= 64);

        if (bv.bitLength() <= 8) {
          rawSimValueTable[map_find({cid, pid}, rawPortOffsets)] =
            (uint8_t) bv.to_type<uint8_t>();
        } else if (bv.bitLength() <= 16) {
          rawSimValueTable[map_find({cid, pid}, rawPortOffsets)] =
            (uint16_t) bv.to_type<uint16_t>();
        } else if (bv.bitLength() <= 32) {
          rawSimValueTable[map_find({cid, pid}, rawPortOffsets)] =
            (uint32_t) bv.to_type<uint32_t>();
        } else if (bv.bitLength() <= 64) {
          rawSimValueTable[map_find({cid, pid}, rawPortOffsets)] =
            (uint64_t) bv.to_type<uint64_t>();
        } else {
          assert(false);
        }

      } else {

        if (!contains_key({cid, pid}, portOffsets)) {
          unsigned long nextInd = simValueTable.size();
          portOffsets[{cid, pid}] = nextInd;
          simValueTable.push_back(bv);
        }

        simValueTable[map_find({cid, pid}, portOffsets)] = bv;
      }
    }

    BitVector getPortValue(const CellId cid,
                           const PortId pid) const {
      if (!compiledRaw) {
        return simValueTable[map_find({cid, pid}, portOffsets)];
      } else {
        int pWidth = def.getCellRefConst(cid).getPortWidth(pid);

        assert(pWidth <= 64);

        return BitVector(pWidth,
                         rawSimValueTable[map_find({cid, pid}, rawPortOffsets)]);
      }
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

    IRInstruction* codeToAssignRegister(const CellId cid,
                                        const std::string& assignCode);
    
    std::vector<IRInstruction*>
    codeToMaterializeOffset(const CellId cid,
                            const PortId pid,
                            const std::string& argName,
                            const std::map<SigPort, unsigned long>& offsets,
                            const bool isPast) const;

    std::vector<IRInstruction*>
    codeToMaterialize(const CellId cid,
                      const PortId pid,
                      const std::string& argName) const;

    std::vector<IRInstruction*>
    codeToMaterializePastValue(const CellId cid,
                               const PortId pid,
                               const std::string& argName) const {
      return codeToMaterializeOffset(cid, pid, argName, pastValueOffsets, true);
    }

    ~ValueStore() {
      if (compiledRaw) {
        free(rawSimValueTable);
      }
    }
    
  };


}
