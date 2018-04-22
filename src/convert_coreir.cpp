#include "convert_coreir.h"

using namespace std;
using namespace CoreIR;

#include <fstream>

namespace FlatCircuit {

  std::vector<std::pair<unsigned int, unsigned int> >
  loadBitStream(const std::string& fileName) {
    std::ifstream t(fileName);
    std::string configBits((std::istreambuf_iterator<char>(t)),
                           std::istreambuf_iterator<char>());

    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    char delimiter = '\n';
    string str = configBits;
    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
      strings.push_back(str.substr(prev, pos - prev));
      prev = pos + 1;
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.push_back(str.substr(prev));

    vector<pair<unsigned int, unsigned int> > configValues;
    cout << "Config lines" << endl;
    for (int i = 0; i < strings.size(); i++) {
      cout << strings[i] << endl;

      string addrStr = strings[i].substr(0, 8);

      unsigned int configAddr;
      std::stringstream ss;
      ss << std::hex << addrStr;
      ss >> configAddr;

      string dataStr = strings[i].substr(9, 18);

      unsigned int configData;
      std::stringstream ss2;
      ss2 << std::hex << dataStr;
      ss2 >> configData;

      configValues.push_back({configAddr, configData});
    }

    return configValues;
  }
  
  int getBitOffset(CoreIR::Select* const sel) {
    Type& tp = *(sel->getType());

    if (isa<NamedType>(&tp)) {
      assert(cast<NamedType>(&tp)->getSize() == 1);

      return 0;
    }

    assert(isBitType(tp));

    if (isNumber(sel->getSelStr())) {
      return std::stoi(sel->getSelStr());
    }

    return 0;
  }

  // Note: Like everything else in this conversion code this function assumes
  // that the select has type bit or array[bit]
  PortId getPortId(CoreIR::Select* const sel) {
    string fstPort = sel->getSelStr();

    if (isNumber(fstPort)) {
      Wireable* pr = cast<Select>(sel)->getParent();
      assert(isa<Select>(pr));
      fstPort = cast<Select>(pr)->getSelStr();

      if (isNumber(fstPort)) {
        cout << "Select failure: " << sel->toString() << ", fstPort = " << fstPort << endl;
        assert(false);
      }
      assert(!isNumber(fstPort));
    }

    // Every select off of self is driven by a port cells output port    
    if (fromSelf(sel)) {
      
      if (sel->getType()->getDir() == Type::DirKind::DK_In) {
        return PORT_ID_IN;
      } else {
        return PORT_ID_OUT;
      }

    }

    if (fstPort == "in") {
      return PORT_ID_IN;
    } else if (fstPort == "out") {
      return PORT_ID_OUT;
    } else if (fstPort == "sel") {
      return PORT_ID_SEL;
    } else if (fstPort == "clk") {
      return PORT_ID_CLK;
    } else if (fstPort == "in0") {
      return PORT_ID_IN0;
    } else if (fstPort == "in1") {
      return PORT_ID_IN1;
    } else if (fstPort == "arst") {
      return PORT_ID_ARST;
    } else if (fstPort == "raddr") {
      return PORT_ID_RADDR;
    } else if (fstPort == "rdata") {
      return PORT_ID_RDATA;
    } else if (fstPort == "waddr") {
      return PORT_ID_WADDR;
    } else if (fstPort == "wdata") {
      return PORT_ID_WDATA;
    } else if (fstPort == "wen") {
      return PORT_ID_WEN;
    } else if (fromSelf(sel)) {
      assert(false);
    }

    cout << "Unsupported port " << fstPort << " in sel " << sel->toString() << endl;
    assert(false);
  }

