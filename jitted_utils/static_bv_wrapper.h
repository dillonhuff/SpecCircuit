#pragma once

#include <algorithm>
#include <bitset>
#include <cassert>
#include <iostream>
#include <stdint.h>
#include <type_traits>

// This is a comment

typedef int8_t  bv_sint8;
typedef int32_t  bv_sint32;

typedef uint8_t  bv_uint8;
typedef uint16_t bv_uint16;
typedef uint32_t bv_uint32;
typedef uint64_t bv_uint64;

#define QBV_UNKNOWN_VALUE 2
#define QBV_HIGH_IMPEDANCE_VALUE 3

namespace bsim {

  static inline std::string hex_digit_to_binary(const char hex_digit) {
    switch (hex_digit) {
    case '0':
      return "0000";
    case '1':
      return "0001";
    case '2':
      return "0010";
    case '3':
      return "0011";
    case '4':
      return "0100";
    case '5':
      return "0101";
    case '6':
      return "0110";
    case '7':
      return "0111";
    case '8':
      return "1000";
    case '9':
      return "1001";
    case 'a':
      return "1010";
    case 'b':
      return "1011";
    case 'c':
      return "1100";
    case 'd':
      return "1101";
    case 'e':
      return "1110";
    case 'f':
      return "1111";
    case 'x':
      return "xxxx";
    case 'z':
      return "zzzz";
      
    default:
      assert(false);
    }

    assert(false);
  }

  class quad_value {
  protected:
    unsigned char value;

  public:
    quad_value() : value(0) {}

    quad_value(unsigned char v) : value(v) {
      assert(v < 4);
    }

    bool same_representation(const quad_value other) const {
      return value == other.value;
    }

    unsigned char get_char() const {
      return value;
    }

    bool is_binary() const {
      return (value == 1) || (value == 0);
    }

    bool is_unknown() const {
      return (value == QBV_UNKNOWN_VALUE);
    }

    bool is_high_impedance() const {
      return (value == QBV_HIGH_IMPEDANCE_VALUE);
    }
    
    quad_value plus(const quad_value& other) const {
      assert(other.is_binary());
      assert(is_binary());

      return quad_value((other.binary_value() + binary_value()) & 0x01);
    }
    
    bool equals(const quad_value& other) const {
      if ((value == QBV_UNKNOWN_VALUE) ||
          (other.value == QBV_UNKNOWN_VALUE)) {
        return false;
      }

      // All high impedance values are equal
      return value == other.value;
    }

    unsigned char binary_value() const {
      assert((value == 1) || (value == 0));
      return value;
    }

    std::string binary_string() const {
      if (value == 1) {
        return "1";
      } else if (value == 0) {
        return "0";
      } else if (value == QBV_UNKNOWN_VALUE) {
        return "x";
      } else if (value == QBV_HIGH_IMPEDANCE_VALUE) {
        return "z";
      }
      assert(false);
    }

    void print(std::ostream& out) const {
      if (value == 1) {
        out << "1";
      } else if (value == 0) {
        out << "0";
      } else if (value == QBV_UNKNOWN_VALUE) {
        out << "x";
      } else if (value == QBV_HIGH_IMPEDANCE_VALUE) {
        out << "z";
      }
    }
  };

  static inline quad_value operator+(const quad_value& a,
                                     const quad_value& b) {
    assert(!a.is_high_impedance());
    assert(!b.is_high_impedance());

    return a.plus(b);
  }

  static inline quad_value operator&(const quad_value& a,
                                     const quad_value& b) {
    assert(!a.is_high_impedance());
    assert(!b.is_high_impedance());

    if (a.is_binary() && (a.binary_value() == 0)) {
      return quad_value(0);
    }

    if (b.is_binary() && (b.binary_value() == 0)) {
      return quad_value(0);
    }

    if (a.is_unknown() || b.is_unknown()) {
      return quad_value(QBV_UNKNOWN_VALUE);
    }

    assert(a.is_binary());
    assert(b.is_binary());

    return quad_value(a.binary_value() & b.binary_value());
  }

