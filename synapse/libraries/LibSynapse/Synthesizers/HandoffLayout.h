#pragma once

#include <LibSynapse/ExecutionPlan.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace LibSynapse {

// What the data plane hands the controller besides the cpu header: its state header, whole,
// right after it. The words every packet to the controller carries, in header order, and the
// value each holds at a hand-off, by the symbol the controller knows it as. The Tofino synthesizer
// fills it while it emits; the controller synthesizer reads those words as those symbols.
struct handoff_layout_t {
  std::vector<std::pair<std::string, unsigned>> state_words;                                  // (word, width in bits)
  std::unordered_map<ep_node_id_t, std::unordered_map<std::string, std::string>> symbol_word; // hand-off -> symbol -> word
};

} // namespace LibSynapse
