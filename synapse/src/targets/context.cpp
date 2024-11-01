#include "context.h"
#include "target.h"
#include "tofino/tofino_context.h"
#include "tofino_cpu/tofino_cpu_context.h"
#include "x86/x86_context.h"
#include "module.h"
#include "../exprs/exprs.h"
#include "../exprs/solver.h"
#include "../bdd/bdd.h"

static void log_bdd_pre_processing(
    const std::vector<map_coalescing_objs_t> &coalescing_candidates) {
  Log::dbg() << "***** BDD pre-processing: *****\n";
  for (const map_coalescing_objs_t &candidate : coalescing_candidates) {
    std::stringstream ss;
    ss << "Coalescing candidate:";
    ss << " map=" << candidate.map;
    ss << ", ";
    ss << "dchain=" << candidate.dchain;
    ss << ", ";
    ss << "vector_key=" << candidate.vector_key;
    ss << ", ";
    ss << "vectors_values=[";

    size_t i = 0;
    for (addr_t vector : candidate.vectors_values) {
      if (i++ > 0) {
        ss << ", ";
      }
      ss << vector;
    }

    ss << "]\n";
    Log::dbg() << ss.str();
  }
  Log::dbg() << "*******************************\n";
}

static time_ns_t
exp_time_from_expire_items_single_map_time(const BDD *bdd,
                                           klee::ref<klee::Expr> time) {
  assert(time->getKind() == klee::Expr::Kind::Add);

  klee::ref<klee::Expr> lhs = time->getKid(0);
  klee::ref<klee::Expr> rhs = time->getKid(1);

  assert(lhs->getKind() == klee::Expr::Kind::Constant);

  const symbol_t &time_symbol = bdd->get_time();
  assert(solver_toolbox.are_exprs_always_equal(rhs, time_symbol.expr));

  u64 unsigned_exp_time = solver_toolbox.value_from_expr(lhs);
  time_ns_t exp_time = ~unsigned_exp_time + 1;

  return exp_time;
}

static std::optional<expiration_data_t> build_expiration_data(const BDD *bdd) {
  std::optional<expiration_data_t> expiration_data;

  const Node *root = bdd->get_root();

  root->visit_nodes([&bdd, &expiration_data](const Node *node) {
    if (node->get_type() != NodeType::CALL) {
      return NodeVisitAction::VISIT_CHILDREN;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "expire_items_single_map") {
      return NodeVisitAction::VISIT_CHILDREN;
    }

    klee::ref<klee::Expr> time = call.args.at("time").expr;
    time_ns_t exp_time = exp_time_from_expire_items_single_map_time(bdd, time);

    symbols_t symbols = call_node->get_locally_generated_symbols();
    symbol_t number_of_freed_flows;
    bool found =
        get_symbol(symbols, "number_of_freed_flows", number_of_freed_flows);
    assert(found && "Symbol number_of_freed_flows not found");

    expiration_data_t data = {
        .expiration_time = exp_time,
        .number_of_freed_flows = number_of_freed_flows,
    };

    expiration_data = data;

    return NodeVisitAction::STOP;
  });

  return expiration_data;
}