  static inline quad_value operator|(const quad_value& a,
                                     const quad_value& b) {

    assert(!a.is_high_impedance());
    assert(!b.is_high_impedance());

    if (a.is_binary() && (a.binary_value() == 1)) {
      return quad_value(1);
    }
    if (b.is_binary() && (b.binary_value() == 1)) {
      return quad_value(1);
    }

    if (a.is_unknown() || b.is_unknown()) {
      return quad_value(QBV_UNKNOWN_VALUE);
    }

    assert(a.is_binary());
    assert(b.is_binary());
    return quad_value(a.binary_value() | b.binary_value());
  }

  static inline quad_value operator^(const quad_value& a,
                                     const quad_value& b) {
    assert(!a.is_high_impedance());
    assert(!b.is_high_impedance());

    if(a.is_unknown() || b.is_unknown()) {
      return quad_value(QBV_UNKNOWN_VALUE);
    }

    assert(a.is_binary());
    assert(b.is_binary());

    return quad_value(a.binary_value() ^ b.binary_value());

  }

  static inline bool operator>(const quad_value& a,
                               const quad_value& b) {

    assert(!a.is_high_impedance());
    assert(!b.is_high_impedance());

    assert(a.is_binary());
    assert(b.is_binary());

    return a.binary_value() > b.binary_value();
  }

  static inline bool operator<(const quad_value& a,
                               const quad_value& b) {
    assert(!a.is_high_impedance());
    assert(!b.is_high_impedance());

    assert(a.is_binary());
    assert(b.is_binary());

    return a.binary_value() < b.binary_value();
  }
  
  static inline quad_value operator~(const quad_value& a) {

    assert(!a.is_high_impedance());

    if(a.is_unknown()) {
      return quad_value(QBV_UNKNOWN_VALUE);
    }
    
    assert(a.is_binary());

    return quad_value((~a.binary_value()) & 0x01);
  }
  
  static inline bool operator==(const quad_value& a,
                                const quad_value& b) {
    assert(!a.is_high_impedance());
    assert(!b.is_high_impedance());

    return a.equals(b);
  }

  static inline bool operator!=(const quad_value& a,
                                const quad_value& b) {
    assert(!a.is_high_impedance());
    assert(!b.is_high_impedance());

    return !(a == b);
  }

  static inline std::ostream& operator<<(std::ostream& out,
					 const quad_value& a) {
    a.print(out);
    return out;
  }

  class bv_wrapper {
    quad_value* bits;
    int N;

  public:

    bv_wrapper() {}

    bv_wrapper(quad_value* const bits_, const int N_) : bits(bits_), N(N_) {}

    bv_wrapper(quad_value* const bits_, const int N_, bool zero) :
      bits(bits_), N(N_) {
      if (zero) {
        for (int i = 0; i < N; i++) {
          set(i, quad_value(0));
        }
      }
    }

    int bitLength() const { return N; }

    quad_value get(const int i) const { return bits[i]; }

    quad_value set(const int i, quad_value val) const { return bits[i] = val; }

    template<typename ConvType>
    ConvType to_type() const {
      ConvType tmp = 0;
      ConvType exp = 1;
      for (int i = 0; i < bitLength(); i++) {
        tmp += exp*get(i).binary_value();
        exp *= 2;
      }
      return tmp;
    }

    bool is_binary() const {
      for (int i = 0; i < ((int) bitLength()); i++) {
        if (!get(i).is_binary()) {
          return false;
        }
      }
      return true;
    }
    
  };

  static inline bool posedge(const bv_wrapper& a, const bv_wrapper& b) {
    assert(a.bitLength() == 1);
    assert(b.bitLength() == 1);

    return (a.get(0) == 0) && (b.get(0) == 1);
  }

  static inline bool negedge(const bv_wrapper& a, const bv_wrapper& b) {
    assert(a.bitLength() == 1);
    assert(b.bitLength() == 1);

    return (a.get(0) == 1) && (b.get(0) == 0);
  }

  static inline void storeToTable(quad_value* values,
                                  const unsigned long offset,
                                  const bv_wrapper& bv) {
    for (unsigned long i = 0; i < (unsigned long) bv.bitLength(); i++) {
      values[offset + i] = bv.get(i);
    }
  }

