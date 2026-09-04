#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

unsigned hash_obj(void *obj, unsigned size_bytes);

// verifast doesn't know about these
unsigned __builtin_ia32_crc32si(unsigned acc, unsigned int x);
unsigned long long __builtin_ia32_crc32di(unsigned long long acc, unsigned long long x);

unsigned count_trailing_zeros(unsigned x);

// Returns the 1-indexed position of the least-significant set bit of x, or 0 if x
// is zero. Equivalently: the number of trailing zeros plus one, with zero mapped
// to zero (matching POSIX ffs). Examples:
//   find_first_set_bit(0b0001) = 1   (lowest bit set)
//   find_first_set_bit(0b1000) = 4   (bit 3 is the lowest set bit)
//   find_first_set_bit(0)      = 0   (no bits set)
unsigned find_first_set_bit(unsigned x);

// Returns the smaller of a and b.
unsigned min(unsigned a, unsigned b);

unsigned power_of_two(unsigned exponent);
unsigned divide(unsigned numerator, unsigned denominator);
// Natural logarithm with a caller-provided fixed-point scale: returns ln(x) * scale.
unsigned ln(unsigned x, unsigned scale);

#endif
