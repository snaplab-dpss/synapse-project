#include <ctime>
#include <fstream>
#include <limits>
#include <math.h>
#include <unistd.h>

#include "ep_visualizer.h"

#include "../system.h"
#include "../execution_plan/execution_plan.h"
#include "../targets/tofino/tofino.h"

#define SHOW_MODULE_NAME(M)                                                                        \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {             \
    function_call(ep_node, node->get_node(), node->get_target(), node->get_name());                \
    return EPVisitor::Action::doChildren;                                                          \
  }

#define VISIT_BRANCH(M)                                                                            \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {             \
    branch(ep_node, node->get_node(), node->get_target(), node->get_name());                       \
    return EPVisitor::Action::doChildren;                                                          \
  }

#define IGNORE_MODULE(M)                                                                           \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {             \
    return EPVisitor::Action::doChildren;                                                          \
  }

namespace synapse {

using namespace tofino;

IGNORE_MODULE(tofino::Ignore)

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const tofino::If *node) {
  std::stringstream label_builder;

  for (klee::ref<klee::Expr> condition : node->get_conditions()) {
    label_builder << "\\n";
    label_builder << pretty_print_expr(condition);
  }

  branch(ep_node, node->get_node(), node->get_target(), label_builder.str());

  return EPVisitor::Action::doChildren;
}

VISIT_BRANCH(tofino::ParserCondition)

SHOW_MODULE_NAME(tofino::ParserReject)
SHOW_MODULE_NAME(tofino::SendToController)
SHOW_MODULE_NAME(tofino::Drop)
SHOW_MODULE_NAME(tofino::Broadcast)
SHOW_MODULE_NAME(tofino::ModifyHeader)
SHOW_MODULE_NAME(tofino::Then)
SHOW_MODULE_NAME(tofino::Else)

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::Recirculate *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  int port = node->get_recirc_port();

  label_builder << "Recirculate (";
  label_builder << port;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const tofino::Forward *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  int dst_device = node->get_dst_device();

  label_builder << "Forward (";
  label_builder << dst_device;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::TableLookup *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  DS_ID tid = node->get_table_id();
  addr_t obj = node->get_obj();

  label_builder << "Table Lookup\n";
  label_builder << "(";
  label_builder << "tid=";
  label_builder << tid;
  label_builder << ", obj=";
  label_builder << obj;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::VectorRegisterLookup *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  const std::unordered_set<DS_ID> &rids = node->get_rids();
  addr_t obj = node->get_obj();

  label_builder << "Register Lookup\n";
  label_builder << "(";
  label_builder << "rids=[";

  int i = 0;
  for (const std::string &rid : rids) {
    label_builder << rid;
    i++;
    if (i < (int)rids.size()) {
      label_builder << ",";
      if (i % 3 == 0) {
        label_builder << "\\n";
      }
    }
  }

  label_builder << "], obj=";
  label_builder << obj;
  label_builder << ")";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::VectorRegisterUpdate *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  const std::unordered_set<DS_ID> &rids = node->get_rids();
  addr_t obj = node->get_obj();

  label_builder << "Register Update\n";

  label_builder << "rids=[";

  int i = 0;
  for (const std::string &rid : rids) {
    label_builder << rid;
    i++;
    if (i < (int)rids.size()) {
      label_builder << ",";
      if (i % 3 == 0) {
        label_builder << "\\n";
      }
    }
  }

  label_builder << "], obj=";
  label_builder << obj;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::FCFSCachedTableRead *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  addr_t obj = node->get_obj();

  tofino::DS_ID id = node->get_cached_table_id();
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const DS *ds = tofino_ctx->get_ds_from_id(id);
  assert(ds->type == DSType::FCFS_CACHED_TABLE && "Invalid DS type");
  const FCFSCachedTable *cached_table = dynamic_cast<const FCFSCachedTable *>(ds);

  label_builder << "FCFS Cached Table Read\n";

  label_builder << "obj=";
  label_builder << obj;
  label_builder << ", ";
  label_builder << "size=";
  label_builder << cached_table->cache_capacity;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::FCFSCachedTableReadOrWrite *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  addr_t obj = node->get_obj();

  tofino::DS_ID id = node->get_cached_table_id();
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const DS *ds = tofino_ctx->get_ds_from_id(id);
  assert(ds->type == DSType::FCFS_CACHED_TABLE && "Invalid DS type");
  const FCFSCachedTable *cached_table = dynamic_cast<const FCFSCachedTable *>(ds);

  label_builder << "FCFS Cached Table Read/Write\n";

  label_builder << "obj=";
  label_builder << obj;
  label_builder << ", ";
  label_builder << "size=";
  label_builder << cached_table->cache_capacity;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::FCFSCachedTableWrite *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  addr_t obj = node->get_obj();

  tofino::DS_ID id = node->get_cached_table_id();
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const DS *ds = tofino_ctx->get_ds_from_id(id);
  assert(ds->type == DSType::FCFS_CACHED_TABLE && "Invalid DS type");
  const FCFSCachedTable *cached_table = dynamic_cast<const FCFSCachedTable *>(ds);

  label_builder << "FCFS Cached Table Write\n";

  label_builder << "obj=";
  label_builder << obj;
  label_builder << ", ";
  label_builder << "size=";
  label_builder << cached_table->cache_capacity;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::FCFSCachedTableDelete *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  addr_t obj = node->get_obj();

  tofino::DS_ID id = node->get_cached_table_id();
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const DS *ds = tofino_ctx->get_ds_from_id(id);
  assert(ds->type == DSType::FCFS_CACHED_TABLE && "Invalid DS type");
  const FCFSCachedTable *cached_table = dynamic_cast<const FCFSCachedTable *>(ds);

  label_builder << "FCFS Cached Table Delete\n";

  label_builder << "obj=";
  label_builder << obj;
  label_builder << ", ";
  label_builder << "size=";
  label_builder << cached_table->cache_capacity;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::ParserExtraction *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  bytes_t size = node->get_length();

  label_builder << "Parse Header (";

  label_builder << size;
  label_builder << "B)";

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::MeterUpdate *node) {
  std::stringstream label_builder;

  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  addr_t obj = node->get_obj();

  label_builder << "Meter Update\n";
  label_builder << "obj=";
  label_builder << obj;

  function_call(ep_node, bdd_node, target, label_builder.str());

  return EPVisitor::Action::doChildren;
}

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::HHTableRead *node) {
  const Node *bdd_node = node->get_node();
  TargetType target = node->get_target();
  addr_t obj = node->get_obj();

  const DS *ds =
      ep->get_ctx().get_target_ctx<TofinoContext>()->get_ds_from_id(node->get_hh_table_id());

  assert(ds->type == DSType::HH_TABLE && "Invalid DS type");
  const HHTable *hh_table = dynamic_cast<const HHTable *>(ds);

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

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino::HHTableConditionalUpdate *node) {
  panic("TODO: HHTableConditionalUpdate");
}

SHOW_MODULE_NAME(tofino::IntegerAllocatorRejuvenate)
SHOW_MODULE_NAME(tofino::IntegerAllocatorAllocate)
SHOW_MODULE_NAME(tofino::IntegerAllocatorIsAllocated)

SHOW_MODULE_NAME(tofino::CMSQuery)
SHOW_MODULE_NAME(tofino::CMSIncrement)
SHOW_MODULE_NAME(tofino::CMSIncAndQuery)
} // namespace synapse