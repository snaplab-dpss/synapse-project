#include <LibSynapse/Visualizers/EPVisualizer.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Modules/Tofino/Tofino.h>

#include <ctime>
#include <fstream>
#include <limits>
#include <cmath>
#include <unistd.h>

#define SHOW_MODULE_NAME(M)                                                                                                                          \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                                                               \
    function_call(ep_node, node->get_node(), node->get_target(), node->get_name());                                                                  \
    return EPVisitor::Action::doChildren;                                                                                                            \
  }

#define VISIT_BRANCH(M)                                                                                                                              \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                                                               \
    branch(ep_node, node->get_node(), node->get_target(), node->get_name());                                                                         \
    return EPVisitor::Action::doChildren;                                                                                                            \
  }

#define IGNORE_MODULE(M)                                                                                                                             \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) { return EPVisitor::Action::doChildren; }

namespace LibSynapse {

using LibCore::pretty_print_expr;

IGNORE_MODULE(Tofino::Ignore)

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::If *node) {
  std::stringstream label_builder;

  for (klee::ref<klee::Expr> condition : node->get_conditions()) {
    label_builder << "\\n";
    label_builder << pretty_print_expr(condition);
  }

  branch(ep_node, node->get_node(), node->get_target(), label_builder.str());

  return EPVisitor::Action::doChildren;
}

VISIT_BRANCH(Tofino::ParserCondition)