Context::Context(const BDD *bdd, const targets_t &targets,
                 const TargetType initial_target, const Profiler &_profiler)
    : profiler(_profiler), expiration_data(build_expiration_data(bdd)) {
  for (const Target *target : targets) {
    target_ctxs[target->type] = target->ctx->clone();
  }

  const std::vector<call_t> &init_calls = bdd->get_init();

  for (const call_t &call : init_calls) {
    if (call.function_name == "map_allocate") {
      klee::ref<klee::Expr> obj = call.args.at("map_out").out;
      addr_t addr = expr_addr_to_obj_addr(obj);
      map_config_t cfg = get_map_config_from_bdd(*bdd, addr);
      map_configs[addr] = cfg;

      map_coalescing_objs_t candidate;
      if (get_map_coalescing_objs_from_bdd(bdd, addr, candidate)) {
        coalescing_candidates.push_back(candidate);
      }

      continue;
    }

    if (call.function_name == "vector_allocate") {
      klee::ref<klee::Expr> obj = call.args.at("vector_out").out;
      addr_t addr = expr_addr_to_obj_addr(obj);
      vector_config_t cfg = get_vector_config_from_bdd(*bdd, addr);
      vector_configs[addr] = cfg;
      continue;
    }

    if (call.function_name == "dchain_allocate") {
      klee::ref<klee::Expr> obj = call.args.at("chain_out").out;
      addr_t addr = expr_addr_to_obj_addr(obj);
      dchain_config_t cfg = get_dchain_config_from_bdd(*bdd, addr);
      dchain_configs[addr] = cfg;
      continue;
    }

    if (call.function_name == "cms_allocate") {
      klee::ref<klee::Expr> obj = call.args.at("cms_out").out;
      addr_t addr = expr_addr_to_obj_addr(obj);
      cms_config_t cfg = get_cms_config_from_bdd(*bdd, addr);
      cms_configs[addr] = cfg;
      continue;
    }

    if (call.function_name == "cht_fill_cht") {
      klee::ref<klee::Expr> obj = call.args.at("cht").expr;
      addr_t addr = expr_addr_to_obj_addr(obj);
      cht_config_t cfg = get_cht_config_from_bdd(*bdd, addr);
      cht_configs[addr] = cfg;
      continue;
    }

    if (call.function_name == "tb_allocate") {
      klee::ref<klee::Expr> obj = call.args.at("tb_out").out;
      addr_t addr = expr_addr_to_obj_addr(obj);
      tb_config_t cfg = get_tb_config_from_bdd(*bdd, addr);
      tb_configs[addr] = cfg;
      continue;
    }

    PANIC("Unknown init call");
  }

  log_bdd_pre_processing(coalescing_candidates);
}

Context::Context(const Context &other)
    : profiler(other.profiler), map_configs(other.map_configs),
      vector_configs(other.vector_configs),
      dchain_configs(other.dchain_configs), cms_configs(other.cms_configs),
      cht_configs(other.cht_configs), tb_configs(other.tb_configs),
      coalescing_candidates(other.coalescing_candidates),
      expiration_data(other.expiration_data), ds_impls(other.ds_impls),
      constraints_per_node(other.constraints_per_node) {
  for (auto &target_ctx_pair : other.target_ctxs) {
    target_ctxs[target_ctx_pair.first] = target_ctx_pair.second->clone();
  }
}

Context::Context(Context &&other)
    : profiler(std::move(other.profiler)),
      map_configs(std::move(other.map_configs)),
      vector_configs(std::move(other.vector_configs)),
      dchain_configs(std::move(other.dchain_configs)),
      cms_configs(std::move(other.cms_configs)),
      cht_configs(std::move(other.cht_configs)),
      tb_configs(std::move(other.tb_configs)),
      coalescing_candidates(std::move(other.coalescing_candidates)),
      expiration_data(std::move(other.expiration_data)),
      ds_impls(std::move(other.ds_impls)),
      target_ctxs(std::move(other.target_ctxs)),
      constraints_per_node(std::move(other.constraints_per_node)) {}

Context::~Context() {
  for (auto &target_ctx_pair : target_ctxs) {
    if (target_ctx_pair.second) {
      delete target_ctx_pair.second;
      target_ctx_pair.second = nullptr;
    }
  }
}

Context &Context::operator=(const Context &other) {
  if (this == &other) {
    return *this;
  }

  for (auto &target_ctx_pair : target_ctxs) {
    if (target_ctx_pair.second) {
      delete target_ctx_pair.second;
      target_ctx_pair.second = nullptr;
    }
  }

  profiler = other.profiler;
  map_configs = other.map_configs;
  vector_configs = other.vector_configs;
  dchain_configs = other.dchain_configs;
  cms_configs = other.cms_configs;
  cht_configs = other.cht_configs;
  tb_configs = other.tb_configs;
  coalescing_candidates = other.coalescing_candidates;
  expiration_data = other.expiration_data;
  ds_impls = other.ds_impls;

  for (auto &target_ctx_pair : other.target_ctxs) {
    target_ctxs[target_ctx_pair.first] = target_ctx_pair.second->clone();
  }

  constraints_per_node = other.constraints_per_node;

  return *this;
}

