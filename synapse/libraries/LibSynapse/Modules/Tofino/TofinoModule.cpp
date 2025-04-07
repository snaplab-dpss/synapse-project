#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>
#include <klee/util/ExprVisitor.h>

namespace LibSynapse {
namespace Tofino {

namespace {
class ActionExprCompatibilityChecker : public klee::ExprVisitor::ExprVisitor {
private:
  std::unordered_set<std::string> symbols;
  int operations;
  bool compatible;

public:
  ActionExprCompatibilityChecker() : operations(0), compatible(true) {}

  bool is_compatible() const { return compatible; }

  Action visit_incompatible_op() {
    compatible = false;
    return Action::skipChildren();
  }

  Action visit_compatible_op() {
    operations++;
    if (operations > 1) {
      compatible = false;
      return Action::skipChildren();
    }
    return Action::doChildren();
  }

  Action visitRead(const klee::ReadExpr &e) override final {
    const std::string name = e.updates.root->name;
    symbols.insert(name);
    if (symbols.size() > 1) {
      compatible = false;
      return Action::skipChildren();
    }
    return Action::doChildren();
  }

  Action visitConcat(const klee::ConcatExpr &e) override final { return Action::doChildren(); }

  Action visitSelect(const klee::SelectExpr &e) override final { return visit_compatible_op(); }
  Action visitExtract(const klee::ExtractExpr &e) override final { return visit_compatible_op(); }
  Action visitZExt(const klee::ZExtExpr &e) override final { return visit_compatible_op(); }
  Action visitSExt(const klee::SExtExpr &e) override final { return visit_compatible_op(); }
  Action visitAdd(const klee::AddExpr &e) override final { return visit_compatible_op(); }
  Action visitSub(const klee::SubExpr &e) override final { return visit_compatible_op(); }
  Action visitNot(const klee::NotExpr &e) override final { return visit_compatible_op(); }
  Action visitAnd(const klee::AndExpr &e) override final { return visit_compatible_op(); }
  Action visitOr(const klee::OrExpr &e) override final { return visit_compatible_op(); }
  Action visitXor(const klee::XorExpr &e) override final { return visit_compatible_op(); }
  Action visitShl(const klee::ShlExpr &e) override final { return visit_compatible_op(); }
  Action visitLShr(const klee::LShrExpr &e) override final { return visit_compatible_op(); }
  Action visitAShr(const klee::AShrExpr &e) override final { return visit_compatible_op(); }
  Action visitEq(const klee::EqExpr &e) override final { return visit_compatible_op(); }
  Action visitNe(const klee::NeExpr &e) override final { return visit_compatible_op(); }
  Action visitUlt(const klee::UltExpr &e) override final { return visit_compatible_op(); }
  Action visitUle(const klee::UleExpr &e) override final { return visit_compatible_op(); }
  Action visitUgt(const klee::UgtExpr &e) override final { return visit_compatible_op(); }
  Action visitUge(const klee::UgeExpr &e) override final { return visit_compatible_op(); }
  Action visitSlt(const klee::SltExpr &e) override final { return visit_compatible_op(); }
  Action visitSle(const klee::SleExpr &e) override final { return visit_compatible_op(); }
  Action visitSgt(const klee::SgtExpr &e) override final { return visit_compatible_op(); }
  Action visitSge(const klee::SgeExpr &e) override final { return visit_compatible_op(); }

  Action visitMul(const klee::MulExpr &e) override final { return visit_incompatible_op(); }
  Action visitUDiv(const klee::UDivExpr &e) override final { return visit_incompatible_op(); }
  Action visitSDiv(const klee::SDivExpr &e) override final { return visit_incompatible_op(); }
  Action visitURem(const klee::URemExpr &e) override final { return visit_incompatible_op(); }
  Action visitSRem(const klee::SRemExpr &e) override final { return visit_incompatible_op(); }
};

MapTable *build_map_table(const EP *ep, const LibBDD::Node *node, const map_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  bits_t key_size = 0;
  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    key_size += key->getWidth();
    keys_size.push_back(key->getWidth());
  }

