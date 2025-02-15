#pragma once

#include <LibBDD/BDD.h>
#include <LibSynapse/Visitor.h>

#include <string>

namespace LibSynapse {

class EP;
class EPNode;
enum class TargetType;

enum class ModuleType {
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
  Tofino_TableLookup,
  Tofino_VectorRegisterLookup,
  Tofino_VectorRegisterUpdate,
  Tofino_FCFSCachedTableRead,
  Tofino_FCFSCachedTableReadWrite,
  Tofino_FCFSCachedTableWrite,
  Tofino_FCFSCachedTableDelete,
  Tofino_MeterUpdate,
  Tofino_HHTableRead,
  Tofino_HHTableConditionalUpdate,
  Tofino_IntegerAllocatorRejuvenate,
  Tofino_IntegerAllocatorAllocate,
  Tofino_IntegerAllocatorIsAllocated,
  Tofino_CMSQuery,
  Tofino_CMSIncrement,
  Tofino_CMSIncAndQuery,
  Tofino_LPMLookup,

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
  Controller_TableAllocate,
  Controller_TableLookup,
  Controller_TableUpdate,
  Controller_TableDelete,
  Controller_FCFSCachedTableAllocate,
  Controller_FCFSCachedTableRead,
  Controller_FCFSCachedTableWrite,
  Controller_FCFSCachedTableDelete,
  Controller_HHTableAllocate,
  Controller_HHTableRead,
  Controller_HHTableConditionalUpdate,
  Controller_HHTableUpdate,
  Controller_HHTableDelete,
  Controller_DchainAllocate,
  Controller_DchainAllocateNewIndex,
  Controller_DchainRejuvenateIndex,
  Controller_DchainIsIndexAllocated,
  Controller_DchainFreeIndex,
  Controller_IntegerAllocatorAllocate,
  Controller_IntegerAllocatorFreeIndex,
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
  Controller_VectorRegisterAllocate,
  Controller_VectorRegisterLookup,
  Controller_VectorRegisterUpdate,
  Controller_TokenBucketAllocate,
  Controller_TokenBucketIsTracing,
  Controller_TokenBucketTrace,
  Controller_TokenBucketUpdateAndCheck,
  Controller_TokenBucketExpire,
  Controller_MeterAllocate,
  Controller_MeterInsert,
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

inline std::ostream &operator<<(std::ostream &os, ModuleType type) {
  switch (type) {
  case ModuleType::InvalidModule:
    os << "InvalidModule";
    break;
  case ModuleType::Tofino_SendToController:
    os << "Tofino_SendToController";
    break;
  case ModuleType::Tofino_Ignore:
    os << "Tofino_Ignore";
    break;
  case ModuleType::Tofino_If:
    os << "Tofino_If";
    break;
  case ModuleType::Tofino_ParserExtraction:
    os << "Tofino_ParserExtraction";
    break;
  case ModuleType::Tofino_ParserCondition:
    os << "Tofino_ParserCondition";
    break;
  case ModuleType::Tofino_ParserReject:
    os << "Tofino_ParserReject";
    break;
  case ModuleType::Tofino_Then:
    os << "Tofino_Then";
    break;
  case ModuleType::Tofino_Else:
    os << "Tofino_Else";
    break;
  case ModuleType::Tofino_Forward:
    os << "Tofino_Forward";
    break;
  case ModuleType::Tofino_Drop:
    os << "Tofino_Drop";
    break;
  case ModuleType::Tofino_Broadcast:
    os << "Tofino_Broadcast";
    break;
  case ModuleType::Tofino_ModifyHeader:
    os << "Tofino_ModifyHeader";
    break;
  case ModuleType::Tofino_TableLookup:
    os << "Tofino_TableLookup";
    break;
  case ModuleType::Tofino_VectorRegisterLookup:
    os << "Tofino_VectorRegisterLookup";
    break;
  case ModuleType::Tofino_VectorRegisterUpdate:
    os << "Tofino_VectorRegisterUpdate";
    break;
  case ModuleType::Tofino_FCFSCachedTableRead:
    os << "Tofino_FCFSCachedTableRead";
    break;
  case ModuleType::Tofino_FCFSCachedTableReadWrite:
    os << "Tofino_FCFSCachedTableReadWrite";
    break;
  case ModuleType::Tofino_FCFSCachedTableWrite:
    os << "Tofino_FCFSCachedTableWrite";
    break;
  case ModuleType::Tofino_FCFSCachedTableDelete:
    os << "Tofino_FCFSCachedTableDelete";
    break;
  case ModuleType::Tofino_MeterUpdate:
    os << "Tofino_MeterUpdate";
    break;
  case ModuleType::Tofino_HHTableRead:
    os << "Tofino_HHTableRead";
    break;
  case ModuleType::Tofino_HHTableConditionalUpdate:
    os << "Tofino_HHTableConditionalUpdate";
    break;
  case ModuleType::Tofino_IntegerAllocatorRejuvenate:
    os << "Tofino_IntegerAllocatorRejuvenate";
    break;
  case ModuleType::Tofino_IntegerAllocatorAllocate:
    os << "Tofino_IntegerAllocatorAllocate";
    break;
  case ModuleType::Tofino_IntegerAllocatorIsAllocated:
    os << "Tofino_IntegerAllocatorIsAllocated";
    break;
  case ModuleType::Tofino_CMSQuery:
    os << "Tofino_CMSQuery";
    break;
  case ModuleType::Tofino_CMSIncrement:
    os << "Tofino_CMSIncrement";
    break;
  case ModuleType::Tofino_CMSIncAndQuery:
    os << "Tofino_CMSIncAndQuery";
    break;
  case ModuleType::Tofino_Recirculate:
    os << "Tofino_Recirculate";
    break;
  case ModuleType::Tofino_LPMLookup:
    os << "Tofino_LPMLookup";
    break;
  case ModuleType::Controller_Ignore:
    os << "Controller_Ignore";
    break;
  case ModuleType::Controller_ParseHeader:
    os << "Controller_ParseHeader";
    break;
  case ModuleType::Controller_ModifyHeader:
    os << "Controller_ModifyHeader";
    break;
  case ModuleType::Controller_ChecksumUpdate:
    os << "Controller_ChecksumUpdate";
    break;
  case ModuleType::Controller_If:
    os << "Controller_If";
    break;
  case ModuleType::Controller_Then:
    os << "Controller_Then";
    break;
  case ModuleType::Controller_Else:
    os << "Controller_Else";
    break;
  case ModuleType::Controller_Forward:
    os << "Controller_Forward";
    break;
  case ModuleType::Controller_Broadcast:
    os << "Controller_Broadcast";
    break;
  case ModuleType::Controller_Drop:
    os << "Controller_Drop";
    break;
  case ModuleType::Controller_TableAllocate:
    os << "Controller_TableAllocate";
    break;
  case ModuleType::Controller_TableLookup:
    os << "Controller_TableLookup";
    break;
  case ModuleType::Controller_TableUpdate:
    os << "Controller_TableUpdate";
    break;
  case ModuleType::Controller_TableDelete:
    os << "Controller_TableDelete";
    break;
  case ModuleType::Controller_FCFSCachedTableAllocate:
    os << "Controller_FCFSCachedTableAllocate";
    break;
  case ModuleType::Controller_FCFSCachedTableRead:
    os << "Controller_FCFSCachedTableRead";
    break;
  case ModuleType::Controller_FCFSCachedTableWrite:
    os << "Controller_FCFSCachedTableWrite";
    break;
  case ModuleType::Controller_FCFSCachedTableDelete:
    os << "Controller_FCFSCachedTableDelete";
    break;
  case ModuleType::Controller_HHTableAllocate:
    os << "Controller_HHTableAllocate";
    break;
  case ModuleType::Controller_HHTableRead:
    os << "Controller_HHTableRead";
    break;
  case ModuleType::Controller_HHTableConditionalUpdate:
    os << "Controller_HHTableConditionalUpdate";
    break;
  case ModuleType::Controller_HHTableUpdate:
    os << "Controller_HHTableUpdate";
    break;
  case ModuleType::Controller_HHTableDelete:
    os << "Controller_HHTableDelete";
    break;
  case ModuleType::Controller_DchainAllocate:
    os << "Controller_DchainAllocate";
    break;
  case ModuleType::Controller_DchainAllocateNewIndex:
    os << "Controller_DchainAllocateNewIndex";
    break;
  case ModuleType::Controller_DchainRejuvenateIndex:
    os << "Controller_DchainRejuvenateIndex";
    break;
  case ModuleType::Controller_DchainIsIndexAllocated:
    os << "Controller_DchainIsIndexAllocated";
    break;
  case ModuleType::Controller_DchainFreeIndex:
    os << "Controller_DchainFreeIndex";
    break;
  case ModuleType::Controller_IntegerAllocatorAllocate:
    os << "Controller_IntegerAllocatorAllocate";
    break;
  case ModuleType::Controller_IntegerAllocatorFreeIndex:
    os << "Controller_IntegerAllocatorFreeIndex";
    break;
  case ModuleType::Controller_VectorAllocate:
    os << "Controller_VectorAllocate";
    break;
  case ModuleType::Controller_VectorRead:
    os << "Controller_VectorRead";
    break;
  case ModuleType::Controller_VectorWrite:
    os << "Controller_VectorWrite";
    break;
  case ModuleType::Controller_MapAllocate:
    os << "Controller_MapAllocate";
    break;
  case ModuleType::Controller_MapGet:
    os << "Controller_MapGet";
    break;
  case ModuleType::Controller_MapPut:
    os << "Controller_MapPut";
    break;
  case ModuleType::Controller_MapErase:
    os << "Controller_MapErase";
    break;
  case ModuleType::Controller_ChtAllocate:
    os << "Controller_ChtAllocate";
    break;
  case ModuleType::Controller_ChtFindBackend:
    os << "Controller_ChtFindBackend";
    break;
  case ModuleType::Controller_HashObj:
    os << "Controller_HashObj";
    break;
  case ModuleType::Controller_VectorRegisterAllocate:
    os << "Controller_VectorRegisterAllocate";
    break;
  case ModuleType::Controller_VectorRegisterLookup:
    os << "Controller_VectorRegisterLookup";
    break;
  case ModuleType::Controller_VectorRegisterUpdate:
    os << "Controller_VectorRegisterUpdate";
    break;
  case ModuleType::Controller_TokenBucketAllocate:
    os << "Controller_TokenBucketAllocate";
    break;
  case ModuleType::Controller_TokenBucketIsTracing:
    os << "Controller_TokenBucketIsTracing";
    break;
  case ModuleType::Controller_TokenBucketTrace:
    os << "Controller_TokenBucketTrace";
    break;
  case ModuleType::Controller_TokenBucketUpdateAndCheck:
    os << "Controller_TokenBucketUpdateAndCheck";
    break;
  case ModuleType::Controller_TokenBucketExpire:
    os << "Controller_TokenBucketExpire";
    break;
  case ModuleType::Controller_MeterAllocate:
    os << "Controller_MeterAllocate";
    break;
  case ModuleType::Controller_MeterInsert:
    os << "Controller_MeterInsert";
    break;
  case ModuleType::Controller_CMSAllocate:
    os << "Controller_CMSAllocate";
    break;
  case ModuleType::Controller_CMSUpdate:
    os << "Controller_CMSUpdate";
    break;
  case ModuleType::Controller_CMSQuery:
    os << "Controller_CMSQuery";
    break;
  case ModuleType::Controller_CMSIncrement:
    os << "Controller_CMSIncrement";
    break;
  case ModuleType::Controller_CMSCountMin:
    os << "Controller_CMSCountMin";
    break;
  case ModuleType::x86_Ignore:
    os << "x86_Ignore";
    break;
  case ModuleType::x86_If:
    os << "x86_If";
    break;
  case ModuleType::x86_Then:
    os << "x86_Then";
    break;
  case ModuleType::x86_Else:
    os << "x86_Else";
    break;
  case ModuleType::x86_Forward:
    os << "x86_Forward";
    break;
  case ModuleType::x86_ParseHeader:
    os << "x86_ParseHeader";
    break;
  case ModuleType::x86_ModifyHeader:
    os << "x86_ModifyHeader";
    break;
  case ModuleType::x86_MapGet:
    os << "x86_MapGet";
    break;
  case ModuleType::x86_MapPut:
    os << "x86_MapPut";
    break;
  case ModuleType::x86_MapErase:
    os << "x86_MapErase";
    break;
  case ModuleType::x86_VectorRead:
    os << "x86_VectorRead";
    break;
  case ModuleType::x86_VectorWrite:
    os << "x86_VectorWrite";
    break;
  case ModuleType::x86_DchainAllocateNewIndex:
    os << "x86_DchainAllocateNewIndex";
    break;
  case ModuleType::x86_DchainRejuvenateIndex:
    os << "x86_DchainRejuvenateIndex";
    break;
  case ModuleType::x86_DchainIsIndexAllocated:
    os << "x86_DchainIsIndexAllocated";
    break;
  case ModuleType::x86_DchainFreeIndex:
    os << "x86_DchainFreeIndex";
    break;
  case ModuleType::x86_CMSIncrement:
    os << "x86_CMSIncrement";
    break;
  case ModuleType::x86_CMSCountMin:
    os << "x86_CMSCountMin";
    break;
  case ModuleType::x86_CMSPeriodicCleanup:
    os << "x86_CMSPeriodicCleanup";
    break;
  case ModuleType::x86_Drop:
    os << "x86_Drop";
    break;
  case ModuleType::x86_Broadcast:
    os << "x86_Broadcast";
    break;
  case ModuleType::x86_ExpireItemsSingleMap:
    os << "x86_ExpireItemsSingleMap";
    break;
  case ModuleType::x86_ExpireItemsSingleMapIteratively:
    os << "x86_ExpireItemsSingleMapIteratively";
    break;
  case ModuleType::x86_ChecksumUpdate:
    os << "x86_ChecksumUpdate";
    break;
  case ModuleType::x86_ChtFindBackend:
    os << "x86_ChtFindBackend";
    break;
  case ModuleType::x86_HashObj:
    os << "x86_HashObj";
    break;
  case ModuleType::x86_TokenBucketIsTracing:
    os << "x86_TokenBucketIsTracing";
    break;
  case ModuleType::x86_TokenBucketTrace:
    os << "x86_TokenBucketTrace";
    break;
  case ModuleType::x86_TokenBucketUpdateAndCheck:
    os << "x86_TokenBucketUpdateAndCheck";
    break;
  case ModuleType::x86_TokenBucketExpire:
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
  const LibBDD::Node *node;

  Module(ModuleType _type, TargetType _target, const std::string &_name, const LibBDD::Node *_node)
      : type(_type), target(_target), next_target(_target), name(_name), node(_node) {}

  Module(ModuleType _type, TargetType _target, TargetType _next_target, const std::string &_name, const LibBDD::Node *_node)
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
  const LibBDD::Node *get_node() const { return node; }

  void set_node(const LibBDD::Node *new_node) { node = new_node; }

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const = 0;
  virtual Module *clone() const                                                                  = 0;
};

} // namespace LibSynapse