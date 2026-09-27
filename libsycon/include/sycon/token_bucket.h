#pragma once

#include "args.h"
#include "libnf.h"
#include "primitives/table.h"

namespace sycon {

// The rate (bytes/s) and burst (bytes) a token bucket is built with: the NF's, unless a test
// overrides them (--test-token-bucket-bytes-per-sec/-burst-bytes: driving a bucket out of
// profile, which the NF's own 17 GB/s bucket never is).
inline meter_spec_t token_bucket_spec(u64 bytes_per_sec, u64 burst_bytes) {
  if (args.test_token_bucket_bytes_per_sec.has_value()) {
    return {*args.test_token_bucket_bytes_per_sec, *args.test_token_bucket_burst_bytes};
  }
  return {bytes_per_sec, burst_bytes};
}

// libnf's token bucket, built through the same override as the data-plane meters.
inline int tb_allocate(u32 capacity, u64 rate, u64 burst, u32 key_size, libnf::TokenBucket **tb_out) {
  const meter_spec_t spec = token_bucket_spec(rate, burst);
  return libnf::tb_allocate(capacity, spec.rate, spec.burst, key_size, tb_out);
}

} // namespace sycon