  assert(data.value->getWidth() == klee::Expr::Int32);

  DS_ID id = "map_table_" + std::to_string(data.obj);

  MapTable *map_table = new MapTable(id, data.capacity, key_size);
  map_table->add_table(node->get_id(), keys_size, data.time_aware);

  if (!tofino_ctx->check_placement(ep, node, map_table)) {
    delete map_table;
    map_table = nullptr;
  }

  return map_table;
}

MapTable *get_map_table(const EP *ep, const LibBDD::Node *node, const map_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(data.obj);
  assert(!ds.empty() && "No map table found");
  assert(ds.size() == 1);
  DS *mt = *ds.begin();

  assert(mt->type == DSType::MAP_TABLE && "Unexpected type");
  return dynamic_cast<MapTable *>(mt);
}

bool can_reuse_map_table(const EP *ep, const LibBDD::Node *node, const map_table_data_t &data) {
  MapTable *map_table = get_map_table(ep, node, data);
  assert(map_table && "Map table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!map_table->has_table(node->get_id()));

  MapTable *clone = dynamic_cast<MapTable *>(map_table->clone());

  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    keys_size.push_back(key->getWidth());
  }

  clone->add_table(node->get_id(), keys_size, data.time_aware);
  map_table = clone;

  bool can_place = tofino_ctx->check_placement(ep, node, map_table);
  delete map_table;

  return can_place;
}

MapTable *reuse_map_table(const EP *ep, const LibBDD::Node *node, const map_table_data_t &data) {
  MapTable *map_table = get_map_table(ep, node, data);
  assert(map_table && "Map table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!map_table->has_table(node->get_id()));

  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    keys_size.push_back(key->getWidth());
  }

  map_table->add_table(node->get_id(), keys_size, data.time_aware);

  if (!tofino_ctx->check_placement(ep, node, map_table)) {
    return nullptr;
  }

  return map_table;
}

VectorTable *build_vector_table(const EP *ep, const LibBDD::Node *node, const vector_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  assert(data.key->getWidth() == klee::Expr::Int32);
  bits_t param_size = data.value->getWidth();

  DS_ID id = "vector_table_" + std::to_string(data.obj);

  VectorTable *vector_table = new VectorTable(id, data.capacity, param_size);
  vector_table->add_table(node->get_id());

  if (!tofino_ctx->check_placement(ep, node, vector_table)) {
    delete vector_table;
    vector_table = nullptr;
  }

  return vector_table;
}

VectorTable *get_vector_table(const EP *ep, const LibBDD::Node *node, const vector_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(data.obj);
  assert(!ds.empty() && "No vector table found");
  assert(ds.size() == 1);
  DS *vt = *ds.begin();

  assert(vt->type == DSType::VECTOR_TABLE && "Unexpected type");
  return dynamic_cast<VectorTable *>(vt);
}

bool can_reuse_vector_table(const EP *ep, const LibBDD::Node *node, const vector_table_data_t &data) {
  VectorTable *vector_table = get_vector_table(ep, node, data);
  assert(vector_table && "Vector table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!vector_table->has_table(node->get_id()));

  VectorTable *clone = dynamic_cast<VectorTable *>(vector_table->clone());

  clone->add_table(node->get_id());
  vector_table = clone;

  bool can_place = tofino_ctx->check_placement(ep, node, vector_table);
  delete vector_table;

  return can_place;
}

VectorTable *reuse_vector_table(const EP *ep, const LibBDD::Node *node, const vector_table_data_t &data) {
  VectorTable *vector_table = get_vector_table(ep, node, data);
  assert(vector_table && "Vector table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!vector_table->has_table(node->get_id()));

  vector_table->add_table(node->get_id());

  if (!tofino_ctx->check_placement(ep, node, vector_table)) {
    return nullptr;
  }

  return vector_table;
}

DchainTable *build_dchain_table(const EP *ep, const LibBDD::Node *node, const dchain_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  assert(data.key->getWidth() == klee::Expr::Int32);

  DS_ID id = "dchain_table_" + std::to_string(data.obj);

  DchainTable *dchain_table = new DchainTable(id, data.capacity);
  dchain_table->add_table(node->get_id());

  if (!tofino_ctx->check_placement(ep, node, dchain_table)) {
    delete dchain_table;
    dchain_table = nullptr;
  }

  return dchain_table;
}

DchainTable *get_dchain_table(const EP *ep, const LibBDD::Node *node, const dchain_table_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(data.obj);
  assert(!ds.empty() && "No dchain table found");
  assert(ds.size() == 1);
  DS *vt = *ds.begin();

  assert(vt->type == DSType::DCHAIN_TABLE && "Unexpected type");
  return dynamic_cast<DchainTable *>(vt);
}

bool can_reuse_dchain_table(const EP *ep, const LibBDD::Node *node, const dchain_table_data_t &data) {
  DchainTable *dchain_table = get_dchain_table(ep, node, data);
  assert(dchain_table && "Dchain table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!dchain_table->has_table(node->get_id()));

  DchainTable *clone = dynamic_cast<DchainTable *>(dchain_table->clone());

  clone->add_table(node->get_id());
  dchain_table = clone;

  bool can_place = tofino_ctx->check_placement(ep, node, dchain_table);
  delete dchain_table;

  return can_place;
}

DchainTable *reuse_dchain_table(const EP *ep, const LibBDD::Node *node, const dchain_table_data_t &data) {
  DchainTable *dchain_table = get_dchain_table(ep, node, data);
  assert(dchain_table && "Dchain table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!dchain_table->has_table(node->get_id()));

  dchain_table->add_table(node->get_id());

  if (!tofino_ctx->check_placement(ep, node, dchain_table)) {
    return nullptr;
  }

  return dchain_table;
}

VectorRegister *build_vector_register(const EP *ep, const LibBDD::Node *node, const vector_register_data_t &data) {
  const TofinoContext *tofino_ctx    = ep->get_ctx().get_target_ctx<TofinoContext>();
  const tna_properties_t &properties = tofino_ctx->get_tna().get_tna_config().properties;

  const std::vector<klee::ref<klee::Expr>> partitions = Register::partition_value(properties, data.value, ep->get_ctx().get_expr_structs());

  std::vector<bits_t> values_sizes;
  for (klee::ref<klee::Expr> partition : partitions) {
    bits_t partition_size = partition->getWidth();
    values_sizes.push_back(partition_size);
  }

  const DS_ID id = "vector_register_" + std::to_string(data.obj);

  VectorRegister *vector_register = new VectorRegister(properties, id, data.capacity, data.index->getWidth(), values_sizes);

  if (!tofino_ctx->check_placement(ep, node, vector_register)) {
    delete vector_register;
    vector_register = nullptr;
  }

  return vector_register;
}

VectorRegister *get_vector_register(const EP *ep, const LibBDD::Node *node, const vector_register_data_t &data) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(data.obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(data.obj);
  assert(!ds.empty() && "No vector registers found");
  assert(ds.size() == 1);
  DS *vr = *ds.begin();

  assert(vr->type == DSType::VECTOR_REGISTER && "Unexpected type");
  return dynamic_cast<VectorRegister *>(vr);
}

FCFSCachedTable *build_fcfs_cached_table(const EP *ep, const LibBDD::Node *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity,
                                         u32 cache_capacity) {
  const Context &ctx                 = ep->get_ctx();
  const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna                     = tofino_ctx->get_tna();
  const tna_properties_t &properties = tna.get_tna_config().properties;

  const DS_ID id                                = "fcfs_cached_table_" + std::to_string(cache_capacity) + "_" + std::to_string(obj);
  const std::vector<klee::ref<klee::Expr>> keys = Register::partition_value(properties, key, ctx.get_expr_structs());

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  FCFSCachedTable *cached_table = new FCFSCachedTable(properties, id, node->get_id(), cache_capacity, capacity, keys_sizes);

  if (!tofino_ctx->check_placement(ep, node, cached_table)) {
    delete cached_table;
    cached_table = nullptr;
  }

  return cached_table;
}

FCFSCachedTable *reuse_fcfs_cached_table(const EP *ep, const LibBDD::Node *node, addr_t obj) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::FCFS_CACHED_TABLE && "Invalid DS type");

  FCFSCachedTable *cached_table = dynamic_cast<FCFSCachedTable *>(*ds.begin());

  if (!cached_table->has_table(node->get_id())) {
    FCFSCachedTable *clone = dynamic_cast<FCFSCachedTable *>(cached_table->clone());
    clone->add_table(node->get_id());
    cached_table = clone;
  }

  if (!tofino_ctx->check_placement(ep, node, cached_table)) {
    cached_table = nullptr;
  }

  return cached_table;
}

HHTable *build_hh_table(const EP *ep, const LibBDD::Node *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 capacity,
                        u32 cms_width, u32 cms_height) {
  const DS_ID id = "hh_table_" + std::to_string(obj);

  const TofinoContext *tofino_ctx    = ep->get_ctx().get_target_ctx<TofinoContext>();
  const tna_properties_t &properties = tofino_ctx->get_tna().get_tna_config().properties;

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  const u8 used_digests = tofino_ctx->get_tna().get_simple_placer().get_used_digests() + 1;
  HHTable *hh_table     = new HHTable(properties, id, node->get_id(), capacity, keys_sizes, cms_width, cms_height, used_digests);

  if (!tofino_ctx->check_placement(ep, node, hh_table)) {
    delete hh_table;
    hh_table = nullptr;
  }

  return hh_table;
}

HHTable *reuse_hh_table(const EP *ep, const LibBDD::Node *node, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::HH_TABLE && "Invalid DS type");

  HHTable *hh_table = dynamic_cast<HHTable *>(*ds.begin());

  if (!tofino_ctx->check_placement(ep, node, hh_table)) {
    hh_table = nullptr;
  }

  return hh_table;
}

CountMinSketch *build_cms(const EP *ep, const LibBDD::Node *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 width, u32 height) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  DS_ID id                           = "cms_" + std::to_string(width) + "x" + std::to_string(height) + "_" + std::to_string(obj);
  const tna_properties_t &properties = tofino_ctx->get_tna().get_tna_config().properties;

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  CountMinSketch *cms = new CountMinSketch(properties, id, keys_sizes, width, height);

  if (!tofino_ctx->check_placement(ep, node, cms)) {
    delete cms;
    cms = nullptr;
  }

  return cms;
}

CountMinSketch *reuse_cms(const EP *ep, const LibBDD::Node *node, addr_t obj) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::COUNT_MIN_SKETCH && "Invalid DS type");

