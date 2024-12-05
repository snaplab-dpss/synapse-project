#pragma once

#include <assert.h>
#include <memory>

#define DECLARE_VISIT(M)                                                       \
  Action visit(const EP *ep, const EPNode *ep_node, const M *node) override;

#define VISIT_NOP(M)                                                           \
  virtual Action visit(const EP *ep, const EPNode *ep_node, const M *m) {      \
    return Action::doChildren;                                                 \
  }

#define VISIT_TODO(M)                                                          \
  virtual Action visit(const EP *ep, const EPNode *ep_node, const M *m) {      \
    assert(false && "TODO");                                                   \
  }

class EP;
class EPNode;

namespace tofino {
class SendToController;
class Recirculate;
class Ignore;
class If;
class Then;
class Else;
class Forward;
class Drop;
class Broadcast;
class ParserExtraction;
class ParserCondition;
class ParserReject;
class ModifyHeader;
class TableLookup;
class VectorRegisterLookup;
class VectorRegisterUpdate;
class FCFSCachedTableRead;
class FCFSCachedTableReadOrWrite;
class FCFSCachedTableWrite;
class FCFSCachedTableDelete;
class MapRegisterRead;
class MapRegisterReadOrWrite;
class MapRegisterWrite;
class MapRegisterDelete;
class MeterUpdate;
class HHTableRead;
class HHTableConditionalUpdate;
class IntegerAllocatorRejuvenate;
class IntegerAllocatorAllocate;
class IntegerAllocatorIsAllocated;
class CMSQuery;
class CMSIncrement;
class CMSIncAndQuery;
} // namespace tofino

namespace tofino_cpu {
class Ignore;
class ParseHeader;
class ModifyHeader;
class ChecksumUpdate;
class If;
class Then;
class Else;
class Forward;
class Broadcast;
class Drop;
class TableLookup;
class TableUpdate;
class DchainAllocateNewIndex;
class DchainRejuvenateIndex;
class DchainIsIndexAllocated;
class DchainFreeIndex;
class VectorRead;
class VectorWrite;
class MapGet;
class MapPut;
class MapErase;
class ChtFindBackend;
class HashObj;
class TableDelete;
class VectorRegisterLookup;
class VectorRegisterUpdate;
class FCFSCachedTableRead;
class FCFSCachedTableWrite;
class FCFSCachedTableDelete;
class MapRegisterRead;
class MapRegisterWrite;
class MapRegisterDelete;
class HHTableRead;
class HHTableConditionalUpdate;
class HHTableUpdate;
class HHTableDelete;
class TBIsTracing;
class TBTrace;
class TBUpdateAndCheck;
class TBExpire;
class MeterInsert;
class IntegerAllocatorFreeIndex;
class CMSUpdate;
class CMSQuery;
class CMSIncrement;
class CMSCountMin;
} // namespace tofino_cpu

namespace x86 {
class Ignore;
class ParseHeader;
class ModifyHeader;
class If;
class Then;
class Else;
class Forward;
class Broadcast;
class Drop;
class MapGet;
class MapPut;
class MapErase;
class ExpireItemsSingleMap;
class ExpireItemsSingleMapIteratively;
class VectorRead;
class VectorWrite;
class ChecksumUpdate;
class DchainAllocateNewIndex;
class DchainRejuvenateIndex;
class DchainIsIndexAllocated;
class DchainFreeIndex;
class CMSIncrement;
class CMSCountMin;
class CMSPeriodicCleanup;
class ChtFindBackend;
class HashObj;
class TBIsTracing;
class TBTrace;
class TBUpdateAndCheck;
class TBExpire;
} // namespace x86

class EPVisitor {
public:
  enum class Action { skipChildren, doChildren };

  virtual void visit(const EP *ep);
  virtual void visit(const EP *ep, const EPNode *ep_node);

  // ========================================
  // Tofino
  // ========================================

  VISIT_NOP(tofino::SendToController)
  VISIT_NOP(tofino::Recirculate)
  VISIT_NOP(tofino::Ignore)
  VISIT_NOP(tofino::If)
  VISIT_NOP(tofino::Then)
  VISIT_NOP(tofino::Else)
  VISIT_NOP(tofino::Forward)
  VISIT_NOP(tofino::Drop)
  VISIT_NOP(tofino::Broadcast)
  VISIT_NOP(tofino::ParserExtraction)
  VISIT_NOP(tofino::ParserCondition)
  VISIT_NOP(tofino::ParserReject)
  VISIT_NOP(tofino::ModifyHeader)
  VISIT_NOP(tofino::TableLookup)
  VISIT_NOP(tofino::VectorRegisterLookup)
  VISIT_NOP(tofino::VectorRegisterUpdate)
  VISIT_NOP(tofino::FCFSCachedTableRead)
  VISIT_NOP(tofino::FCFSCachedTableReadOrWrite)
  VISIT_NOP(tofino::FCFSCachedTableWrite)
  VISIT_NOP(tofino::FCFSCachedTableDelete)
  VISIT_NOP(tofino::MapRegisterRead)
  VISIT_NOP(tofino::MapRegisterReadOrWrite)
  VISIT_NOP(tofino::MapRegisterWrite)
  VISIT_NOP(tofino::MapRegisterDelete)
  VISIT_NOP(tofino::MeterUpdate)
  VISIT_NOP(tofino::HHTableRead)
  VISIT_NOP(tofino::HHTableConditionalUpdate)
  VISIT_NOP(tofino::IntegerAllocatorRejuvenate)
  VISIT_NOP(tofino::IntegerAllocatorAllocate)
  VISIT_NOP(tofino::IntegerAllocatorIsAllocated)
  VISIT_NOP(tofino::CMSQuery)
  VISIT_NOP(tofino::CMSIncrement)
  VISIT_NOP(tofino::CMSIncAndQuery)