const Profiler &Context::get_profiler() const { return profiler; }
Profiler &Context::get_mutable_profiler() { return profiler; }

const map_config_t &Context::get_map_config(addr_t addr) const {
  assert(map_configs.find(addr) != map_configs.end());
  return map_configs.at(addr);
}

const vector_config_t &Context::get_vector_config(addr_t addr) const {
  assert(vector_configs.find(addr) != vector_configs.end());
  return vector_configs.at(addr);
}

const dchain_config_t &Context::get_dchain_config(addr_t addr) const {
  assert(dchain_configs.find(addr) != dchain_configs.end());
  return dchain_configs.at(addr);
}

const cms_config_t &Context::get_cms_config(addr_t addr) const {
  assert(cms_configs.find(addr) != cms_configs.end());
  return cms_configs.at(addr);
}

const cht_config_t &Context::get_cht_config(addr_t addr) const {
  assert(cht_configs.find(addr) != cht_configs.end());
  return cht_configs.at(addr);
}

const tb_config_t &Context::get_tb_config(addr_t addr) const {
  assert(tb_configs.find(addr) != tb_configs.end());
  return tb_configs.at(addr);
}

std::optional<map_coalescing_objs_t>
Context::get_map_coalescing_objs(addr_t obj) const {
  for (const map_coalescing_objs_t &candidate : coalescing_candidates) {
    if (candidate.map == obj || candidate.dchain == obj ||
        candidate.vector_key == obj ||
        candidate.vectors_values.find(obj) != candidate.vectors_values.end()) {
      return candidate;
    }
  }

  return std::nullopt;
}

const std::optional<expiration_data_t> &Context::get_expiration_data() const {
  return expiration_data;
}

template <>
const tofino::TofinoContext *
Context::get_target_ctx<tofino::TofinoContext>() const {
  TargetType type = TargetType::Tofino;
  assert(target_ctxs.find(type) != target_ctxs.end());
  return dynamic_cast<const tofino::TofinoContext *>(target_ctxs.at(type));
}

template <>
const tofino_cpu::TofinoCPUContext *
Context::get_target_ctx<tofino_cpu::TofinoCPUContext>() const {
  TargetType type = TargetType::TofinoCPU;
  assert(target_ctxs.find(type) != target_ctxs.end());
  return dynamic_cast<const tofino_cpu::TofinoCPUContext *>(
      target_ctxs.at(type));
}

template <>
const x86::x86Context *Context::get_target_ctx<x86::x86Context>() const {
  TargetType type = TargetType::x86;
  assert(target_ctxs.find(type) != target_ctxs.end());
  return dynamic_cast<const x86::x86Context *>(target_ctxs.at(type));
}

template <>
tofino::TofinoContext *
Context::get_mutable_target_ctx<tofino::TofinoContext>() {
  TargetType type = TargetType::Tofino;
  assert(target_ctxs.find(type) != target_ctxs.end());
  return dynamic_cast<tofino::TofinoContext *>(target_ctxs.at(type));
}

template <>
tofino_cpu::TofinoCPUContext *
Context::get_mutable_target_ctx<tofino_cpu::TofinoCPUContext>() {
  TargetType type = TargetType::TofinoCPU;
  assert(target_ctxs.find(type) != target_ctxs.end());
  return dynamic_cast<tofino_cpu::TofinoCPUContext *>(target_ctxs.at(type));
}

template <>
x86::x86Context *Context::get_mutable_target_ctx<x86::x86Context>() {
  TargetType type = TargetType::x86;
  assert(target_ctxs.find(type) != target_ctxs.end());
  return dynamic_cast<x86::x86Context *>(target_ctxs.at(type));
}

void Context::save_ds_impl(addr_t obj, DSImpl impl) {
  assert(can_impl_ds(obj, impl) && "Incompatible implementation");
  ds_impls[obj] = impl;
}

bool Context::has_ds_impl(addr_t obj) const {
  return ds_impls.find(obj) != ds_impls.end();
}

bool Context::check_ds_impl(addr_t obj, DSImpl decision) const {
  auto found_it = ds_impls.find(obj);
  return found_it != ds_impls.end() && found_it->second == decision;
}

