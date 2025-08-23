#pragma once

#include <LibBDD/BDD.h>
#include <LibSynapse/Visitor.h>
#include <LibSynapse/Target.h>

// For debugging
#include <LibCore/Debug.h>
#include <LibSynapse/Visualizers/ProfilerVisualizer.h>

#include <string>

namespace LibSynapse {

using LibBDD::BDDNode;

class EP;
class EPNode;

enum class ModuleCategory {
  InvalidModule,

  // ========================================
  // Tofino
  // ========================================

  Tofino_Ignore,
  Tofino_SendToController,
  Tofino_Forward,
  Tofino_Drop,
  Tofino_Broadcast,
  Tofino_Recirculate,
  Tofino_If,
  Tofino_Then,
  Tofino_Else,
  Tofino_ParserExtraction,
  Tofino_ParserCondition,
  Tofino_ParserReject,
  Tofino_ModifyHeader,
  Tofino_MapTableLookup,
  Tofino_GuardedMapTableLookup,
  Tofino_GuardedMapTableGuardCheck,
  Tofino_VectorTableLookup,
  Tofino_DchainTableLookup,
  Tofino_VectorRegisterLookup,
  Tofino_VectorRegisterUpdate,
  Tofino_FCFSCachedTableRead,
  Tofino_FCFSCachedTableReadWrite,
  Tofino_FCFSCachedTableWrite,
  Tofino_FCFSCachedTableDelete,
  Tofino_MeterUpdate,
  Tofino_HHTableRead,
  Tofino_HHTableOutOfBandUpdate,
  Tofino_IntegerAllocatorRejuvenate,
  Tofino_IntegerAllocatorAllocate,
  Tofino_IntegerAllocatorIsAllocated,
  Tofino_CMSQuery,
  Tofino_CMSIncrement,
  Tofino_CMSIncAndQuery,
  Tofino_LPMLookup,
  Tofino_CuckooHashTableReadWrite,

  // ========================================
  // Controller
  // ========================================

  Controller_Ignore,
  Controller_ParseHeader,
  Controller_ModifyHeader,
  Controller_ChecksumUpdate,
  Controller_If,
  Controller_Then,
  Controller_Else,
  Controller_Forward,
  Controller_Broadcast,
  Controller_Drop,
  Controller_AbortTransaction,

  Controller_DataplaneMapTableAllocate,
  Controller_DataplaneMapTableLookup,
  Controller_DataplaneMapTableUpdate,
  Controller_DataplaneMapTableDelete,
  Controller_DataplaneVectorTableAllocate,
  Controller_DataplaneVectorTableLookup,
  Controller_DataplaneVectorTableUpdate,
  Controller_DataplaneDchainTableAllocate,
  Controller_DataplaneDchainTableIsIndexAllocated,
  Controller_DataplaneDchainTableAllocateNewIndex,
  Controller_DataplaneDchainTableFreeIndex,
  Controller_DataplaneDchainTableRefreshIndex,
  Controller_DataplaneVectorRegisterAllocate,
  Controller_DataplaneVectorRegisterLookup,
  Controller_DataplaneVectorRegisterUpdate,
  Controller_DataplaneFCFSCachedTableAllocate,
  Controller_DataplaneFCFSCachedTableRead,
  Controller_DataplaneFCFSCachedTableWrite,
  Controller_DataplaneFCFSCachedTableDelete,
  Controller_DataplaneHHTableAllocate,
  Controller_DataplaneHHTableRead,
  Controller_DataplaneHHTableUpdate,
  Controller_DataplaneHHTableDelete,
  Controller_DataplaneHHTableIsIndexAllocated,
  Controller_DataplaneHHTableOutOfBandUpdate,
  Controller_DataplaneIntegerAllocatorAllocate,
  Controller_DataplaneIntegerAllocatorFreeIndex,
  Controller_DataplaneMeterAllocate,
  Controller_DataplaneMeterInsert,
  Controller_DataplaneGuardedMapTableAllocate,
  Controller_DataplaneGuardedMapTableLookup,
  Controller_DataplaneGuardedMapTableUpdate,
  Controller_DataplaneGuardedMapTableDelete,
  Controller_DataplaneGuardedMapTableGuardCheck,
  Controller_DataplaneCMSAllocate,
  Controller_DataplaneCMSQuery,
  Controller_DataplaneCuckooHashTableAllocate,

