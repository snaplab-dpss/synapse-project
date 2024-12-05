#include <ctime>
#include <fstream>
#include <limits>
#include <math.h>
#include <unistd.h>

#include "ep_visualizer.h"

#include "../log.h"
#include "../execution_plan/execution_plan.h"
#include "../targets/tofino_cpu/tofino_cpu.h"

#define SHOW_MODULE_NAME(M)                                                    \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,          \
                                 const M *node) {                              \
    function_call(ep_node, node->get_node(), node->get_target(),               \
                  node->get_name());                                           \
    return EPVisitor::Action::doChildren;                                      \
  }

#define VISIT_BRANCH(M)                                                        \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,          \
                                 const M *node) {                              \
    branch(ep_node, node->get_node(), node->get_target(), node->get_name());   \
    return EPVisitor::Action::doChildren;                                      \
  }

#define IGNORE_MODULE(M)                                                       \
  EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,          \
                                 const M *node) {                              \
    return EPVisitor::Action::doChildren;                                      \
  }

IGNORE_MODULE(tofino_cpu::Ignore)

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node,
                               const tofino_cpu::If *node) {
  std::stringstream label_builder;

  label_builder << "\\n";
  label_builder << pretty_print_expr(node->get_condition());

  branch(ep_node, node->get_node(), node->get_target(), label_builder.str());

  return EPVisitor::Action::doChildren;
}

SHOW_MODULE_NAME(tofino_cpu::ParseHeader)
SHOW_MODULE_NAME(tofino_cpu::ModifyHeader)
SHOW_MODULE_NAME(tofino_cpu::ChecksumUpdate)
SHOW_MODULE_NAME(tofino_cpu::Then)
SHOW_MODULE_NAME(tofino_cpu::Else)
SHOW_MODULE_NAME(tofino_cpu::Forward)
SHOW_MODULE_NAME(tofino_cpu::Broadcast)
SHOW_MODULE_NAME(tofino_cpu::Drop)
SHOW_MODULE_NAME(tofino_cpu::TableLookup)
SHOW_MODULE_NAME(tofino_cpu::TableUpdate)
SHOW_MODULE_NAME(tofino_cpu::TableDelete)
SHOW_MODULE_NAME(tofino_cpu::DchainAllocateNewIndex)
SHOW_MODULE_NAME(tofino_cpu::DchainRejuvenateIndex)
SHOW_MODULE_NAME(tofino_cpu::DchainIsIndexAllocated)
SHOW_MODULE_NAME(tofino_cpu::DchainFreeIndex)
SHOW_MODULE_NAME(tofino_cpu::VectorRead)
SHOW_MODULE_NAME(tofino_cpu::VectorWrite)
SHOW_MODULE_NAME(tofino_cpu::MapGet)
SHOW_MODULE_NAME(tofino_cpu::MapPut)
SHOW_MODULE_NAME(tofino_cpu::MapErase)
SHOW_MODULE_NAME(tofino_cpu::ChtFindBackend)
SHOW_MODULE_NAME(tofino_cpu::HashObj)
SHOW_MODULE_NAME(tofino_cpu::VectorRegisterLookup)
SHOW_MODULE_NAME(tofino_cpu::VectorRegisterUpdate)
SHOW_MODULE_NAME(tofino_cpu::FCFSCachedTableRead)
SHOW_MODULE_NAME(tofino_cpu::FCFSCachedTableWrite)
SHOW_MODULE_NAME(tofino_cpu::FCFSCachedTableDelete)
SHOW_MODULE_NAME(tofino_cpu::MapRegisterRead)
SHOW_MODULE_NAME(tofino_cpu::MapRegisterWrite)
SHOW_MODULE_NAME(tofino_cpu::MapRegisterDelete)
SHOW_MODULE_NAME(tofino_cpu::HHTableRead)
SHOW_MODULE_NAME(tofino_cpu::HHTableConditionalUpdate)
SHOW_MODULE_NAME(tofino_cpu::HHTableUpdate)
SHOW_MODULE_NAME(tofino_cpu::HHTableDelete)
SHOW_MODULE_NAME(tofino_cpu::TBIsTracing)
SHOW_MODULE_NAME(tofino_cpu::TBTrace)
SHOW_MODULE_NAME(tofino_cpu::TBUpdateAndCheck)
SHOW_MODULE_NAME(tofino_cpu::TBExpire)
SHOW_MODULE_NAME(tofino_cpu::MeterInsert)
SHOW_MODULE_NAME(tofino_cpu::IntegerAllocatorFreeIndex)
SHOW_MODULE_NAME(tofino_cpu::CMSUpdate)
SHOW_MODULE_NAME(tofino_cpu::CMSQuery)
SHOW_MODULE_NAME(tofino_cpu::CMSIncrement)
SHOW_MODULE_NAME(tofino_cpu::CMSCountMin)