  CellType primitiveForMod(CoreIR::Instance* const inst) {
    string name = getQualifiedOpName(*inst);
    if (name == "coreir.wrap") {
      return CELL_TYPE_PASSTHROUGH;
    } else if ((name == "coreir.or") || (name == "corebit.or")) {
      return CELL_TYPE_OR;
    } else if (name == "coreir.orr") {
      return CELL_TYPE_ORR;
    } else if (name == "coreir.eq") {
      return CELL_TYPE_EQ;
    } else if ((name == "coreir.mux") || (name == "corebit.mux")) {
      return CELL_TYPE_MUX;
    } else if (name == "coreir.reg_arst") {
      return CELL_TYPE_REG_ARST;
    } else if (name == "coreir.const" || name == "corebit.const") {
      return CELL_TYPE_CONST;
    } else if ((name == "coreir.and") || (name == "corebit.and")) {
      return CELL_TYPE_AND;
    } else if (name == "coreir.neq") {
      return CELL_TYPE_NEQ;
    } else if (name == "coreir.ult") {
      return CELL_TYPE_ULT;
    } else if (name == "coreir.ugt") {
      return CELL_TYPE_UGT;
    } else if ((name == "coreir.not") || (name == "corebit.not")) {
      return CELL_TYPE_NOT;
    } else if (name == "coreir.reg") {
      return CELL_TYPE_REG;
    } else if (name == "coreir.ashr") {
      return CELL_TYPE_ASHR;
    } else if (name == "coreir.lshr") {
      return CELL_TYPE_LSHR;
    } else if (name == "coreir.shl") {
      return CELL_TYPE_SHL;
    } else if (name == "coreir.andr") {
      return CELL_TYPE_ANDR;
    } else if (name == "coreir.zext") {
      return CELL_TYPE_ZEXT;
    } else if (name == "coreir.add") {
      return CELL_TYPE_ADD;
    } else if (name == "coreir.sub") {
      return CELL_TYPE_SUB;
    } else if (name == "coreir.mul") {
      return CELL_TYPE_MUL;
    } else if ((name == "coreir.xor") || (name == "corebit.xor")) {
      return CELL_TYPE_XOR;
    } else if (name == "coreir.slice") {
      return CELL_TYPE_SLICE;
    } else if (name == "coreir.uge") {
      return CELL_TYPE_UGE;
    } else if (name == "coreir.ule") {
      return CELL_TYPE_ULE;
    } else if (name == "coreir.mem") {
      return CELL_TYPE_MEM;
    } else {
      cout << "Error: Unsupported module type = " << name << endl;
    }
    assert(false);
  }

  std::map<Parameter, BitVector>
  paramsForMod(const Env& e, CoreIR::Instance* const inst) {
    string name = getQualifiedOpName(*inst);

    CellType tp = primitiveForMod(inst);
    if (name == "coreir.wrap") {
      // TODO: Fix width issue
      return {{PARAM_WIDTH, BitVector(32, 1)}};
    } else if (tp == CELL_TYPE_SLICE) {

      int width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();
      int lo = inst->getModuleRef()->getGenArgs().at("lo")->get<int>();
      int hi = inst->getModuleRef()->getGenArgs().at("hi")->get<int>();

      return {{PARAM_WIDTH, BitVector(32, width)},
          {PARAM_LOW, BitVector(32, lo)},
            {PARAM_HIGH, BitVector(32, hi)}};

    } else if (tp == CELL_TYPE_ZEXT) {

      int width_in = inst->getModuleRef()->getGenArgs().at("width_in")->get<int>();
      int width_out = inst->getModuleRef()->getGenArgs().at("width_out")->get<int>();

      return {{PARAM_IN_WIDTH, BitVector(32, width_in)},
          {PARAM_OUT_WIDTH, BitVector(32, width_out)}};
      
    } else if (isUnop(tp) || isBinop(tp) || (tp == CELL_TYPE_MUX)) {
      //cout << "Processing instance " << inst->toString() << endl;

      int width = 1;
      if (inst->getModuleRef()->getNamespace()->getName() == "coreir") {
        width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();
      } else {
        assert(inst->getModuleRef()->getNamespace()->getName() == "corebit");
      }

      //cout << "Done processing instance " << inst->toString() << endl;

      return {{PARAM_WIDTH, BitVector(32, width)}};

    } else if (name == "coreir.reg_arst") {

      int width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();

      //cout << "Creating reg_arst" << endl;
      bool rstPos = inst->getModArgs().at("arst_posedge")->get<bool>();
      bool clkPos = inst->getModArgs().at("clk_posedge")->get<bool>();
      BitVector init =
        inst->getModArgs().at("init")->get<BitVector>();      

      //cout << "Done creating reg_arst params" << endl;
      return {{PARAM_WIDTH, BitVector(32, width)},
          {PARAM_INIT_VALUE, init},
            {PARAM_CLK_POSEDGE, BitVector(1, clkPos)},
              {PARAM_ARST_POSEDGE, BitVector(1, rstPos)}};

    } else if (name == "coreir.reg") {
      int width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();

      //cout << "Creating reg_arst" << endl;
      bool clkPos = inst->getModArgs().at("clk_posedge")->get<bool>();
      BitVector init =
        inst->getModArgs().at("init")->get<BitVector>();      

      return {{PARAM_WIDTH, BitVector(32, width)},
          {PARAM_INIT_VALUE, init},
            {PARAM_CLK_POSEDGE, BitVector(1, clkPos)}};
      
    } else if (name == "coreir.const" || name == "corebit.const") {

      int width = 1;
      if (name == "coreir.const") {
        width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();
      }

      BitVector init;

      if (name == "coreir.const") {
        init = inst->getModArgs().at("value")->get<BitVector>();
      } else {
        bool val = inst->getModArgs().at("value")->get<bool>();
        init = BitVector(1, val == true);
      }
      
      return {{PARAM_WIDTH, BitVector(32, width)}, {PARAM_INIT_VALUE, init}};

    } else if (tp == CELL_TYPE_MEM) {
      BitVector depth =
        BitVector(32, inst->getModuleRef()->getGenArgs().at("depth")->get<int>());
      BitVector width =
        BitVector(32, inst->getModuleRef()->getGenArgs().at("width")->get<int>());

      bool hasInit = inst->getModuleRef()->getGenArgs().at("has_init")->get<bool>();
      map<Parameter, BitVector> params{{PARAM_MEM_WIDTH, width},
          {PARAM_MEM_DEPTH, depth},
            {PARAM_HAS_INIT, BitVector(1, hasInit)}};
      if (hasInit) {
        BitVector init = inst->getModArgs().at("init")->get<BitVector>();
        params.insert({PARAM_HAS_INIT, init});
      }
      return params;
    } else {
      cout << "Error: Unsupported parameters module type = " << name << endl;
    }
    assert(false);
  }

