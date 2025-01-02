#include <ctime>
#include <fstream>
#include <limits>
#include <math.h>
#include <unistd.h>

#include "ep_visualizer.h"

#include "../log.h"
#include "../execution_plan/execution_plan.h"
#include "../targets/controller/controller.h"

#define SHOW_MODULE_NAME(M)                                                              \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {   \
    function_call(ep_node, node->get_node(), node->get_target(), node->get_name());      \
    return EPVisitor::Action::doChildren;                                                \
  }

#define VISIT_BRANCH(M)                                                                  \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {   \
    branch(ep_node, node->get_node(), node->get_target(), node->get_name());             \
    return EPVisitor::Action::doChildren;                                                \
  }

#define IGNORE_MODULE(M)                                                                 \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const M *node) {   \
    return EPVisitor::Action::doChildren;                                                \
  }

namespace synapse {
IGNORE_MODULE(controller::Ignore)

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const controller::If *node) {
  std::stringstream label_builder;

  label_builder << "\\n";
  label_builder << pretty_print_expr(node->get_condition());

  branch(ep_node, node->get_node(), node->get_target(), label_builder.str());

  return EPVisitor::Action::doChildren;
}

SHOW_MODULE_NAME(controller::ParseHeader)
SHOW_MODULE_NAME(controller::ModifyHeader)
SHOW_MODULE_NAME(controller::ChecksumUpdate)
SHOW_MODULE_NAME(controller::Then)
SHOW_MODULE_NAME(controller::Else)
SHOW_MODULE_NAME(controller::Forward)
SHOW_MODULE_NAME(controller::Broadcast)
SHOW_MODULE_NAME(controller::Drop)
SHOW_MODULE_NAME(controller::TableLookup)
SHOW_MODULE_NAME(controller::TableUpdate)
SHOW_MODULE_NAME(controller::TableDelete)
SHOW_MODULE_NAME(controller::DchainAllocateNewIndex)
SHOW_MODULE_NAME(controller::DchainRejuvenateIndex)
SHOW_MODULE_NAME(controller::DchainIsIndexAllocated)
SHOW_MODULE_NAME(controller::DchainFreeIndex)
SHOW_MODULE_NAME(controller::VectorRead)
SHOW_MODULE_NAME(controller::VectorWrite)
SHOW_MODULE_NAME(controller::MapGet)
SHOW_MODULE_NAME(controller::MapPut)
SHOW_MODULE_NAME(controller::MapErase)
SHOW_MODULE_NAME(controller::ChtFindBackend)
SHOW_MODULE_NAME(controller::HashObj)
SHOW_MODULE_NAME(controller::VectorRegisterLookup)
SHOW_MODULE_NAME(controller::VectorRegisterUpdate)
SHOW_MODULE_NAME(controller::FCFSCachedTableRead)
SHOW_MODULE_NAME(controller::FCFSCachedTableWrite)
SHOW_MODULE_NAME(controller::FCFSCachedTableDelete)
SHOW_MODULE_NAME(controller::MapRegisterRead)
SHOW_MODULE_NAME(controller::MapRegisterWrite)
SHOW_MODULE_NAME(controller::MapRegisterDelete)
SHOW_MODULE_NAME(controller::HHTableRead)
SHOW_MODULE_NAME(controller::HHTableConditionalUpdate)
SHOW_MODULE_NAME(controller::HHTableUpdate)
SHOW_MODULE_NAME(controller::HHTableDelete)
SHOW_MODULE_NAME(controller::TBIsTracing)
SHOW_MODULE_NAME(controller::TBTrace)
SHOW_MODULE_NAME(controller::TBUpdateAndCheck)
SHOW_MODULE_NAME(controller::TBExpire)
SHOW_MODULE_NAME(controller::MeterInsert)
SHOW_MODULE_NAME(controller::IntegerAllocatorFreeIndex)
SHOW_MODULE_NAME(controller::CMSUpdate)
SHOW_MODULE_NAME(controller::CMSQuery)
SHOW_MODULE_NAME(controller::CMSIncrement)
SHOW_MODULE_NAME(controller::CMSCountMin)
} // namespace synapse