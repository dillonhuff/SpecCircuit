#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include "coreir.h"
#include "coreir/libs/rtlil.h"
#include "coreir/libs/commonlib.h"

#include "simulator.h"
#include "circuit.h"

using namespace std;
using namespace CoreIR;

namespace FlatCircuit {

  std::vector<std::pair<unsigned int, unsigned int> >
  loadBitStream(const std::string& fileName) {
    //std::ifstream t("./test/hwmaster_pw2_sixteen.bsa");
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
    } else if (fromSelf(sel)) {
      assert(false);

    }

    cout << "Unsupported port " << fstPort << " in sel " << sel->toString() << endl;
    assert(false);
  }

  //CellType primitiveForMod(const Env& e, CoreIR::Instance* const inst) {
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

      Cell& fstCell = cDef.getCellRef(elemsToCells.at(fstSrc));
      Cell& sndCell = cDef.getCellRef(elemsToCells.at(sndSrc));

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

  // Need to build
  TEST_CASE("Loading cgra") {

    Context* c = newContext();
    Namespace* g = c->getGlobal();

    CoreIRLoadLibrary_rtlil(c);

    Module* top;
    if (!loadFromFile(c,"../EventSim/test/top.json", &top)) {
      cout << "Could not Load from json!!" << endl;
      c->die();
    }

    top = c->getModule("global.top");

    assert(top != nullptr);

    c->runPasses({"rungenerators", "split-inouts","delete-unused-inouts","deletedeadinstances","add-dummy-inputs", "packconnections", "removeconstduplicates", "flatten", "cullzexts", "removeconstduplicates"});

    Env circuitEnv = convertFromCoreIR(c, top);

    CellDefinition& def = circuitEnv.getDef(top->getName());

    Simulator sim(circuitEnv, def);
    sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 0));
    sim.update();
    sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 1));
    sim.update();
    sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 0));
    sim.update();

    sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 1));
    sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 3));
    sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));

    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
    sim.update();
    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 1));
    sim.update();
    
    sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 0));
    sim.setFreshValue("in_3", PORT_ID_OUT, BitVec(16, 239));

    sim.update();

    REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 239));
    
    
    deleteContext(c);

  }

  TEST_CASE("Simulating a mux loop") {

    Context* c = newContext();
    Namespace* g = c->getGlobal();

    uint width = 2;

    Type* twoMuxType =
      c->Record({
          {"in", c->BitIn()->Arr(width)},
            {"sel", c->BitIn()},
              {"out", c->Bit()->Arr(width)}
        });

    Module* twoMux = c->getGlobal()->newModuleDecl("twoMux", twoMuxType);
    ModuleDef* def = twoMux->newModuleDef();

    def->addInstance("mux0",
                     "coreir.mux",
                     {{"width", Const::make(c, width)}});

    def->connect("self.sel", "mux0.sel");
    def->connect("self.in", "mux0.in0");
    def->connect("mux0.out", "mux0.in1");
    def->connect("mux0.out", "self.out");

    twoMux->setDef(def);

    cout << "Creating twoMux simulation" << endl;

    c->runPasses({"rungenerators", "split-inouts","delete-unused-inouts","deletedeadinstances","add-dummy-inputs", "packconnections", "removeconstduplicates", "flatten", "cullzexts", "removeconstduplicates"});

    Env circuitEnv = convertFromCoreIR(c, twoMux);

    REQUIRE(circuitEnv.getCellDefs().size() == 1);

    CellDefinition& cDef = circuitEnv.getDef(twoMux->getName());
    
    Simulator state(circuitEnv, cDef);

    state.setFreshValue("sel", BitVector(1, 0));
    state.setFreshValue("in", BitVector(width, "11"));
    state.update();

    REQUIRE(state.getBitVec("out") == BitVector(width, "11"));
      
    deleteContext(c);
    
  }

  Env loadFromCoreIR(const std::string& topName,
                     const std::string& fileName) {
    Context* c = newContext();
    Namespace* g = c->getGlobal();

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
  
  TEST_CASE("Loading Connect box") {
    Env circuitEnv = loadFromCoreIR("global.cb_unq1", "./test/cb_unq1.json");
    CellDefinition& def = circuitEnv.getDef("cb_unq1"); //top->getName());
    const Cell& clkPort = def.getPortCell("clk");

    REQUIRE(clkPort.getCellType() == CELL_TYPE_PORT);
    REQUIRE(clkPort.getParameterValue(PARAM_PORT_TYPE).to_type<int>() == PORT_CELL_FOR_INPUT);
    REQUIRE(clkPort.hasPort(PORT_ID_OUT));

    const Cell& resPort = def.getPortCell("out");

    REQUIRE(resPort.getCellType() == CELL_TYPE_PORT);
    REQUIRE(resPort.getParameterValue(PARAM_PORT_TYPE).to_type<int>() == PORT_CELL_FOR_OUTPUT);
    REQUIRE(resPort.hasPort(PORT_ID_IN));

    // Check connections

    bool allOutBitsDriven;
    int outWidth = resPort.getPortWidth(PORT_ID_IN);
    
    REQUIRE(outWidth == 16);

    const SignalBus& drivers = resPort.getDrivers(PORT_ID_IN);

    for (auto sigBit : drivers.signals) {
      cout << toString(sigBit) << endl;
    }

    assert(drivers.signals.size() == 16);
    for (int i = 0; i < outWidth; i++) {

      REQUIRE(notEmpty(drivers.signals[i]));

    }

    // Simulate the connect box
    Simulator sim(circuitEnv, def);
    sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 0));
    sim.update();
    sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 1));
    sim.update();
    sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 0));
    sim.update();

    sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 1));
    sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 3));
    sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));

    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
    sim.update();
    
    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 1));
    sim.update();

    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
    sim.update();
    
    sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 0));
    sim.setFreshValue("in_3", PORT_ID_OUT, BitVec(16, 239));
    sim.update();

    REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 239));

    sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 1));
    sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 6));
    sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));

    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
    sim.update();
    
    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 1));
    sim.update();

    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
    sim.update();
    
    sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 0));
    sim.setFreshValue("in_6", PORT_ID_OUT, BitVec(16, 7));
    sim.update();

    REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 7));

    sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 2));
    sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));
    sim.setFreshValue("in_2", PORT_ID_OUT, BitVec(16, 0));
    sim.setFreshValue("in_6", PORT_ID_OUT, BitVec(16, 9));
    sim.update();

    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
    sim.update();
    
    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 1));
    sim.update();

    REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 9));
    
  }

  TEST_CASE("CGRA PE tile") {
    Context* c = newContext();
    Namespace* g = c->getGlobal();

    CoreIRLoadLibrary_rtlil(c);

    Module* top;
    if (!loadFromFile(c,"./test/pe_tile_new_unq1.json", &top)) {
      cout << "Could not Load from json!!" << endl;
      c->die();
    }

    top = c->getModule("global.pe_tile_new_unq1");

    assert(top != nullptr);

    c->runPasses({"rungenerators", "split-inouts","delete-unused-inouts","deletedeadinstances","add-dummy-inputs", "packconnections", "removeconstduplicates", "flatten", "cullzexts", "removeconstduplicates"});

    if (!saveToFile(g, "./test/flat_pe_tile_new_unq1.json")) {
      cout << "Could not Load from json!!" << endl;
      c->die();
    }

    
    Env circuitEnv = convertFromCoreIR(c, top);

    REQUIRE(circuitEnv.getCellDefs().size() == 1);

    CellDefinition& def = circuitEnv.getDef(top->getName());

    auto configValues = loadBitStream("./test/hwmaster_pw2_sixteen.bsa");

    // NOTE: Unknown value on cg_en causes problems?
    Simulator sim(circuitEnv, def);

    sim.setFreshValue("tile_id", BitVector("16'h15"));

    sim.setFreshValue("in_BUS1_S1_T0", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T1", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T2", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T3", BitVector("1'h1"));
    sim.setFreshValue("in_BUS1_S1_T4", BitVector("1'h1"));

    cout << "Set tile_id" << endl;

    sim.setFreshValue("reset", BitVector("1'h0"));
    sim.update();
    sim.setFreshValue("reset", BitVector("1'h1"));
    sim.update();
    sim.setFreshValue("reset", BitVector("1'h0"));
    sim.update();

    cout << "Reset chip" << endl;
    for (int i = 0; i < configValues.size(); i++) {

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      cout << "Evaluating " << i << endl;

      unsigned int configAddr = configValues[i].first;
      unsigned int configData = configValues[i].second;

      sim.setFreshValue("config_addr", BitVec(32, configAddr));
      sim.setFreshValue("config_data", BitVec(32, configData));

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 0));
      sim.update();

      sim.setFreshValue("clk_in", BitVec(1, 1));
      sim.update();
      
    }

    cout << "Done configuring PE tile" << endl;

    sim.setFreshValue("config_addr", BitVec(32, 0));
    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();

    int top_val = 5;

    sim.setFreshValue("in_BUS16_S2_T0", BitVec(16, top_val));

    sim.setFreshValue("in_BUS16_S0_T0", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S0_T1", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S0_T2", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S0_T3", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S0_T4", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T0", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T1", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T2", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T3", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S1_T4", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T0", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T1", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T2", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T3", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S2_T4", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T0", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T1", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T2", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T3", BitVec(16, top_val));
    sim.setFreshValue("in_BUS16_S3_T4", BitVec(16, top_val));

    // cout << "Data0 = " << sim.getBitVec("test_pe$self.data0") << endl;
    // cout << "Data1 = " << sim.getBitVec("test_pe$self.data1") << endl;
    // cout << "res   = " << sim.getBitVec("test_pe$self.res") << endl;

    // cout << "cb0 out = " << sim.getBitVec("cb_data0$self.out") << endl;
    // cout << "cb1 out = " << sim.getBitVec("cb_data1$self.out") << endl;
    
    cout << "Done setting inputs" << endl;

    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 0));
    sim.update();

    sim.setFreshValue("clk_in", BitVec(1, 1));
    sim.update();
    
    cout << "Outputs" << endl;
    cout << sim.getBitVec("out_BUS16_S0_T0") << endl;
    cout << sim.getBitVec("out_BUS16_S0_T1") << endl;
    cout << sim.getBitVec("out_BUS16_S0_T2") << endl;
    cout << sim.getBitVec("out_BUS16_S0_T3") << endl;
    cout << sim.getBitVec("out_BUS16_S0_T4") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T0") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T1") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T2") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T3") << endl;
    cout << sim.getBitVec("out_BUS16_S1_T4") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T0") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T1") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T2") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T3") << endl;
    cout << sim.getBitVec("out_BUS16_S2_T4") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T0") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T1") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T2") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T3") << endl;
    cout << sim.getBitVec("out_BUS16_S3_T4") << endl;

    // cout << "Values in cb0" << endl;
    // for (auto val : sim.portValues) {
    //   SigPort sp = val.first;
    //   BitVector bv = val.second;

    //   string name = sim.def.cellName(sp.cell);
    //   string prefix = name.substr(0, name.find("$"));
    //   string suffix = name.substr(name.find("$") + 1);

    //   if (prefix == "cb_data0") {
    //     // string prefix2 = suffix.substr(0, suffix.find("$"));
    //     // string suffix2 = suffix.substr(suffix.find("$") + 1);

    //     //if (prefix2 == "sb_wide") {
    //     CellId cid = sp.cell;
    //     const Cell& cell = sim.def.getCellRefConst(cid);
    //     cout << "\t" << sim.def.cellName(sp.cell) << ", " << portIdString(sp.port) << " --> " << bv << endl;
    //     if (cell.getCellType() == CELL_TYPE_MUX) {
    //       cout << "\tIs a mux that sends to " << endl;
    //       auto receivers = cell.getPortReceivers(PORT_ID_OUT);
    //       assert(receivers.size() == cell.getPortWidth(PORT_ID_OUT));
    //       for (auto bitReceivers : receivers) {
    //         for (auto sigBit : bitReceivers) {
    //           cout << "\t\t" << toString(sim.def, sigBit) << endl;
    //         }

    //       }
    //     }
    //       //}
    //   }
    // }

    // cout << "Values in test_opt_reg_a" << endl;
    // for (auto val : sim.portValues) {
    //   SigPort sp = val.first;
    //   BitVector bv = val.second;

    //   string name = sim.def.cellName(sp.cell);
    //   string prefix = name.substr(0, name.find("$"));
    //   string suffix = name.substr(name.find("$") + 1);

    //   if (prefix == "test_pe") {
    //     string prefix2 = suffix.substr(0, suffix.find("$"));
    //     string suffix2 = suffix.substr(suffix.find("$") + 1);

    //     if (prefix2 == "test_opt_reg_a") {
    //       const Cell& c = sim.def.getCellRefConst(sp.cell);
    //       cout << "\t" << sim.def.cellName(sp.cell) << ", " << portIdString(sp.port) << " --> " << bv << endl;

    //       if (isBinop(c.getCellType())) {
    //         auto in0Drivers = c.getDrivers(PORT_ID_IN0);
    //         cout << "\t\tIn0 = " << sim.materializeInput({sp.cell, PORT_ID_IN0}) << " has drivers" << endl;
    //         for (auto sigBit : in0Drivers.signals) {
    //           cout << "\t\t\t" << toString(sim.def, sigBit) << endl;
    //         }

    //         auto in1Drivers = c.getDrivers(PORT_ID_IN1);
    //         cout << "\t\tIn1 = " << sim.materializeInput({sp.cell, PORT_ID_IN1}) << " has drivers" << endl;
    //         for (auto sigBit : in1Drivers.signals) {
    //           cout << "\t\t\t" << toString(sim.def, sigBit) << endl;
    //         }

    //       }
    //     }
    //   }
    // }
    
    // cout << "Multiply value" << endl;

    // cout << "test_pe$test_pe_comp$test_mult_add$__DOLLAR__mul__DOLLAR____DOT____FORWARD_SLASH__test_mult_add__DOT__sv__COLON__64__DOLLAR__523$extendA = " <<
    //   sim.getBitVec("test_pe$test_pe_comp$test_mult_add$__DOLLAR__mul__DOLLAR____DOT____FORWARD_SLASH__test_mult_add__DOT__sv__COLON__64__DOLLAR__523$extendA", PORT_ID_OUT) << endl;

    // cout << "test_pe$test_pe_comp$test_mult_add$__DOLLAR__mul__DOLLAR____DOT____FORWARD_SLASH__test_mult_add__DOT__sv__COLON__64__DOLLAR__523$extendB = " <<
    //   sim.getBitVec("test_pe$test_pe_comp$test_mult_add$__DOLLAR__mul__DOLLAR____DOT____FORWARD_SLASH__test_mult_add__DOT__sv__COLON__64__DOLLAR__523$extendB", PORT_ID_OUT) << endl;

    //test_pe$test_pe_comp$test_mult_add$__DOLLAR__mul__DOLLAR____DOT____FORWARD_SLASH__test_mult_add__DOT__sv__COLON__64__DOLLAR__523$op0, PORT_ID_OUT --> 0000000000000000000000000000000000
    
    // cout << "Values related to outputs" << endl;
    // vector<SigPort> trace = sim.traceValue("out_BUS16_S0_T0", PORT_ID_IN);
    
    // for (auto port : trace) {
    //   cout << "\t" << sigPortString(def, port) << endl;
    // }

    REQUIRE(sim.getBitVec("out_BUS16_S0_T0", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T1", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T2", PORT_ID_IN) == BitVec(16, top_val*2));
    REQUIRE(sim.getBitVec("out_BUS16_S3_T3", PORT_ID_IN) == BitVec(16, top_val*2));
  
    deleteContext(c);
  }
  
}
