#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibSynapse/Modules/Tofino/TNA/SimplerPlacer.h>
#include <LibSynapse/Modules/Tofino/TNA/Parser.h>
#include <LibSynapse/Modules/Tofino/TNA/TNAProperties.h>
#include <LibCore/Types.h>

#include <unordered_map>

namespace LibSynapse {
namespace Tofino {

class TNA {
public:
  // Actually, we usually only get around 90% of usage from the dataplane tables.
  // Higher than that and we start getting collisions, and errors trying to insert new entries.
  constexpr const static double TABLE_CAPACITY_EFFICIENCY{0.9};

  const tna_config_t tna_config;

private:
  SimplePlacer simple_placer;

public:
  Parser parser;

  TNA(const tna_config_t &config);
  TNA(const TNA &other);

  const SimplePlacer &get_simple_placer() const { return simple_placer; }

  // Tofino compiler complains if we access more than 4 bytes of the packet on
  // the same if statement.
  bool condition_meets_phv_limit(klee::ref<klee::Expr> expr) const;

  void place(const DS *ds, const std::unordered_set<DS_ID> &deps);
  PlacementStatus can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const;
  PlacementStatus can_place_many(const std::vector<std::unordered_set<DS *>> &ds, const std::unordered_set<DS_ID> &deps) const;

  void debug() const;
};

} // namespace Tofino
} // namespace LibSynapse