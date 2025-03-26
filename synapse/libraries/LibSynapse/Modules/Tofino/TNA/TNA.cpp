#include <LibSynapse/Modules/Tofino/TNA/TNA.h>

namespace LibSynapse {
namespace Tofino {

TNA::TNA(const tna_config_t &config) : properties(config.properties), ports(config.ports), simple_placer(&properties) {}

TNA::TNA(const TNA &other) : properties(other.properties), ports(other.ports), simple_placer(other.simple_placer), parser(other.parser) {}

bool TNA::condition_meets_phv_limit(klee::ref<klee::Expr> expr) const {
  int total_packet_bytes_read          = 0;
  LibCore::symbolic_reads_t bytes_read = LibCore::get_unique_symbolic_reads(expr);
  for (const LibCore::symbolic_read_t &byte_read : bytes_read) {
    if (byte_read.symbol == "packet_chunks") {
      total_packet_bytes_read += 1;
    }
  }
  return total_packet_bytes_read <= properties.max_packet_bytes_in_condition;
}

void TNA::place(const DS *ds, const std::unordered_set<DS_ID> &deps) { simple_placer.place(ds, deps); }

PlacementStatus TNA::can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const { return simple_placer.can_place(ds, deps); }

PlacementStatus TNA::can_place_many(const std::vector<std::unordered_set<DS *>> &candidates, const std::unordered_set<DS_ID> &_deps) const {
  SimplePlacer snapshot(simple_placer);
  std::unordered_set<DS_ID> deps = _deps;

  for (const std::unordered_set<DS *> &indep_ds : candidates) {
    std::unordered_set<DS_ID> new_deps;

    for (const DS *ds : indep_ds) {
      PlacementStatus status = snapshot.can_place(ds, deps);

      if (status != PlacementStatus::SUCCESS) {
        return status;
      }

      snapshot.place(ds, deps);
      new_deps.insert(ds->id);
    }

    deps.insert(new_deps.begin(), new_deps.end());
  }

  return PlacementStatus::SUCCESS;
}

void TNA::debug() const { simple_placer.debug(); }

} // namespace Tofino
} // namespace LibSynapse