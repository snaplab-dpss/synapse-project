#pragma once

#include <memory>

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
  EPVisitor()                  = default;
  EPVisitor(const EPVisitor &) = default;
  EPVisitor(EPVisitor &&)      = default;
  virtual ~EPVisitor()         = default;

  enum class Action { skipChildren, doChildren };
  virtual void visit(const EP *);
  virtual void visit(const EP *, const EPNode *);

  // ========================================
  // Tofino
  // ========================================

  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::SendToController *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::Recirculate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::Ignore *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::If *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::Then *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::Else *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::Forward *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::Drop *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::Broadcast *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::ParserExtraction *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::ParserCondition *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::ParserReject *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::ModifyHeader *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::TableLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterLookup *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterUpdate *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableRead *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableReadOrWrite *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableWrite *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableDelete *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::MeterUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::HHTableRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::HHTableConditionalUpdate *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::IntegerAllocatorRejuvenate *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::IntegerAllocatorAllocate *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::IntegerAllocatorIsAllocated *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::CMSQuery *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::CMSIncrement *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const tofino::CMSIncAndQuery *m) {
    return Action::doChildren;
  }

  // ========================================
  // Tofino CPU
  // ========================================

  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Ignore *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::ParseHeader *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::ModifyHeader *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::ChecksumUpdate *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::If *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Then *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Else *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Forward *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Broadcast *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::Drop *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TableLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TableUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TableDelete *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::DchainAllocateNewIndex *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::DchainRejuvenateIndex *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::DchainIsIndexAllocated *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::DchainFreeIndex *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::VectorRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::VectorWrite *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::MapGet *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::MapPut *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::MapErase *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::ChtFindBackend *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HashObj *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::VectorRegisterLookup *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::VectorRegisterUpdate *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::FCFSCachedTableRead *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::FCFSCachedTableWrite *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::FCFSCachedTableDelete *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HHTableRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HHTableConditionalUpdate *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HHTableUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::HHTableDelete *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TBIsTracing *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TBTrace *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TBUpdateAndCheck *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::TBExpire *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::MeterInsert *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::IntegerAllocatorFreeIndex *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::CMSUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::CMSQuery *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::CMSIncrement *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const ctrl::CMSCountMin *m) { return Action::doChildren; }

  // ========================================
  // x86
  // ========================================

  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::Ignore *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::If *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::Then *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::Else *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::Forward *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::Broadcast *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::Drop *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ParseHeader *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ModifyHeader *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::MapGet *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMap *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMapIteratively *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainRejuvenateIndex *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::VectorRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::VectorWrite *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainAllocateNewIndex *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::MapPut *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ChecksumUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainIsIndexAllocated *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSIncrement *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSCountMin *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSPeriodicCleanup *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::MapErase *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainFreeIndex *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ChtFindBackend *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::HashObj *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::TBIsTracing *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::TBTrace *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::TBUpdateAndCheck *m) {
    return Action::doChildren;
  }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::TBExpire *m) { return Action::doChildren; }

protected:
  virtual void log(const EPNode *) const;
};
} // namespace synapse