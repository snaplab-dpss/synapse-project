#include <LibSynapse/Context.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/ParserCondition.h>
#include <LibCore/Solver.h>
#include <LibCore/Expr.h>
#include <LibCore/Debug.h>

namespace LibSynapse {

namespace {

time_ns_t exp_time_from_expire_items_single_map_time(const LibBDD::BDD *bdd, klee::ref<klee::Expr> time) {
  assert(time->getKind() == klee::Expr::Kind::Add && "Invalid time expression");

  klee::ref<klee::Expr> lhs = time->getKid(0);
  klee::ref<klee::Expr> rhs = time->getKid(1);

  assert(lhs->getKind() == klee::Expr::Kind::Constant && "Invalid time expression");

  const LibCore::symbol_t &time_symbol = bdd->get_time();
  assert(LibCore::solver_toolbox.are_exprs_always_equal(rhs, time_symbol.expr) && "Invalid time expression");

  const u64 unsigned_exp_time = LibCore::solver_toolbox.value_from_expr(lhs);
  const time_ns_t exp_time    = ~unsigned_exp_time + 1;

  return exp_time;
}

std::optional<expiration_data_t> build_expiration_data(const LibBDD::BDD *bdd) {
  std::optional<expiration_data_t> expiration_data;

  const LibBDD::Node *root = bdd->get_root();

  root->visit_nodes([&bdd, &expiration_data](const LibBDD::Node *node) {
    if (node->get_type() != LibBDD::NodeType::Call) {
      return LibBDD::NodeVisitAction::Continue;
    }

    const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
    const LibBDD::call_t &call    = call_node->get_call();

    if (call.function_name != "expire_items_single_map") {
      return LibBDD::NodeVisitAction::Continue;
    }

    klee::ref<klee::Expr> time = call.args.at("time").expr;
    const time_ns_t exp_time   = exp_time_from_expire_items_single_map_time(bdd, time);

    const LibCore::symbol_t number_of_freed_flows = call_node->get_local_symbol("number_of_freed_flows");

    const expiration_data_t data{
        .expiration_time       = exp_time,
        .number_of_freed_flows = number_of_freed_flows,
    };

    expiration_data = data;

    return LibBDD::NodeVisitAction::Stop;
  });

  return expiration_data;
}

} // namespace

void Context::bdd_pre_processing_get_coalescing_candidates(const LibBDD::BDD *bdd) {
  const std::vector<LibBDD::Call *> &init = bdd->get_init();

  for (const LibBDD::Call *call_node : init) {
    const LibBDD::call_t &call = call_node->get_call();

    if (call.function_name == "map_allocate") {
      klee::ref<klee::Expr> obj = call.args.at("map_out").out;
      const addr_t addr         = LibCore::expr_addr_to_obj_addr(obj);

      LibBDD::map_coalescing_objs_t candidate;
      if (bdd->get_map_coalescing_objs(addr, candidate)) {
        coalescing_candidates.push_back(candidate);
      }
    }
  }
}

void Context::bdd_pre_processing_get_ds_configs(const LibBDD::BDD *bdd) {
  const std::vector<LibBDD::Call *> &init = bdd->get_init();

  for (const LibBDD::Call *call_node : init) {
    const LibBDD::call_t &call = call_node->get_call();

    if (call.function_name == "map_allocate") {
      klee::ref<klee::Expr> obj      = call.args.at("map_out").out;
      const addr_t addr              = LibCore::expr_addr_to_obj_addr(obj);
      const LibBDD::map_config_t cfg = get_map_config_from_bdd(*bdd, addr);
      map_configs[addr]              = cfg;
      continue;
    }

    if (call.function_name == "vector_allocate") {
      klee::ref<klee::Expr> obj         = call.args.at("vector_out").out;
      const addr_t addr                 = LibCore::expr_addr_to_obj_addr(obj);
      const LibBDD::vector_config_t cfg = get_vector_config_from_bdd(*bdd, addr);
      vector_configs[addr]              = cfg;
      continue;
    }

    if (call.function_name == "dchain_allocate") {
      klee::ref<klee::Expr> obj         = call.args.at("chain_out").out;
      const addr_t addr                 = LibCore::expr_addr_to_obj_addr(obj);
      const LibBDD::dchain_config_t cfg = get_dchain_config_from_bdd(*bdd, addr);
      dchain_configs[addr]              = cfg;
      continue;
    }

    if (call.function_name == "cms_allocate") {
      klee::ref<klee::Expr> obj      = call.args.at("cms_out").out;
      const addr_t addr              = LibCore::expr_addr_to_obj_addr(obj);
      const LibBDD::cms_config_t cfg = get_cms_config_from_bdd(*bdd, addr);
      cms_configs[addr]              = cfg;
      continue;
    }

    if (call.function_name == "cht_fill_cht") {
      klee::ref<klee::Expr> obj      = call.args.at("cht").expr;
      const addr_t addr              = LibCore::expr_addr_to_obj_addr(obj);
      const LibBDD::cht_config_t cfg = get_cht_config_from_bdd(*bdd, addr);
      cht_configs[addr]              = cfg;
      continue;
    }

    if (call.function_name == "tb_allocate") {
      klee::ref<klee::Expr> obj     = call.args.at("tb_out").out;
      const addr_t addr             = LibCore::expr_addr_to_obj_addr(obj);
      const LibBDD::tb_config_t cfg = get_tb_config_from_bdd(*bdd, addr);
      tb_configs[addr]              = cfg;
      continue;
    }
  }
}

void Context::bdd_pre_processing_get_structural_fields(const LibBDD::BDD *bdd) {
  bdd->get_root()->visit_nodes([this](const LibBDD::Node *node) {
    if (node->get_type() != LibBDD::NodeType::Call) {
      return LibBDD::NodeVisitAction::Continue;
    }

    const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
    const LibBDD::call_t &call    = call_node->get_call();

    if (call.function_name == "packet_borrow_next_chunk") {
      LibCore::expr_struct_t header;
      if (call_node->guess_header_fields_from_packet_borrow(header)) {
        expr_structs.push_back(header);
      }
    } else if (call.function_name == "vector_borrow") {
      LibCore::expr_struct_t value_struct;
      if (call_node->guess_value_fields_from_vector_borrow(value_struct)) {
        bool found = false;
        for (LibCore::expr_struct_t &existing_struct : expr_structs) {
          const bool same_expr = LibCore::solver_toolbox.are_exprs_always_equal(existing_struct.expr, value_struct.expr);

          if (!same_expr) {
            continue;
          }

          found = true;

          if (existing_struct.fields.size() < value_struct.fields.size()) {
            existing_struct.fields = value_struct.fields;
          }

          break;
        }

        if (!found) {
          expr_structs.push_back(value_struct);
        }
      }
    }

    return LibBDD::NodeVisitAction::Continue;
  });
}

void Context::bdd_pre_processing_build_tofino_parser(const LibBDD::BDD *bdd) {
  const TargetType type = TargetType::Tofino;

  if (target_ctxs.find(type) == target_ctxs.end()) {
    return;
  }

  Tofino::TofinoContext *tofino_ctx = dynamic_cast<Tofino::TofinoContext *>(target_ctxs.at(type));

  struct parser_operations_t : LibBDD::cookie_t {
    std::vector<const LibBDD::Node *> nodes;

    parser_operations_t() {}
    parser_operations_t(const parser_operations_t &other) : nodes(other.nodes) {}

    const LibBDD::Node *get_last_op(const LibBDD::Node *node, std::optional<bool> &direction) {
      if (nodes.empty()) {
        return nullptr;
      }

      const LibBDD::Node *last_op = nodes.back();

      if (last_op->get_type() == LibBDD::NodeType::Branch) {
        const LibBDD::Branch *branch_node = dynamic_cast<const LibBDD::Branch *>(last_op);

        const LibBDD::Node *on_true  = branch_node->get_on_true();
        const LibBDD::Node *on_false = branch_node->get_on_false();

        if (node->is_reachable(on_true->get_id())) {
          direction = true;
        } else if (node->is_reachable(on_false->get_id())) {
          direction = false;
        } else {
          panic("Invalid parser state: node %lu is not reachable from last parser state %lu", node->get_id(), last_op->get_id());
        }
      }

      return last_op;
    }

    std::unique_ptr<cookie_t> clone() const override { return std::make_unique<parser_operations_t>(*this); }
  };

  bdd->get_root()->visit_nodes(
      [this, tofino_ctx](const LibBDD::Node *node, LibBDD::cookie_t *cookie) {
        parser_operations_t *parser_ops = dynamic_cast<parser_operations_t *>(cookie);

        switch (node->get_type()) {
        case LibBDD::NodeType::Call: {
          const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
          const LibBDD::call_t &call    = call_node->get_call();

          if (call.function_name == "packet_borrow_next_chunk") {
            klee::ref<klee::Expr> hdr = call.extra_vars.at("the_chunk").second;

            std::optional<bool> direction;
            const LibBDD::Node *last_parser_op = parser_ops->get_last_op(node, direction);

            tofino_ctx->parser_transition(node, hdr, last_parser_op, direction);
            parser_ops->nodes.push_back(node);
          }

        } break;
        case LibBDD::NodeType::Branch: {
          const LibBDD::Branch *branch_node = dynamic_cast<const LibBDD::Branch *>(node);
          klee::ref<klee::Expr> condition   = branch_node->get_condition();

          if (branch_node->is_parser_condition()) {
            const Tofino::parser_selection_t selection = Tofino::ParserConditionFactory::build_parser_select(condition);

            std::optional<bool> direction;
            const LibBDD::Node *last_parser_op = parser_ops->get_last_op(node, direction);

            tofino_ctx->parser_select(node, selection, last_parser_op, direction);
            parser_ops->nodes.push_back(node);
          }
        } break;
        case LibBDD::NodeType::Route: {
          const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);

          std::optional<bool> direction;
          const LibBDD::Node *last_parser_op = parser_ops->get_last_op(node, direction);

          if (route_node->get_operation() == LibBDD::RouteOp::Drop && last_parser_op &&
              last_parser_op->get_type() == LibBDD::NodeType::Branch) {
            tofino_ctx->parser_reject(node, last_parser_op, direction);
          } else {
            tofino_ctx->parser_accept(node, last_parser_op, direction);
          }

          parser_ops->nodes.push_back(node);
        } break;
        }

        return LibBDD::NodeVisitAction::Continue;
      },
      std::make_unique<parser_operations_t>());
}

