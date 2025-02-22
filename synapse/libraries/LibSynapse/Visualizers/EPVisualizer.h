#pragma once

#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Graphviz.h>

#include <vector>

namespace LibSynapse {

class EPViz : public EPVisitor, public LibCore::Graphviz {
public:
  EPViz();

  static void visualize(const EP *ep, bool interrupt);

  void visit(const EP *ep) override;
  void visit(const EP *ep, const EPNode *ep_node) override;

  // ========================================
  // Tofino
  // ========================================

  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToController *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Recirculate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserExtraction *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserCondition *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserReject *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::MapTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::DchainTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableReadWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::MeterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableConditionalUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::IntegerAllocatorRejuvenate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::IntegerAllocatorAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::IntegerAllocatorIsAllocated *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncAndQuery *node) override final;

  // ========================================
  // Controller
  // ========================================

  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::ParseHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::ChecksumUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapTableUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainTableUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorTableUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocateNewIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainRejuvenateIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainIsIndexAllocated *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainFreeIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapGet *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapPut *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapErase *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::ChtFindBackend *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HashObj *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableConditionalUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketIsTracing *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketTrace *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketUpdateAndCheck *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketExpire *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MeterInsert *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::IntegerAllocatorFreeIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSCountMin *node) override final;

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
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketIsTracing *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketTrace *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketUpdateAndCheck *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketExpire *node) override final;

protected:
  void log(const EPNode *ep_node) const override final;

private:
  void function_call(const EPNode *ep_node, const LibBDD::Node *node, TargetType target, const std::string &label);
  void branch(const EPNode *ep_node, const LibBDD::Node *node, TargetType target, const std::string &label);
};

} // namespace LibSynapse