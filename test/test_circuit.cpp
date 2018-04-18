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

  CellType primitiveForMod(const Env& e, CoreIR::Instance* const inst) {
    string name = getQualifiedOpName(*inst);
    if (name == "coreir.wrap") {
      return CELL_TYPE_PASSTHROUGH;
    } else if (name == "coreir.or") {
      return CELL_TYPE_OR;
    } else if (name == "coreir.orr") {
      return CELL_TYPE_ORR;
    } else if (name == "coreir.eq") {
      return CELL_TYPE_EQ;
    } else if (name == "coreir.mux") {
      return CELL_TYPE_MUX;
    } else if (name == "coreir.reg_arst") {
      return CELL_TYPE_REG_ARST;
    } else if (name == "coreir.const" || name == "corebit.const") {
      return CELL_TYPE_CONST;
    } else {
      cout << "Error: Unsupported module type = " << name << endl;
    }
    assert(false);
  }

  std::map<Parameter, BitVector>
  paramsForMod(const Env& e, CoreIR::Instance* const inst) {
    string name = getQualifiedOpName(*inst);
    if (name == "coreir.wrap") {
      // TODO: Fix width issue
      //int width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();
      return {{PARAM_WIDTH, BitVector(32, 1)}};
    } else if (name == "coreir.or") {
      int width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();
      return {{PARAM_WIDTH, BitVector(32, width)}};
    } else if (name == "coreir.orr") {
      int width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();
      return {{PARAM_WIDTH, BitVector(32, width)}};
    } else if (name == "coreir.eq") {
      int width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();
      return {{PARAM_WIDTH, BitVector(32, width)}};
    } else if (name == "coreir.mux") {
      int width = inst->getModuleRef()->getGenArgs().at("width")->get<int>();
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
          {PARAM_INIT_VALUE, init}};

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
      cout << "Error: Unsupported module type = " << name << endl;
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

      CellType instType = primitiveForMod(e, inst);
      map<Parameter, BitVector> params = paramsForMod(e, inst);

      CellId cid = cDef.addCell(inst->toString(), instType, params);
      elemsToCells.insert({inst, cid});
      //cout << "Added instance " << inst->toString() << endl;
    }

    //cout << "Added all instances" << endl;

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

  TEST_CASE("Loading Connect box") {
    Context* c = newContext();
    Namespace* g = c->getGlobal();

    CoreIRLoadLibrary_rtlil(c);

    Module* top;
    if (!loadFromFile(c,"./test/cb_unq1.json", &top)) {
      cout << "Could not Load from json!!" << endl;
      c->die();
    }

    top = c->getModule("global.cb_unq1");

    assert(top != nullptr);

    c->runPasses({"rungenerators", "split-inouts","delete-unused-inouts","deletedeadinstances","add-dummy-inputs", "packconnections", "removeconstduplicates", "flatten", "cullzexts", "removeconstduplicates"});

    Env circuitEnv = convertFromCoreIR(c, top);

    REQUIRE(circuitEnv.getCellDefs().size() == 1);

    CellDefinition& def = circuitEnv.getDef(top->getName());

    REQUIRE(def.numCells() == (top->getDef()->getInstances().size() + top->getType()->getFields().size()));

    REQUIRE(def.getPortNames().size() == top->getType()->getFields().size());

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
    sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 1));
    sim.setFreshValue("reset", PORT_ID_OUT, BitVec(1, 0));

    sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 1));
    sim.setFreshValue("config_data", PORT_ID_OUT, BitVec(32, 3));
    sim.setFreshValue("config_addr", PORT_ID_OUT, BitVec(32, 0));

    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 0));
    sim.setFreshValue("clk", PORT_ID_OUT, BitVec(1, 1));
    
    sim.setFreshValue("config_en", PORT_ID_OUT, BitVec(1, 0));
    sim.setFreshValue("in_3", PORT_ID_OUT, BitVec(16, 239));

    sim.update();

    REQUIRE(sim.getBitVec("out", PORT_ID_IN) == BitVec(16, 239));
    
    deleteContext(c);
  }
}
