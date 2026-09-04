#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>

namespace LibSynapse {
namespace Tofino {

// Shared base for the per-function compute-lookup modules. Each concrete module
// (one per file: ComputeTableCountTrailingZeros, ComputeTableFindFirstSetBit,
// ComputeTablePowerOfTwo, ComputeTableLn) maps one stateless math call to a match
// table whose const entries are precomputed from that function. They differ only
// in which call they match and how their entries are built; everything else (the
// table build/placement and the P4 emission) is shared.
class ComputeTable : public TofinoModule {
protected:
  DS_ID table_id;
  klee::ref<klee::Expr> in;
  klee::ref<klee::Expr> out;

public:
  ComputeTable(ModuleType _type, const std::string &_name, const BDDNode *_node, DS_ID _table_id, klee::ref<klee::Expr> _in,
               klee::ref<klee::Expr> _out)
      : TofinoModule(_type, _name, _node), table_id(_table_id), in(_in), out(_out) {}

  DS_ID get_table_id() const { return table_id; }
  klee::ref<klee::Expr> get_in() const { return in; }
  klee::ref<klee::Expr> get_out() const { return out; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {table_id}; }
};

// True if `node` is a Call to `fn`.
bool compute_table_matches(const BDDNode *node, const std::string &fn);

// Build the EP for a compute table: create the const-populated Table, place it,
// and attach the (already-constructed) module. The Table id is taken from the module.
std::unique_ptr<EP> build_compute_table_ep(const EP *ep, const BDDNode *node, ComputeTable *module, klee::ref<klee::Expr> in, TableMatch match,
                                           const std::vector<table_entry_t> &entries);

} // namespace Tofino
} // namespace LibSynapse
