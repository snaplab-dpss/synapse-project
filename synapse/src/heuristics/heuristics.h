#pragma once

#include "bfs.h"
#include "dfs.h"
#include "random.h"
#include "gallium.h"
#include "greedy.h"
#include "max_tput.h"
#include "ds_pref.h"

namespace synapse {
enum class HeuristicOption { BFS, DFS, RANDOM, GALLIUM, GREEDY, MAX_TPUT, DS_PREF };
}