bool Context::can_impl_ds(addr_t obj, DSImpl decision) const {
  auto found_it = ds_impls.find(obj);
  return found_it == ds_impls.end() || found_it->second == decision;
}

const std::unordered_map<addr_t, DSImpl> &Context::get_ds_impls() const {
  return ds_impls;
}

void Context::update_constraints_per_node(ep_node_id_t node,
                                          const constraints_t &constraints) {
  assert(constraints_per_node.find(node) == constraints_per_node.end());
  constraints_per_node[node] = constraints;
}

constraints_t Context::get_node_constraints(const EPNode *node) const {
  assert(node);

  while (node) {
    ep_node_id_t node_id = node->get_id();
    auto found_it = constraints_per_node.find(node_id);

    if (found_it != constraints_per_node.end()) {
      return found_it->second;
    }

    node = node->get_prev();

    if (node) {
      assert(node->get_children().size() == 1 && "Ambiguous constraints");
    }
  }

  return {};
}

void Context::add_hit_rate_estimation(const constraints_t &constraints,
                                      klee::ref<klee::Expr> new_constraint,
                                      hit_rate_t estimation_rel) {
  Log::dbg() << "Adding hit rate estimation " << estimation_rel << "\n";
  for (const klee::ref<klee::Expr> &constraint : constraints) {
    Log::dbg() << "   " << expr_to_string(constraint, true) << "\n";
  }

  profiler.insert_relative(constraints, new_constraint, estimation_rel);

  Log::dbg() << "\n";
  Log::dbg() << "Resulting Hit rate:\n";
  profiler.debug();
  Log::dbg() << "\n";
}

void Context::remove_hit_rate_node(const constraints_t &constraints) {
  profiler.remove(constraints);
}

void Context::scale_profiler(const constraints_t &constraints,
                             hit_rate_t factor) {
  profiler.scale(constraints, factor);
}

std::ostream &operator<<(std::ostream &os, DSImpl impl) {
  switch (impl) {
  case DSImpl::Tofino_Table:
    os << "Tofino::Table";
    break;
  case DSImpl::Tofino_VectorRegister:
    os << "Tofino::Register";
    break;
  case DSImpl::Tofino_FCFSCachedTable:
    os << "Tofino::FCFSCachedTable";
    break;
  case DSImpl::Tofino_Meter:
    os << "Tofino::Meter";
    break;
  case DSImpl::TofinoCPU_Dchain:
    os << "TofinoCPU::Dchain";
    break;
  case DSImpl::TofinoCPU_Vector:
    os << "TofinoCPU::Vector";
    break;
  case DSImpl::TofinoCPU_CMS:
    os << "TofinoCPU::CMS";
    break;
  case DSImpl::TofinoCPU_Map:
    os << "TofinoCPU::Map";
    break;
  case DSImpl::TofinoCPU_Cht:
    os << "TofinoCPU::Cht";
    break;
  case DSImpl::TofinoCPU_TB:
    os << "TofinoCPU::TB";
    break;
  case DSImpl::x86_Map:
    os << "x86::Map";
    break;
  case DSImpl::x86_Vector:
    os << "x86::Vector";
    break;
  case DSImpl::x86_Dchain:
    os << "x86::Dchain";
    break;
  case DSImpl::x86_CMS:
    os << "x86::CMS";
    break;
  case DSImpl::x86_Cht:
    os << "x86::Cht";
    break;
  case DSImpl::x86_TB:
    os << "x86::TB";
    break;
  }
  return os;
}

void Context::debug() const {
  Log::dbg() << "~~~~~~~~~~~~~~~~~~~~~~~~ Context ~~~~~~~~~~~~~~~~~~~~~~~~\n";
  Log::dbg() << "Implementations: [\n";
  for (const auto &[obj, impl] : ds_impls) {
    Log::dbg() << "    " << obj << ": " << impl << "\n";
  }
  Log::dbg() << "]\n";

  for (const auto &[target, ctx] : target_ctxs) {
    ctx->debug();
  }

  Log::dbg() << "\n";
  Log::dbg() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
}