  static inline
  void
  loadFromTable(bv_wrapper& bv,
                bsim::quad_value* values,
                const unsigned long offset,
                const unsigned long width) {
    for (unsigned long i = 0; i < (unsigned long) width; i++) {
      bv.set(i, values[offset + i]);
    }
  }
  
  static inline void
  loadBitFromTable(bsim::quad_value* values,
                   bv_wrapper& bv,
                   const unsigned long receiverOffset,
                   const unsigned long sourceBV,
                   unsigned long sourceOffset) {
    bv.set(receiverOffset, values[sourceBV + sourceOffset]);
  }

  static inline
  void
  static_bv_and(bv_wrapper& a_and_b,
                const bv_wrapper& a,
                const bv_wrapper& b) {
    for (int i = 0; i < a.bitLength(); i++) {
      a_and_b.set(i, a.get(i) & b.get(i));
    }

  }

  static inline
  void
  static_bv_xor(bv_wrapper& a_and_b,
                const bv_wrapper& a,
                const bv_wrapper& b) {
    for (int i = 0; i < a.bitLength(); i++) {
      a_and_b.set(i, a.get(i) ^ b.get(i));
    }

  }
  
  static inline
  void
  static_bv_or(bv_wrapper& a_or_b,
                const bv_wrapper& a,
                const bv_wrapper& b) {
    for (int i = 0; i < a.bitLength(); i++) {
      a_or_b.set(i, a.get(i) | b.get(i));
    }

  }

  static inline
  void
  zero_extend(bv_wrapper& res,
              const bv_wrapper& in) {
    for (int i = 0; i < in.bitLength(); i++) {
      res.set(i, in.get(i));
    }

    for (int i = in.bitLength(); i < res.bitLength(); i++) {
      res.set(i, quad_value(0));
    }
  }

  static inline
  void
  logical_not(bv_wrapper& not_a, const bv_wrapper& a) {
    for (int i = 0; i < a.bitLength(); i++) {
      not_a.set(i, ~a.get(i));
    }
  }

  static inline
  void
  orr(bv_wrapper& orr_a,
      const bv_wrapper& a) {
    for (int i = 0; i < a.bitLength(); i++) {
      if (a.get(i) == 1) {
        orr_a.set(0, quad_value(1));
        return;
        //return static_quad_value_bit_vector<1>(1);
      }
    }

    orr_a.set(0, quad_value(0));
    //return static_quad_value_bit_vector<1>(0);
  }

  static inline
  void equals(bv_wrapper& res,
              const bv_wrapper& a,
              const bv_wrapper& b) {

    if (a.bitLength() != b.bitLength()) {
      res.set(0, quad_value(0));
      return;
    }

    for (int i = 0; i < a.bitLength(); i++) {
      if (a.get(i) != b.get(i)) {
        res.set(0, quad_value(0));
        return;
        //return false;
      }
    }

    res.set(0, quad_value(1));
    //return true;
    
  }

  static inline
  void not_equals(bv_wrapper& res,
                  const bv_wrapper& a,
                  const bv_wrapper& b) {

    if (a.bitLength() != b.bitLength()) {
      res.set(0, quad_value(1));
      return;
    }

    for (int i = 0; i < a.bitLength(); i++) {
      if (a.get(i) != b.get(i)) {
        res.set(0, quad_value(1));
        return;
      }
    }

    res.set(0, quad_value(0));
    
  }
  
  static inline void set_unknown(bv_wrapper& res) {
    for (int i = 0; i < res.bitLength(); i++) {
      res.set(i, quad_value(QBV_UNKNOWN_VALUE));
    }
  }

  static inline
  void
  add_general_width_bv(bv_wrapper& res,
                       const bv_wrapper& a,
  		       const bv_wrapper& b) {

    //static_quad_value_bit_vector<N> res;
    unsigned char carry = 0;
    for (int i = 0; i < ((int) a.bitLength()); i++) {

      if (!a.get(i).is_binary() ||
          !b.get(i).is_binary()) {
        set_unknown(res);
        return;
        //return unknown_bv<N>();
      }

      unsigned char sum = a.get(i).binary_value() + b.get(i).binary_value() + carry;

      carry = 0;

      unsigned char z_i = sum & 0x01;
      res.set(i, quad_value(z_i));

      if (sum >= 2) {
  	carry = 1;
      }

    }

    //return res;
  }