  Controller_DchainAllocate,
  Controller_DchainAllocateNewIndex,
  Controller_DchainRejuvenateIndex,
  Controller_DchainIsIndexAllocated,
  Controller_DchainFreeIndex,
  Controller_VectorAllocate,
  Controller_VectorRead,
  Controller_VectorWrite,
  Controller_MapAllocate,
  Controller_MapGet,
  Controller_MapPut,
  Controller_MapErase,
  Controller_ChtAllocate,
  Controller_ChtFindBackend,
  Controller_HashObj,
  Controller_TokenBucketAllocate,
  Controller_TokenBucketIsTracing,
  Controller_TokenBucketTrace,
  Controller_TokenBucketUpdateAndCheck,
  Controller_TokenBucketExpire,
  Controller_CMSAllocate,
  Controller_CMSUpdate,
  Controller_CMSQuery,
  Controller_CMSIncrement,
  Controller_CMSCountMin,

  // ========================================
  // x86
  // ========================================

  x86_Ignore,
  x86_If,
  x86_Then,
  x86_Else,
  x86_Forward,
  x86_Drop,
  x86_Broadcast,
  x86_ParseHeader,
  x86_ModifyHeader,
  x86_ChecksumUpdate,
  x86_MapGet,
  x86_MapPut,
  x86_MapErase,
  x86_VectorRead,
  x86_VectorWrite,
  x86_DchainAllocateNewIndex,
  x86_DchainRejuvenateIndex,
  x86_DchainIsIndexAllocated,
  x86_DchainFreeIndex,
  x86_CMSIncrement,
  x86_CMSCountMin,
  x86_CMSPeriodicCleanup,
  x86_ExpireItemsSingleMap,
  x86_ExpireItemsSingleMapIteratively,
  x86_ChtFindBackend,
  x86_HashObj,
  x86_TokenBucketIsTracing,
  x86_TokenBucketTrace,
  x86_TokenBucketUpdateAndCheck,
  x86_TokenBucketExpire,
};

struct ModuleType {
  ModuleCategory type;
  std::string instance_id;

