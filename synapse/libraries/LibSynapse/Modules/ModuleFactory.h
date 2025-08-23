#pragma once

#include <LibSynapse/Context.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/EPNode.h>
#include <LibSynapse/Modules/Module.h>
#include <LibBDD/BDD.h>
#include <LibCore/SymbolManager.h>

#include <optional>

namespace LibSynapse {

using LibCore::symbol_t;
using LibCore::SymbolManager;
using LibCore::Symbols;

struct decision_t {
  const EP *ep;
  bdd_node_id_t node;
  ModuleType module;
  std::unordered_map<std::string, i32> params;

  decision_t() : ep(nullptr), node(0), module(ModuleCategory::InvalidModule, "NO DEVICE") {}

  decision_t(const EP *_ep, bdd_node_id_t _node, ModuleType _module) : ep(_ep), node(_node), module(_module) {}

  decision_t(const EP *_ep, bdd_node_id_t _node, ModuleType _module, const std::unordered_map<std::string, i32> &_params)
      : ep(_ep), node(_node), module(_module), params(_params) {}

  decision_t(const decision_t &other) : ep(other.ep), node(other.node), module(other.module), params(other.params) {}

  decision_t(decision_t &&other) : ep(std::move(other.ep)), node(other.node), module(other.module), params(std::move(other.params)) {}

  decision_t &operator=(const decision_t &other) {
    ep     = other.ep;
    node   = other.node;
    module = ModuleType(other.module.type, other.module.instance_id);
    params = other.params;
    return *this;
  }
};

struct spec_impl_t {
  decision_t decision;
  Context ctx;
  std::optional<TargetType> next_target;
  bdd_node_ids_t skip;

  spec_impl_t(const decision_t &_decision, const Context &_ctx) : decision(_decision), ctx(_ctx) {}
};

struct impl_t {
  decision_t decision;
  std::unique_ptr<EP> result;
  bool bdd_reordered;

  impl_t(std::unique_ptr<EP> _result) : result(std::move(_result)) {}

  impl_t(const decision_t &_decision, std::unique_ptr<EP> _result, bool _bdd_reordered)
      : decision(_decision), result(std::move(_result)), bdd_reordered(_bdd_reordered) {}

  impl_t(const impl_t &other)            = delete;
  impl_t &operator=(const impl_t &other) = delete;

  impl_t(impl_t &&other) : decision(std::move(other.decision)), result(std::move(other.result)), bdd_reordered(other.bdd_reordered) {}

  impl_t &operator=(impl_t &&other) {
    if (this != &other) {
      decision      = std::move(other.decision);
      result        = std::move(other.result);
      bdd_reordered = other.bdd_reordered;
    }
    return *this;
  }
};

class ModuleFactory {
protected:
  ModuleType type;
  TargetType target;
  std::string name;

public:
  ModuleFactory(ModuleType _type, TargetType _target, const std::string &_name) : type(_type), target(_target), name(_name) {}

  virtual ~ModuleFactory() {}

  std::vector<impl_t> implement(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager, bool reorder_bdd) const;
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const = 0;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const     = 0;

  ModuleType get_type() const { return type; }
  TargetType get_target() const { return target; }
  const std::string &get_name() const { return name; }

protected:
  decision_t decide(const EP *ep, const BDDNode *node, std::unordered_map<std::string, i32> params = {}) const;
  impl_t implement(const EP *ep, const BDDNode *node, std::unique_ptr<EP> result, std::unordered_map<std::string, i32> params = {}) const;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const = 0;
};

} // namespace LibSynapse
