#include <ctime>
#include <fstream>
#include <limits>
#include <math.h>
#include <unistd.h>

#include "ep_visualizer.h"

#include "../system.h"
#include "../execution_plan/execution_plan.h"
#include "../targets/controller/controller.h"

#define SHOW_MODULE_NAME(M)                                                                                            \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                                 \
    function_call(ep_node, node->get_node(), node->get_target(), node->get_name());                                    \
    return EPVisitor::Action::doChildren;                                                                              \
  }

#define VISIT_BRANCH(M)                                                                                                \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                                 \
    branch(ep_node, node->get_node(), node->get_target(), node->get_name());                                           \
    return EPVisitor::Action::doChildren;                                                                              \
  }

#define IGNORE_MODULE(M)                                                                                               \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {                                 \
    return EPVisitor::Action::doChildren;                                                                              \
  }

namespace synapse {
IGNORE_MODULE(ctrl::Ignore)

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const ctrl::If *node) {
  std::stringstream label_builder;

  label_builder << "\\n";
  label_builder << pretty_print_expr(node->get_condition());

  branch(ep_node, node->get_node(), node->get_target(), label_builder.str());

  return EPVisitor::Action::doChildren;
}

SHOW_MODULE_NAME(ctrl::ParseHeader)
SHOW_MODULE_NAME(ctrl::ModifyHeader)
SHOW_MODULE_NAME(ctrl::ChecksumUpdate)
SHOW_MODULE_NAME(ctrl::Then)
SHOW_MODULE_NAME(ctrl::Else)
SHOW_MODULE_NAME(ctrl::Forward)
SHOW_MODULE_NAME(ctrl::Broadcast)
SHOW_MODULE_NAME(ctrl::Drop)
SHOW_MODULE_NAME(ctrl::TableLookup)
SHOW_MODULE_NAME(ctrl::TableUpdate)
SHOW_MODULE_NAME(ctrl::TableDelete)
SHOW_MODULE_NAME(ctrl::DchainAllocateNewIndex)
SHOW_MODULE_NAME(ctrl::DchainRejuvenateIndex)
SHOW_MODULE_NAME(ctrl::DchainIsIndexAllocated)
SHOW_MODULE_NAME(ctrl::DchainFreeIndex)
SHOW_MODULE_NAME(ctrl::VectorRead)
SHOW_MODULE_NAME(ctrl::VectorWrite)
SHOW_MODULE_NAME(ctrl::MapGet)
SHOW_MODULE_NAME(ctrl::MapPut)
SHOW_MODULE_NAME(ctrl::MapErase)
SHOW_MODULE_NAME(ctrl::ChtFindBackend)
SHOW_MODULE_NAME(ctrl::HashObj)
SHOW_MODULE_NAME(ctrl::VectorRegisterLookup)
SHOW_MODULE_NAME(ctrl::VectorRegisterUpdate)
SHOW_MODULE_NAME(ctrl::FCFSCachedTableRead)
SHOW_MODULE_NAME(ctrl::FCFSCachedTableWrite)
SHOW_MODULE_NAME(ctrl::FCFSCachedTableDelete)
SHOW_MODULE_NAME(ctrl::HHTableRead)
SHOW_MODULE_NAME(ctrl::HHTableConditionalUpdate)
SHOW_MODULE_NAME(ctrl::HHTableUpdate)
SHOW_MODULE_NAME(ctrl::HHTableDelete)
SHOW_MODULE_NAME(ctrl::TBIsTracing)
SHOW_MODULE_NAME(ctrl::TBTrace)
SHOW_MODULE_NAME(ctrl::TBUpdateAndCheck)
SHOW_MODULE_NAME(ctrl::TBExpire)
SHOW_MODULE_NAME(ctrl::MeterInsert)
SHOW_MODULE_NAME(ctrl::IntegerAllocatorFreeIndex)
SHOW_MODULE_NAME(ctrl::CMSUpdate)
SHOW_MODULE_NAME(ctrl::CMSQuery)
SHOW_MODULE_NAME(ctrl::CMSIncrement)
SHOW_MODULE_NAME(ctrl::CMSCountMin)
} // namespace synapse