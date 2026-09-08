#pragma once

#include <array>
#include <memory>

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
  // A stateless compute step, and the compute actions it placed its ops in (in order): the
  // steps that follow may share them.
  bool compute_step = false;
  std::vector<std::string> compute_actions;

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
  // What the pass decided for each node so far (a consumed node maps to the step that took
  // it), so a step can look at its own BDD ancestry, whatever order the pass visits nodes in:
  // the run of compute steps it may share actions with, the producers of its operands, and
  // the recirculations (pass boundaries) on its path.
  struct node_info_t {
    ModuleType module;
    bool compute_step;
    bool recirculated;
    std::vector<std::string> actions;                       // Compute actions the step placed ops in.
    std::unordered_map<std::string, std::string> produced;  // Symbol -> data structure computing it.
  };
  // Persistent, so that copying a pass's state (done per lookahead) is cheap: fixed-size chunks
  // of shared entries, and a write copies only the chunk it touches.
  class node_infos_t {
  private:
    static constexpr size_t CHUNK = 64;
    using chunk_t                 = std::array<std::shared_ptr<const node_info_t>, CHUNK>;
    std::vector<std::shared_ptr<chunk_t>> chunks;

  public:
    const node_info_t *find(bdd_node_id_t id) const {
      const size_t c = id / CHUNK;
      if (c >= chunks.size() || !chunks[c]) {
        return nullptr;
      }
      return (*chunks[c])[id % CHUNK].get();
    }

    void set(bdd_node_id_t id, std::shared_ptr<const node_info_t> info) {
      const size_t c = id / CHUNK;
      if (c >= chunks.size()) {
        chunks.resize(c + 1);
      }
      if (!chunks[c]) {
        chunks[c] = std::make_shared<chunk_t>();
      } else if (chunks[c].use_count() > 1) {
        chunks[c] = std::make_shared<chunk_t>(*chunks[c]);
      }
      (*chunks[c])[id % CHUNK] = std::move(info);
    }
  };
  node_infos_t by_node;

  const node_info_t *find_node_info(bdd_node_id_t id) const { return by_node.find(id); }
  void note(bdd_node_id_t id, node_info_t info) { by_node.set(id, std::make_shared<const node_info_t>(std::move(info))); }

  speculations_t copy_and_append(const spec_impl_t &spec_impl) const {
    speculations_t new_speculations = *this;
    new_speculations.append(spec_impl);
    return new_speculations;
  }

  void append(const spec_impl_t &spec_impl) {
    speculations_per_node.emplace_back(spec_impl);
    ctx = spec_impl.ctx;
    const auto info = std::make_shared<const node_info_t>(
        node_info_t{spec_impl.decision.module, spec_impl.compute_step, spec_impl.recirculated, spec_impl.compute_actions, spec_impl.produced});
    by_node.set(spec_impl.decision.node, info);
    for (bdd_node_id_t consumed : spec_impl.skip) {
      by_node.set(consumed, info);
    }
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