  Env convertFromCoreIR(CoreIR::Context* const c,
                         CoreIR::Module* const top) {
    Env e;

    if (!top->hasDef()) {
      return e;
    }

    CellType topType = e.addCellType(top->getName());
    auto& cDef = e.getDef(topType);

    //map<Instance*, CellId> instsToCells;
    map<Wireable*, CellId> elemsToCells;
    
    for (auto r : top->getType()->getRecord()) {
      auto field = r.first;
      auto tp = r.second;

      PortType portTp = tp->getDir() == Type::DirKind::DK_In ? PORT_TYPE_IN : PORT_TYPE_OUT;

      assert((tp->getDir() == Type::DirKind::DK_Out) ||
             (tp->getDir() == Type::DirKind::DK_In));

      int portWidth = 1;

      if (isa<ArrayType>(tp)) {
        ArrayType* artp = cast<ArrayType>(tp);

        assert(isBitType(*(artp->getElemType())));
        
        portWidth = artp->getLen();
      } else if (isBitType(*tp)) {
        portWidth = 1;
      } else if (isa<NamedType>(tp)) {
        NamedType* ntp = cast<NamedType>(tp);

        assert(ntp->getSize() == 1);

        portWidth = ntp->getSize();
      } else {
        cout << "Field " << field << " has unsupported type " << tp->toString() << endl;
        assert(false);
      }

      cDef.addPort(field, portWidth, portTp);
      CellId c = cDef.getPortCellId(field);
      elemsToCells.insert({top->getDef()->sel("self")->sel(field), c});
    }


    for (auto instR : top->getDef()->getInstances()) {
      Instance* inst = instR.second;

      //cout << "Adding instance " << inst->toString() << endl;
      
      Module* instMod = inst->getModuleRef();

      // Only handle primitives for now
      assert(!instMod->hasDef());

      CellType instType = primitiveForMod(inst);
      map<Parameter, BitVector> params = paramsForMod(e, inst);

      CellId cid = cDef.addCell(inst->toString(), instType, params);
      elemsToCells.insert({inst, cid});
      //cout << "Added instance " << inst->toString() << endl;
    }

    cout << "Added all instances" << endl;

    for (auto conn : top->getDef()->getConnections()) {
      Wireable* fst = conn.first;
      Wireable* snd = conn.second;

      assert(isa<Select>(fst));
      assert(isa<Select>(snd));

      Wireable* fstSrc = extractSource(cast<Select>(fst));
      Wireable* sndSrc = extractSource(cast<Select>(snd));

      assert(dbhc::contains_key(fstSrc, elemsToCells));
      assert(dbhc::contains_key(sndSrc, elemsToCells));

      //Cell& fstCell = cDef.getCellRef(elemsToCells.at(fstSrc));
      //Cell& sndCell = cDef.getCellRef(elemsToCells.at(sndSrc));

      CellId driverId;
      CellId receiverId;

      Select* driverSel;
      Select* receiverSel;

      if (fst->getType()->getDir() == Type::DirKind::DK_Out) {
        driverSel = cast<Select>(fst);
        receiverSel = cast<Select>(snd);
        
        driverId = elemsToCells.at(fstSrc);
        receiverId = elemsToCells.at(sndSrc);
      } else {
        driverSel = cast<Select>(snd);
        receiverSel = cast<Select>(fst);
        
        driverId = elemsToCells.at(sndSrc);
        receiverId = elemsToCells.at(fstSrc);
      }

      Cell& driver = cDef.getCellRef(driverId);
      Cell& receiver = cDef.getCellRef(receiverId);

      // Cases:
      // Bit by bit connection
      //   - Each bit could be: bit select off array or bit output
      // Array by array connection

      // Port on driver driving connectoin
      PortId driverPort = getPortId(driverSel);

      // Port on receiver receiving connection
      PortId receiverPort = getPortId(receiverSel);

      // cout << "Adding drivers for" << endl;
      // cout << "\tDriver           : " << driverSel->toString() << endl;
      // cout << "\tReceiverSel      : " << receiverSel->toString() << endl;

      // cout << "\tDriver Cell      : " << driverId << endl;
      // cout << "\tReceiver Cell    : " << receiverId << endl;

      // cout << "\tDriver Port      : " << driverPort << endl;
      // cout << "\tReceiver Port    : " << receiverPort << endl;

      // NOTE: Assuming named types are clk or reset
      bool isBitConn = isBitType(*(driverSel->getType())) || isa<NamedType>(driverSel->getType());
      if (isBitConn) {
        assert(isBitType(*(receiverSel->getType())) || isa<NamedType>(receiverSel->getType()));

        // TODO: Compute real connection offsets
        int driverOffset = getBitOffset(driverSel);
        int receiverOffset = getBitOffset(receiverSel);

        SignalBit driverBit{driverId, driverPort, driverOffset};
        SignalBit receiverBit{receiverId, receiverPort, receiverOffset};

        // Connection types: Bit to bit, or array of bits to array of bits

        receiver.setDriver(receiverPort, receiverOffset, driverBit);

        //cout << "Set driver on receiver port" << endl;

        driver.addReceiver(driverPort, driverOffset, receiverBit);

        //cout << "Done adding drivers" << endl;
      } else {
        assert(isBitArray(*(driverSel->getType())));
        assert(isBitArray(*(receiverSel->getType())));

        int width = driver.getPortWidth(driverPort);
        for (int i = 0; i < width; i++) {

          SignalBit driverBit{driverId, driverPort, i};
          SignalBit receiverBit{receiverId, receiverPort, i};

          receiver.setDriver(receiverPort, i, driverBit);
          driver.addReceiver(driverPort, i, receiverBit);
          
        }

        // cout << "After connecting" << endl;
        // for (auto sigBit : receiver.getDrivers(receiverPort).signals) {
        //   cout << "\t" << toString(sigBit) << endl;
        // }
        
      }
    }
    
    return e;
  }

  Env loadFromCoreIR(const std::string& topName,
                     const std::string& fileName) {
    Context* c = newContext();
    //Namespace* g = c->getGlobal();

    CoreIRLoadLibrary_rtlil(c);

    Module* top;
    //if (!loadFromFile(c,"./test/cb_unq1.json", &top)) {
    if (!loadFromFile(c, fileName, &top)) {
      cout << "Could not Load from json!!" << endl;
      c->die();
    }

    top = c->getModule(topName); //("global.cb_unq1");

    assert(top != nullptr);

    c->runPasses({"rungenerators", "split-inouts","delete-unused-inouts","deletedeadinstances","add-dummy-inputs", "packconnections", "removeconstduplicates", "flatten", "cullzexts", "removeconstduplicates"});

    Env circuitEnv = convertFromCoreIR(c, top);

    deleteContext(c);
    
    return circuitEnv;
  }

  
}
