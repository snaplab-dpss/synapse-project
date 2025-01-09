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

  Action visit(const EP *ep, const EPNode *ep_node, const tofino::SendToController *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Recirculate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::ParserExtraction *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::ParserCondition *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::ParserReject *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::TableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableReadOrWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::MeterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::HHTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::HHTableConditionalUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::IntegerAllocatorRejuvenate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::IntegerAllocatorAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::IntegerAllocatorIsAllocated *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::CMSQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::CMSIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::CMSIncAndQuery *node) override final;

  // ========================================
  // Tofino CPU
  // ========================================

  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::ParseHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::ChecksumUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TableUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::DchainAllocateNewIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::DchainRejuvenateIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::DchainIsIndexAllocated *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::DchainFreeIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::VectorRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::VectorWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::MapGet *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::MapPut *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::MapErase *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::ChtFindBackend *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HashObj *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::FCFSCachedTableWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::FCFSCachedTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HHTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HHTableConditionalUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HHTableUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HHTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TBIsTracing *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TBTrace *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TBUpdateAndCheck *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TBExpire *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::MeterInsert *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::IntegerAllocatorFreeIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::CMSUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::CMSQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::CMSIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const ctrl::CMSCountMin *node) override final;

  // ========================================
  // x86
  // ========================================

  Action visit(const EP *ep, const EPNode *ep_node, const x86::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ParseHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ChecksumUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::MapGet *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::MapPut *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::MapErase *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMap *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMapIteratively *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::VectorRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::VectorWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainRejuvenateIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainAllocateNewIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainIsIndexAllocated *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainFreeIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSCountMin *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSPeriodicCleanup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::HashObj *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ChtFindBackend *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TBIsTracing *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TBTrace *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TBUpdateAndCheck *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TBExpire *node) override final;

protected:
  void log(const EPNode *ep_node) const override final;

private:
  void function_call(const EPNode *ep_node, const Node *node, TargetType target, const std::string &label);
  void branch(const EPNode *ep_node, const Node *node, TargetType target, const std::string &label);
};
} // namespace synapse