void Context::bdd_pre_processing_log() {
  std::cerr << "***** BDD pre-processing: *****\n";
  std::cerr << "Coalescing candidates:\n";
  for (const LibBDD::map_coalescing_objs_t &candidate : coalescing_candidates) {
    std::cerr << "  ";
    std::cerr << " map=" << candidate.map << ", dchain=" << candidate.dchain << ", vectors=[";
    size_t i = 0;
    for (addr_t vector : candidate.vectors) {
      if (i++ > 0) {
        std::cerr << ", ";
      }
      std::cerr << vector;
    }
    std::cerr << "]\n";
  }

  std::cerr << "\n";
  std::cerr << "Structural estimations:\n";
  for (const LibCore::expr_struct_t &expr_struct : expr_structs) {
    std::cerr << "--------------------------------\n";
    std::cerr << "Expr: " << LibCore::expr_to_string(expr_struct.expr, true) << "\n";
    std::cerr << "Fields:\n";
    for (const klee::ref<klee::Expr> &field : expr_struct.fields) {
      std::cerr << "  " << LibCore::expr_to_string(field, true) << "\n";
    }
    std::cerr << "--------------------------------\n";
  }

  std::cerr << "*******************************\n";
}

Context::Context(const LibBDD::BDD *bdd, const TargetsView &targets, const targets_config_t &targets_config, const Profiler &_profiler)
    : profiler(_profiler), perf_oracle(targets_config, profiler.get_avg_pkt_bytes()), expiration_data(build_expiration_data(bdd)) {
  for (const TargetView &target : targets.elements) {
    target_ctxs[target.type] = target.base_ctx->clone();
  }

  bdd_pre_processing_get_coalescing_candidates(bdd);
  bdd_pre_processing_get_ds_configs(bdd);
  bdd_pre_processing_get_structural_fields(bdd);
  bdd_pre_processing_build_tofino_parser(bdd);
  bdd_pre_processing_log();
}

