#pragma once

#include <LibSynapse/Context.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Decision.h>

#include <LibBDD/BDD.h>

#include <LibCore/SymbolManager.h>

#include <optional>

namespace LibSynapse {

using LibCore::Symbols;

class ModuleFactory {
protected:
  ModuleType type;
  TargetType target;
  std::string name;

public:
  ModuleFactory(ModuleType _type, TargetType _target, const std::string &_name) : type(_type), target(_target), name(_name) {}

  virtual ~ModuleFactory() {}

  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const = 0;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const                     = 0;

  std::vector<impl_t> implement(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager, bool reorder_bdd) const;

  ModuleType get_type() const { return type; }
  TargetType get_target() const { return target; }
  const std::string &get_name() const { return name; }

protected:
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const = 0;

  decision_t decide(const EP *ep, const BDDNode *node, std::unordered_map<std::string, i32> params = {}) const;
  impl_t implement(const EP *ep, const BDDNode *node, std::unique_ptr<EP> result, std::unordered_map<std::string, i32> params = {}) const;
  void speculate_sending_to_controller(const EP *ep, const BDDNode *node, Context &ctx, hit_rate_t relative_hr_sent_to_controller) const;
};

} // namespace LibSynapse