#pragma once

#include "tofino_module.h"

namespace tofino {

class Ignore : public TofinoModule {
public:
  Ignore(const Node *node)
      : TofinoModule(ModuleType::Tofino_Ignore, "Ignore", node) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Ignore *cloned = new Ignore(node);
    return cloned;
  }
};

class IgnoreGenerator : public TofinoModuleGenerator {
private:
  std::unordered_set<std::string> functions_to_always_ignore = {
      "expire_items_single_map",
      "tb_expire",
      "nf_set_rte_ipv4_udptcp_checksum",
  };

public:
  IgnoreGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_Ignore, "Ignore") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (!should_ignore(ep, node)) {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (!should_ignore(ep, node)) {
      return impls;
    }

    Module *module = new Ignore(node);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  bool should_ignore(const EP *ep, const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (functions_to_always_ignore.find(call.function_name) !=
        functions_to_always_ignore.end()) {
      return true;
    }

    const Context &ctx = ep->get_ctx();

    if (can_ignore_fcfs_cached_table_op(ctx, call)) {
      return true;
    }

    if (call.function_name == "dchain_allocate_new_index" ||
        call.function_name == "dchain_rejuvenate_index") {
      return can_ignore_dchain_op(ctx, call);
    }

    if (call.function_name == "vector_return") {
      return is_vector_return_without_modifications(ep, call_node);
    }

    return false;
  }

  bool can_ignore_fcfs_cached_table_op(const Context &ctx,
                                       const call_t &call) const {
    if (call.function_name != "dchain_free_index") {
      return false;
    }

    klee::ref<klee::Expr> dchain = call.args.at("chain").expr;
    addr_t dchain_addr = expr_addr_to_obj_addr(dchain);

    std::optional<map_coalescing_objs_t> map_objs =
        ctx.get_map_coalescing_objs(dchain_addr);

    if (!map_objs.has_value()) {
      return false;
    }

    if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable)) {
      return false;
    }

    return true;
  }

  // We can ignore dchain_rejuvenate_index if the dchain is only used for
  // linking a map with vectors. It doesn't even matter if the data structures
  // are coalesced or not, we can freely ignore it regardless.
  bool can_ignore_dchain_op(const Context &ctx, const call_t &call) const {
    assert(call.function_name == "dchain_allocate_new_index" ||
           call.function_name == "dchain_rejuvenate_index");

    klee::ref<klee::Expr> chain = call.args.at("chain").expr;
    addr_t chain_addr = expr_addr_to_obj_addr(chain);

    std::optional<map_coalescing_objs_t> map_objs =
        ctx.get_map_coalescing_objs(chain_addr);

    if (!map_objs.has_value()) {
      return false;
    }

    // These are the data structures that can perform rejuvenations.
    if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_Table) &&
        !ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable)) {
      return false;
    }

    return true;
  }
};

} // namespace tofino
