#pragma once

#include <memory>

namespace LibSynapse {

class EP;
class EPNode;

namespace Tofino {
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
class MapTableLookup;
class GuardedMapTableLookup;
class GuardedMapTableGuardCheck;
class DchainTableLookup;
class VectorTableLookup;
class VectorRegisterLookup;
class VectorRegisterUpdate;
class FCFSCachedTableRead;
class FCFSCachedTableReadWrite;
class FCFSCachedTableWrite;
class FCFSCachedTableDelete;
class MeterUpdate;
class HHTableRead;
class HHTableOutOfBandUpdate;
class IntegerAllocatorRejuvenate;
class IntegerAllocatorAllocate;
class IntegerAllocatorIsAllocated;
class CMSQuery;
class CMSIncrement;
class CMSIncAndQuery;
class LPMLookup;
class CuckooHashTableReadWrite;
} // namespace Tofino

namespace Controller {
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
class AbortTransaction;

class DataplaneMapTableAllocate;
class DataplaneMapTableLookup;
class DataplaneMapTableUpdate;
class DataplaneMapTableDelete;
class DataplaneGuardedMapTableAllocate;
class DataplaneGuardedMapTableLookup;
class DataplaneGuardedMapTableGuardCheck;
class GuardedMapTableGuardCheck;
class DataplaneGuardedMapTableUpdate;
class DataplaneGuardedMapTableDelete;
class DataplaneVectorTableAllocate;
class DataplaneVectorTableLookup;
class DataplaneVectorTableUpdate;
class DataplaneDchainTableAllocate;
class DataplaneDchainTableIsIndexAllocated;
class DataplaneDchainTableAllocateNewIndex;
class DataplaneDchainTableFreeIndex;
class DataplaneDchainTableRefreshIndex;
class DataplaneVectorRegisterAllocate;
class DataplaneVectorRegisterLookup;
class DataplaneVectorRegisterUpdate;
class DataplaneFCFSCachedTableAllocate;
class DataplaneFCFSCachedTableRead;
class DataplaneFCFSCachedTableWrite;
class DataplaneFCFSCachedTableDelete;
class DataplaneHHTableAllocate;
class DataplaneHHTableRead;
class DataplaneHHTableUpdate;
class DataplaneHHTableDelete;
class DataplaneHHTableIsIndexAllocated;
class DataplaneHHTableOutOfBandUpdate;
class DataplaneMeterAllocate;
class DataplaneMeterInsert;
class DataplaneIntegerAllocatorAllocate;
class DataplaneIntegerAllocatorFreeIndex;
class DataplaneCMSAllocate;
class DataplaneCMSQuery;
class DataplaneCuckooHashTableAllocate;

class DchainAllocate;
class DchainAllocateNewIndex;
class DchainRejuvenateIndex;
class DchainIsIndexAllocated;
class DchainFreeIndex;
class VectorAllocate;
class VectorRead;
class VectorWrite;
class MapAllocate;
class MapGet;
class MapPut;
class MapErase;
class ChtAllocate;
class ChtFindBackend;
class HashObj;
class TokenBucketAllocate;
class TokenBucketIsTracing;
class TokenBucketTrace;
class TokenBucketUpdateAndCheck;
class TokenBucketExpire;
class CMSAllocate;
class CMSUpdate;
class CMSQuery;
class CMSIncrement;
class CMSCountMin;
} // namespace Controller

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
class SendToDevice;
class TokenBucketIsTracing;
class TokenBucketTrace;
class TokenBucketUpdateAndCheck;
class TokenBucketExpire;
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
  virtual void visit_children(const EP *, const EPNode *);

  // ========================================
  // Tofino
  // ========================================

  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToController *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Recirculate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Ignore *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::If *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Then *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Else *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Forward *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Drop *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Broadcast *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserExtraction *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserCondition *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserReject *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ModifyHeader *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::MapTableLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableGuardCheck *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::DchainTableLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorTableLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableReadWrite *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableWrite *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableDelete *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::MeterUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableOutOfBandUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::IntegerAllocatorRejuvenate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::IntegerAllocatorAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::IntegerAllocatorIsAllocated *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSQuery *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncrement *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncAndQuery *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::LPMLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CuckooHashTableReadWrite *m) { return Action::doChildren; }

  // ========================================
  // Controller
  // ========================================

  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::Ignore *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::ParseHeader *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::ModifyHeader *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::ChecksumUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::If *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::Then *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::Else *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::Forward *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::Broadcast *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::Drop *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::AbortTransaction *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMapTableAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMapTableLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMapTableUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMapTableDelete *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableGuardCheck *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneGuardedMapTableDelete *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableIsIndexAllocated *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableAllocateNewIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableFreeIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneDchainTableRefreshIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorTableAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorTableLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorTableUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocateNewIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainRejuvenateIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainIsIndexAllocated *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainFreeIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorWrite *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapGet *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapPut *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapErase *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::ChtAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::ChtFindBackend *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::HashObj *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorRegisterAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorRegisterLookup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneVectorRegisterUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneFCFSCachedTableAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneFCFSCachedTableRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneFCFSCachedTableWrite *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneFCFSCachedTableDelete *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableIsIndexAllocated *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableOutOfBandUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneHHTableDelete *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketIsTracing *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketTrace *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketUpdateAndCheck *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketExpire *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMeterAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneMeterInsert *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneIntegerAllocatorAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneIntegerAllocatorFreeIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneCMSAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneCMSQuery *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSAllocate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSQuery *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSIncrement *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSCountMin *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const Controller::DataplaneCuckooHashTableAllocate *m) { return Action::doChildren; }

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
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMap *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMapIteratively *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainRejuvenateIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::VectorRead *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::VectorWrite *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainAllocateNewIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::MapPut *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ChecksumUpdate *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainIsIndexAllocated *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSIncrement *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSCountMin *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSPeriodicCleanup *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::MapErase *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainFreeIndex *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::ChtFindBackend *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::HashObj *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::SendToDevice *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketIsTracing *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketTrace *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketUpdateAndCheck *m) { return Action::doChildren; }
  virtual Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketExpire *m) { return Action::doChildren; }

protected:
  virtual void log(const EPNode *) const;
};

} // namespace LibSynapse