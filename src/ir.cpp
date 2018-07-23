#include "ir.h"

using namespace std;

namespace FlatCircuit {

  std::string parens(const std::string& s) {
    return "(" + s + ")";
  }

  std::string shlCode(const std::string& receiver,
                      const std::string& arg0,
                      const std::string& arg1) {
    string resStr = parens(arg0 + " << " + arg1);
    string str = receiver + " = " + resStr;
    return str;
  }

  std::string ashrCode(const std::string& receiver,
                       const std::string& arg0,
                       const std::string& arg1,
                       const Cell& cell) {
    int argWidth = cell.getPortWidth(PORT_ID_IN0);
    string highBitSet =
      parens(parens(arg0 + " >> " + to_string(argWidth - 1)) + " & 0xb1");

    string maskShift =
      "std::max((int) " + parens(to_string(argWidth) + " - " + arg1) + ", 0)";
    string oneMask = parens(parens("1 << " + arg1) + " - 1") + " << " + maskShift;

    string rawShift = parens(arg0 + " >> " + arg1);
    string shiftRes =
      parens(highBitSet + " ? " +
             parens(rawShift + " | " + oneMask) + " : " +
             parens(rawShift + " & " + "~" + parens(oneMask)));

    string str = receiver + " = " + shiftRes;
    return str;
  }
  
  std::string sliceString(const std::string& receiver,
                          const std::string& src,
                          const int low,
                          const int high) {
    string resStr;
    resStr +=
      parens(parens(src + " >> " + to_string(low)) + " & " + maskWidth(high - low));
    
    string str;
    str += receiver + " = " + resStr;
    return str;
  }
    
  std::string orrStr(const std::string& name) {
    return "!!(" + name + ")";
  }

  std::string IRBinop::twoStateCppCode(ValueStore& valueStore) const {

    CellType tp = cell.getCellType();
    switch (tp) {
    case CELL_TYPE_MUL:
      return ln(receiver + " = (" + arg0 + " * " + arg1 + ")");

    case CELL_TYPE_ADD:
      return ln(receiver + " = (" + arg0 + " + " + arg1 + ")");

    case CELL_TYPE_SUB:
      return ln(receiver + " = (" + arg0 + " - " + arg1 + ")");
      
    case CELL_TYPE_AND:
      return ln(receiver + " = (" + arg0 + " & " + arg1 + ")");

    case CELL_TYPE_OR:
      return ln(receiver + " = (" + arg0 + " | " + arg1 + ")");

    case CELL_TYPE_XOR:
      return ln(receiver + " = (" + arg0 + " ^ " + arg1 + ")");
      
    case CELL_TYPE_EQ:
      return ln(receiver + " = (" + arg0 + " == " + arg1 + ")");

    case CELL_TYPE_NEQ:
      return ln(receiver + " = (" + arg0 + " != " + arg1 + ")");

    case CELL_TYPE_UGE:
      return ln(receiver + " = (" + arg0 + " >= " + arg1 + ")");

    case CELL_TYPE_ULE:
      return ln(receiver + " = (" + arg0 + " <= " + arg1 + ")");

    case CELL_TYPE_ULT:
      return ln(receiver + " = (" + arg0 + " < " + arg1 + ")");

    case CELL_TYPE_UGT:
      return ln(receiver + " = (" + arg0 + " > " + arg1 + ")");

    case CELL_TYPE_SHL:
      return ln(shlCode(receiver, arg0, arg1));

    case CELL_TYPE_ASHR:
      return ln(ashrCode(receiver, arg0, arg1, cell));
      
    default:
      std::cout << "Error: Unsupported binop " << FlatCircuit::toString(tp) << std::endl;
      assert(false);
      //      return "// Binop " + receiver + "\n";
    }

  }
  
  std::string IRBinop::toString(ValueStore& valueStore) const {
    CellType tp = cell.getCellType();
    switch (tp) {
    case CELL_TYPE_AND:
      return ln(receiver + " = (" + arg0 + " & " + arg1 + ")") +
        ln(xMask(receiver) + " = (" + xMask(arg0) + " | " + xMask(arg1) + ")") +
        ln(zMask(receiver) + " = (" + zMask(arg0) + " | " + zMask(arg1) + ")");

    case CELL_TYPE_UGE:
      return ln(receiver + " = (" + arg0 + " >= " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));

    case CELL_TYPE_ULE:
      return ln(receiver + " = (" + arg0 + " <= " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));
      