SHOW_MODULE_NAME(Tofino::ParserReject)
SHOW_MODULE_NAME(Tofino::SendToController)
SHOW_MODULE_NAME(Tofino::Drop)
SHOW_MODULE_NAME(Tofino::Broadcast)
SHOW_MODULE_NAME(Tofino::ModifyHeader)
SHOW_MODULE_NAME(Tofino::Then)
SHOW_MODULE_NAME(Tofino::Else)

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::Recirculate *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  const TargetType target = node->get_target();

  label_builder << "Recirculate";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::Forward *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node          = node->get_node();
  TargetType target                = node->get_target();
  klee::ref<klee::Expr> dst_device = node->get_dst_device();

  label_builder << "Forward (";
  label_builder << pretty_print_expr(dst_device, false);
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::MapTableLookup *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  Tofino::DS_ID tid       = node->get_id();
  addr_t obj              = node->get_obj();

  label_builder << "Map Table Lookup\n";
  label_builder << "(";
  label_builder << "tid=";
  label_builder << tid;
  label_builder << ", obj=";
  label_builder << obj;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableLookup *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  Tofino::DS_ID tid       = node->get_id();
  addr_t obj              = node->get_obj();

  label_builder << "Guarded Map Table Lookup\n";
  label_builder << "(";
  label_builder << "tid=";
  label_builder << tid;
  label_builder << ", obj=";
  label_builder << obj;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableGuardCheck *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  Tofino::DS_ID tid       = node->get_id();
  addr_t obj              = node->get_obj();

  label_builder << "Guarded Map Table Guard Check\n";
  label_builder << "(";
  label_builder << "tid=";
  label_builder << tid;
  label_builder << ", obj=";
  label_builder << obj;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorTableLookup *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  Tofino::DS_ID tid       = node->get_id();
  addr_t obj              = node->get_obj();

  label_builder << "Vector Table Lookup\n";
  label_builder << "(";
  label_builder << "tid=";
  label_builder << tid;
  label_builder << ", obj=";
  label_builder << obj;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::DchainTableLookup *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  Tofino::DS_ID tid       = node->get_id();
  addr_t obj              = node->get_obj();

  label_builder << "Dchain Table Lookup\n";
  label_builder << "(";
  label_builder << "tid=";
  label_builder << tid;
  label_builder << ", obj=";
  label_builder << obj;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterLookup *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  Tofino::DS_ID id        = node->get_id();
  addr_t obj              = node->get_obj();

  label_builder << "Vector Register Lookup\n";
  label_builder << "(";
  label_builder << "id=" << id << ", ";
  label_builder << "obj=" << obj;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterUpdate *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  Tofino::DS_ID id        = node->get_id();
  addr_t obj              = node->get_obj();

  label_builder << "Vector Register Update\n";
  label_builder << "(";
  label_builder << "id=" << id << ", ";
  label_builder << "obj=" << obj;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableRead *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  addr_t obj              = node->get_obj();

  Tofino::DS_ID id                        = node->get_cached_table_id();
  const Context &ctx                      = ep->get_ctx();
  const Tofino::TofinoContext *tofino_ctx = ctx.get_target_ctx<Tofino::TofinoContext>();
  const Tofino::DS *ds                    = tofino_ctx->get_data_structures().get_ds_from_id(id);
  assert(ds->type == Tofino::DSType::FCFSCachedTable && "Invalid Tofino::DS type");
  const Tofino::FCFSCachedTable *cached_table = dynamic_cast<const Tofino::FCFSCachedTable *>(ds);

  label_builder << "FCFS Cached Table Read\n";

  label_builder << "obj=";
  label_builder << obj;
  label_builder << ", ";
  label_builder << "size=";
  label_builder << cached_table->cache_capacity;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableReadWrite *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  addr_t obj              = node->get_obj();

  Tofino::DS_ID id                        = node->get_cached_table_id();
  const Context &ctx                      = ep->get_ctx();
  const Tofino::TofinoContext *tofino_ctx = ctx.get_target_ctx<Tofino::TofinoContext>();
  const Tofino::DS *ds                    = tofino_ctx->get_data_structures().get_ds_from_id(id);
  assert(ds->type == Tofino::DSType::FCFSCachedTable && "Invalid Tofino::DS type");
  const Tofino::FCFSCachedTable *cached_table = dynamic_cast<const Tofino::FCFSCachedTable *>(ds);

  label_builder << "FCFS Cached Table Read/Write\n";

  label_builder << "obj=";
  label_builder << obj;
  label_builder << ", ";
  label_builder << "size=";
  label_builder << cached_table->cache_capacity;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableWrite *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  addr_t obj              = node->get_obj();

  Tofino::DS_ID id                        = node->get_cached_table_id();
  const Context &ctx                      = ep->get_ctx();
  const Tofino::TofinoContext *tofino_ctx = ctx.get_target_ctx<Tofino::TofinoContext>();
  const Tofino::DS *ds                    = tofino_ctx->get_data_structures().get_ds_from_id(id);
  assert(ds->type == Tofino::DSType::FCFSCachedTable && "Invalid Tofino::DS type");
  const Tofino::FCFSCachedTable *cached_table = dynamic_cast<const Tofino::FCFSCachedTable *>(ds);

  label_builder << "FCFS Cached Table Write\n";

  label_builder << "obj=";
  label_builder << obj;
  label_builder << ", ";
  label_builder << "size=";
  label_builder << cached_table->cache_capacity;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableDelete *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  addr_t obj              = node->get_obj();

  Tofino::DS_ID id                        = node->get_cached_table_id();
  const Context &ctx                      = ep->get_ctx();
  const Tofino::TofinoContext *tofino_ctx = ctx.get_target_ctx<Tofino::TofinoContext>();
  const Tofino::DS *ds                    = tofino_ctx->get_data_structures().get_ds_from_id(id);
  assert(ds->type == Tofino::DSType::FCFSCachedTable && "Invalid Tofino::DS type");
  const Tofino::FCFSCachedTable *cached_table = dynamic_cast<const Tofino::FCFSCachedTable *>(ds);

  label_builder << "FCFS Cached Table Delete\n";

  label_builder << "obj=";
  label_builder << obj;
  label_builder << ", ";
  label_builder << "size=";
  label_builder << cached_table->cache_capacity;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserExtraction *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  bytes_t size            = node->get_length();

  label_builder << "Parse Header (";

  label_builder << size;
  label_builder << "B)";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::MeterUpdate *node) {
  std::stringstream label_builder;

  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  addr_t obj              = node->get_obj();

  label_builder << "Meter Update\n";
  label_builder << "obj=";
  label_builder << obj;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableRead *node) {
  const BDDNode *bdd_node = node->get_node();
  TargetType target       = node->get_target();
  addr_t obj              = node->get_obj();

  const Tofino::DS *ds = ep->get_ctx().get_target_ctx<Tofino::TofinoContext>()->get_data_structures().get_ds_from_id(node->get_hh_table_id());

  assert(ds->type == Tofino::DSType::HHTable && "Invalid Tofino::DS type");
  const Tofino::HHTable *hh_table = dynamic_cast<const Tofino::HHTable *>(ds);

  std::stringstream label_builder;
  label_builder << "HH Table Read\n";
  label_builder << "obj=";
  label_builder << obj;
  label_builder << ", ";
  label_builder << "CMS=";
  label_builder << hh_table->cms_width;
  label_builder << "x";
  label_builder << hh_table->cms_height;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

SHOW_MODULE_NAME(Tofino::HHTableOutOfBandUpdate)

SHOW_MODULE_NAME(Tofino::IntegerAllocatorRejuvenate)
SHOW_MODULE_NAME(Tofino::IntegerAllocatorAllocate)
SHOW_MODULE_NAME(Tofino::IntegerAllocatorIsAllocated)

SHOW_MODULE_NAME(Tofino::CMSQuery)
SHOW_MODULE_NAME(Tofino::CMSIncrement)
SHOW_MODULE_NAME(Tofino::CMSIncAndQuery)

} // namespace LibSynapse