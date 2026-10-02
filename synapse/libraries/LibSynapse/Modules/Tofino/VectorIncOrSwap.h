#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// vector_inc_or_swap on a vector register of {key, count} pair cells: one register action counts
// the carried pair into the cell (same key), swaps it in (lighter cell, or an evicting call) or
// keeps the cell, and returns the old pair plus which happened; the carried pair after the call
// is then set from those. HashPipe's stage, with the carried pair riding the packet's metadata.
class VectorIncOrSwap : public TofinoModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> pair_in;
  klee::ref<klee::Expr> pair_out;
  bits_t key_size;
  bool evict;
  bool pair_out_used; // Nothing reads the pair carried out of the last stage: no need to compute it.

public:
  VectorIncOrSwap(const BDDNode *_node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _pair_in,
                  klee::ref<klee::Expr> _pair_out, bits_t _key_size, bool _evict, bool _pair_out_used)
      : TofinoModule(ModuleType::Tofino_VectorIncOrSwap, "VectorIncOrSwap", _node), id(_id), obj(_obj), index(_index), pair_in(_pair_in),
        pair_out(_pair_out), key_size(_key_size), evict(_evict), pair_out_used(_pair_out_used) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new VectorIncOrSwap(node, id, obj, index, pair_in, pair_out, key_size, evict, pair_out_used); }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_pair_in() const { return pair_in; }
  klee::ref<klee::Expr> get_pair_out() const { return pair_out; }
  bits_t get_key_size() const { return key_size; }
  bool get_evict() const { return evict; }
  bool get_pair_out_used() const { return pair_out_used; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {id}; }
};

class VectorIncOrSwapFactory : public TofinoModuleFactory {
public:
  VectorIncOrSwapFactory() : TofinoModuleFactory(ModuleType::Tofino_VectorIncOrSwap, "VectorIncOrSwap") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
