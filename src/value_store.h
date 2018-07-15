#pragma once

#include <map>

#include "circuit.h"
#include "cpp_utils.h"

namespace FlatCircuit {

  class IRInstruction;

  class QBitTable {

    std::vector<unsigned char> simValueTable;
    std::vector<unsigned char> bitMaskTable;
    std::vector<unsigned char> zMaskTable;

  public:

    BitVector getBitVector(const unsigned long offset,
                           const unsigned long width) const {
      BitVector bv(width, 0);
      unsigned long byteWidth = storedByteLength(width);

      unsigned long bvOffset = 0;

      for (unsigned long i = 0; i < byteWidth; i++) {
        unsigned long byteOffset = offset + i;
        for (unsigned long bitOffset = 0; bitOffset < 8; bitOffset++) {

          if (bvOffset >= (unsigned long) bv.bitLength()) {
            break;
          }
          assert(byteOffset < simValueTable.size());

          bv.set(bvOffset,
                 bsim::quad_value((simValueTable.at(byteOffset) >> bitOffset) & 0x01));

          bool isX = (bitMaskTable.at(byteOffset) >> bitOffset) & 0x01;
          if (isX) {
            bv.set(bvOffset, bsim::quad_value(QBV_UNKNOWN_VALUE));
          }

          bool isz = (zMaskTable.at(byteOffset) >> bitOffset) & 0x01;
          if (isZ) {
            bv.set(bvOffset, bsim::quad_value(QBV_HIGH_IMPEDANCE));
          }

          bvOffset += 1;
        }

      }

      return bv;
    }

    std::pair<unsigned long, unsigned long>
    bitOffsetInBytes(const unsigned long bitOffset) {
      return {(bitOffset / 8), bitOffset % 8};
    }

    void setBitVector(const unsigned long offset,
                      const BitVector& bv) {

      //std::cout << "setBitVector at " << offset << " to " << bv << std::endl;
      for (unsigned long i = 0; i < (unsigned long) bv.bitLength(); i++) {

        std::pair<unsigned long, unsigned long> bitOffset =
          bitOffsetInBytes(i);
        if (bv.get(i).is_binary()) {

          simValueTable[offset + bitOffset.first] ^=
            (-bv.get(i).binary_value() ^ simValueTable[offset + bitOffset.first]) & (1UL << bitOffset.second);

          bitMaskTable[offset + bitOffset.first] &=
            ~(0x00 | (1 << bitOffset.second));

          zMaskTable[offset + bitOffset.first] &=
            ~(0x00 | (1 << bitOffset.second));
          
        } else if (bv.get(i).is_high_impedance()) {

          bitMaskTable[offset + bitOffset.first] &=
            ~(0x00 | (1 << bitOffset.second));
          
          zMaskTable[offset + bitOffset.first] |=
            0 | (1 << bitOffset.second);

        } else {
          bitMaskTable[offset + bitOffset.first] |=
            0 | (1 << bitOffset.second);

          zMaskTable[offset + bitOffset.first] &=
            ~(0x00 | (1 << bitOffset.second));
        }
      }
      //std::cout << "done setting" << std::endl;
      
    }
    
    unsigned long addBitVector(const unsigned long width) {
      BitVector bv = bsim::unknown_bv(width);
      auto nextInd = addBitVector(bv);
      return nextInd;
    }

    unsigned long addBitVector(const BitVector& bv) {

      unsigned long nextInd = simValueTable.size();

      for (unsigned long i = 0;
           i < (unsigned long) storedByteLength(bv.bitLength());
           i++) {
        simValueTable.push_back(0);
        bitMaskTable.push_back(0);
      }

      setBitVector(nextInd, bv);

      // for (unsigned long i = 0; i < (unsigned long) bv.bitLength(); i++) {
      //   simValueTable.push_back(bv.get(i));
      // }

      return nextInd;
    }
    
    unsigned long size() const {
      return simValueTable.size();
    }

    std::vector<unsigned char>& getValueVector() {
      return simValueTable;
    }

    std::vector<unsigned char>& getXMaskVector() {
      return bitMaskTable;
    }

    void debugPrintTableValues() const {
      for (int i = 0; i < (int) bitMaskTable.size(); i++) {
        std::cout << "\tvalue[" << i << "] = " << (int) simValueTable[i] << std::endl;
        std::cout << "\tmask  [" << i << "] = " << (int) bitMaskTable[i] << std::endl;
      }
    }
    