    case CELL_TYPE_UGT:
      return ln(receiver + " = (" + arg0 + " > " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));

    case CELL_TYPE_ULT:
      return ln(receiver + " = (" + arg0 + " < " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));
      
    case CELL_TYPE_OR:
      return ln(receiver + " = (" + arg0 + " | " + arg1 + ")") +
        ln(xMask(receiver) + " = (" + xMask(arg0) + " | " + xMask(arg1) + ")") +
        ln(zMask(receiver) + " = (" + zMask(arg0) + " | " + zMask(arg1) + ")");

    case CELL_TYPE_XOR:
      return ln(receiver + " = (" + arg0 + " ^ " + arg1 + ")") +
        ln(xMask(receiver) + " = (" + xMask(arg0) + " | " + xMask(arg1) + ")");
      
    case CELL_TYPE_ADD:
      return ln(receiver + " = (" + arg0 + " + " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));

    case CELL_TYPE_MUL:
      return ln(receiver + " = (" + arg0 + " * " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));

    case CELL_TYPE_SUB:
      return ln(receiver + " = (" + arg0 + " - " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));
        

    case CELL_TYPE_LSHR:
      return ln("lshr(" + receiver + ", " + arg0 + ", " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));
        
        
    case CELL_TYPE_ASHR:
      return ln(ashrCode(receiver, arg0, arg1, cell)) +
      //return ln("ashr(" + receiver + ", " + arg0 + ", " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));
        
        
    case CELL_TYPE_SHL:
      return ln(shlCode(receiver, arg0, arg1)) + //ln("shl(" + receiver + ", " + arg0 + ", " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));
        
        
    case CELL_TYPE_EQ:
      return ln(receiver + " = (" + arg0 + " == " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));        

    case CELL_TYPE_NEQ:
      return ln(receiver + " = (" + arg0 + " != " + arg1 + ")") +
        ln(xMask(receiver) + " = " + orrStr(xMask(arg0)) + " || " + orrStr(xMask(arg1))) +
        ln(zMask(receiver) + " = " + orrStr(zMask(arg0)) + " || " + orrStr(zMask(arg1)));

    default:
      std::cout << "Error: Unsupported binop " << FlatCircuit::toString(tp) << std::endl;
      assert(false);
    }

  }

  std::string bitMask(const int width) {
    assert(width > 0);

    std::string ones = "";
    for (int i = 0; i < width; i++) {
      ones += "1";
    }
    return "0b" + ones;
  }
  std::string IRUnop::twoStateCppCode(ValueStore& valueStore) const {
    CellType unop = cell.getCellType();

    switch (unop) {

    case CELL_TYPE_PASSTHROUGH:
      return ln(receiver + " = " + arg);

    case CELL_TYPE_NOT:
      return ln(receiver + " = (~(" + arg + ") & " + bitMask(cell.getPortWidth(PORT_ID_OUT)) + ")");

    case CELL_TYPE_ORR:
      return ln(receiver + " = !!(" + arg + ")");

    case CELL_TYPE_SLICE:
      return ln(sliceString(receiver,
                            arg,
                            bvToInt(cell.getParameterValue(PARAM_LOW)),
                            bvToInt(cell.getParameterValue(PARAM_HIGH))));
      
    // case CELL_TYPE_SLICE:
    //   return ln("slice(" + receiver + ", " + arg + ", " +
    //             to_string(bvToInt(cell.getParameterValue(PARAM_LOW))) + ", " +
    //             to_string(bvToInt(cell.getParameterValue(PARAM_HIGH))) + ")");
      
    case CELL_TYPE_ZEXT:
      return ln(receiver + " = " + arg);

    default:
      std::cout << "IRUnop error: " << FlatCircuit::toString(unop) << std::endl;
      assert(false);
    }

  }

  std::string IRUnop::toString(ValueStore& valueStore) const {
    CellType unop = cell.getCellType();

    switch (unop) {

    case CELL_TYPE_PASSTHROUGH:
      return ln(receiver + " = " + arg) +
        ln(xMask(receiver) + " = " + xMask(arg)) +
        ln(zMask(receiver) + " = " + zMask(arg));

    case CELL_TYPE_NOT:
      // NOTE: Not is a no-op for the x mask
      return ln(receiver + " = ~(" + arg + ") & " + maskWidth(cell.getPortWidth(PORT_ID_OUT))) +
        ln(xMask(receiver) + " = " + xMask(arg)) +
        ln(zMask(receiver) + " = " + zMask(arg));

    case CELL_TYPE_ORR:
      return ln(receiver + " = !!(" + arg + ")") +
        ln(xMask(receiver) + " = !!(" + xMask(arg) + ")") +
        ln(zMask(receiver) + " = !!(" + zMask(arg) + ")");

    case CELL_TYPE_SLICE:
      return ln(sliceString(receiver,
                            arg,
                            bvToInt(cell.getParameterValue(PARAM_LOW)),
                            bvToInt(cell.getParameterValue(PARAM_HIGH)))) +
        ln(sliceString(xMask(receiver),
                       xMask(arg),
                       bvToInt(cell.getParameterValue(PARAM_LOW)),
                       bvToInt(cell.getParameterValue(PARAM_HIGH)))) +
        ln(sliceString(zMask(receiver),
                       zMask(arg),
                       bvToInt(cell.getParameterValue(PARAM_LOW)),
                       bvToInt(cell.getParameterValue(PARAM_HIGH))));
        

      // return ln("slice(" + receiver + ", " + arg + ", " +
      //           to_string(bvToInt(cell.getParameterValue(PARAM_LOW))) + ", " +
      //           to_string(bvToInt(cell.getParameterValue(PARAM_HIGH))) + ")") +
      //   ln("slice(" + xMask(receiver) + ", " + xMask(arg) + ", " +
      //      to_string(bvToInt(cell.getParameterValue(PARAM_LOW))) + ", " +
      //      to_string(bvToInt(cell.getParameterValue(PARAM_HIGH))) + ")") +
      //   ln("slice(" + zMask(receiver) + ", " + zMask(arg) + ", " +
      //      to_string(bvToInt(cell.getParameterValue(PARAM_LOW))) + ", " +
      //      to_string(bvToInt(cell.getParameterValue(PARAM_HIGH))) + ")");
      
    case CELL_TYPE_ZEXT:
      return ln(receiver + " = " + arg) +
        ln(xMask(receiver) + " = " + xMask(arg)) +
        ln(zMask(receiver) + " = " + zMask(arg));

    default:
      std::cout << "IRUnop error: " << FlatCircuit::toString(unop) << std::endl;
      assert(false);
    }
  }
  
}
