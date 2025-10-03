#pragma once

#include "LibSynapse/Target.h"
#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/x86/DataStructures/DataStructures.h>
#include <LibSynapse/Modules/x86/x86Context.h>

namespace LibSynapse {
namespace x86 {
class x86Module : public Module {
public:
  x86Module(ModuleType _type, const std::string &_name, const BDDNode *_node)
      : Module(_type, TargetType(TargetArchitecture::x86, _type.instance_id), _name, _node) {}

  x86Module(ModuleType _type, TargetType _next_type, const std::string &_name, const BDDNode *_node)
      : Module(_type, TargetType(TargetArchitecture::x86, _type.instance_id), _next_type, _name, _node) {}
};

struct map_data_t {
  addr_t obj;
  u32 capacity;
  klee::ref<klee::Expr> original_key;
  klee::ref<klee::Expr> value;
  std::optional<symbol_t> hit;
};

struct vector_data_t {
  addr_t obj;
  u32 capacity;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
};

struct Map_Data {
  map_data_t *m;
  vector_data_t *v;
};

class x86ModuleFactory : public ModuleFactory {
protected:
  ModuleType type;
  TargetType target;

public:
  x86ModuleFactory(ModuleType _type, const std::string &_name)
      : ModuleFactory(_type, TargetType(TargetArchitecture::x86, _type.instance_id), _name) {}

  static Symbols get_relevant_dataplane_state(const EP *ep, const BDDNode *node, TargetType type);
};

} // namespace x86
} // namespace LibSynapse