  ModuleType(ModuleCategory _type, const std::string &_instance_id) : type(_type), instance_id(_instance_id) {}
  bool operator==(const ModuleType &other) const { return type == other.type && instance_id == other.instance_id; }
};

inline std::ostream &operator<<(std::ostream &os, ModuleType type) {
  os << type.instance_id << ": ";
  switch (type.type) {
  case ModuleCategory::InvalidModule:
    os << "InvalidModule";
    break;
  case ModuleCategory::Tofino_SendToController:
    os << "Tofino_SendToController";
    break;
  case ModuleCategory::Tofino_Ignore:
    os << "Tofino_Ignore";
    break;
  case ModuleCategory::Tofino_If:
    os << "Tofino_If";
    break;
  case ModuleCategory::Tofino_ParserExtraction:
    os << "Tofino_ParserExtraction";
    break;
  case ModuleCategory::Tofino_ParserCondition:
    os << "Tofino_ParserCondition";
    break;
  case ModuleCategory::Tofino_ParserReject:
    os << "Tofino_ParserReject";
    break;
  case ModuleCategory::Tofino_Then:
    os << "Tofino_Then";
    break;
  case ModuleCategory::Tofino_Else:
    os << "Tofino_Else";
    break;
  case ModuleCategory::Tofino_Forward:
    os << "Tofino_Forward";
    break;
  case ModuleCategory::Tofino_Drop:
    os << "Tofino_Drop";
    break;
  case ModuleCategory::Tofino_Broadcast:
    os << "Tofino_Broadcast";
    break;
  case ModuleCategory::Tofino_ModifyHeader:
    os << "Tofino_ModifyHeader";
    break;
  case ModuleCategory::Tofino_MapTableLookup:
    os << "Tofino_MapTableLookup";
    break;
  case ModuleCategory::Tofino_GuardedMapTableLookup:
    os << "Tofino_GuardedMapTableLookup";
    break;
  case ModuleCategory::Tofino_GuardedMapTableGuardCheck:
    os << "Tofino_GuardedMapTableGuardCheck";
    break;
  case ModuleCategory::Tofino_DchainTableLookup:
    os << "Tofino_DchainTableLookup";
    break;
  case ModuleCategory::Tofino_VectorTableLookup:
    os << "Tofino_VectorTableLookup";
    break;
  case ModuleCategory::Tofino_VectorRegisterLookup:
    os << "Tofino_VectorRegisterLookup";
    break;
  case ModuleCategory::Tofino_VectorRegisterUpdate:
    os << "Tofino_VectorRegisterUpdate";
    break;
  case ModuleCategory::Tofino_FCFSCachedTableRead:
    os << "Tofino_FCFSCachedTableRead";
    break;
  case ModuleCategory::Tofino_FCFSCachedTableReadWrite:
    os << "Tofino_FCFSCachedTableReadWrite";
    break;
  case ModuleCategory::Tofino_FCFSCachedTableWrite:
    os << "Tofino_FCFSCachedTableWrite";
    break;
  case ModuleCategory::Tofino_FCFSCachedTableDelete:
    os << "Tofino_FCFSCachedTableDelete";
    break;
  case ModuleCategory::Tofino_MeterUpdate:
    os << "Tofino_MeterUpdate";
    break;
  case ModuleCategory::Tofino_HHTableRead:
    os << "Tofino_HHTableRead";
    break;
  case ModuleCategory::Tofino_HHTableOutOfBandUpdate:
    os << "Tofino_HHTableOutOfBandUpdate";
    break;
  case ModuleCategory::Tofino_IntegerAllocatorRejuvenate:
    os << "Tofino_IntegerAllocatorRejuvenate";
    break;
  case ModuleCategory::Tofino_IntegerAllocatorAllocate:
    os << "Tofino_IntegerAllocatorAllocate";
    break;
  case ModuleCategory::Tofino_IntegerAllocatorIsAllocated:
    os << "Tofino_IntegerAllocatorIsAllocated";
    break;
  case ModuleCategory::Tofino_CMSQuery:
    os << "Tofino_CMSQuery";
    break;
  case ModuleCategory::Tofino_CMSIncrement:
    os << "Tofino_CMSIncrement";
    break;
  case ModuleCategory::Tofino_CMSIncAndQuery:
    os << "Tofino_CMSIncAndQuery";
    break;
  case ModuleCategory::Tofino_Recirculate:
    os << "Tofino_Recirculate";
    break;
  case ModuleCategory::Tofino_LPMLookup:
    os << "Tofino_LPMLookup";
    break;
  case ModuleType::Tofino_CuckooHashTableReadWrite:
    os << "Tofino_CuckooHashTableReadWrite";
    break;
  case ModuleType::Controller_Ignore:
    os << "Controller_Ignore";
    break;
  case ModuleCategory::Controller_ParseHeader:
    os << "Controller_ParseHeader";
    break;
  case ModuleCategory::Controller_ModifyHeader:
    os << "Controller_ModifyHeader";
    break;
  case ModuleCategory::Controller_ChecksumUpdate:
    os << "Controller_ChecksumUpdate";
    break;
  case ModuleCategory::Controller_If:
    os << "Controller_If";
    break;
  case ModuleCategory::Controller_Then:
    os << "Controller_Then";
    break;
  case ModuleCategory::Controller_Else:
    os << "Controller_Else";
    break;
  case ModuleCategory::Controller_Forward:
    os << "Controller_Forward";
    break;
  case ModuleCategory::Controller_Broadcast:
    os << "Controller_Broadcast";
    break;
  case ModuleCategory::Controller_Drop:
    os << "Controller_Drop";
    break;
  case ModuleCategory::Controller_AbortTransaction:
    os << "Controller_AbortTransaction";
    break;
  case ModuleCategory::Controller_DataplaneMapTableAllocate:
    os << "Controller_DataplaneMapTableAllocate";
    break;
  case ModuleCategory::Controller_DataplaneMapTableLookup:
    os << "Controller_DataplaneMapTableLookup";
    break;
  case ModuleCategory::Controller_DataplaneMapTableUpdate:
    os << "Controller_DataplaneMapTableUpdate";
    break;
  case ModuleCategory::Controller_DataplaneMapTableDelete:
    os << "Controller_DataplaneMapTableDelete";
    break;
  case ModuleCategory::Controller_DataplaneGuardedMapTableAllocate:
    os << "Controller_DataplaneGuardedMapTableAllocate";
    break;
  case ModuleCategory::Controller_DataplaneGuardedMapTableLookup:
    os << "Controller_DataplaneGuardedMapTableLookup";
    break;
  case ModuleCategory::Controller_DataplaneGuardedMapTableUpdate:
    os << "Controller_DataplaneGuardedMapTableUpdate";
    break;
  case ModuleCategory::Controller_DataplaneGuardedMapTableDelete:
    os << "Controller_DataplaneGuardedMapTableDelete";
    break;
  case ModuleCategory::Controller_DataplaneGuardedMapTableGuardCheck:
    os << "Controller_DataplaneGuardedMapTableGuardCheck";
    break;
  case ModuleCategory::Controller_DataplaneDchainTableAllocate:
    os << "Controller_DataplaneDchainTableAllocate";
    break;
  case ModuleCategory::Controller_DataplaneDchainTableIsIndexAllocated:
    os << "Controller_DataplaneDchainTableIsIndexAllocated";
    break;
  case ModuleCategory::Controller_DataplaneDchainTableAllocateNewIndex:
    os << "Controller_DataplaneDchainTableAllocateNewIndex";
    break;
  case ModuleCategory::Controller_DataplaneDchainTableFreeIndex:
    os << "Controller_DataplaneDchainTableFreeIndex";
    break;
  case ModuleCategory::Controller_DataplaneDchainTableRefreshIndex:
    os << "Controller_DataplaneDchainTableRefreshIndex";
    break;
  case ModuleCategory::Controller_DataplaneVectorTableAllocate:
    os << "Controller_DataplaneVectorTableAllocate";
    break;
  case ModuleCategory::Controller_DataplaneVectorTableLookup:
    os << "Controller_DataplaneVectorTableLookup";
    break;
  case ModuleCategory::Controller_DataplaneVectorTableUpdate:
    os << "Controller_DataplaneVectorTableUpdate";
    break;
  case ModuleCategory::Controller_DataplaneFCFSCachedTableAllocate:
    os << "Controller_DataplaneFCFSCachedTableAllocate";
    break;
  case ModuleCategory::Controller_DataplaneFCFSCachedTableRead:
    os << "Controller_DataplaneFCFSCachedTableRead";
    break;
  case ModuleCategory::Controller_DataplaneFCFSCachedTableWrite:
    os << "Controller_DataplaneFCFSCachedTableWrite";
    break;
  case ModuleCategory::Controller_DataplaneFCFSCachedTableDelete:
    os << "Controller_DataplaneFCFSCachedTableDelete";
    break;
  case ModuleCategory::Controller_DataplaneHHTableAllocate:
    os << "Controller_DataplaneHHTableAllocate";
    break;
  case ModuleCategory::Controller_DataplaneHHTableRead:
    os << "Controller_DataplaneHHTableRead";
    break;
  case ModuleCategory::Controller_DataplaneHHTableUpdate:
    os << "Controller_DataplaneHHTableUpdate";
    break;
  case ModuleCategory::Controller_DataplaneHHTableIsIndexAllocated:
    os << "Controller_DataplaneHHTableIsIndexAllocated";
    break;
  case ModuleType::Controller_DataplaneHHTableOutOfBandUpdate:
    os << "Controller_DataplaneHHTableOutOfBandUpdate";
    break;
  case ModuleType::Controller_DataplaneHHTableDelete:
    os << "Controller_DataplaneHHTableDelete";
    break;
  case ModuleCategory::Controller_DchainAllocate:
    os << "Controller_DchainAllocate";
    break;
  case ModuleCategory::Controller_DchainAllocateNewIndex:
    os << "Controller_DchainAllocateNewIndex";
    break;
  case ModuleCategory::Controller_DchainRejuvenateIndex:
    os << "Controller_DchainRejuvenateIndex";
    break;
  case ModuleCategory::Controller_DchainIsIndexAllocated:
    os << "Controller_DchainIsIndexAllocated";
    break;
  case ModuleCategory::Controller_DchainFreeIndex:
    os << "Controller_DchainFreeIndex";
    break;
  case ModuleCategory::Controller_DataplaneIntegerAllocatorAllocate:
    os << "Controller_DataplaneIntegerAllocatorAllocate";
    break;
  case ModuleCategory::Controller_DataplaneIntegerAllocatorFreeIndex:
    os << "Controller_DataplaneIntegerAllocatorFreeIndex";
    break;
  case ModuleCategory::Controller_DataplaneCMSAllocate:
    os << "Controller_DataplaneCMSAllocate";
    break;
  case ModuleCategory::Controller_DataplaneCMSQuery:
    os << "Controller_DataplaneCMSQuery";
    break;
  case ModuleCategory::Controller_VectorAllocate:
    os << "Controller_VectorAllocate";
    break;
  case ModuleCategory::Controller_VectorRead:
    os << "Controller_VectorRead";
    break;
  case ModuleCategory::Controller_VectorWrite:
    os << "Controller_VectorWrite";
    break;
  case ModuleCategory::Controller_MapAllocate:
    os << "Controller_MapAllocate";
    break;
  case ModuleCategory::Controller_MapGet:
    os << "Controller_MapGet";
    break;
  case ModuleCategory::Controller_MapPut:
    os << "Controller_MapPut";
    break;
  case ModuleCategory::Controller_MapErase:
    os << "Controller_MapErase";
    break;
  case ModuleCategory::Controller_ChtAllocate:
    os << "Controller_ChtAllocate";
    break;
  case ModuleCategory::Controller_ChtFindBackend:
    os << "Controller_ChtFindBackend";
    break;
  case ModuleCategory::Controller_HashObj:
    os << "Controller_HashObj";
    break;
  case ModuleCategory::Controller_DataplaneVectorRegisterAllocate:
    os << "Controller_DataplaneVectorRegisterAllocate";
    break;
  case ModuleCategory::Controller_DataplaneVectorRegisterLookup:
    os << "Controller_DataplaneVectorRegisterLookup";
    break;
  case ModuleCategory::Controller_DataplaneVectorRegisterUpdate:
    os << "Controller_DataplaneVectorRegisterUpdate";
    break;
  case ModuleCategory::Controller_TokenBucketAllocate:
    os << "Controller_TokenBucketAllocate";
    break;
  case ModuleCategory::Controller_TokenBucketIsTracing:
    os << "Controller_TokenBucketIsTracing";
    break;
  case ModuleCategory::Controller_TokenBucketTrace:
    os << "Controller_TokenBucketTrace";
    break;
  case ModuleCategory::Controller_TokenBucketUpdateAndCheck:
    os << "Controller_TokenBucketUpdateAndCheck";
    break;
  case ModuleCategory::Controller_TokenBucketExpire:
    os << "Controller_TokenBucketExpire";
    break;
  case ModuleCategory::Controller_DataplaneMeterAllocate:
    os << "Controller_DataplaneMeterAllocate";
    break;
  case ModuleCategory::Controller_DataplaneMeterInsert:
    os << "Controller_DataplaneMeterInsert";
    break;
  case ModuleCategory::Controller_CMSAllocate:
    os << "Controller_CMSAllocate";
    break;
  case ModuleCategory::Controller_CMSUpdate:
    os << "Controller_CMSUpdate";
    break;
  case ModuleCategory::Controller_CMSQuery:
    os << "Controller_CMSQuery";
    break;
  case ModuleCategory::Controller_CMSIncrement:
    os << "Controller_CMSIncrement";
    break;
  case ModuleCategory::Controller_CMSCountMin:
    os << "Controller_CMSCountMin";
    break;
  case ModuleType::Controller_DataplaneCuckooHashTableAllocate:
    os << "Controller_DataplaneCuckooHashTableAllocate";
    break;
  case ModuleType::x86_Ignore:
    os << "x86_Ignore";
    break;
  case ModuleCategory::x86_If:
    os << "x86_If";
    break;
  case ModuleCategory::x86_Then:
    os << "x86_Then";
    break;
  case ModuleCategory::x86_Else:
    os << "x86_Else";
    break;
  case ModuleCategory::x86_Forward:
    os << "x86_Forward";
    break;
  case ModuleCategory::x86_ParseHeader:
    os << "x86_ParseHeader";
    break;
  case ModuleCategory::x86_ModifyHeader:
    os << "x86_ModifyHeader";
    break;
  case ModuleCategory::x86_MapGet:
    os << "x86_MapGet";
    break;
  case ModuleCategory::x86_MapPut:
    os << "x86_MapPut";
    break;
  case ModuleCategory::x86_MapErase:
    os << "x86_MapErase";
    break;
  case ModuleCategory::x86_VectorRead:
    os << "x86_VectorRead";
    break;
  case ModuleCategory::x86_VectorWrite:
    os << "x86_VectorWrite";
    break;
  case ModuleCategory::x86_DchainAllocateNewIndex:
    os << "x86_DchainAllocateNewIndex";
    break;
  case ModuleCategory::x86_DchainRejuvenateIndex:
    os << "x86_DchainRejuvenateIndex";
    break;
  case ModuleCategory::x86_DchainIsIndexAllocated:
    os << "x86_DchainIsIndexAllocated";
    break;
  case ModuleCategory::x86_DchainFreeIndex:
    os << "x86_DchainFreeIndex";
    break;
  case ModuleCategory::x86_CMSIncrement:
    os << "x86_CMSIncrement";
    break;
  case ModuleCategory::x86_CMSCountMin:
    os << "x86_CMSCountMin";
    break;
  case ModuleCategory::x86_CMSPeriodicCleanup:
    os << "x86_CMSPeriodicCleanup";
    break;
  case ModuleCategory::x86_Drop:
    os << "x86_Drop";
    break;
  case ModuleCategory::x86_Broadcast:
    os << "x86_Broadcast";
    break;
  case ModuleCategory::x86_ExpireItemsSingleMap:
    os << "x86_ExpireItemsSingleMap";
    break;
  case ModuleCategory::x86_ExpireItemsSingleMapIteratively:
    os << "x86_ExpireItemsSingleMapIteratively";
    break;
  case ModuleCategory::x86_ChecksumUpdate:
    os << "x86_ChecksumUpdate";
    break;
  case ModuleCategory::x86_ChtFindBackend:
    os << "x86_ChtFindBackend";
    break;
  case ModuleCategory::x86_HashObj:
    os << "x86_HashObj";
    break;
  case ModuleCategory::x86_TokenBucketIsTracing:
    os << "x86_TokenBucketIsTracing";
    break;
  case ModuleCategory::x86_TokenBucketTrace:
    os << "x86_TokenBucketTrace";
    break;
  case ModuleCategory::x86_TokenBucketUpdateAndCheck:
    os << "x86_TokenBucketUpdateAndCheck";
    break;
  case ModuleCategory::x86_TokenBucketExpire:
    os << "x86_TokenBucketExpire";
    break;
  }

  return os;
}

inline std::string to_string(ModuleType type) {
  std::stringstream ss;
  ss << type;
  return ss.str();
}

class Module {
protected:
  ModuleType type;
  TargetType target;
  TargetType next_target;
  std::string name;
  const BDDNode *node;

  Module(ModuleType _type, TargetType _target, const std::string &_name, const BDDNode *_node)
      : type(_type), target(_target), next_target(_target), name(_name), node(_node) {}

  Module(ModuleType _type, TargetType _target, TargetType _next_target, const std::string &_name, const BDDNode *_node)
      : type(_type), target(_target), next_target(_next_target), name(_name), node(_node) {}

public:
  Module(const Module &other) = delete;
  Module(Module &&other)      = delete;

  Module &operator=(const Module &other) = delete;

  virtual ~Module() {}

  ModuleType get_type() const { return type; }
  const std::string &get_name() const { return name; }
  TargetType get_target() const { return target; }
  TargetType get_next_target() const { return next_target; }
  const BDDNode *get_node() const { return node; }

  void set_node(const BDDNode *new_node) { node = new_node; }

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const = 0;
  virtual Module *clone() const                                                                  = 0;
};

} // namespace LibSynapse

namespace std {
template <> struct hash<LibSynapse::ModuleType> {
  std::size_t operator()(const LibSynapse::ModuleType &mt) const {
    return std::hash<int>()(static_cast<int>(mt.type)) ^ std::hash<std::string>()(mt.instance_id);
  }
};
} // namespace std