  CountMinSketch *cms = dynamic_cast<CountMinSketch *>(*ds.begin());

  if (!tofino_ctx->check_placement(ep, node, cms)) {
    return nullptr;
  }

  return cms;
}

} // namespace

TofinoContext *TofinoModuleFactory::get_mutable_tofino_ctx(EP *ep) {
  Context &ctx = ep->get_mutable_ctx();
  return ctx.get_mutable_target_ctx<TofinoContext>();
}

const TofinoContext *TofinoModuleFactory::get_tofino_ctx(const EP *ep) {
  const Context &ctx = ep->get_ctx();
  return ctx.get_target_ctx<TofinoContext>();
}

TNA &TofinoModuleFactory::get_mutable_tna(EP *ep) {
  TofinoContext *ctx = get_mutable_tofino_ctx(ep);
  return ctx->get_mutable_tna();
}

const TNA &TofinoModuleFactory::get_tna(const EP *ep) {
  const TofinoContext *ctx = get_tofino_ctx(ep);
  return ctx->get_tna();
}

bool TofinoModuleFactory::expr_fits_in_action(klee::ref<klee::Expr> expr) {
  ActionExprCompatibilityChecker checker;
  checker.visit(expr);
  return checker.is_compatible();
}

MapTable *TofinoModuleFactory::build_or_reuse_map_table(const EP *ep, const LibBDD::Node *node, const map_table_data_t &data) {
  MapTable *map_table;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable);

  if (already_placed) {
    map_table = reuse_map_table(ep, node, data);
  } else {
    map_table = build_map_table(ep, node, data);
  }

  return map_table;
}

