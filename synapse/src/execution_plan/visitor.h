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
    ASSERT(false, "TODO");                                                               \
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

namespace controller {
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
} // namespace controller

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

  VISIT_NOP(controller::Ignore)
  VISIT_NOP(controller::ParseHeader)
  VISIT_NOP(controller::ModifyHeader)
  VISIT_NOP(controller::ChecksumUpdate)
  VISIT_NOP(controller::If)
  VISIT_NOP(controller::Then)
  VISIT_NOP(controller::Else)
  VISIT_NOP(controller::Forward)
  VISIT_NOP(controller::Broadcast)
  VISIT_NOP(controller::Drop)
  VISIT_NOP(controller::TableLookup)
  VISIT_NOP(controller::TableUpdate)
  VISIT_NOP(controller::TableDelete)
  VISIT_NOP(controller::DchainAllocateNewIndex)
  VISIT_NOP(controller::DchainRejuvenateIndex)
  VISIT_NOP(controller::DchainIsIndexAllocated)
  VISIT_NOP(controller::DchainFreeIndex)
  VISIT_NOP(controller::VectorRead)
  VISIT_NOP(controller::VectorWrite)
  VISIT_NOP(controller::MapGet)
  VISIT_NOP(controller::MapPut)
  VISIT_NOP(controller::MapErase)
  VISIT_NOP(controller::ChtFindBackend)
  VISIT_NOP(controller::HashObj)
  VISIT_NOP(controller::VectorRegisterLookup)
  VISIT_NOP(controller::VectorRegisterUpdate)
  VISIT_NOP(controller::FCFSCachedTableRead)
  VISIT_NOP(controller::FCFSCachedTableWrite)
  VISIT_NOP(controller::FCFSCachedTableDelete)
  VISIT_NOP(controller::MapRegisterRead)
  VISIT_NOP(controller::MapRegisterWrite)
  VISIT_NOP(controller::MapRegisterDelete)
  VISIT_NOP(controller::HHTableRead)
  VISIT_NOP(controller::HHTableConditionalUpdate)
  VISIT_NOP(controller::HHTableUpdate)
  VISIT_NOP(controller::HHTableDelete)
  VISIT_NOP(controller::TBIsTracing)
  VISIT_NOP(controller::TBTrace)
  VISIT_NOP(controller::TBUpdateAndCheck)
  VISIT_NOP(controller::TBExpire)
  VISIT_NOP(controller::MeterInsert)
  VISIT_NOP(controller::IntegerAllocatorFreeIndex)
  VISIT_NOP(controller::CMSUpdate)
  VISIT_NOP(controller::CMSQuery)
  VISIT_NOP(controller::CMSIncrement)
  VISIT_NOP(controller::CMSCountMin)

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