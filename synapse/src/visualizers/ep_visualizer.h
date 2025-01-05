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
  DECLARE_VISIT(tofino::HHTableConditionalUpdate)
  DECLARE_VISIT(tofino::IntegerAllocatorRejuvenate)
  DECLARE_VISIT(tofino::IntegerAllocatorAllocate)
  DECLARE_VISIT(tofino::IntegerAllocatorIsAllocated)
  DECLARE_VISIT(tofino::CMSQuery)
  DECLARE_VISIT(tofino::CMSIncrement)
  DECLARE_VISIT(tofino::CMSIncAndQuery)

  // ========================================
  // Tofino CPU
  // ========================================

  DECLARE_VISIT(ctrl::Ignore)
  DECLARE_VISIT(ctrl::ParseHeader)
  DECLARE_VISIT(ctrl::ModifyHeader)
  DECLARE_VISIT(ctrl::ChecksumUpdate)
  DECLARE_VISIT(ctrl::If)
  DECLARE_VISIT(ctrl::Then)
  DECLARE_VISIT(ctrl::Else)
  DECLARE_VISIT(ctrl::Forward)
  DECLARE_VISIT(ctrl::Broadcast)
  DECLARE_VISIT(ctrl::Drop)
  DECLARE_VISIT(ctrl::TableLookup)
  DECLARE_VISIT(ctrl::TableUpdate)
  DECLARE_VISIT(ctrl::TableDelete)
  DECLARE_VISIT(ctrl::DchainAllocateNewIndex)
  DECLARE_VISIT(ctrl::DchainRejuvenateIndex)
  DECLARE_VISIT(ctrl::DchainIsIndexAllocated)
  DECLARE_VISIT(ctrl::DchainFreeIndex)
  DECLARE_VISIT(ctrl::VectorRead)
  DECLARE_VISIT(ctrl::VectorWrite)
  DECLARE_VISIT(ctrl::MapGet)
  DECLARE_VISIT(ctrl::MapPut)
  DECLARE_VISIT(ctrl::MapErase)
  DECLARE_VISIT(ctrl::ChtFindBackend)
  DECLARE_VISIT(ctrl::HashObj)
  DECLARE_VISIT(ctrl::VectorRegisterLookup)
  DECLARE_VISIT(ctrl::VectorRegisterUpdate)
  DECLARE_VISIT(ctrl::FCFSCachedTableRead)
  DECLARE_VISIT(ctrl::FCFSCachedTableWrite)
  DECLARE_VISIT(ctrl::FCFSCachedTableDelete)
  DECLARE_VISIT(ctrl::MapRegisterRead)
  DECLARE_VISIT(ctrl::MapRegisterWrite)
  DECLARE_VISIT(ctrl::MapRegisterDelete)
  DECLARE_VISIT(ctrl::HHTableRead)
  DECLARE_VISIT(ctrl::HHTableConditionalUpdate)
  DECLARE_VISIT(ctrl::HHTableUpdate)
  DECLARE_VISIT(ctrl::HHTableDelete)
  DECLARE_VISIT(ctrl::TBIsTracing)
  DECLARE_VISIT(ctrl::TBTrace)
  DECLARE_VISIT(ctrl::TBUpdateAndCheck)
  DECLARE_VISIT(ctrl::TBExpire)
  DECLARE_VISIT(ctrl::MeterInsert)
  DECLARE_VISIT(ctrl::IntegerAllocatorFreeIndex)
  DECLARE_VISIT(ctrl::CMSUpdate)
  DECLARE_VISIT(ctrl::CMSQuery)
  DECLARE_VISIT(ctrl::CMSIncrement)
  DECLARE_VISIT(ctrl::CMSCountMin)

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