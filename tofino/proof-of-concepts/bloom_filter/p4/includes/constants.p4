#ifndef __CONSTANTS__
#define __CONSTANTS__

#define HASH_SIZE_BITS 8
#define WIDTH 1024

typedef bit<32> entry_t;
typedef bit<HASH_SIZE_BITS> hash_t;

// Randomly generated
const bit<32> HASH_SALT_0 = 0xfbc31fc7;
const bit<32> HASH_SALT_1 = 0x2681580b;
const bit<32> HASH_SALT_2 = 0x486d7e2f;

#endif /* __CONSTANTS__ */