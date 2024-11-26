#pragma once

#include <vector>

#include "../bdd/bdd.h"
#include "../graphviz/graphviz.h"
#include "../targets/target.h"
#include "../execution_plan/visitor.h"

class EPViz : public EPVisitor, public Graphviz {
public:
  EPViz();

  static void visualize(const EP *ep, bool interrupt);

  void visit(const EP *ep) override;
  void visit(const EP *ep, const EPNode *ep_node) override;

  /********************************************
   *
   *                  Tofino
   *
   ********************************************/

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
  DECLARE_VISIT(tofino::MeterUpdate)
  DECLARE_VISIT(tofino::HHTableRead)
  DECLARE_VISIT(tofino::IntegerAllocatorRejuvenate)
  DECLARE_VISIT(tofino::IntegerAllocatorAllocate)
  DECLARE_VISIT(tofino::IntegerAllocatorIsAllocated)
  DECLARE_VISIT(tofino::CMSQuery)
  DECLARE_VISIT(tofino::CMSIncrement)
  DECLARE_VISIT(tofino::CMSIncAndQuery)

  /********************************************
   *
   *              Tofino CPU
   *
   ********************************************/

  DECLARE_VISIT(tofino_cpu::Ignore)
  DECLARE_VISIT(tofino_cpu::ParseHeader)
  DECLARE_VISIT(tofino_cpu::ModifyHeader)
  DECLARE_VISIT(tofino_cpu::ChecksumUpdate)
  DECLARE_VISIT(tofino_cpu::If)
  DECLARE_VISIT(tofino_cpu::Then)
  DECLARE_VISIT(tofino_cpu::Else)
  DECLARE_VISIT(tofino_cpu::Forward)
  DECLARE_VISIT(tofino_cpu::Broadcast)
  DECLARE_VISIT(tofino_cpu::Drop)
  DECLARE_VISIT(tofino_cpu::TableLookup)
  DECLARE_VISIT(tofino_cpu::TableUpdate)
  DECLARE_VISIT(tofino_cpu::TableDelete)
  DECLARE_VISIT(tofino_cpu::DchainAllocateNewIndex)
  DECLARE_VISIT(tofino_cpu::DchainRejuvenateIndex)
  DECLARE_VISIT(tofino_cpu::DchainIsIndexAllocated)
  DECLARE_VISIT(tofino_cpu::DchainFreeIndex)
  DECLARE_VISIT(tofino_cpu::VectorRead)
  DECLARE_VISIT(tofino_cpu::VectorWrite)
  DECLARE_VISIT(tofino_cpu::MapGet)
  DECLARE_VISIT(tofino_cpu::MapPut)
  DECLARE_VISIT(tofino_cpu::MapErase)
  DECLARE_VISIT(tofino_cpu::ChtFindBackend)
  DECLARE_VISIT(tofino_cpu::HashObj)
  DECLARE_VISIT(tofino_cpu::VectorRegisterLookup)
  DECLARE_VISIT(tofino_cpu::VectorRegisterUpdate)
  DECLARE_VISIT(tofino_cpu::FCFSCachedTableRead)
  DECLARE_VISIT(tofino_cpu::FCFSCachedTableWrite)
  DECLARE_VISIT(tofino_cpu::FCFSCachedTableDelete)
  DECLARE_VISIT(tofino_cpu::HHTableRead)
  DECLARE_VISIT(tofino_cpu::HHTableConditionalUpdate)
  DECLARE_VISIT(tofino_cpu::HHTableUpdate)
  DECLARE_VISIT(tofino_cpu::HHTableDelete)
  DECLARE_VISIT(tofino_cpu::TBIsTracing)
  DECLARE_VISIT(tofino_cpu::TBTrace)
  DECLARE_VISIT(tofino_cpu::TBUpdateAndCheck)
  DECLARE_VISIT(tofino_cpu::TBExpire)
  DECLARE_VISIT(tofino_cpu::MeterInsert)
  DECLARE_VISIT(tofino_cpu::IntegerAllocatorFreeIndex)
  DECLARE_VISIT(tofino_cpu::CMSUpdate)
  DECLARE_VISIT(tofino_cpu::CMSQuery)
  DECLARE_VISIT(tofino_cpu::CMSIncrement)
  DECLARE_VISIT(tofino_cpu::CMSCountMin)

  /********************************************
   *
   *                  x86
   *
   ********************************************/

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

protected:
  virtual void log(const EPNode *ep_node) const override;

private:
  void function_call(const EPNode *ep_node, const Node *node, TargetType target,
                     const std::string &label);
  void branch(const EPNode *ep_node, const Node *node, TargetType target,
              const std::string &label);
};