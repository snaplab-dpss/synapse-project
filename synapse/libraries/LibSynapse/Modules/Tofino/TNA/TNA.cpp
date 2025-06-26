#include <LibSynapse/Modules/Tofino/TNA/TNA.h>

namespace LibSynapse {
namespace Tofino {

bool TNA::condition_meets_phv_limit(klee::ref<klee::Expr> expr) const {
  bytes_t total_packet_bytes_read      = 0U;
  LibCore::symbolic_reads_t bytes_read = LibCore::get_unique_symbolic_reads(expr);
  for (const LibCore::symbolic_read_t &byte_read : bytes_read) {
    if (byte_read.symbol == "packet_chunks") {
      total_packet_bytes_read += 1;
    }
  }
  return total_packet_bytes_read <= tna_config.properties.max_packet_bytes_in_condition;
}

void TNA::debug() const { pipeline.debug(); }

} // namespace Tofino
} // namespace LibSynapse