bool TofinoModuleFactory::can_build_or_reuse_map_table(const EP *ep, const LibBDD::Node *node, const map_table_data_t &data) {
  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable);

  if (already_placed) {
    return can_reuse_map_table(ep, node, data);
  }

  MapTable *map_table = build_map_table(ep, node, data);

  if (!map_table) {
    return false;
  }

  delete map_table;
  return true;
}

VectorTable *TofinoModuleFactory::build_or_reuse_vector_table(const EP *ep, const LibBDD::Node *node, const vector_table_data_t &data) {
  VectorTable *vector_table;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorTable);

  if (already_placed) {
    vector_table = reuse_vector_table(ep, node, data);
  } else {
    vector_table = build_vector_table(ep, node, data);
  }

  return vector_table;
}

bool TofinoModuleFactory::can_build_or_reuse_vector_table(const EP *ep, const LibBDD::Node *node, const vector_table_data_t &data) {
  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorTable);

  if (already_placed) {
    return can_reuse_vector_table(ep, node, data);
  }

  VectorTable *vector_table = build_vector_table(ep, node, data);

  if (!vector_table) {
    return false;
  }

  delete vector_table;
  return true;
}

DchainTable *TofinoModuleFactory::build_or_reuse_dchain_table(const EP *ep, const LibBDD::Node *node, const dchain_table_data_t &data) {
  DchainTable *dchain_table;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_DchainTable);

  if (already_placed) {
    dchain_table = reuse_dchain_table(ep, node, data);
  } else {
    dchain_table = build_dchain_table(ep, node, data);
  }

  return dchain_table;
}

