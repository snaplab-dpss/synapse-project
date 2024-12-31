#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class MeterInsert : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> success;

public:
  MeterInsert(const Node *node, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _success)
      : TofinoCPUModule(ModuleType::TofinoCPU_MeterInsert, "MeterInsert", node),
        obj(_obj), keys(_keys), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    MeterInsert *cloned = new MeterInsert(node, obj, keys, success);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_success() const { return success; }

  tofino::Meter *get_meter(const EP *ep) const {
    const Context &ctx = ep->get_ctx();
    const tofino::TofinoContext *tofino_ctx =
        ctx.get_target_ctx<tofino::TofinoContext>();
    const std::unordered_set<tofino::DS *> &data_structures =
        tofino_ctx->get_ds(obj);
    ASSERT(data_structures.size() == 1, "Invalid number of data structures");
    ASSERT((*data_structures.begin())->type == tofino::DSType::METER,
           "Invalid data structure type");
    return static_cast<tofino::Meter *>(*data_structures.begin());
  }
};

class MeterInsertGenerator : public TofinoCPUModuleGenerator {
public:
  MeterInsertGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_MeterInsert,
                                 "MeterInsert") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino_cpu
