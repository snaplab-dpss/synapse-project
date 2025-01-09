#pragma once

#include <unordered_set>

#include "../data_structures/data_structures.h"

namespace synapse {
namespace tofino {

enum class PlacementStatus {
  SUCCESS,
  TOO_LARGE,
  TOO_MANY_KEYS,
  XBAR_CONSUME_EXCEEDS_LIMIT,
  NO_AVAILABLE_STAGE,
  INCONSISTENT_PLACEMENT,
  SELF_DEPENDENCE,
  UNKNOWN,
};

std::ostream &operator<<(std::ostream &os, const PlacementStatus &status);

struct Stage {
  int stage_id;
  bits_t available_sram;
  bits_t available_tcam;
  bits_t available_map_ram;
  bits_t available_exact_match_xbar;
  int available_logical_ids;
  std::unordered_set<DS_ID> tables;
};

struct PlacementRequest {
  const DS *ds;
  std::unordered_set<DS_ID> deps;
};

struct TNAProperties;

class SimplePlacer {
private:
  const TNAProperties *properties;
  std::vector<Stage> stages;
  std::vector<PlacementRequest> placement_requests;

public:
  SimplePlacer(const TNAProperties *properties);
  SimplePlacer(const SimplePlacer &other);
  ~SimplePlacer();

  void place(const DS *ds, const std::unordered_set<DS_ID> &deps);
  PlacementStatus can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const;

  void debug() const;

private:
  struct placement_t;

  void place_primitive_ds(const DS *ds, const std::unordered_set<DS_ID> &deps);

  bool is_placed(DS_ID ds_id) const;
  bool is_self_dependent(DS_ID ds_id, const std::unordered_set<DS_ID> &deps) const;
  PlacementStatus is_consistent(DS_ID ds_id, const std::unordered_set<DS_ID> &deps) const;

  PlacementStatus find_placements(const DS *ds, const std::unordered_set<DS_ID> &deps, std::vector<placement_t> &placements) const;
  PlacementStatus find_placements_table(const Table *table, const std::unordered_set<DS_ID> &deps,
                                        std::vector<placement_t> &placements) const;
  PlacementStatus find_placements_reg(const Register *reg, const std::unordered_set<DS_ID> &deps,
                                      std::vector<placement_t> &placements) const;
  PlacementStatus find_placements_meter(const Meter *table, const std::unordered_set<DS_ID> &deps,
                                        std::vector<placement_t> &placements) const;

  PlacementStatus find_placements_hash(const Hash *hash, const std::unordered_set<DS_ID> &deps, std::vector<placement_t> &placements) const;

  void concretize_placement(Stage &stage, const placement_t &placement);

  void save_placement_request(const DS *ds, const std::unordered_set<DS_ID> &deps);
  void replace_placement_request(const DS *ds, const std::unordered_set<DS_ID> &deps, const std::vector<Stage> &new_stages);
};

} // namespace tofino
} // namespace synapse