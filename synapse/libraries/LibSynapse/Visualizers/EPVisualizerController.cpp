#include <LibSynapse/Visualizers/EPVisualizer.h>
#include <LibSynapse/Modules/Controller/Controller.h>

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

IGNORE_MODULE(Controller::Ignore)

EPVisitor::Action EPViz::visit(const EP *ep, const EPNode *ep_node, const Controller::If *node) {
  std::stringstream label_builder;

  label_builder << "\\n";
  label_builder << LibCore::pretty_print_expr(node->get_condition());

  branch(ep_node, node->get_node(), node->get_target(), label_builder.str());

  return EPVisitor::Action::doChildren;
}

SHOW_MODULE_NAME(Controller::ParseHeader)
SHOW_MODULE_NAME(Controller::ModifyHeader)
SHOW_MODULE_NAME(Controller::ChecksumUpdate)
SHOW_MODULE_NAME(Controller::Then)
SHOW_MODULE_NAME(Controller::Else)
SHOW_MODULE_NAME(Controller::Forward)
SHOW_MODULE_NAME(Controller::Broadcast)
SHOW_MODULE_NAME(Controller::Drop)
SHOW_MODULE_NAME(Controller::DataplaneMapTableLookup)
SHOW_MODULE_NAME(Controller::DataplaneMapTableUpdate)
SHOW_MODULE_NAME(Controller::DataplaneMapTableDelete)
SHOW_MODULE_NAME(Controller::DataplaneVectorTableLookup)
SHOW_MODULE_NAME(Controller::DataplaneVectorTableUpdate)
SHOW_MODULE_NAME(Controller::DataplaneDchainTableAllocateNewIndex)
SHOW_MODULE_NAME(Controller::DataplaneDchainTableFreeIndex)
SHOW_MODULE_NAME(Controller::DataplaneDchainTableIsIndexAllocated)
SHOW_MODULE_NAME(Controller::DataplaneDchainTableRefreshIndex)
SHOW_MODULE_NAME(Controller::DchainAllocateNewIndex)
SHOW_MODULE_NAME(Controller::DchainRejuvenateIndex)
SHOW_MODULE_NAME(Controller::DchainIsIndexAllocated)
SHOW_MODULE_NAME(Controller::DchainFreeIndex)
SHOW_MODULE_NAME(Controller::VectorRead)
SHOW_MODULE_NAME(Controller::VectorWrite)
SHOW_MODULE_NAME(Controller::MapGet)
SHOW_MODULE_NAME(Controller::MapPut)
SHOW_MODULE_NAME(Controller::MapErase)
SHOW_MODULE_NAME(Controller::ChtFindBackend)
SHOW_MODULE_NAME(Controller::HashObj)
SHOW_MODULE_NAME(Controller::DataplaneVectorRegisterLookup)
SHOW_MODULE_NAME(Controller::DataplaneVectorRegisterUpdate)
SHOW_MODULE_NAME(Controller::DataplaneFCFSCachedTableRead)
SHOW_MODULE_NAME(Controller::DataplaneFCFSCachedTableWrite)
SHOW_MODULE_NAME(Controller::DataplaneFCFSCachedTableDelete)
SHOW_MODULE_NAME(Controller::DataplaneHHTableRead)
SHOW_MODULE_NAME(Controller::DataplaneHHTableConditionalUpdate)
SHOW_MODULE_NAME(Controller::DataplaneHHTableUpdate)
SHOW_MODULE_NAME(Controller::DataplaneHHTableDelete)
SHOW_MODULE_NAME(Controller::TokenBucketIsTracing)
SHOW_MODULE_NAME(Controller::TokenBucketTrace)
SHOW_MODULE_NAME(Controller::TokenBucketUpdateAndCheck)
SHOW_MODULE_NAME(Controller::TokenBucketExpire)
SHOW_MODULE_NAME(Controller::DataplaneMeterInsert)
SHOW_MODULE_NAME(Controller::DataplaneIntegerAllocatorFreeIndex)
SHOW_MODULE_NAME(Controller::CMSUpdate)
SHOW_MODULE_NAME(Controller::CMSQuery)
SHOW_MODULE_NAME(Controller::CMSIncrement)
SHOW_MODULE_NAME(Controller::CMSCountMin)

} // namespace LibSynapse