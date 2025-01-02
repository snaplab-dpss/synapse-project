#pragma once

#include <vector>

#include "../bdd/bdd.h"
#include "../graphviz/graphviz.h"
#include "../targets/target.h"
#include "../execution_plan/visitor.h"

namespace synapse {
class EPViz : public EPVisitor, public Graphviz {
public:
  EPViz();

  static void visualize(const EP *ep, bool interrupt);

  void visit(const EP *ep) override;
  void visit(const EP *ep, const EPNode *ep_node) override;

  // ========================================
  // Tofino
  // ========================================

  DECLARE_VISIT(tofino::SendToController)
  DECLARE_VISIT(tofino::Recirculate)
  DECLARE_VISIT(tofino::Ignore)
  DECLARE_VISIT(tofino::If)
  DECLARE_VISIT(tofino::Then)
  DECLARE_VISIT(tofino::Else)
  DECLARE_VISIT(tofino::Forward)
  DECLARE_VISIT(tofino::Drop)
  DECLARE_VISIT(tofino::Broadcast)
  DECLARE_VISIT(tofino::ParserExtraction)
  DECLARE_VISIT(tofino::ParserCondition)
  DECLARE_VISIT(tofino::ParserReject)
  DECLARE_VISIT(tofino::ModifyHeader)
  DECLARE_VISIT(tofino::TableLookup)
  DECLARE_VISIT(tofino::VectorRegisterLookup)
  DECLARE_VISIT(tofino::VectorRegisterUpdate)
  DECLARE_VISIT(tofino::FCFSCachedTableRead)
  DECLARE_VISIT(tofino::FCFSCachedTableReadOrWrite)
  DECLARE_VISIT(tofino::FCFSCachedTableWrite)
  DECLARE_VISIT(tofino::FCFSCachedTableDelete)
  DECLARE_VISIT(tofino::MapRegisterRead)
  DECLARE_VISIT(tofino::MapRegisterReadOrWrite)
  DECLARE_VISIT(tofino::MapRegisterWrite)
  DECLARE_VISIT(tofino::MapRegisterDelete)
  DECLARE_VISIT(tofino::MeterUpdate)
  DECLARE_VISIT(tofino::HHTableRead)
  DECLARE_VISIT(tofino::IntegerAllocatorRejuvenate)
  DECLARE_VISIT(tofino::IntegerAllocatorAllocate)
  DECLARE_VISIT(tofino::IntegerAllocatorIsAllocated)
  DECLARE_VISIT(tofino::CMSQuery)
  DECLARE_VISIT(tofino::CMSIncrement)
  DECLARE_VISIT(tofino::CMSIncAndQuery)

  // ========================================
  // Tofino CPU
  // ========================================

  DECLARE_VISIT(controller::Ignore)
  DECLARE_VISIT(controller::ParseHeader)
  DECLARE_VISIT(controller::ModifyHeader)
  DECLARE_VISIT(controller::ChecksumUpdate)
  DECLARE_VISIT(controller::If)
  DECLARE_VISIT(controller::Then)
  DECLARE_VISIT(controller::Else)
  DECLARE_VISIT(controller::Forward)
  DECLARE_VISIT(controller::Broadcast)
  DECLARE_VISIT(controller::Drop)
  DECLARE_VISIT(controller::TableLookup)
  DECLARE_VISIT(controller::TableUpdate)
  DECLARE_VISIT(controller::TableDelete)
  DECLARE_VISIT(controller::DchainAllocateNewIndex)
  DECLARE_VISIT(controller::DchainRejuvenateIndex)
  DECLARE_VISIT(controller::DchainIsIndexAllocated)
  DECLARE_VISIT(controller::DchainFreeIndex)
  DECLARE_VISIT(controller::VectorRead)
  DECLARE_VISIT(controller::VectorWrite)
  DECLARE_VISIT(controller::MapGet)
  DECLARE_VISIT(controller::MapPut)
  DECLARE_VISIT(controller::MapErase)
  DECLARE_VISIT(controller::ChtFindBackend)
  DECLARE_VISIT(controller::HashObj)
  DECLARE_VISIT(controller::VectorRegisterLookup)
  DECLARE_VISIT(controller::VectorRegisterUpdate)
  DECLARE_VISIT(controller::FCFSCachedTableRead)
  DECLARE_VISIT(controller::FCFSCachedTableWrite)
  DECLARE_VISIT(controller::FCFSCachedTableDelete)
  DECLARE_VISIT(controller::MapRegisterRead)
  DECLARE_VISIT(controller::MapRegisterWrite)
  DECLARE_VISIT(controller::MapRegisterDelete)
  DECLARE_VISIT(controller::HHTableRead)
  DECLARE_VISIT(controller::HHTableConditionalUpdate)
  DECLARE_VISIT(controller::HHTableUpdate)
  DECLARE_VISIT(controller::HHTableDelete)
  DECLARE_VISIT(controller::TBIsTracing)
  DECLARE_VISIT(controller::TBTrace)
  DECLARE_VISIT(controller::TBUpdateAndCheck)
  DECLARE_VISIT(controller::TBExpire)
  DECLARE_VISIT(controller::MeterInsert)
  DECLARE_VISIT(controller::IntegerAllocatorFreeIndex)
  DECLARE_VISIT(controller::CMSUpdate)
  DECLARE_VISIT(controller::CMSQuery)
  DECLARE_VISIT(controller::CMSIncrement)
  DECLARE_VISIT(controller::CMSCountMin)

  // ========================================
  // x86
  // ========================================

  DECLARE_VISIT(x86::Ignore)
  DECLARE_VISIT(x86::If)
  DECLARE_VISIT(x86::Then)
  DECLARE_VISIT(x86::Else)
  DECLARE_VISIT(x86::Forward)
  DECLARE_VISIT(x86::Broadcast)
  DECLARE_VISIT(x86::Drop)
  DECLARE_VISIT(x86::ParseHeader)
  DECLARE_VISIT(x86::ModifyHeader)
  DECLARE_VISIT(x86::ChecksumUpdate)
  DECLARE_VISIT(x86::MapGet)
  DECLARE_VISIT(x86::MapPut)
  DECLARE_VISIT(x86::MapErase)
  DECLARE_VISIT(x86::ExpireItemsSingleMap)
  DECLARE_VISIT(x86::ExpireItemsSingleMapIteratively)
  DECLARE_VISIT(x86::VectorRead)
  DECLARE_VISIT(x86::VectorWrite)
  DECLARE_VISIT(x86::DchainRejuvenateIndex)
  DECLARE_VISIT(x86::DchainAllocateNewIndex)
  DECLARE_VISIT(x86::DchainIsIndexAllocated)
  DECLARE_VISIT(x86::DchainFreeIndex)
  DECLARE_VISIT(x86::CMSIncrement)
  DECLARE_VISIT(x86::CMSCountMin)
  DECLARE_VISIT(x86::CMSPeriodicCleanup)
  DECLARE_VISIT(x86::HashObj)
  DECLARE_VISIT(x86::ChtFindBackend)
  DECLARE_VISIT(x86::TBIsTracing)
  DECLARE_VISIT(x86::TBTrace)
  DECLARE_VISIT(x86::TBUpdateAndCheck)
  DECLARE_VISIT(x86::TBExpire)

protected:
  virtual void log(const EPNode *ep_node) const override;

private:
  void function_call(const EPNode *ep_node, const Node *node, TargetType target,
                     const std::string &label);
  void branch(const EPNode *ep_node, const Node *node, TargetType target,
              const std::string &label);
};
} // namespace synapse