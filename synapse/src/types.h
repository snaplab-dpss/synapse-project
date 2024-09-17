#pragma once

#include <stdint.h>
#include <unordered_set>

#define MIN_THROUGHPUT_GIGABIT_PER_SEC 1   // Gbps
#define MAX_THROUGHPUT_GIGABIT_PER_SEC 100 // Gbps

#define MIN_PACKET_SIZE_BYTES 64
#define MAX_PACKET_SIZE_BYTES 1500

#define TRILLION 1000000000000LLU
#define BILLION 1000000000LLU

#define THOUSAND 1000LLU
#define MILLION 1000000LLU
#define BILLION 1000000000LLU

#define UINT_16_SWAP_ENDIANNESS(p) ((((p) & 0xff) << 8) | ((p) >> 8 & 0xff))

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef u32 bits_t;
typedef u32 bytes_t;

typedef u64 addr_t;
typedef u64 obj_t;
typedef std::unordered_set<obj_t> objs_t;

typedef i64 time_s_t;
typedef i64 time_ms_t;
typedef i64 time_us_t;
typedef i64 time_ns_t;
typedef i64 time_ps_t;

typedef u64 pps_t;
typedef u64 bps_t;

typedef double hit_rate_t;

#define bit_to_byte(B) ((B) / 8)
#define byte_to_bit(B) ((B) * 8)

#define gbps_to_bps(R) ((R) * BILLION)

#define min_to_s(T) ((T) * 60)

#define s_to_min(T) ((T) / (double)(60))
#define s_to_us(T) ((T) * MILLION)
#define s_to_ns(T) ((T) * BILLION)

#define us_to_s(T) ((double)(T) / (double)(MILLION))
#define us_to_ns(T) ((T) * THOUSAND)

#define ns_to_s(T) ((double)(T) / (double)(BILLION))
#define ns_to_us(T) ((T) / THOUSAND)
#define ns_to_ps(T) ((time_ps_t)((T) * THOUSAND))

#define ps_to_ns(T) ((time_ns_t)((double)(T) / (double)THOUSAND))