Context::Context(const Context &other)
    : profiler(other.profiler), perf_oracle(other.perf_oracle), map_configs(other.map_configs), vector_configs(other.vector_configs),
      dchain_configs(other.dchain_configs), cms_configs(other.cms_configs), cht_configs(other.cht_configs), tb_configs(other.tb_configs),
      coalescing_candidates(other.coalescing_candidates), expiration_data(other.expiration_data), expr_structs(other.expr_structs),
      ds_impls(other.ds_impls) {
  for (auto &target_ctx_pair : other.target_ctxs) {
    target_ctxs[target_ctx_pair.first] = target_ctx_pair.second->clone();
  }
}

Context::Context(Context &&other)
    : profiler(std::move(other.profiler)), perf_oracle(std::move(other.perf_oracle)), map_configs(std::move(other.map_configs)),
      vector_configs(std::move(other.vector_configs)), dchain_configs(std::move(other.dchain_configs)),
      cms_configs(std::move(other.cms_configs)), cht_configs(std::move(other.cht_configs)), tb_configs(std::move(other.tb_configs)),
      coalescing_candidates(std::move(other.coalescing_candidates)), expiration_data(std::move(other.expiration_data)),
      expr_structs(std::move(other.expr_structs)), ds_impls(std::move(other.ds_impls)), target_ctxs(std::move(other.target_ctxs)) {}

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

  profiler              = other.profiler;
  perf_oracle           = other.perf_oracle;
  map_configs           = other.map_configs;
  vector_configs        = other.vector_configs;
  dchain_configs        = other.dchain_configs;
  cms_configs           = other.cms_configs;
  cht_configs           = other.cht_configs;
  tb_configs            = other.tb_configs;
  coalescing_candidates = other.coalescing_candidates;
  expiration_data       = other.expiration_data;
  expr_structs          = other.expr_structs;
  ds_impls              = other.ds_impls;

  for (auto &target_ctx_pair : other.target_ctxs) {
    target_ctxs[target_ctx_pair.first] = target_ctx_pair.second->clone();
  }

  return *this;
}