bool TofinoModuleFactory::can_build_or_reuse_dchain_table(const EP *ep, const LibBDD::Node *node, const dchain_table_data_t &data) {
  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_DchainTable);

  if (already_placed) {
    return can_reuse_dchain_table(ep, node, data);
  }

  DchainTable *dchain_table = build_dchain_table(ep, node, data);

  if (!dchain_table) {
    return false;
  }

  delete dchain_table;
  return true;
}

VectorRegister *TofinoModuleFactory::build_or_reuse_vector_register(const EP *ep, const LibBDD::Node *node, const vector_register_data_t &data) {
  VectorRegister *vector_register;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (already_placed) {
    vector_register = get_vector_register(ep, node, data);
  } else {
    vector_register = build_vector_register(ep, node, data);
  }

  return vector_register;
}

bool TofinoModuleFactory::can_build_or_reuse_vector_register(const EP *ep, const LibBDD::Node *node, const vector_register_data_t &data) {
  const Context &ctx       = ep->get_ctx();
  bool regs_already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (regs_already_placed) {
    VectorRegister *vector_register = get_vector_register(ep, node, data);
    return vector_register != nullptr;
  }

  VectorRegister *vector_register = build_vector_register(ep, node, data);

  if (vector_register == nullptr) {
    return false;
  }

  delete vector_register;
  return true;
}

FCFSCachedTable *TofinoModuleFactory::build_or_reuse_fcfs_cached_table(const EP *ep, const LibBDD::Node *node, addr_t obj, klee::ref<klee::Expr> key,
                                                                       u32 capacity, u32 cache_capacity) {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = reuse_fcfs_cached_table(ep, node, obj);
  } else {
    cached_table = build_fcfs_cached_table(ep, node, obj, key, capacity, cache_capacity);
  }

  return cached_table;
}