    // std::vector<bsim::quad_value>& getValueVector() {
    //   return simValueTable;
    // }
    
  };

  class ValueStore {

    QBitTable simValueTable;

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

    ValueStore(CellDefinition& def_);

    unsigned char debugGetRawTable(const int index) {
      assert(compiledRaw);
      return rawSimValueTable[index];
    }

    BitVector simTableValue(const CellId cid, const PortId pid) const {
      return simValueTable.getBitVector(map_find({cid, pid}, portOffsets),
                                        def.getCellRefConst(cid).getPortWidth(pid));
    }

    void setCompiledRaw() {
      compiledRaw = true;

      // Copy over register, memory, and constants to the raw buffer
      for (auto ctp : def.getCellMap()) {
        CellId cid = ctp.first;
        const Cell& cell = def.getCellRefConst(cid);
        if (cell.getCellType() == CELL_TYPE_CONST) {
          setPortValue(cid,
                       PORT_ID_OUT,
                       simTableValue(cid, PORT_ID_OUT));
        } else if (isRegister(cell.getCellType())) {
          // TODO: Need to add register support
          setRegisterValue(cid,
                           simTableValue(cid, PORT_ID_OUT));

          setPortValue(cid,
                       PORT_ID_OUT,
                       simTableValue(cid, PORT_ID_OUT));
        }
      }

    }

    void buildRawValueTable() {
      // unsigned long rawOffset = 0;
      // std::map<unsigned long, unsigned long> quadOffsetsToRawOffsets;
      // for (unsigned long i = 0; i < simValueTable.size(); i++) {
      //   quadOffsetsToRawOffsets[i] = rawOffset;
      //   // Adding extra buffer space
      //   rawOffset += storedByteLength(simValueTable.bitVectorAt(i).bitLength()) + 1;
      // }

      //rawTableSize = rawOffset;

      unsigned long rawTableSize = tableSize();

      // std::cout << "Raw table offset map" << std::endl;
      // for (auto ent : quadOffsetsToRawOffsets) {
      //   std::cout << "\t" << ent.first << " --> " << ent.second << std::endl;
      // }

      for (auto sp : portOffsets) {
        rawPortOffsets[sp.first] = sp.second; //map_find(sp.second, quadOffsetsToRawOffsets);
        // std::cout << "Offset for port = " <<
        //   rawMemoryOffsets[sp.first] << std::endl;
      }

      for (auto sp : pastValueOffsets) {
        rawPastValueOffsets[sp.first] = sp.second; //map_find(sp.second, quadOffsetsToRawOffsets);
      }

      for (auto sp : memoryOffsets) {
        rawMemoryOffsets[sp.first] = sp.second; //map_find(sp.second, quadOffsetsToRawOffsets);
        // std::cout << "Offset for memory = " <<
        //   rawMemoryOffsets[sp.first] << std::endl;
      }

      for (auto sp : registerOffsets) {
        rawRegisterOffsets[sp.first] = sp.second; //map_find(sp.second, quadOffsetsToRawOffsets);
      }

      rawSimValueTable =
        static_cast<unsigned char*>(malloc(rawTableSize));
      memset(rawSimValueTable, 0, rawTableSize);
      
    }

    void debugPrintTableValues() const;

    //std::vector<bsim::quad_value>& getValueTable() { return simValueTable.getValueVector(); }

    std::vector<unsigned char>& getValueTable() { return simValueTable.getValueVector(); }
    std::vector<unsigned char>& getXMaskTable() { return simValueTable.getXMaskVector(); }

    unsigned char* getRawValueTable() { return rawSimValueTable; }

    unsigned long getMemoryOffset(const CellId cid) const {
      return map_find(cid, memoryOffsets);
    }

    unsigned long getRawMemoryOffset(const CellId cid) const {
      return map_find(cid, rawMemoryOffsets);
    }
    
    unsigned long getRegisterOffset(const CellId cid) const {
      return map_find(cid, registerOffsets);
    }

    unsigned long getRawRegisterOffset(const CellId cid) const {
      return map_find(cid, rawRegisterOffsets);
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

      return map_find({cid, pid}, rawPastValueOffsets);
    }
    