const Profiler &Context::get_profiler() const { return profiler; }
Profiler &Context::get_mutable_profiler() { return profiler; }

const PerfOracle &Context::get_perf_oracle() const { return perf_oracle; }
PerfOracle &Context::get_mutable_perf_oracle() { return perf_oracle; }

const LibBDD::map_config_t &Context::get_map_config(addr_t addr) const {
  assert(map_configs.find(addr) != map_configs.end() && "Map not found");
  return map_configs.at(addr);
}

const LibBDD::vector_config_t &Context::get_vector_config(addr_t addr) const {
  assert(vector_configs.find(addr) != vector_configs.end() && "Vector not found");
  return vector_configs.at(addr);
}

const LibBDD::dchain_config_t &Context::get_dchain_config(addr_t addr) const {
  assert(dchain_configs.find(addr) != dchain_configs.end() && "Dchain not found");
  return dchain_configs.at(addr);
}

const LibBDD::cms_config_t &Context::get_cms_config(addr_t addr) const {
  assert(cms_configs.find(addr) != cms_configs.end() && "CMS not found");
  return cms_configs.at(addr);
}

const LibBDD::cht_config_t &Context::get_cht_config(addr_t addr) const {
  assert(cht_configs.find(addr) != cht_configs.end() && "CHT not found");
  return cht_configs.at(addr);
}

