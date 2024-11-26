#pragma once

#include <stdint.h>
#include <unordered_set>
#include <vector>

#include <klee/Constraints.h>

#define THOUSANDS_SEPARATOR "'"

#define TRILLION 1000000000000LLU
#define BILLION 1000000000LLU

#define THOUSAND 1000LLU
#define MILLION 1000000LLU
#define BILLION 1000000000LLU

#define NEWTON_MAX_ITERATIONS 10
#define NEWTON_PRECISION 1e-3

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
typedef u64 Bps_t;

typedef u64 fpm_t; // churn in flows per minute
typedef u64 fps_t; // churn in flows per second

typedef double hit_rate_t;

#define MIN_THROUGHPUT_GIGABIT_PER_SEC ((gbps_t)1)
#define MAX_THROUGHPUT_GIGABIT_PER_SEC ((gbps_t)100)

#define MIN_PACKET_SIZE_BYTES ((byte_t)64)
#define MAX_PACKET_SIZE_BYTES ((byte_t)1500)

#define bit_to_byte(B) ((byte_t)((B) / 8))
#define byte_to_bit(B) ((bit_t)((B) * 8))
#define align_to_byte(B) ((B) % 8 == 0 ? (B) : (B) + 8 - (B) % 8)

#define gbps_to_bps(R) ((bps_t)((R) * BILLION))

#define min_to_s(T) ((time_s_t)((T) * 60))

#define s_to_min(T) ((time_min_t)((T) / (double)(60)))
#define s_to_us(T) ((time_us_t)((T) * MILLION))
#define s_to_ns(T) ((time_ns_t)((T) * BILLION))

#define us_to_s(T) ((time_s_t)((double)(T) / (double)(MILLION)))
#define us_to_ns(T) ((time_ns_t)((T) * THOUSAND))

#define ns_to_s(T) ((time_s_t)((double)(T) / (double)(BILLION)))
#define ns_to_us(T) ((time_us_t)((T) / THOUSAND))
#define ns_to_ps(T) ((time_ps_t)((T) * THOUSAND))

#define ps_to_ns(T) ((time_ns_t)((double)(T) / (double)THOUSAND))

#define STABLE_TPUT_PRECISION ((pps_t)100) // pps

typedef u64 ep_id_t;
typedef u64 ep_node_id_t;
typedef std::vector<klee::ref<klee::Expr>> constraints_t;