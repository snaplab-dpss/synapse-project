#pragma once

#include <string>

#include "../bdd/bdd.h"

#include "../visualizers/ep_visualizer.h"
#include "../visualizers/profiler_visualizer.h"
#include "../execution_plan/visitor.h"
#include "../util.h"
#include "../log.h"

namespace synapse {
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
  Tofino_FCFSCachedTableReadOrWrite,
  Tofino_FCFSCachedTableWrite,
  Tofino_FCFSCachedTableDelete,

  Tofino_MapRegisterRead,
  Tofino_MapRegisterReadOrWrite,
  Tofino_MapRegisterWrite,
  Tofino_MapRegisterDelete,

  Tofino_MeterUpdate,

  Tofino_HHTableRead,
  Tofino_HHTableConditionalUpdate,

  Tofino_IntegerAllocatorRejuvenate,
  Tofino_IntegerAllocatorAllocate,
  Tofino_IntegerAllocatorIsAllocated,

  Tofino_CMSQuery,
  Tofino_CMSIncrement,
  Tofino_CMSIncAndQuery,

  // ========================================
  // Tofino CPU
  // ========================================

  TofinoCPU_Ignore,
  TofinoCPU_ParseHeader,
  TofinoCPU_ModifyHeader,
  TofinoCPU_ChecksumUpdate,

  TofinoCPU_If,
  TofinoCPU_Then,
  TofinoCPU_Else,

  TofinoCPU_Forward,
  TofinoCPU_Broadcast,
  TofinoCPU_Drop,

  TofinoCPU_TableLookup,
  TofinoCPU_TableUpdate,
  TofinoCPU_TableDelete,

  TofinoCPU_FCFSCachedTableRead,
  TofinoCPU_FCFSCachedTableWrite,
  TofinoCPU_FCFSCachedTableDelete,

  TofinoCPU_MapRegisterRead,
  TofinoCPU_MapRegisterWrite,
  TofinoCPU_MapRegisterDelete,

  TofinoCPU_HHTableRead,
  TofinoCPU_HHTableConditionalUpdate,
  TofinoCPU_HHTableUpdate,
  TofinoCPU_HHTableDelete,

  TofinoCPU_DchainAllocateNewIndex,
  TofinoCPU_DchainRejuvenateIndex,
  TofinoCPU_DchainIsIndexAllocated,
  TofinoCPU_DchainFreeIndex,

  TofinoCPU_IntegerAllocatorFreeIndex,

  TofinoCPU_VectorRead,
  TofinoCPU_VectorWrite,

  TofinoCPU_MapGet,
  TofinoCPU_MapPut,
  TofinoCPU_MapErase,

  TofinoCPU_ChtFindBackend,

  TofinoCPU_HashObj,

  TofinoCPU_VectorRegisterLookup,
  TofinoCPU_VectorRegisterUpdate,

  TofinoCPU_TBIsTracing,
  TofinoCPU_TBTrace,
  TofinoCPU_TBUpdateAndCheck,
  TofinoCPU_TBExpire,

  TofinoCPU_MeterInsert,

  TofinoCPU_CMSUpdate,
  TofinoCPU_CMSQuery,
  TofinoCPU_CMSIncrement,
  TofinoCPU_CMSCountMin,

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

