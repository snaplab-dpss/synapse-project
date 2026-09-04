#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

unsigned hash_obj(void *obj, unsigned size_bytes);

// verifast doesn't know about these
unsigned __builtin_ia32_crc32si(unsigned acc, unsigned int x);
unsigned long long __builtin_ia32_crc32di(unsigned long long acc, unsigned long long x);

unsigned count_trailing_zeros(unsigned x);
unsigned power_of_two(unsigned exponent);
unsigned divide(unsigned numerator, unsigned denominator);
// Natural logarithm with a caller-provided fixed-point scale: returns ln(x) * scale.
unsigned ln(unsigned x, unsigned scale);

#endif
