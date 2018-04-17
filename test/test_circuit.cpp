#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include "coreir.h"
#include "coreir/libs/rtlil.h"
#include "coreir/libs/commonlib.h"

#include "circuit.h"

using namespace std;
using namespace CoreIR;

namespace FlatCircuit {

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
      return {};
    } else if (name == "coreir.or") {
      return {};
    } else if (name == "coreir.orr") {
      return {};
    } else if (name == "coreir.eq") {
      return {};
    } else if (name == "coreir.mux") {
      return {};
    } else if (name == "coreir.reg_arst") {
      return {};
    } else if (name == "coreir.const" || name == "corebit.const") {
      return {};
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
    }

    map<Instance*, CellId> instsToCells;

    for (auto instR : top->getDef()->getInstances()) {
      Instance* inst = instR.second;
      Module* instMod = inst->getModuleRef();

      // Only handle primitives for now
      assert(!instMod->hasDef());

      CellType instType = primitiveForMod(e, inst);
      map<Parameter, BitVector> params = paramsForMod(e, inst);

      CellId cid = cDef.addCell(inst->toString(), instType, params);
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

    REQUIRE(clkPort.getPortReceivers(PORT_ID_OUT).size() > 0);
    
    deleteContext(c);
  }
}
