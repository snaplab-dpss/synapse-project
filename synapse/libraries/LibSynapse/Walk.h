#pragma once

#include <LibBDD/Nodes/Node.h>
#include <LibCore/Types.h>
#include <LibSynapse/ExecutionPlan.h>

#include <optional>
#include <string>
#include <vector>

namespace LibSynapse {

class EP;
class Heuristic;
class ModuleFactory;
struct impl_t;

// A walk through the search under a human's, or a file's, control.
//
// The search is best-first: every step pops the best unfinished plan, offers every module that can
// implement its next node, and pushes the children. The walk intervenes at each step that offers
// more than one child: it shows them, takes a pick, and forces it by adding its id to the
// forced-decision set, which every heuristic scores first. The pick's subtree is then explored
// before its siblings under any heuristic, the siblings stay in the open set below it, and a pick
// that dies backtracks to the most recent alternative (run with --backtrack to stop there).
//
// Picks are recorded by the BDD node processed, the module, and the node that comes next in the
// chosen plan -- never by plan id, which depends on generation order -- so a recorded walk replays
// across changes to synapse, and stops with the offered set the first time it cannot be followed.
//
// This is a debugging instrument. It answers "is this plan reachable at all", not "would the
// search choose it".
namespace Walk {

struct config_t {
  std::string file;      // decisions are read from here and appended to it; empty means the walk is off
  bool interactive;      // prompt at steps the file does not cover; otherwise stop there
};

void configure(const config_t &cfg);
bool enabled();

// Called by the search loop.
void begin_step(const EP *ep, const LibBDD::BDDNode *node);
void offered(const ModuleFactory *factory, const std::vector<impl_t> &impls, const Heuristic *heuristic);

// Called from the factories, through ModuleFactory::decline.
void declined(const ModuleFactory *factory, const std::string &reason);

// Called from the reorderer with what it enumerated at an anchor.
void reorder_candidates(LibBDD::bdd_node_id_t anchor, bool direction, const std::vector<LibBDD::bdd_node_id_t> &candidates);

struct choice_t {
  enum class Kind { None, Force, Stop } kind;
  ep_id_t ep; // for Force
};

// After every factory has run for the step: decide. None when there was nothing to choose between.
choice_t choose();

} // namespace Walk
} // namespace LibSynapse
