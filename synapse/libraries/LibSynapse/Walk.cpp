#include <LibSynapse/Walk.h>
#include <LibSynapse/Decision.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Heuristics/Heuristic.h>
#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <deque>
#include <map>
#include <sstream>

namespace LibSynapse {
namespace Walk {

namespace {

using LibBDD::bdd_node_id_t;
using LibBDD::BDDNode;

// One child offered at the current step.
struct offer_t {
  ep_id_t ep;
  std::string factory;
  std::string module;
  bdd_node_id_t node;                // the BDD node this child processed
  std::optional<bdd_node_id_t> next; // what the child's plan does next; none when it is finished
  bool reordered;
  std::string score;
};

// One recorded pick.
struct rule_t {
  std::string module;
  std::optional<bdd_node_id_t> next;
};

struct reorder_note_t {
  bdd_node_id_t anchor;
  bool direction;
  std::vector<bdd_node_id_t> candidates;
};

config_t config;
bool on = false;

// What the file says, by the BDD node the pick was made at, in the order the picks were made. A
// node can be visited more than once -- a module that does not consume its node, such as
// Recirculate, leaves it to be visited again -- and the picks at those visits may differ, so they
// are consumed in order: at each visit only the node's oldest unconsumed rule is tried.
std::map<bdd_node_id_t, std::deque<rule_t>> rules;
size_t rules_read = 0;

// The step in progress.
const EP *step_ep        = nullptr;
const BDDNode *step_node = nullptr;
std::vector<offer_t> offers;
std::vector<std::pair<std::string, std::string>> declines;
std::vector<reorder_note_t> reorder_notes;
u64 step_count = 0;

std::string next_to_string(const std::optional<bdd_node_id_t> &next) { return next ? std::to_string(*next) : "end"; }

std::optional<bdd_node_id_t> next_from_string(const std::string &s) {
  if (s == "end") {
    return std::nullopt;
  }
  return static_cast<bdd_node_id_t>(std::stoull(s));
}

void load_rules() {
  std::ifstream in(config.file);
  if (!in) {
    return;
  }
  std::string line;
  while (std::getline(in, line)) {
    const size_t hash = line.find('#');
    if (hash != std::string::npos) {
      line = line.substr(0, hash);
    }
    std::istringstream ss(line);
    std::string node_kv, module_kv, next_kv;
    if (!(ss >> node_kv >> module_kv >> next_kv)) {
      continue; // blank or comment
    }
    auto value = [](const std::string &kv, const char *key) -> std::optional<std::string> {
      const std::string prefix = std::string(key) + "=";
      if (kv.rfind(prefix, 0) != 0) {
        return std::nullopt;
      }
      return kv.substr(prefix.size());
    };
    const auto node = value(node_kv, "node");
    const auto mod  = value(module_kv, "module");
    const auto nxt  = value(next_kv, "next");
    if (!node || !mod || !nxt) {
      std::cerr << "[walk] ignoring malformed line: " << line << "\n";
      continue;
    }
    rules[static_cast<bdd_node_id_t>(std::stoull(*node))].push_back(rule_t{*mod, next_from_string(*nxt)});
    rules_read++;
  }
  std::cerr << "[walk] " << rules_read << " decision(s) read from " << config.file << "\n";
}

void record(const offer_t &pick) {
  std::ofstream out(config.file, std::ios::app);
  out << "node=" << pick.node << " module=" << pick.module << " next=" << next_to_string(pick.next) << "  # step " << step_count << ", ep " << pick.ep
      << "\n";
}

void show() {
  std::cerr << "\n[walk] step " << step_count << ": plan " << step_ep->get_id() << " at BDD node " << step_node->get_id() << ":"
            << step_node->dump(true) << "\n";
  for (size_t i = 0; i < offers.size(); i++) {
    const offer_t &o = offers[i];
    std::cerr << "  [" << i << "] " << o.module << " (" << o.factory << ")  node=" << o.node << "  next=" << next_to_string(o.next);
    if (o.reordered) {
      std::cerr << "  [R: " << next_to_string(o.next) << " moved up]";
    }
    std::cerr << "  score=" << o.score << "  ep=" << o.ep << "\n";
  }
  if (!declines.empty()) {
    std::cerr << "  declined:\n";
    for (const auto &[factory, reason] : declines) {
      std::cerr << "    " << factory << ": " << reason << "\n";
    }
  }
  for (const reorder_note_t &note : reorder_notes) {
    std::cerr << "  reorder candidates at anchor " << note.anchor << (note.direction ? "/true" : "/false") << ":";
    if (note.candidates.empty()) {
      std::cerr << " none";
    }
    for (bdd_node_id_t c : note.candidates) {
      std::cerr << " " << c;
    }
    std::cerr << "\n";
  }
}

std::optional<size_t> replay_pick() {
  auto it = rules.find(step_node->get_id());
  if (it == rules.end() || it->second.empty()) {
    return std::nullopt;
  }
  const rule_t &rule = it->second.front();
  for (size_t i = 0; i < offers.size(); i++) {
    if (rule.module == offers[i].module && rule.next == offers[i].next) {
      it->second.pop_front();
      return i;
    }
  }
  std::cerr << "[walk] the recorded pick for this visit, " << rule.module << " next=" << next_to_string(rule.next)
            << ", is not among the offered children\n";
  return std::nullopt;
}

std::optional<size_t> prompt() {
  while (true) {
    // On its own line, ending with a newline, so a driver reading line by line sees it.
    std::cerr << "[walk] pick [0-" << offers.size() - 1 << "], or q to stop:\n" << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
      std::cerr << "\n[walk] no input; stopping\n";
      return std::nullopt;
    }
    if (line == "q" || line == "Q") {
      return std::nullopt;
    }
    try {
      const size_t i = std::stoul(line);
      if (i < offers.size()) {
        return i;
      }
    } catch (...) {
    }
  }
}

} // namespace

void configure(const config_t &cfg) {
  config = cfg;
  on     = !config.file.empty();
  if (on) {
    load_rules();
  }
}

bool enabled() { return on; }

void begin_step(const EP *ep, const BDDNode *node) {
  if (!on) {
    return;
  }
  step_ep   = ep;
  step_node = node;
  offers.clear();
  declines.clear();
  reorder_notes.clear();
  step_count++;
}

void offered(const ModuleFactory *factory, const std::vector<impl_t> &impls, const Heuristic *heuristic) {
  if (!on) {
    return;
  }
  for (const impl_t &impl : impls) {
    const EP *ep = impl.result.get();
    std::stringstream score;
    score << heuristic->get_score(ep);
    const BDDNode *next = ep->get_next_node();
    offers.push_back(offer_t{
        ep->get_id(),
        factory->get_name(),
        to_string(impl.decision.module),
        impl.decision.node,
        next ? std::optional<bdd_node_id_t>(next->get_id()) : std::nullopt,
        impl.bdd_reordered,
        score.str(),
    });
  }
}

void declined(const ModuleFactory *factory, const std::string &reason) {
  if (!on) {
    return;
  }
  declines.emplace_back(factory->get_name(), reason);
}

void reorder_candidates(bdd_node_id_t anchor, bool direction, const std::vector<bdd_node_id_t> &candidates) {
  if (!on) {
    return;
  }
  for (const reorder_note_t &note : reorder_notes) {
    if (note.anchor == anchor && note.direction == direction) {
      return; // reordering runs once per child; the enumeration is the same each time
    }
  }
  reorder_notes.push_back({anchor, direction, candidates});
}

choice_t choose() {
  if (!on || offers.size() < 2) {
    return {choice_t::Kind::None, 0};
  }

  show();

  std::optional<size_t> pick = replay_pick();
  if (pick) {
    std::cerr << "[walk] replayed: [" << *pick << "] " << offers[*pick].module << " next=" << next_to_string(offers[*pick].next) << "\n";
    return {choice_t::Kind::Force, offers[*pick].ep};
  }

  if (!config.interactive) {
    std::cerr << "[walk] no recorded decision matches any offered child at BDD node " << step_node->get_id() << "; stopping\n";
    return {choice_t::Kind::Stop, 0};
  }

  pick = prompt();
  if (!pick) {
    return {choice_t::Kind::Stop, 0};
  }
  record(offers[*pick]);
  return {choice_t::Kind::Force, offers[*pick].ep};
}

} // namespace Walk
} // namespace LibSynapse
