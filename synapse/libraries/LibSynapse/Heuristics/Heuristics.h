#pragma once

#include <LibSynapse/Heuristics/BFS.h>
#include <LibSynapse/Heuristics/DFS.h>
#include <LibSynapse/Heuristics/Gallium.h>
#include <LibSynapse/Heuristics/Greedy.h>
#include <LibSynapse/Heuristics/MaxTput.h>
#include <LibSynapse/Heuristics/Random.h>
#include <LibSynapse/Heuristics/DSPref.h>

namespace LibSynapse {

enum class HeuristicOption { BFS, DFS, GALLIUM, GREEDY, MAX_TPUT, RANDOM, DS_PREF };

}