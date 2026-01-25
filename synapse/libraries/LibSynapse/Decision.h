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

  spec_impl_t(const decision_t &_decision, const Context &_ctx) : decision(_decision), ctx(_ctx), recirculated(false) {}
};

struct speculations_t {
  std::vector<spec_impl_t> speculations_per_node;
  Context ctx; // Context after applying all speculations.

  speculations_t append(const spec_impl_t &speculation) const {
    speculations_t new_speculations = {
        .speculations_per_node = speculations_per_node,
        .ctx                   = speculation.ctx,
    };
    new_speculations.speculations_per_node.push_back(speculation);
    return new_speculations;
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