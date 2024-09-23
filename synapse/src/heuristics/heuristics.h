#pragma once

#include "bfs.h"
#include "dfs.h"
#include "random.h"
#include "gallium.h"
#include "greedy.h"
#include "max_tput.h"

#define EXPLICIT_HEURISTIC_TEMPLATE_CLASS_INSTANTIATION(C)                     \
  template class C<BFSCfg>;                                                    \
  template class C<DFSCfg>;                                                    \
  template class C<RandomCfg>;                                                 \
  template class C<GalliumCfg>;                                                \
  template class C<GreedyCfg>;                                                 \
  template class C<MaxTputCfg>;