const LibBDD::tb_config_t &Context::get_tb_config(addr_t addr) const {
  assert(tb_configs.find(addr) != tb_configs.end() && "TB not found");
  return tb_configs.at(addr);
}

std::optional<LibBDD::map_coalescing_objs_t> Context::get_map_coalescing_objs(addr_t obj) const {
  for (const LibBDD::map_coalescing_objs_t &candidate : coalescing_candidates) {
    bool match = false;

    match = match || candidate.map == obj;
    match = match || candidate.dchain == obj;
    match = match || candidate.vectors.find(obj) != candidate.vectors.end();

    if (match) {
      return candidate;
    }
  }

  return std::nullopt;
}

const std::optional<expiration_data_t> &Context::get_expiration_data() const { return expiration_data; }

const std::vector<LibCore::expr_struct_t> &Context::get_expr_structs() const { return expr_structs; }

void Context::save_ds_impl(addr_t obj, DSImpl impl) {
  assert(can_impl_ds(obj, impl) && "Incompatible implementation");
  ds_impls[obj] = impl;
}

bool Context::has_ds_impl(addr_t obj) const { return ds_impls.find(obj) != ds_impls.end(); }
DSImpl Context::get_ds_impl(addr_t obj) const { return ds_impls.at(obj); }

bool Context::check_ds_impl(addr_t obj, DSImpl decision) const {
  auto found_it = ds_impls.find(obj);
  return found_it != ds_impls.end() && found_it->second == decision;
}

bool Context::can_impl_ds(addr_t obj, DSImpl decision) const {
  auto found_it = ds_impls.find(obj);
  return found_it == ds_impls.end() || found_it->second == decision;
}

const std::unordered_map<addr_t, DSImpl> &Context::get_ds_impls() const { return ds_impls; }

std::ostream &operator<<(std::ostream &os, DSImpl impl) {
  switch (impl) {
  case DSImpl::Tofino_MapTable:
    os << "Tofino::MapTable";
    break;
  case DSImpl::Tofino_VectorTable:
    os << "Tofino::VectorTable";
    break;
  case DSImpl::Tofino_DchainTable:
    os << "Tofino::DchainTable";
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
  case DSImpl::Tofino_LPM:
    os << "Tofino::LPM";
    break;
  case DSImpl::Controller_DoubleChain:
    os << "Controller::Dchain";
    break;
  case DSImpl::Controller_Vector:
    os << "Controller::Vector";
    break;
  case DSImpl::Controller_CountMinSketch:
    os << "Controller::CMS";
    break;
  case DSImpl::Controller_Map:
    os << "Controller::Map";
    break;
  case DSImpl::Controller_ConsistentHashTable:
    os << "Controller::Cht";
    break;
  case DSImpl::Controller_TokenBucket:
    os << "Controller::TB";
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
  std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~ Context ~~~~~~~~~~~~~~~~~~~~~~~~\n";
  std::cerr << "Implementations: [\n";
  for (const auto &[obj, impl] : ds_impls) {
    std::cerr << "    " << obj << ": " << impl << "\n";
  }
  std::cerr << "]\n";

  for (const auto &[target, ctx] : target_ctxs) {
    ctx->debug();
  }

  perf_oracle.debug();

  std::cerr << "\n";
  std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
}

void Context::translate(LibCore::SymbolManager *symbol_manager, const std::vector<LibBDD::translated_symbol_t> &translated_symbols) {
  std::unordered_map<std::string, std::string> translations;
  for (const auto &[old_symbol, new_symbol] : translated_symbols) {
    translations[old_symbol.name] = new_symbol.name;
  }

  for (LibCore::expr_struct_t &expr_struct : expr_structs) {
    expr_struct.expr = symbol_manager->translate(expr_struct.expr, translations);
    for (klee::ref<klee::Expr> &field : expr_struct.fields) {
      field = symbol_manager->translate(field, translations);
    }
  }
}

} // namespace LibSynapse