#pragma once

#include <memory>

#define DECLARE_VISIT(M)                                                                 \
  Action visit(const EP *ep, const EPNode *ep_node, const M *node) override;

#define VISIT_NOP(M)                                                                     \
  virtual Action visit(const EP *ep, const EPNode *ep_node, const M *m) {                \
    return Action::doChildren;                                                           \
  }

#define VISIT_TODO(M)                                                                    \
  virtual Action visit(const EP *ep, const EPNode *ep_node, const M *m) {                \
    SYNAPSE_ASSERT(false, "TODO");                                                       \
  }

namespace synapse {
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

namespace ctrl {
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
} // namespace ctrl

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
  EPVisitor() = default;
  EPVisitor(const EPVisitor &) = default;
  EPVisitor(EPVisitor &&) = default;
  virtual ~EPVisitor() = default;

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

  VISIT_NOP(ctrl::Ignore)
  VISIT_NOP(ctrl::ParseHeader)
  VISIT_NOP(ctrl::ModifyHeader)
  VISIT_NOP(ctrl::ChecksumUpdate)
  VISIT_NOP(ctrl::If)
  VISIT_NOP(ctrl::Then)
  VISIT_NOP(ctrl::Else)
  VISIT_NOP(ctrl::Forward)
  VISIT_NOP(ctrl::Broadcast)
  VISIT_NOP(ctrl::Drop)
  VISIT_NOP(ctrl::TableLookup)
  VISIT_NOP(ctrl::TableUpdate)
  VISIT_NOP(ctrl::TableDelete)
  VISIT_NOP(ctrl::DchainAllocateNewIndex)
  VISIT_NOP(ctrl::DchainRejuvenateIndex)
  VISIT_NOP(ctrl::DchainIsIndexAllocated)
  VISIT_NOP(ctrl::DchainFreeIndex)
  VISIT_NOP(ctrl::VectorRead)
  VISIT_NOP(ctrl::VectorWrite)
  VISIT_NOP(ctrl::MapGet)
  VISIT_NOP(ctrl::MapPut)
  VISIT_NOP(ctrl::MapErase)
  VISIT_NOP(ctrl::ChtFindBackend)
  VISIT_NOP(ctrl::HashObj)
  VISIT_NOP(ctrl::VectorRegisterLookup)
  VISIT_NOP(ctrl::VectorRegisterUpdate)
  VISIT_NOP(ctrl::FCFSCachedTableRead)
  VISIT_NOP(ctrl::FCFSCachedTableWrite)
  VISIT_NOP(ctrl::FCFSCachedTableDelete)
  VISIT_NOP(ctrl::MapRegisterRead)
  VISIT_NOP(ctrl::MapRegisterWrite)
  VISIT_NOP(ctrl::MapRegisterDelete)
  VISIT_NOP(ctrl::HHTableRead)
  VISIT_NOP(ctrl::HHTableConditionalUpdate)
  VISIT_NOP(ctrl::HHTableUpdate)
  VISIT_NOP(ctrl::HHTableDelete)
  VISIT_NOP(ctrl::TBIsTracing)
  VISIT_NOP(ctrl::TBTrace)
  VISIT_NOP(ctrl::TBUpdateAndCheck)
  VISIT_NOP(ctrl::TBExpire)
  VISIT_NOP(ctrl::MeterInsert)
  VISIT_NOP(ctrl::IntegerAllocatorFreeIndex)
  VISIT_NOP(ctrl::CMSUpdate)
  VISIT_NOP(ctrl::CMSQuery)
  VISIT_NOP(ctrl::CMSIncrement)
  VISIT_NOP(ctrl::CMSCountMin)

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
} // namespace synapse