  x86_TBIsTracing,
  x86_TBTrace,
  x86_TBUpdateAndCheck,
  x86_TBExpire,
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
  case ModuleType::Tofino_FCFSCachedTableReadOrWrite:
    os << "Tofino_FCFSCachedTableReadOrWrite";
    break;
  case ModuleType::Tofino_FCFSCachedTableWrite:
    os << "Tofino_FCFSCachedTableWrite";
    break;
  case ModuleType::Tofino_FCFSCachedTableDelete:
    os << "Tofino_FCFSCachedTableDelete";
    break;
  case ModuleType::Tofino_MapRegisterRead:
    os << "Tofino_MapRegisterRead";
    break;
  case ModuleType::Tofino_MapRegisterReadOrWrite:
    os << "Tofino_MapRegisterReadOrWrite";
    break;
  case ModuleType::Tofino_MapRegisterWrite:
    os << "Tofino_MapRegisterWrite";
    break;
  case ModuleType::Tofino_MapRegisterDelete:
    os << "Tofino_MapRegisterDelete";
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
  case ModuleType::TofinoCPU_Ignore:
    os << "TofinoCPU_Ignore";
    break;
  case ModuleType::TofinoCPU_ParseHeader:
    os << "TofinoCPU_ParseHeader";
    break;
  case ModuleType::TofinoCPU_ModifyHeader:
    os << "TofinoCPU_ModifyHeader";
    break;
  case ModuleType::TofinoCPU_ChecksumUpdate:
    os << "TofinoCPU_ChecksumUpdate";
    break;
  case ModuleType::TofinoCPU_If:
    os << "TofinoCPU_If";
    break;
  case ModuleType::TofinoCPU_Then:
    os << "TofinoCPU_Then";
    break;
  case ModuleType::TofinoCPU_Else:
    os << "TofinoCPU_Else";
    break;
  case ModuleType::TofinoCPU_Forward:
    os << "TofinoCPU_Forward";
    break;
  case ModuleType::TofinoCPU_Broadcast:
    os << "TofinoCPU_Broadcast";
    break;
  case ModuleType::TofinoCPU_Drop:
    os << "TofinoCPU_Drop";
    break;
  case ModuleType::TofinoCPU_TableLookup:
    os << "TofinoCPU_TableLookup";
    break;
  case ModuleType::TofinoCPU_TableUpdate:
    os << "TofinoCPU_TableUpdate";
    break;
  case ModuleType::TofinoCPU_TableDelete:
    os << "TofinoCPU_TableDelete";
    break;
  case ModuleType::TofinoCPU_FCFSCachedTableRead:
    os << "TofinoCPU_FCFSCachedTableRead";
    break;
  case ModuleType::TofinoCPU_FCFSCachedTableWrite:
    os << "TofinoCPU_FCFSCachedTableWrite";
    break;
  case ModuleType::TofinoCPU_FCFSCachedTableDelete:
    os << "TofinoCPU_FCFSCachedTableDelete";
    break;
  case ModuleType::TofinoCPU_MapRegisterRead:
    os << "TofinoCPU_MapRegisterRead";
    break;
  case ModuleType::TofinoCPU_MapRegisterWrite:
    os << "TofinoCPU_MapRegisterWrite";
    break;
  case ModuleType::TofinoCPU_MapRegisterDelete:
    os << "TofinoCPU_MapRegisterDelete";
    break;
  case ModuleType::TofinoCPU_HHTableRead:
    os << "TofinoCPU_HHTableRead";
    break;
  case ModuleType::TofinoCPU_HHTableConditionalUpdate:
    os << "TofinoCPU_HHTableConditionalUpdate";
    break;
  case ModuleType::TofinoCPU_HHTableUpdate:
    os << "TofinoCPU_HHTableUpdate";
    break;
  case ModuleType::TofinoCPU_HHTableDelete:
    os << "TofinoCPU_HHTableDelete";
    break;
  case ModuleType::TofinoCPU_DchainAllocateNewIndex:
    os << "TofinoCPU_DchainAllocateNewIndex";
    break;
  case ModuleType::TofinoCPU_DchainRejuvenateIndex:
    os << "TofinoCPU_DchainRejuvenateIndex";
    break;
  case ModuleType::TofinoCPU_DchainIsIndexAllocated:
    os << "TofinoCPU_DchainIsIndexAllocated";
    break;
  case ModuleType::TofinoCPU_DchainFreeIndex:
    os << "TofinoCPU_DchainFreeIndex";
    break;
  case ModuleType::TofinoCPU_IntegerAllocatorFreeIndex:
    os << "TofinoCPU_IntegerAllocatorFreeIndex";
    break;
  case ModuleType::TofinoCPU_VectorRead:
    os << "TofinoCPU_VectorRead";
    break;
  case ModuleType::TofinoCPU_VectorWrite:
    os << "TofinoCPU_VectorWrite";
    break;
  case ModuleType::TofinoCPU_MapGet:
    os << "TofinoCPU_MapGet";
    break;
  case ModuleType::TofinoCPU_MapPut:
    os << "TofinoCPU_MapPut";
    break;
  case ModuleType::TofinoCPU_MapErase:
    os << "TofinoCPU_MapErase";
    break;
  case ModuleType::TofinoCPU_ChtFindBackend:
    os << "TofinoCPU_ChtFindBackend";
    break;
  case ModuleType::TofinoCPU_HashObj:
    os << "TofinoCPU_HashObj";
    break;
  case ModuleType::TofinoCPU_VectorRegisterLookup:
    os << "TofinoCPU_VectorRegisterLookup";
    break;
  case ModuleType::TofinoCPU_VectorRegisterUpdate:
    os << "TofinoCPU_VectorRegisterUpdate";
    break;
  case ModuleType::TofinoCPU_TBIsTracing:
    os << "TofinoCPU_TBIsTracing";
    break;
  case ModuleType::TofinoCPU_TBTrace:
    os << "TofinoCPU_TBTrace";
    break;
  case ModuleType::TofinoCPU_TBUpdateAndCheck:
    os << "TofinoCPU_TBUpdateAndCheck";
    break;
  case ModuleType::TofinoCPU_TBExpire:
    os << "TofinoCPU_TBExpire";
    break;
  case ModuleType::TofinoCPU_MeterInsert:
    os << "TofinoCPU_MeterInsert";
    break;
  case ModuleType::TofinoCPU_CMSUpdate:
    os << "TofinoCPU_CMSUpdate";
    break;
  case ModuleType::TofinoCPU_CMSQuery:
    os << "TofinoCPU_CMSQuery";
    break;
  case ModuleType::TofinoCPU_CMSIncrement:
    os << "TofinoCPU_CMSIncrement";
    break;
  case ModuleType::TofinoCPU_CMSCountMin:
    os << "TofinoCPU_CMSCountMin";
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
  case ModuleType::x86_TBIsTracing:
    os << "x86_TBIsTracing";
    break;
  case ModuleType::x86_TBTrace:
    os << "x86_TBTrace";
    break;
  case ModuleType::x86_TBUpdateAndCheck:
    os << "x86_TBUpdateAndCheck";
    break;
  case ModuleType::x86_TBExpire:
    os << "x86_TBExpire";
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
  const Node *node;

  Module(ModuleType _type, TargetType _target, const std::string &_name,
         const Node *_node)
      : type(_type), target(_target), next_target(_target), name(_name), node(_node) {}

  Module(ModuleType _type, TargetType _target, TargetType _next_target,
         const std::string &_name, const Node *_node)
      : type(_type), target(_target), next_target(_next_target), name(_name),
        node(_node) {}

public:
  Module(const Module &other) = delete;
  Module(Module &&other) = delete;

  Module &operator=(const Module &other) = delete;

  virtual ~Module() {}

  ModuleType get_type() const { return type; }
  const std::string &get_name() const { return name; }
  TargetType get_target() const { return target; }
  TargetType get_next_target() const { return next_target; }
  const Node *get_node() const { return node; }

  void set_node(const Node *new_node) { node = new_node; }

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const = 0;
  virtual Module *clone() const = 0;
};
} // namespace synapse