FCFSCachedTable *TofinoModuleFactory::get_fcfs_cached_table(const EP *ep, const LibBDD::Node *node, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::FCFS_CACHED_TABLE && "Invalid DS type");

  return dynamic_cast<FCFSCachedTable *>(*ds.begin());
}

bool TofinoModuleFactory::can_get_or_build_fcfs_cached_table(const EP *ep, const LibBDD::Node *node, addr_t obj, klee::ref<klee::Expr> key,
                                                             u32 capacity, u32 cache_capacity) {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = get_fcfs_cached_table(ep, node, obj);
    assert(false && "FIXME: we should check if we can reuse this data structure, as it requires another table will have to be created");

    if (!cached_table || cached_table->cache_capacity != cache_capacity) {
      return false;
    }

    return true;
  }

  cached_table = build_fcfs_cached_table(ep, node, obj, key, capacity, cache_capacity);

  if (!cached_table) {
    return false;
  }

  delete cached_table;
  return true;
}

LibCore::Symbols TofinoModuleFactory::get_relevant_dataplane_state(const EP *ep, const LibBDD::Node *node) {
  const LibBDD::node_ids_t &roots = ep->get_target_roots(TargetType::Tofino);

  LibCore::Symbols generated_symbols = node->get_prev_symbols(roots);
  generated_symbols.add(ep->get_bdd()->get_device());
  generated_symbols.add(ep->get_bdd()->get_time());

  LibCore::Symbols future_used_symbols;
  node->visit_nodes([&future_used_symbols](const LibBDD::Node *node) {
    const LibCore::Symbols local_future_symbols = node->get_used_symbols();
    future_used_symbols.add(local_future_symbols);
    return LibBDD::NodeVisitAction::Continue;
  });

  return generated_symbols.intersect(future_used_symbols);
}

std::vector<u32> TofinoModuleFactory::enum_fcfs_cache_cap(u32 capacity) {
  std::vector<u32> capacities;

  u32 cache_capacity = 8;
  while (cache_capacity < capacity) {
    capacities.push_back(cache_capacity);
    cache_capacity *= 2;

    // Overflow check
    assert((capacities.empty() || capacities.back() < cache_capacity) && "Overflow");
  }

  return capacities;
}

hit_rate_t TofinoModuleFactory::get_fcfs_cache_success_rate(const Context &ctx, const LibBDD::Node *node, klee::ref<klee::Expr> key,
                                                            u32 cache_capacity) {
  const std::vector<klee::ref<klee::Expr>> constraints = node->get_ordered_branch_constraints();
  const flow_stats_t flow_stats                        = ctx.get_profiler().get_flow_stats(constraints, key);

  const u64 avg_pkts_per_flow = flow_stats.pkts / flow_stats.flows;
  const u64 cached_packets    = std::min(flow_stats.pkts, avg_pkts_per_flow * cache_capacity);
  const hit_rate_t hit_rate(cached_packets, flow_stats.pkts);

  // std::cerr << "node: " << node->dump(true, true) << "\n";
  // std::cerr << "avg_pkts_per_flow: " << avg_pkts_per_flow << std::endl;
  // std::cerr << "cached_packets: " << cached_packets << std::endl;
  // std::cerr << "hit_rate: " << hit_rate << std::endl;
  // dbg_pause();

  return hit_rate;
}

HHTable *TofinoModuleFactory::build_or_reuse_hh_table(const EP *ep, const LibBDD::Node *node, addr_t obj,
                                                      const std::vector<klee::ref<klee::Expr>> &keys, u32 capacity, u32 cms_width, u32 cms_height) {
  HHTable *hh_table = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    hh_table = reuse_hh_table(ep, node, obj);
  } else {
    hh_table = build_hh_table(ep, node, obj, keys, capacity, cms_width, cms_height);
  }

  return hh_table;
}

