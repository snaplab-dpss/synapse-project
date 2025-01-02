#include <ctime>
#include <fstream>
#include <limits>
#include <math.h>
#include <unistd.h>

#include "ep_visualizer.h"

#include "../log.h"
#include "../execution_plan/execution_plan.h"
#include "../targets/x86/x86.h"

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
IGNORE_MODULE(x86::Ignore)

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const x86::If *node) {
  std::stringstream label_builder;

  label_builder << "\\n";
  label_builder << pretty_print_expr(node->get_condition());

  branch(ep_node, node->get_node(), node->get_target(), label_builder.str());

  return EPVisitor::Action::doChildren;
}

SHOW_MODULE_NAME(x86::Then)
SHOW_MODULE_NAME(x86::Else)
SHOW_MODULE_NAME(x86::Forward)
SHOW_MODULE_NAME(x86::Broadcast)
SHOW_MODULE_NAME(x86::Drop)
SHOW_MODULE_NAME(x86::ParseHeader)
SHOW_MODULE_NAME(x86::ModifyHeader)
SHOW_MODULE_NAME(x86::ChecksumUpdate)
SHOW_MODULE_NAME(x86::MapGet)
SHOW_MODULE_NAME(x86::MapPut)
SHOW_MODULE_NAME(x86::MapErase)
SHOW_MODULE_NAME(x86::ExpireItemsSingleMap)
SHOW_MODULE_NAME(x86::ExpireItemsSingleMapIteratively)
SHOW_MODULE_NAME(x86::VectorRead)
SHOW_MODULE_NAME(x86::VectorWrite)
SHOW_MODULE_NAME(x86::DchainAllocateNewIndex)
SHOW_MODULE_NAME(x86::DchainIsIndexAllocated)
SHOW_MODULE_NAME(x86::DchainRejuvenateIndex)
SHOW_MODULE_NAME(x86::DchainFreeIndex)
SHOW_MODULE_NAME(x86::CMSIncrement)
SHOW_MODULE_NAME(x86::CMSCountMin)
SHOW_MODULE_NAME(x86::CMSPeriodicCleanup)
SHOW_MODULE_NAME(x86::HashObj)
SHOW_MODULE_NAME(x86::ChtFindBackend)
SHOW_MODULE_NAME(x86::TBIsTracing)
SHOW_MODULE_NAME(x86::TBTrace)
SHOW_MODULE_NAME(x86::TBUpdateAndCheck)
SHOW_MODULE_NAME(x86::TBExpire)
} // namespace synapse