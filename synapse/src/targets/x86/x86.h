#pragma once

#include "../target.h"

#include "ignore.h"
#include "if.h"
#include "then.h"
#include "else.h"
#include "forward.h"
#include "broadcast.h"
#include "drop.h"
#include "parse_header.h"
#include "modify_header.h"
#include "checksum_update.h"
#include "expire_items_single_map.h"
#include "expire_items_single_map_iteratively.h"
#include "map_get.h"
#include "map_put.h"
#include "map_erase.h"
#include "vector_read.h"
#include "vector_write.h"
#include "dchain_allocate_new_index.h"
#include "dchain_is_index_allocated.h"
#include "dchain_rejuvenate_index.h"
#include "dchain_free_index.h"
#include "cms_count_min.h"
#include "cms_increment.h"
#include "cms_periodic_cleanup.h"
#include "hash_obj.h"
#include "cht_find_backend.h"
#include "tb_expire.h"
#include "tb_is_tracing.h"
#include "tb_trace.h"
#include "tb_update_and_check.h"

#include "x86_context.h"

namespace synapse {
namespace x86 {

struct x86Target : public Target {
  x86Target()
      : Target(
            TargetType::x86,
            []() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<IgnoreFactory>());
              f.push_back(std::make_unique<ForwardFactory>());
              f.push_back(std::make_unique<BroadcastFactory>());
              f.push_back(std::make_unique<DropFactory>());
              f.push_back(std::make_unique<ParseHeaderFactory>());
              f.push_back(std::make_unique<ModifyHeaderFactory>());
              f.push_back(std::make_unique<ChecksumUpdateFactory>());
              f.push_back(std::make_unique<IfFactory>());
              f.push_back(std::make_unique<ThenFactory>());
              f.push_back(std::make_unique<ElseFactory>());
              f.push_back(std::make_unique<ExpireItemsSingleMapFactory>());
              f.push_back(std::make_unique<ExpireItemsSingleMapIterativelyFactory>());
              f.push_back(std::make_unique<MapGetFactory>());
              f.push_back(std::make_unique<MapPutFactory>());
              f.push_back(std::make_unique<MapEraseFactory>());
              f.push_back(std::make_unique<VectorReadFactory>());
              f.push_back(std::make_unique<VectorWriteFactory>());
              f.push_back(std::make_unique<DchainRejuvenateIndexFactory>());
              f.push_back(std::make_unique<DchainAllocateNewIndexFactory>());
              f.push_back(std::make_unique<DchainIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<DchainFreeIndexFactory>());
              f.push_back(std::make_unique<CMSCountMinFactory>());
              f.push_back(std::make_unique<CMSIncrementFactory>());
              f.push_back(std::make_unique<CMSPeriodicCleanupFactory>());
              f.push_back(std::make_unique<HashObjFactory>());
              f.push_back(std::make_unique<ChtFindBackendFactory>());
              f.push_back(std::make_unique<TBExpireFactory>());
              f.push_back(std::make_unique<TBIsTracingFactory>());
              f.push_back(std::make_unique<TBTraceFactory>());
              f.push_back(std::make_unique<TBUpdateAndCheckFactory>());
              return f;
            }(),
            std::make_unique<x86Context>()) {}
};

} // namespace x86
} // namespace synapse