#ifndef __CONSTANTS__
#define __CONSTANTS__

typedef bit<9> dev_port_t;

#define KEY_SIZE_BITS 8
#define EXPIRATOR_CAPACITY (1 << KEY_SIZE_BITS)
#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds

typedef bit<8>  key_t;
typedef bit<8>  op_t;
typedef bit<8>  bool_t;
typedef bit<32> time_t;

const time_t EXPIRATION_TIME = 5 * ONE_SECOND;

const op_t READ  = 0;
const op_t WRITE = 1;

#endif /* __CONSTANTS__ */