  // ========================================
  // Tofino CPU
  // ========================================

  VISIT_NOP(tofino_cpu::Ignore)
  VISIT_NOP(tofino_cpu::ParseHeader)
  VISIT_NOP(tofino_cpu::ModifyHeader)
  VISIT_NOP(tofino_cpu::ChecksumUpdate)
  VISIT_NOP(tofino_cpu::If)
  VISIT_NOP(tofino_cpu::Then)
  VISIT_NOP(tofino_cpu::Else)
  VISIT_NOP(tofino_cpu::Forward)
  VISIT_NOP(tofino_cpu::Broadcast)
  VISIT_NOP(tofino_cpu::Drop)
  VISIT_NOP(tofino_cpu::TableLookup)
  VISIT_NOP(tofino_cpu::TableUpdate)
  VISIT_NOP(tofino_cpu::TableDelete)
  VISIT_NOP(tofino_cpu::DchainAllocateNewIndex)
  VISIT_NOP(tofino_cpu::DchainRejuvenateIndex)
  VISIT_NOP(tofino_cpu::DchainIsIndexAllocated)
  VISIT_NOP(tofino_cpu::DchainFreeIndex)
  VISIT_NOP(tofino_cpu::VectorRead)
  VISIT_NOP(tofino_cpu::VectorWrite)
  VISIT_NOP(tofino_cpu::MapGet)
  VISIT_NOP(tofino_cpu::MapPut)
  VISIT_NOP(tofino_cpu::MapErase)
  VISIT_NOP(tofino_cpu::ChtFindBackend)
  VISIT_NOP(tofino_cpu::HashObj)
  VISIT_NOP(tofino_cpu::VectorRegisterLookup)
  VISIT_NOP(tofino_cpu::VectorRegisterUpdate)
  VISIT_NOP(tofino_cpu::FCFSCachedTableRead)
  VISIT_NOP(tofino_cpu::FCFSCachedTableWrite)
  VISIT_NOP(tofino_cpu::FCFSCachedTableDelete)
  VISIT_NOP(tofino_cpu::MapRegisterRead)
  VISIT_NOP(tofino_cpu::MapRegisterWrite)
  VISIT_NOP(tofino_cpu::MapRegisterDelete)
  VISIT_NOP(tofino_cpu::HHTableRead)
  VISIT_NOP(tofino_cpu::HHTableConditionalUpdate)
  VISIT_NOP(tofino_cpu::HHTableUpdate)
  VISIT_NOP(tofino_cpu::HHTableDelete)
  VISIT_NOP(tofino_cpu::TBIsTracing)
  VISIT_NOP(tofino_cpu::TBTrace)
  VISIT_NOP(tofino_cpu::TBUpdateAndCheck)
  VISIT_NOP(tofino_cpu::TBExpire)
  VISIT_NOP(tofino_cpu::MeterInsert)
  VISIT_NOP(tofino_cpu::IntegerAllocatorFreeIndex)
  VISIT_NOP(tofino_cpu::CMSUpdate)
  VISIT_NOP(tofino_cpu::CMSQuery)
  VISIT_NOP(tofino_cpu::CMSIncrement)
  VISIT_NOP(tofino_cpu::CMSCountMin)

  // ========================================
  // x86
  // ========================================

  VISIT_NOP(x86::Ignore)
  VISIT_NOP(x86::If)
  VISIT_NOP(x86::Then)
  VISIT_NOP(x86::Else)
  VISIT_NOP(x86::Forward)
  VISIT_NOP(x86::Broadcast)
  VISIT_NOP(x86::Drop)
  VISIT_NOP(x86::ParseHeader)
  VISIT_NOP(x86::ModifyHeader)
  VISIT_NOP(x86::MapGet)
  VISIT_NOP(x86::ExpireItemsSingleMap)
  VISIT_NOP(x86::ExpireItemsSingleMapIteratively)
  VISIT_NOP(x86::DchainRejuvenateIndex)
  VISIT_NOP(x86::VectorRead)
  VISIT_NOP(x86::VectorWrite)
  VISIT_NOP(x86::DchainAllocateNewIndex)
  VISIT_NOP(x86::MapPut)
  VISIT_NOP(x86::ChecksumUpdate)
  VISIT_NOP(x86::DchainIsIndexAllocated)
  VISIT_NOP(x86::CMSIncrement)
  VISIT_NOP(x86::CMSCountMin)
  VISIT_NOP(x86::CMSPeriodicCleanup)
  VISIT_NOP(x86::MapErase)
  VISIT_NOP(x86::DchainFreeIndex)
  VISIT_NOP(x86::ChtFindBackend)
  VISIT_NOP(x86::HashObj)
  VISIT_NOP(x86::TBIsTracing)
  VISIT_NOP(x86::TBTrace)
  VISIT_NOP(x86::TBUpdateAndCheck)
  VISIT_NOP(x86::TBExpire)

protected:
  virtual void log(const EPNode *ep_node) const;
};