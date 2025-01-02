#include "context.h"
#include "../targets/targets.h"
#include "../exprs/exprs.h"
#include "../exprs/solver.h"
#include "../bdd/bdd.h"

namespace synapse {
namespace {
void log_bdd_pre_processing(
    const std::vector<map_coalescing_objs_t> &coalescing_candidates) {
  Log::dbg() << "***** BDD pre-processing: *****\n";
  for (const map_coalescing_objs_t &candidate : coalescing_candidates) {
    std::stringstream ss;
    ss << "Coalescing candidate:";
    ss << " map=" << candidate.map;
    ss << ", ";
    ss << "dchain=" << candidate.dchain;
    ss << ", ";
    ss << "vectors=[";

    size_t i = 0;
    for (addr_t vector : candidate.vectors) {
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

time_ns_t exp_time_from_expire_items_single_map_time(const BDD *bdd,
                                                     klee::ref<klee::Expr> time) {
  ASSERT(time->getKind() == klee::Expr::Kind::Add, "Invalid time expression");

  klee::ref<klee::Expr> lhs = time->getKid(0);
  klee::ref<klee::Expr> rhs = time->getKid(1);

  ASSERT(lhs->getKind() == klee::Expr::Kind::Constant, "Invalid time expression");

  const symbol_t &time_symbol = bdd->get_time();
  ASSERT(solver_toolbox.are_exprs_always_equal(rhs, time_symbol.expr),
         "Invalid time expression");

  u64 unsigned_exp_time = solver_toolbox.value_from_expr(lhs);
  time_ns_t exp_time = ~unsigned_exp_time + 1;

  return exp_time;
}

std::optional<expiration_data_t> build_expiration_data(const BDD *bdd) {
  std::optional<expiration_data_t> expiration_data;

  const Node *root = bdd->get_root();

  root->visit_nodes([&bdd, &expiration_data](const Node *node) {
    if (node->get_type() != NodeType::Call) {
      return NodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "expire_items_single_map") {
      return NodeVisitAction::Continue;
    }

    klee::ref<klee::Expr> time = call.args.at("time").expr;
    time_ns_t exp_time = exp_time_from_expire_items_single_map_time(bdd, time);

    symbols_t symbols = call_node->get_locally_generated_symbols();
    symbol_t number_of_freed_flows;
    bool found = get_symbol(symbols, "number_of_freed_flows", number_of_freed_flows);
    ASSERT(found, "Symbol number_of_freed_flows not found");

    expiration_data_t data = {
        .expiration_time = exp_time,
        .number_of_freed_flows = number_of_freed_flows,
    };

    expiration_data = data;

    return NodeVisitAction::Stop;
  });

  return expiration_data;
}
} // namespace

Context::Context(const BDD *bdd, const TargetsView &targets, const toml::table &config,
                 const Profiler &_profiler)
    : profiler(_profiler), perf_oracle(config, profiler.get_avg_pkt_bytes()),
      expiration_data(build_expiration_data(bdd)) {
  for (const TargetView &target : targets.elements) {
    target_ctxs[target.type] = target.base_ctx->clone();
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
    : profiler(other.profiler), perf_oracle(other.perf_oracle),
      map_configs(other.map_configs), vector_configs(other.vector_configs),
      dchain_configs(other.dchain_configs), cms_configs(other.cms_configs),
      cht_configs(other.cht_configs), tb_configs(other.tb_configs),
      coalescing_candidates(other.coalescing_candidates),
      expiration_data(other.expiration_data), ds_impls(other.ds_impls) {
  for (auto &target_ctx_pair : other.target_ctxs) {
    target_ctxs[target_ctx_pair.first] = target_ctx_pair.second->clone();
  }
}

Context::Context(Context &&other)
    : profiler(std::move(other.profiler)), perf_oracle(std::move(other.perf_oracle)),
      map_configs(std::move(other.map_configs)),
      vector_configs(std::move(other.vector_configs)),
      dchain_configs(std::move(other.dchain_configs)),
      cms_configs(std::move(other.cms_configs)),
      cht_configs(std::move(other.cht_configs)), tb_configs(std::move(other.tb_configs)),
      coalescing_candidates(std::move(other.coalescing_candidates)),
      expiration_data(std::move(other.expiration_data)),
      ds_impls(std::move(other.ds_impls)), target_ctxs(std::move(other.target_ctxs)) {}

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
  perf_oracle = other.perf_oracle;
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

  return *this;
}

const Profiler &Context::get_profiler() const { return profiler; }
Profiler &Context::get_mutable_profiler() { return profiler; }

const PerfOracle &Context::get_perf_oracle() const { return perf_oracle; }
PerfOracle &Context::get_mutable_perf_oracle() { return perf_oracle; }

const map_config_t &Context::get_map_config(addr_t addr) const {
  ASSERT(map_configs.find(addr) != map_configs.end(), "Map not found");
  return map_configs.at(addr);
}

const vector_config_t &Context::get_vector_config(addr_t addr) const {
  ASSERT(vector_configs.find(addr) != vector_configs.end(), "Vector not found");
  return vector_configs.at(addr);
}

const dchain_config_t &Context::get_dchain_config(addr_t addr) const {
  ASSERT(dchain_configs.find(addr) != dchain_configs.end(), "Dchain not found");
  return dchain_configs.at(addr);
}

const cms_config_t &Context::get_cms_config(addr_t addr) const {
  ASSERT(cms_configs.find(addr) != cms_configs.end(), "CMS not found");
  return cms_configs.at(addr);
}

const cht_config_t &Context::get_cht_config(addr_t addr) const {
  ASSERT(cht_configs.find(addr) != cht_configs.end(), "CHT not found");
  return cht_configs.at(addr);
}

const tb_config_t &Context::get_tb_config(addr_t addr) const {
  ASSERT(tb_configs.find(addr) != tb_configs.end(), "TB not found");
  return tb_configs.at(addr);
}

std::optional<map_coalescing_objs_t> Context::get_map_coalescing_objs(addr_t obj) const {
  for (const map_coalescing_objs_t &candidate : coalescing_candidates) {
    if (candidate.map == obj || candidate.dchain == obj ||
        candidate.vectors.find(obj) != candidate.vectors.end()) {
      return candidate;
    }
  }

  return std::nullopt;
}

const std::optional<expiration_data_t> &Context::get_expiration_data() const {
  return expiration_data;
}

void Context::save_ds_impl(addr_t obj, DSImpl impl) {
  ASSERT(can_impl_ds(obj, impl), "Incompatible implementation");
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
  case DSImpl::Tofino_MapRegister:
    os << "Tofino::MapRegister";
    break;
  case DSImpl::Tofino_Meter:
    os << "Tofino::Meter";
    break;
  case DSImpl::Tofino_HeavyHitterTable:
    os << "Tofino::HHTable";
    break;
  case DSImpl::Tofino_IntegerAllocator:
    os << "Tofino::IntegerAllocator";
    break;
  case DSImpl::Tofino_CountMinSketch:
    os << "Tofino::CMS";
    break;
  case DSImpl::Tofino_CuckooHashTable:
    os << "Tofino::CuckooHashTable";
    break;
  case DSImpl::TofinoCPU_DoubleChain:
    os << "TofinoCPU::Dchain";
    break;
  case DSImpl::TofinoCPU_Vector:
    os << "TofinoCPU::Vector";
    break;
  case DSImpl::TofinoCPU_CountMinSketch:
    os << "TofinoCPU::CMS";
    break;
  case DSImpl::TofinoCPU_Map:
    os << "TofinoCPU::Map";
    break;
  case DSImpl::TofinoCPU_ConsistentHashTable:
    os << "TofinoCPU::Cht";
    break;
  case DSImpl::TofinoCPU_TokenBucket:
    os << "TofinoCPU::TB";
    break;
  case DSImpl::x86_Map:
    os << "x86::Map";
    break;
  case DSImpl::x86_Vector:
    os << "x86::Vector";
    break;
  case DSImpl::x86_DoubleChain:
    os << "x86::Dchain";
    break;
  case DSImpl::x86_CountMinSketch:
    os << "x86::CMS";
    break;
  case DSImpl::x86_ConsistentHashTable:
    os << "x86::Cht";
    break;
  case DSImpl::x86_TokenBucket:
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

  perf_oracle.debug();

  Log::dbg() << "\n";
  Log::dbg() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
}
} // namespace synapse