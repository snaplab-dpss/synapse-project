#pragma once

#include "bfs.hpp"
#include "dfs.hpp"
#include "random.hpp"
#include "gallium.hpp"
#include "greedy.hpp"
#include "max_tput.hpp"
#include "ds_pref.hpp"

namespace synapse {
enum class HeuristicOption { BFS, DFS, RANDOM, GALLIUM, GREEDY, MAX_TPUT, DS_PREF };
}