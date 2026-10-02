#include <LibSynapse/Modules/Tofino/Crc32HasherHash.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

namespace {

// The width the hash is used at: a hash that only indexes a register through `hash & (2^k - 1)`
// (the next node's index) is computed k bits wide, so the hash unit's output is the index itself
// and no stage is spent masking it.
bits_t used_hash_width(const Call *node, klee::ref<klee::Expr> hash) {
  const BDDNode *next = node->get_next();
  if (!next || next->get_type() != BDDNodeType::Call) {
    return hash->getWidth();
  }
  const call_t &call = dynamic_cast<const Call *>(next)->get_call();
  if (call.function_name != "vector_inc_or_swap") {
    return hash->getWidth();
  }
  klee::ref<klee::Expr> index = call.args.at("index").expr;
  if (index->getKind() != klee::Expr::And || !LibCore::is_constant(index->getKid(1)) ||
      !solver_toolbox.are_exprs_always_equal(index->getKid(0), hash)) {
    return hash->getWidth();
  }
  const u64 mask = solver_toolbox.value_from_expr(index->getKid(1));
  if (mask == 0 || (mask & (mask + 1)) != 0) {
    return hash->getWidth();
  }
  bits_t k = 0;
  while ((1ull << k) <= mask) {
    k++;
  }
  return k;
}

struct hash_data_t {
  addr_t hasher;
  klee::ref<klee::Expr> in;
  klee::ref<klee::Expr> hash;
  DS_ID hash_id;
  std::vector<bits_t> keys;
  bits_t size;
  crc32_config_t polynomial;
};

bool matches(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  return dynamic_cast<const Call *>(node)->get_call().function_name == "crc32_hasher_hash";
}

hash_data_t get_hash_data(const Context &ctx, const Call *node) {
  const call_t &call = node->get_call();

  klee::ref<klee::Expr> hasher_expr = call.args.at("hasher").expr;
  klee::ref<klee::Expr> in          = call.args.at("data").in;
  klee::ref<klee::Expr> hash        = call.ret;

  const addr_t hasher                     = expr_addr_to_obj_addr(hasher_expr);
  const crc32_hasher_config_t &hasher_cfg = ctx.get_crc32_hasher_config(hasher);

  // A bank entry keeps its name; any other configuration is emitted as it is.
  crc32_config_t polynomial{"hasher", hasher_cfg.polynomial, hasher_cfg.reversed, hasher_cfg.init, hasher_cfg.xor_out};
  for (const crc32_config_t &entry : CRC32_BANK) {
    if (entry.coeff == hasher_cfg.polynomial && entry.reversed == hasher_cfg.reversed && entry.init == hasher_cfg.init &&
        entry.xor_out == hasher_cfg.xor_out) {
      polynomial = entry;
      break;
    }
  }

  return hash_data_t{
      .hasher     = hasher,
      .in         = in,
      .hash       = hash,
      .hash_id    = "hash_" + std::to_string(node->get_id()),
      .keys       = {in->getWidth()},
      .size       = used_hash_width(node, hash),
      .polynomial = polynomial,
  };
}

} // namespace

std::optional<spec_impl_t> Crc32HasherHashFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!matches(node)) {
    return {};
  }
  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> Crc32HasherHashFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!matches(node)) {
    return {};
  }

  const hash_data_t data = get_hash_data(ep->get_ctx(), dynamic_cast<const Call *>(node));

  Hash *hash = new Hash(data.hash_id, data.keys, data.size, data.polynomial);

  Module *module  = new Crc32HasherHash(node, data.hash_id, data.hasher, data.in, data.hash);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, data.hasher, hash);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> Crc32HasherHashFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!matches(node)) {
    return {};
  }

  const hash_data_t data = get_hash_data(ctx, dynamic_cast<const Call *>(node));

  return std::make_unique<Crc32HasherHash>(node, data.hash_id, data.hasher, data.in, data.hash);
}

} // namespace Tofino
} // namespace LibSynapse
