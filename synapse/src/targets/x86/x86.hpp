#pragma once

#include "../target.hpp"

#include "ignore.hpp"
#include "if.hpp"
#include "then.hpp"
#include "else.hpp"
#include "forward.hpp"
#include "broadcast.hpp"
#include "drop.hpp"
#include "parse_header.hpp"
#include "modify_header.hpp"
#include "checksum_update.hpp"
#include "expire_items_single_map.hpp"
#include "expire_items_single_map_iteratively.hpp"
#include "map_get.hpp"
#include "map_put.hpp"
#include "map_erase.hpp"
#include "vector_read.hpp"
#include "vector_write.hpp"
#include "dchain_allocate_new_index.hpp"
#include "dchain_is_index_allocated.hpp"
#include "dchain_rejuvenate_index.hpp"
#include "dchain_free_index.hpp"
#include "cms_count_min.hpp"
#include "cms_increment.hpp"
#include "cms_periodic_cleanup.hpp"
#include "hash_obj.hpp"
#include "cht_find_backend.hpp"
#include "tb_expire.hpp"
#include "tb_is_tracing.hpp"
#include "tb_trace.hpp"
#include "tb_update_and_check.hpp"

#include "x86_context.hpp"

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