  static inline
  void set_bv(const bv_wrapper& dest,
              const bv_wrapper& src) {
    for (int i = 0; i < dest.bitLength(); i++) {
      dest.set(i, src.get(i));
    }
  }

  static inline
  void
  mul_bv(bv_wrapper& res,
         const bv_wrapper& a,
         const bv_wrapper& b,
         bv_wrapper& full_len,
         bv_wrapper& shifted_a,
         bv_wrapper& accum_temp) {

    int Width = a.bitLength();

    for (int i = 0; i < Width; i++) {
      if (b.get(i) == 1) {

  	//static_quad_value_bit_vector<N + N> shifted_a;
        for (int j = 0; j < i; j++) {
          shifted_a.set(j, 0);
        }

  	for (int j = 0; j < Width; j++) {
  	  shifted_a.set(j + i, a.get(j));
  	}

        for (int j = 0; j < Width; j++) {
          accum_temp.set(j, quad_value(0));
        }

        add_general_width_bv(accum_temp, full_len, shifted_a);
        set_bv(full_len, accum_temp);

  	// full_len =
  	//   add_general_width_bv(full_len, shifted_a);
      }
    }

    //static_quad_value_bit_vector<N> res;
    for (int i = 0; i < Width; i++) {
      res.set(i, full_len.get(i));
    }

  }

  static inline
  void
  slice(bv_wrapper& res,
        const bv_wrapper& a,
        const int start,
        const int end) {
    //static_quad_value_bit_vector<N> res(end - start);

    //assert(false);

    //for (int i = 0; i < res.bitLength(); i++) {
    for (int i = 0; i < end; i++) {
      res.set(i, a.get(i + start));
    }

    //return res;
  }
  
  static inline
  void
  sub_general_width_bv(bv_wrapper& diff,
                       const bv_wrapper& a,
  		       const bv_wrapper& b,
                       bv_wrapper& a_cpy) {
    int Width = a.bitLength();

    // TODO: Remove this dynamic allocation!
    // quad_value* qvs = (quad_value*) malloc(sizeof(quad_value)*a.bitLength());
    // bv_wrapper a_cpy(qvs, a.bitLength(), true);
    set_bv(a_cpy, a);

    for (int i = 0; i < Width; i++) {

      if ((a_cpy.get(i) == 0) &&
          (b.get(i) == 1)) {

        int j = i + 1;

        diff.set(i, 1);	  

        // Modify to carry
        while ((j < Width) && (a_cpy.get(j) != 1)) {
          a_cpy.set(j, 1);
          j++;
        }

        if (j >= Width) {
        } else {
          a_cpy.set(j, 0);
        }

      } else if (a_cpy.get(i) == b.get(i)) {
        diff.set(i, 0);
      } else if ((a_cpy.get(i) == 1) &&
        	 (b.get(i) == 0)) {
        diff.set(i, 1);
      } else {
        set_unknown(diff);
        return;
        //return unknown_bv<N>();
      }
    }

    //free(qvs);

    //return diff;
  }    

  static inline
  void
  greater_than(bv_wrapper& res,
               const bv_wrapper& a,
               const bv_wrapper& b) {
    if (!a.is_binary() || !b.is_binary()) {
      res.set(0, quad_value(0));
      return;
      //return false;
    }

    int N = a.bitLength();
    for (int i = N - 1; i >= 0; i--) {
      if (a.get(i) > b.get(i)) {
        res.set(0, quad_value(1));
        return;
  	//return true;
      }

      if (a.get(i) < b.get(i)) {
        res.set(0, quad_value(0));
        return;
  	//return false;
      }
    }

    res.set(0, quad_value(0));
    return;

    //return false;
  }