bool TofinoModuleFactory::can_build_or_reuse_hh_table(const EP *ep, const LibBDD::Node *node, addr_t obj,
                                                      const std::vector<klee::ref<klee::Expr>> &keys, u32 capacity, u32 cms_width, u32 cms_height) {
  HHTable *hh_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable);

  if (already_placed) {
    const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>();
    const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

    assert(ds.size() == 1 && "Invalid number of DS");
    assert((*ds.begin())->type == DSType::HH_TABLE && "Invalid DS type");

    hh_table = dynamic_cast<HHTable *>(*ds.begin());

    if (!tofino_ctx->check_placement(ep, node, hh_table)) {
      hh_table = nullptr;
      return false;
    }

    if (hh_table->cms_width != cms_width || hh_table->cms_height != cms_height) {
      return false;
    }

    return true;
  }

  hh_table = build_hh_table(ep, node, obj, keys, capacity, cms_width, cms_height);

  if (!hh_table) {
    return false;
  }

  delete hh_table;
  return true;
}

hit_rate_t TofinoModuleFactory::get_hh_table_hit_success_rate(const Context &ctx, const LibBDD::Node *node, klee::ref<klee::Expr> key, u32 capacity) {
  const std::vector<klee::ref<klee::Expr>> constraints = node->get_ordered_branch_constraints();
  const flow_stats_t flow_stats                        = ctx.get_profiler().get_flow_stats(constraints, key);

  u64 top_k = 0;
  for (size_t k = 0; k <= capacity && k < flow_stats.pkts_per_flow.size(); k++) {
    top_k += flow_stats.pkts_per_flow[k];
  }

  assert(top_k <= flow_stats.pkts && "Invalid top_k");
  const hit_rate_t hit_rate(top_k, flow_stats.pkts);

  return hit_rate;
}

bool TofinoModuleFactory::can_build_or_reuse_cms(const EP *ep, const LibBDD::Node *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys,
                                                 u32 width, u32 height) {
  CountMinSketch *cms = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_CountMinSketch);

  if (already_placed) {
    const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>();
    const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

    assert(ds.size() == 1 && "Invalid number of DS");
    assert((*ds.begin())->type == DSType::COUNT_MIN_SKETCH && "Invalid DS type");

    cms = dynamic_cast<CountMinSketch *>(*ds.begin());

    if (!tofino_ctx->check_placement(ep, node, cms)) {
      cms = nullptr;
      return false;
    }

    if (cms->width != width || cms->height != height) {
      return false;
    }

    return true;
  }

  cms = build_cms(ep, node, obj, keys, width, height);

  if (!cms) {
    return false;
  }

  delete cms;
  return true;
}

CountMinSketch *TofinoModuleFactory::build_or_reuse_cms(const EP *ep, const LibBDD::Node *node, addr_t obj,
                                                        const std::vector<klee::ref<klee::Expr>> &keys, u32 width, u32 height) {
  CountMinSketch *cms = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_CountMinSketch)) {
    cms = reuse_cms(ep, node, obj);
  } else {
    cms = build_cms(ep, node, obj, keys, width, height);
  }

  return cms;
}

LPM *TofinoModuleFactory::build_lpm(const EP *ep, const LibBDD::Node *node, addr_t obj) {
  const TofinoContext *tofino_ctx    = ep->get_ctx().get_target_ctx<TofinoContext>();
  const DS_ID id                     = "lpm_" + std::to_string(obj);
  const tna_properties_t &properties = tofino_ctx->get_tna().get_tna_config().properties;

  LPM *lpm = new LPM(id, properties.total_ports);

  if (!tofino_ctx->check_placement(ep, node, lpm)) {
    delete lpm;
    lpm = nullptr;
  }

  return lpm;
}

bool TofinoModuleFactory::can_build_lpm(const EP *ep, const LibBDD::Node *node, addr_t obj) {
  LPM *lpm = build_lpm(ep, node, obj);

  if (!lpm) {
    return false;
  }

  delete lpm;
  return true;
}

} // namespace Tofino
} // namespace LibSynapse