#include "ir.h"

using namespace std;

// Alternative X value strategy: Simulate the X values with raw code
// and then simulate X value propagation with another raw bit vector.
// Not sure how well this would work in practice. Can a precise X-value
// strategy be implemented this way?
// Another question: this strategy uses 2 real bits per 4-value sim bit, which
// is theoretically optimal, but can the operations on X values be done in
// parallel?

// For and, or, xor I think the answer is yes:
// Suppose 1 indicates the presence of an x value:

// and((a0, ax0), (a1, ax1)) = ((a0 & a1), (ax0 | ax1))?

// ax0 | ax1 gives an x value anywhere that either one had an x value

// The x value policy I currently have in interpretation is so imprecise
// that I could probably implement it in this split scheme.

// Implementing other operations precisely might be doable the same way: Ex:
// ult((a, ax), (b, bx)) = (ult_bit(a, b) or unknown if either one has an x value
// before the other has a 1 in its binary, ())

// Q: Do simulators implement this semantics? Apparently iverilog is not
// very precise with its x value propagation. So maybe this is the way to go.

namespace FlatCircuit {

  // std::string bvWrapperDecl(const std::string name,
  //                           const std::string& buf_name,
  //                           const int width) {
  //   return ln("bv_wrapper " + name + "( " + buf_name + ", " + to_string(width) + ", true )");
  // }

  // std::string mul_quad_state_code(const std::string& receiver,
  //                                 const std::string& arg0,
  //                                 const std::string& arg1,
  //                                 const Cell& cell) {
  //   string code = "";
  //   string res_temp = arg0 + "_" + arg1 + "_mul_res_temp";
  //   string shift_temp = arg0 + "_" + arg1 + "_mul_shift_temp";
  //   string accum_temp = arg0 + "_" + arg1 + "_mul_accum_temp";

  //   string res_temp_buffer = res_temp + "_buffer";
  //   string shift_temp_buffer = shift_temp + "_buffer";
  //   string accum_temp_buffer = accum_temp + "_buffer";
    
  //   string res_line = ln("mul_bv(" + receiver + ", " + arg0 + ", " + arg1 + ", " + res_temp + ", " + shift_temp + ", " + accum_temp + ")");
  //   code += res_line;
  //   return code;
  // }


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
        ln(xMask(receiver) + " = (" + xMask(arg0) + " | " + xMask(arg1) + ")");

    case CELL_TYPE_UGE:
      return ln("greater_than_or_equal(" + receiver + ", " + arg0 + ", " + arg1 + ")");
      // return ln(receiver + " = BitVector((" +
      //           arg0 + " > " + arg1 + ") || (" +
      //           arg0 + " == " + arg1 + "))");

    case CELL_TYPE_ULE:
      return ln("less_than_or_equal(" + receiver + ", " + arg0 + ", " + arg1 + ")");
      // return ln(receiver + " = BitVector((" +
      //           arg0 + " < " + arg1 + ") || (" +
      //           arg0 + " == " + arg1 + "))");

    case CELL_TYPE_UGT:
      //return ln(receiver + " = BitVector((" + arg0 + " > " + arg1 + "))");
      return ln("greater_than(" + receiver + ", " + arg0 + ", " + arg1 + ")");

    case CELL_TYPE_ULT:
      return ln("less_than(" + receiver + ", " + arg0 + ", " + arg1 + ")");
      //return ln(receiver + " = BitVector((" + arg0 + " < " + arg1 + "))");
        
    case CELL_TYPE_OR:
      return ln("static_bv_or(" + receiver + ", " + arg0 + ", " + arg1 + ")");
      //return ln(receiver + " = (" + arg0 + " | " + arg1 + ")");

    case CELL_TYPE_XOR:
      return ln("static_bv_xor(" + receiver + ", " + arg0 + ", " + arg1 + ")");
      //return ln(receiver + " = (" + arg0 + " ^ " + arg1 + ")");

    case CELL_TYPE_ADD:
      //return ln(receiver + " = add_general_width_bv(" + arg0 + ", " + arg1 + ")");
      return ln("add_general_width_bv(" + receiver + ", " + arg0 + ", " + arg1 + ")");

    case CELL_TYPE_MUL:
      //return ln(receiver + " = mul_general_width_bv(" + arg0 + ", " + arg1 + ")");