    unsigned long portValueOffset(const CellId cid,
                                  const PortId pid) {
      if (!contains_key({cid, pid}, portOffsets)) {
        std::cout << "Error: " << sigPortString(def, {cid, pid}) << " has no offset" << std::endl;
        assert(false);
      }

      return map_find({cid, pid}, portOffsets);
    }

    BitVector getMemoryValue(const CellId cid,
                             const int offset) const {
      assert(offset >= 0);

      int memWidth = def.getCellRefConst(cid).getMemWidth();
      return simValueTable.getBitVector(map_find(cid, memoryOffsets) + ((unsigned long) storedByteLength(memWidth)*offset), memWidth);

    }

    void setMemoryValue(const CellId cid,
                        const int addr,
                        const BitVector& writeData) {

      assert(contains_key(cid, memoryOffsets));
      assert(addr >= 0);

      int memWidth = def.getCellRefConst(cid).getMemWidth();
      simValueTable.setBitVector(map_find(cid, memoryOffsets) + ((unsigned long) storedByteLength(memWidth)*addr), writeData);
    }

    void debugPrintRawValueTable() const;

    void setPortValueTwoState(const CellId cid,
                              const PortId pid,
                              const int width,
                              const unsigned long value) {
      assert(compiledRaw);

      int pWidth = def.getCellRefConst(cid).getPortWidth(pid);

      assert(pWidth == width);

      if (width <= 8) {
        *((uint8_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
          (uint8_t) value;
      } else if (width <= 16) {
        *((uint16_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
          (uint16_t) value;
      } else if (width <= 32) {
        *((uint32_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
          (uint32_t) value;
      } else if (width <= 64) {
        *((uint64_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
          (uint64_t) value;
      } else {
        assert(false);
      }
      
    }

    void resetPortValue(const CellId cid,
                      const PortId pid,
                      const BitVector& bv) {
      if (!contains_key({cid, pid}, portOffsets)) {
        unsigned long nextInd = simValueTable.size();
        portOffsets[{cid, pid}] = nextInd;
        simValueTable.addBitVector(bv);
        //simValueTable.push_back(bv);
      } else {
        simValueTable.setBitVector(map_find({cid, pid}, portOffsets), bv);
        //simValueTable[map_find({cid, pid}, portOffsets)] = bv;
      }
    }

    void resetPastValue(const CellId cid,
                        const PortId pid,
                        const BitVector& bv) {
      if (!contains_key({cid, pid}, pastValueOffsets)) {
        unsigned long nextInd = simValueTable.size();
        pastValueOffsets[{cid, pid}] = nextInd;
        //simValueTable.push_back(bv);
        simValueTable.addBitVector(bv);
      } else {
        simValueTable.setBitVector(map_find({cid, pid}, pastValueOffsets), bv);
        //simValueTable[map_find({cid, pid}, pastValueOffsets)] = bv;
      }
    }
    
    void setPortValue(const CellId cid,
                      const PortId pid,
                      const BitVector& bv) {

      if (compiledRaw) {
        //        std::cout << "Setting raw offset " << map_find({cid, pid}, rawPortOffsets) << " with bit vector " << bv << " with bit length " << bv.bitLength() << std::endl;
        int pWidth = def.getCellRefConst(cid).getPortWidth(pid);

        assert(pWidth <= 64);

        bool isBinary = bv.is_binary();

        if (isBinary) {
          if (bv.bitLength() <= 8) {
            *((uint8_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
              (uint8_t) bv.to_type<uint8_t>();
          } else if (bv.bitLength() <= 16) {
            //          std::cout << "Setting " << sigPortString(def, {cid, pid}) << " to " << bv.to_type<uint16_t>() << std::endl;
            *((uint16_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
              (uint16_t) bv.to_type<uint16_t>();

            //debugPrintRawValueTable();
          } else if (bv.bitLength() <= 32) {
            *((uint32_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
              (uint32_t) bv.to_type<uint32_t>();
          } else if (bv.bitLength() <= 64) {
            *((uint64_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
              (uint64_t) bv.to_type<uint64_t>();
          } else {
            assert(false);
          }
        } else {
          if (bv.bitLength() <= 8) {
            *((uint8_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
              (uint8_t) 0;
          } else if (bv.bitLength() <= 16) {
            //          std::cout << "Setting " << sigPortString(def, {cid, pid}) << " to " << bv.to_type<uint16_t>() << std::endl;
            *((uint16_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
              (uint16_t) 0;
            //debugPrintRawValueTable();
          } else if (bv.bitLength() <= 32) {
            *((uint32_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
              (uint32_t) 0;
          } else if (bv.bitLength() <= 64) {
            *((uint64_t*) (rawSimValueTable + map_find({cid, pid}, rawPortOffsets))) =
              (uint64_t) 0;
          } else {
            assert(false);
          }
          
        }
      } else {

        if (!contains_key({cid, pid}, portOffsets)) {
          std::cout << "Error: " << sigPortString(def, {cid, pid}) << " has no offset" << std::endl;
          assert(false);
        }

        simValueTable.setBitVector(map_find({cid, pid}, portOffsets), bv);
        //simValueTable[map_find({cid, pid}, portOffsets)] = bv;
      }
    }

    BitVector getPortValue(const CellId cid,
                           const PortId pid) const {
      if (!compiledRaw) {
        //std::cout << "Getting port value for " << sigPortString(def, {cid, pid}) << std::endl;
        return simTableValue(cid, pid);
        //return simValueTable[map_find({cid, pid}, portOffsets)];
      } else {
        int pWidth = def.getCellRefConst(cid).getPortWidth(pid);

        assert(pWidth <= 64);

        if (pWidth <= 8) {
          return BitVector(pWidth,
                           *((uint8_t*)
                             (rawSimValueTable +
                              map_find({cid, pid}, rawPortOffsets))));
        } else if (pWidth <= 16) {
          return BitVector(pWidth,
                           *((uint16_t*)
                             (rawSimValueTable +
                              map_find({cid, pid}, rawPortOffsets))));

        } else if (pWidth <= 32) {
          return BitVector(pWidth,
                           *((uint32_t*)
                             (rawSimValueTable +
                              map_find({cid, pid}, rawPortOffsets))));

        } else if (pWidth <= 64) {
          return BitVector(pWidth,
                           *((uint64_t*)
                             (rawSimValueTable +
                              map_find({cid, pid}, rawPortOffsets))));
        } else {
          assert(false);
        }
      }
    }

    void setRegisterValue(const CellId cid,
                          const BitVector& bv) {
      if (!compiledRaw) {
        assert(contains_key(cid, registerOffsets));

        //simValueTable[map_find(cid, registerOffsets)] = bv;
        simValueTable.setBitVector(map_find(cid, registerOffsets), bv);

      } else {

        bool isBinary = bv.is_binary();
        
        int pWidth =
          bvToInt(def.getCellRefConst(cid).getParameterValue(PARAM_WIDTH));

        assert(pWidth <= 64);

        if (isBinary) {
          if (bv.bitLength() <= 8) {
            *((uint8_t*) (rawSimValueTable + map_find(cid, rawRegisterOffsets))) =
              (uint8_t) bv.to_type<uint8_t>();
          } else if (bv.bitLength() <= 16) {
            *((uint16_t*) (rawSimValueTable + map_find(cid, rawRegisterOffsets))) =
              (uint16_t) bv.to_type<uint16_t>();
          } else if (bv.bitLength() <= 32) {
            *((uint32_t*) (rawSimValueTable + map_find(cid, rawRegisterOffsets))) =
              (uint32_t) bv.to_type<uint32_t>();
          } else if (bv.bitLength() <= 64) {
            *((uint64_t*) (rawSimValueTable + map_find(cid, rawRegisterOffsets))) =
              (uint64_t) bv.to_type<uint64_t>();
          } else {
            assert(false);
          }
        } else {
          if (bv.bitLength() <= 8) {
            *((uint8_t*) (rawSimValueTable + map_find(cid, rawRegisterOffsets))) =
              (uint8_t) 0;
          } else if (bv.bitLength() <= 16) {
            *((uint16_t*) (rawSimValueTable + map_find(cid, rawRegisterOffsets))) =
              (uint16_t) 0;
          } else if (bv.bitLength() <= 32) {
            *((uint32_t*) (rawSimValueTable + map_find(cid, rawRegisterOffsets))) =
              (uint32_t) 0;
          } else if (bv.bitLength() <= 64) {
            *((uint64_t*) (rawSimValueTable + map_find(cid, rawRegisterOffsets))) =
              (uint64_t) 0;
          } else {
            assert(false);
          }
        }
      }
    }

    BitVector getRegisterValue(const CellId cid) const {
      //return simValueTable[map_find(cid, registerOffsets)];
      return simValueTable.getBitVector(map_find(cid, registerOffsets),
                                        def.getCellRefConst(cid).getPortWidth(PORT_ID_OUT));
    }

    void setPastValue(const CellId cid,
                      const PortId pid,
                      const BitVector& bv) {
      if (!compiledRaw) {
        unsigned long offset = pastValueOffset(cid, pid);
        simValueTable.setBitVector(offset, bv);
        //simValueTable[offset] = bv;
      } else {
        int pWidth = def.getCellRefConst(cid).getPortWidth(pid);

        assert(pWidth <= 64);

        if (bv.bitLength() <= 8) {
          *((uint8_t*) (rawSimValueTable + map_find({cid, pid}, rawPastValueOffsets))) =
            (uint8_t) bv.to_type<uint8_t>();
        } else if (bv.bitLength() <= 16) {
          //          std::cout << "Setting " << sigPortString(def, {cid, pid}) << " to " << bv.to_type<uint16_t>() << std::endl;
          *((uint16_t*) (rawSimValueTable + map_find({cid, pid}, rawPastValueOffsets))) =
            (uint16_t) bv.to_type<uint16_t>();

          //debugPrintRawValueTable();
        } else if (bv.bitLength() <= 32) {
          *((uint32_t*) (rawSimValueTable + map_find({cid, pid}, rawPastValueOffsets))) =
            (uint32_t) bv.to_type<uint32_t>();
        } else if (bv.bitLength() <= 64) {
          *((uint64_t*) (rawSimValueTable + map_find({cid, pid}, rawPastValueOffsets))) =
            (uint64_t) bv.to_type<uint64_t>();
        } else {
          assert(false);
        }


      }
    }

    BitVector getPastValue(const CellId cid,
                           const PortId pid) {

      return materializePastValue({cid, pid});
    }

    unsigned long pastValueOffset(const CellId cid,
                                  const PortId pid) {
      const Cell& cell = def.getCellRefConst(cid);

      if (cell.getPortType(pid) == PORT_TYPE_OUT) {

        if (!contains_key({cid, pid}, pastValueOffsets)) {
          std::cout << "Error: " << sigPortString(def, {cid, pid}) << " has no past value offset" << std::endl;
          assert(false);
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

    BitVector materializeInput(const SigPort sigPort) const {
      int width = def.getCellRefConst(sigPort.cell).getPortWidth(sigPort.port);

      BitVector val(width, 0);

      auto& sigBus = def.getCellRef(sigPort.cell).getDrivers(sigPort.port);

      assert(((int) sigBus.signals.size()) == width);

      for (int i = 0; i < (int) sigBus.signals.size(); i++) {
        SignalBit b = sigBus.signals.at(i);

        assert(notEmpty(b));

        val.set(i, getPortValue(b.cell, b.port).get(b.offset));
      }
        
      return val;
    }

    BitVector materializePastValue(const SigPort sigPort) const {
      if (contains_key(sigPort, pastValueOffsets)) {
          return simValueTable.getBitVector(map_find(sigPort, pastValueOffsets),
                                            def.getCellRefConst(sigPort.cell).getPortWidth(sigPort.port));
      } else {
        int width = def.getCellRefConst(sigPort.cell).getPortWidth(sigPort.port);

        BitVector val(width, 0);

        auto& sigBus = def.getCellRef(sigPort.cell).getDrivers(sigPort.port);

        assert(((int) sigBus.signals.size()) == width);

        for (int i = 0; i < (int) sigBus.signals.size(); i++) {
          SignalBit b = sigBus.signals.at(i);

          assert(notEmpty(b));

          auto driverBV =
            simValueTable.getBitVector(map_find({b.cell, b.port},
                                                pastValueOffsets),
                                       def.getCellRefConst(b.cell).getPortWidth(b.port));
          
          val.set(i, driverBV.get(b.offset));
        }
        
        return val;
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

    unsigned long tableSize() const {
      return simValueTable.size();
    }

    ~ValueStore() {
      if (compiledRaw) {
        free(rawSimValueTable);
      }
    }

  };


}
