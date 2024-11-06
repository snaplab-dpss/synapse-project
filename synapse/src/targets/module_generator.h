#pragma once

#include <optional>

#include "target.h"
#include "module.h"
#include "../bdd/bdd.h"
#include "../execution_plan/execution_plan.h"
#include "../util.h"

class Context;

struct decision_t {
  const EP *ep;
  node_id_t node;
  ModuleType module;
  std::unordered_map<std::string, i32> params;

  decision_t() : ep(nullptr), node(0), module(ModuleType::INVALID_MODULE) {}

  decision_t(const EP *_ep, node_id_t _node, ModuleType _module)
      : ep(_ep), node(_node), module(_module) {}

  decision_t(const EP *_ep, node_id_t _node, ModuleType _module,
             const std::unordered_map<std::string, i32> &_params)
      : ep(_ep), node(_node), module(_module), params(_params) {}

  decision_t(const decision_t &other)
      : ep(other.ep), node(other.node), module(other.module),
        params(other.params) {}

  decision_t &operator=(const decision_t &other) {
    ep = other.ep;
    node = other.node;
    module = other.module;
    params = other.params;
    return *this;
  }
};

struct spec_impl_t {
  decision_t decision;
  Context ctx;
  std::optional<TargetType> next_target;
  nodes_t skip;

  spec_impl_t(const decision_t &_decision, const Context &_ctx)
      : decision(_decision), ctx(_ctx) {}
};

struct impl_t {
  decision_t decision;
  EP *result;
  bool bdd_reordered;

  impl_t(EP *_result) : result(_result) {}

  impl_t(const decision_t &_decision, EP *_result, bool _bdd_reordered)
      : decision(_decision), result(_result), bdd_reordered(_bdd_reordered) {}

  impl_t(const impl_t &other)
      : decision(other.decision), result(other.result),
        bdd_reordered(other.bdd_reordered) {}

  impl_t &operator=(const impl_t &other) {
    decision = other.decision;
    result = other.result;
    bdd_reordered = other.bdd_reordered;
    return *this;
  }
};

class ModuleGenerator {
protected:
  ModuleType type;
  TargetType target;
  std::string name;

public:
  ModuleGenerator(ModuleType _type, TargetType _target,
                  const std::string &_name)
      : type(_type), target(_target), name(_name) {}

  virtual ~ModuleGenerator() {}

  std::vector<impl_t> generate(const EP *ep, const Node *node,
                               bool reorder_bdd) const;

  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const = 0;

  ModuleType get_type() const { return type; }
  TargetType get_target() const { return target; }
  const std::string &get_name() const { return name; }

protected:
  decision_t decide(const EP *ep, const Node *node,
                    std::unordered_map<std::string, i32> params = {}) const;

  impl_t implement(const EP *ep, const Node *node, EP *result,
                   std::unordered_map<std::string, i32> params = {}) const;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const = 0;
};