      return ln("mul_bv(" + receiver + ", " + arg0 + ", " + arg1 + ", " + temps[0] + ", " + temps[1] + ", " + temps[2] + ")");

      //return mul_quad_state_code(receiver, arg0, arg1, cell);
      //return ln("mul_bv(" + receiver + ", " + arg0 + ", " + arg1 + ")");

    case CELL_TYPE_SUB:
      //return ln(receiver + " = sub_general_width_bv(" + arg0 + ", " + arg1 + ")");
      return ln("sub_general_width_bv(" + receiver + ", " + arg0 + ", " + arg1 + ", " + temps[0] + ")");

    case CELL_TYPE_LSHR:
      //return ln(receiver + " = lshr(" + arg0 + ", " + arg1 + ")");
      return ln("lshr(" + receiver + ", " + arg0 + ", " + arg1 + ")");
        
    case CELL_TYPE_ASHR:
      //return ln(receiver + " = ashr(" + arg0 + ", " + arg1 + ")");
      return ln("ashr(" + receiver + ", " + arg0 + ", " + arg1 + ")");
        
    case CELL_TYPE_SHL:
      //return ln(receiver + " = shl(" + arg0 + ", " + arg1 + ")");
      return ln("shl(" + receiver + ", " + arg0 + ", " + arg1 + ")");
        
    case CELL_TYPE_EQ:
      //return ln(receiver + " = BitVector(" + arg0 + " == " + arg1 + ")");
      return ln("equals(" + receiver + ", " + arg0 + ", " + arg1 + ")");

    case CELL_TYPE_NEQ:
      return ln("not_equals(" + receiver + ", " + arg0 + ", " + arg1 + ")");
      //return ln(receiver + " = BitVector(" + arg0 + " != " + arg1 + ")");
        
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

    // case CELL_TYPE_SLICE:
    //   return ln(receiver + " = slice(" + arg + ", " +
    //             to_string(bvToInt(cell.getParameterValue(PARAM_LOW))) + ", " +
    //             to_string(bvToInt(cell.getParameterValue(PARAM_HIGH))) + ")");
        
    case CELL_TYPE_ZEXT:
      return ln(receiver + " = " + arg);

      // return ln(receiver + " = zero_extend(" +
      //           to_string(cell.getPortWidth(PORT_ID_OUT)) + 
      //           ", " + arg + ")");
    default:
      //      return ln("// Unop");
      std::cout << "IRUnop error: " << FlatCircuit::toString(unop) << std::endl;
      assert(false);
    }

    //    return "// Unop " + receiver + "\n";
  }
    
  std::string IRUnop::toString(ValueStore& valueStore) const {
    CellType unop = cell.getCellType();

    switch (unop) {

    case CELL_TYPE_PASSTHROUGH:
      return ln("set_bv(" + receiver + ", " + arg + ")");
        //return ln(receiver + " = " + arg);

    case CELL_TYPE_NOT:
      //return ln(receiver + " = ~(" + arg + ")");
      return ln("logical_not(" + receiver + ", " + arg + ")");

    case CELL_TYPE_ORR:
      return ln("orr(" + receiver + ", " + arg + ")");

    case CELL_TYPE_SLICE:
      return ln("slice(" + receiver + ", " + arg + ", " +
                to_string(bvToInt(cell.getParameterValue(PARAM_LOW))) + ", " +
                to_string(bvToInt(cell.getParameterValue(PARAM_HIGH))) + ")");
      
      // return ln(receiver + " = slice(" + arg + ", " +
      //           to_string(bvToInt(cell.getParameterValue(PARAM_LOW))) + ", " +
      //           to_string(bvToInt(cell.getParameterValue(PARAM_HIGH))) + ")");
        
    case CELL_TYPE_ZEXT:
      return ln("zero_extend(" + receiver + ", " + arg + ")");

      // return ln(receiver + " = zero_extend<" + to_string(cell.getPortWidth(PORT_ID_IN)) + ", " + to_string(cell.getPortWidth(PORT_ID_OUT)) + ">(" +
      //           to_string(cell.getPortWidth(PORT_ID_OUT)) + 
      //           ", " + arg + ")");
    default:
      std::cout << "IRUnop error: " << FlatCircuit::toString(unop) << std::endl;
      assert(false);
    }
  }
  
}
