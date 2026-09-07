#pragma once

#include <LibSynapse/Context.h>
#include <LibSynapse/EPNode.h>
#include <LibSynapse/Modules/Module.h>

#include <LibBDD/BDD.h>

#include <optional>

namespace LibSynapse {

class EP;

using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;

struct decision_t {
  const EP *ep;
  bdd_node_id_t node;
  ModuleType module;
  std::unordered_map<std::string, i32> params;

  decision_t() : ep(nullptr), node(0), module(ModuleType::InvalidModule) {}

  decision_t(const EP *_ep, bdd_node_id_t _node, ModuleType _module) : ep(_ep), node(_node), module(_module) {}

  decision_t(const EP *_ep, bdd_node_id_t _node, ModuleType _module, const std::unordered_map<std::string, i32> &_params)
      : ep(_ep), node(_node), module(_module), params(_params) {}

  decision_t(const decision_t &other) : ep(other.ep), node(other.node), module(other.module), params(other.params) {}

  decision_t(decision_t &&other) : ep(std::move(other.ep)), node(other.node), module(other.module), params(std::move(other.params)) {}

  decision_t &operator=(const decision_t &other) {
    ep     = other.ep;
    node   = other.node;
    module = other.module;
    params = other.params;
    return *this;
  }
};

struct spec_impl_t {
  decision_t decision;
  Context ctx;
  std::optional<TargetType> next_target;
  bool recirculated;
  bdd_node_ids_t skip;
  // Symbols this speculation computes in the data plane, with the data structure (id)
  // holding each: what speculations of their consumers depend on.
  std::unordered_map<std::string, std::string> produced;

  spec_impl_t(const decision_t &_decision, const Context &_ctx) : decision(_decision), ctx(_ctx), recirculated(false) {}
};

struct spec_impl_lite_t {
  decision_t decision;
  std::optional<TargetType> next_target;
  bool recirculated;
  bdd_node_ids_t skip;

  spec_impl_lite_t(const spec_impl_t &_spec_impl)
      : decision(_spec_impl.decision), next_target(_spec_impl.next_target), recirculated(_spec_impl.recirculated), skip(_spec_impl.skip) {}
};

struct speculations_t {
  std::vector<spec_impl_lite_t> speculations_per_node;
  Context ctx;
  // Data-plane producers of symbols speculated so far in the current pass (see
  // spec_impl_t::produced); a speculated recirculation starts a new pass and empties it.
  std::unordered_map<std::string, std::string> producers;
  bool recirculated_since_leaf = false;

  speculations_t copy_and_append(const spec_impl_t &spec_impl) const {
    speculations_t new_speculations = *this;
    new_speculations.append(spec_impl);
    return new_speculations;
  }

  void append(const spec_impl_t &spec_impl) {
    speculations_per_node.emplace_back(spec_impl);
    ctx = spec_impl.ctx;
    if (spec_impl.recirculated) {
      producers.clear();
      recirculated_since_leaf = true;
    }
    producers.insert(spec_impl.produced.begin(), spec_impl.produced.end());
  }
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

} // namespace LibSynapse