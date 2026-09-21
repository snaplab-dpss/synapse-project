#pragma once

#include <LibSynapse/ExecutionPlan.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace LibSynapse {

// What the data plane hands the controller besides the cpu header: its state header, whole,
// right after it. The words every packet to the controller carries, in header order, and the
// value each holds at a hand-off, by the symbol the controller knows it as. The Tofino synthesizer
// fills it while it emits; the controller synthesizer reads those words as those symbols.
struct handoff_layout_t {
  // Whether the cpu header carries, first among its extra fields, the ingress clock at the
  // hand-off (ingress_mac_tstamp[47:16]): the controller's `now` for the packet, since the host's
  // is another clock and the values the data plane computed from its own time (a cookie's ticks)
  // would not match those the controller recomputes.
  bool ships_time = false;
  std::vector<std::pair<std::string, unsigned>> state_words;                                  // (word, width in bits)
  std::unordered_map<ep_node_id_t, std::unordered_map<std::string, std::string>> symbol_word; // hand-off -> symbol -> word
  // Symbols the data plane ships as a P4 bool: the word holds one meaningful bit, its least
  // significant, and @padding above it. bf-p4c overlays that padding with fields of headers it
  // believes cannot be live at the same time, so on a recirculated packet the word reaches the
  // controller with those bits set. The controller reads the whole word, so it has to drop them.
  std::unordered_set<std::string> bool_symbols;
};

} // namespace LibSynapse