  static inline void
  greater_than_or_equal(bv_wrapper& res,
                        const bv_wrapper& a,
                        const bv_wrapper& b) {
    // res.set(0, quad_value(0));
    // return;
    if (!a.is_binary() || !b.is_binary()) {
      res.set(0, quad_value(0));
      return;
      //return false;
    }

    greater_than(res, a, b);
    if (res.get(0) == quad_value(1)) {
      return;
    }

    equals(res, a, b);
    //return (a > b) || (a == b);
  }

  static inline void
  less_than_or_equal(bv_wrapper& res,
                        const bv_wrapper& a,
                        const bv_wrapper& b) {
    greater_than(res, a, b);

    res.set(0, ~res.get(0));
  }
  
  static inline void
  less_than(bv_wrapper& res,
            const bv_wrapper& a,
            const bv_wrapper& b) {
    equals(res, a, b);
    if (res.get(0) == quad_value(1)) {
      res.set(0, quad_value(0));
      return;
    }

    less_than_or_equal(res, a, b);
  }

  
  static inline
  bv_uint64 get_shift_int(const bv_wrapper& shift_amount) {
    bv_uint64 shift_int = 0;
    if (shift_amount.bitLength() > 64) {
      assert(false);
    }

    else if (shift_amount.bitLength() > 32) {
      shift_int = shift_amount.to_type<bv_uint64>();
    }

    else if (shift_amount.bitLength() > 16) {
      shift_int = (bv_uint64) (shift_amount.to_type<bv_uint32>());
    }

    else if (shift_amount.bitLength() > 8) {
      shift_int = (bv_uint64) (shift_amount.to_type<bv_uint16>());
    } else {
      shift_int = (bv_uint64) (shift_amount.to_type<bv_uint8>());
    }

    //std::cout << "shift_int = " << shift_int << std::endl;
    assert(shift_int < 65);

    return shift_int;
  }

  static inline
  void
  lshr(bv_wrapper& res,
       const bv_wrapper& a,
       const bv_wrapper& shift_amount) {

    if (!a.is_binary() || !shift_amount.is_binary()) {
      set_unknown(res);
      return;
      //return unknown_bv(a.bitLength());
    }
    
    //static_quad_value_bit_vector<N> res(a.bitLength());

    bv_uint64 shift_int = get_shift_int(shift_amount);

    if (shift_int == 0) {
      set_bv(res, a);
      return;
      //return a;
    }

    //unsigned char sign_bit = a.get(a.bitLength() - 1);
    for (uint i = a.bitLength() - 1; i >= shift_int; i--) {
      res.set(i - shift_int, a.get(i));
    }

    for (uint i = a.bitLength() - 1; i >= (a.bitLength() - shift_int); i--) {
      res.set(i, 0);
    }

    //return res;
  }

  // Arithmetic shift right
  static inline
  void
  ashr(bv_wrapper& res,
       const bv_wrapper& a,
       const bv_wrapper& shift_amount) {

    if (!a.is_binary() || !shift_amount.is_binary()) {
      //return unknown_bv(a.bitLength());
      set_unknown(res);
      return;
    }

    if (shift_amount.to_type<int>() == 0) {
      //static_quad_value_bit_vector<N>(shift_amount.bitLength(), 0)) {
      set_bv(res, a);
      return;
      //returnb a;
    }

    //static_quad_value_bit_vector<N> res(a.bitLength());

    bv_uint64 shift_int = get_shift_int(shift_amount);

    quad_value sign_bit = a.get(a.bitLength() - 1);
    for (uint i = a.bitLength() - 1; i >= shift_int; i--) {
      res.set(i - shift_int, a.get(i));
    }

    int last_index = (int)a.bitLength() - shift_int;
    for (int i = a.bitLength() - 1; i >= last_index && i >= 0; i--) {
      res.set(i, sign_bit);
    }

    //return res;
  }
  
  static inline
  void
  shl(bv_wrapper& res,
      const bv_wrapper& a,
      const bv_wrapper& shift_amount) {

    if (!a.is_binary() || !shift_amount.is_binary()) {
      set_unknown(res);
      return;
    }

    bv_uint64 shift_int = get_shift_int(shift_amount);
    for (int i = shift_int; i < a.bitLength(); i++) {
      res.set(i, a.get(i - shift_int));
    }

  }
  
}
