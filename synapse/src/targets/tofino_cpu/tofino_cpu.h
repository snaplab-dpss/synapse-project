#pragma once

#include "../target.h"

#include "ignore.h"
#include "parse_header.h"
#include "modify_header.h"
#include "if.h"
#include "then.h"
#include "else.h"
#include "forward.h"
#include "broadcast.h"
#include "drop.h"
#include "table_lookup.h"
#include "table_update.h"
#include "table_delete.h"
#include "dchain_allocate_new_index.h"
#include "dchain_is_index_allocated.h"
#include "dchain_rejuvenate_index.h"
#include "dchain_free_index.h"
#include "vector_read.h"
#include "vector_write.h"
#include "map_get.h"
#include "map_put.h"
#include "map_erase.h"
#include "checksum_update.h"
#include "cht_find_backend.h"
#include "hash_obj.h"
#include "vector_register_lookup.h"
#include "vector_register_update.h"
#include "fcfs_cached_table_read.h"
#include "fcfs_cached_table_write.h"
#include "fcfs_cached_table_delete.h"
#include "map_register_read.h"
#include "map_register_write.h"
#include "map_register_delete.h"
#include "hh_table_read.h"
#include "hh_table_update.h"
#include "hh_table_conditional_update.h"
#include "hh_table_delete.h"
#include "tb_is_tracing.h"
#include "tb_trace.h"
#include "tb_update_and_check.h"
#include "tb_expire.h"
#include "meter_insert.h"
#include "int_alloc_free_index.h"
#include "cms_update.h"
#include "cms_query.h"
#include "cms_increment.h"
#include "cms_count_min.h"

#include "tofino_cpu_context.h"

namespace tofino_cpu {

struct TofinoCPUTarget : public Target {
  TofinoCPUTarget()
      : Target(
            TargetType::TofinoCPU,
            []() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<IgnoreFactory>());
              f.push_back(std::make_unique<ParseHeaderFactory>());
              f.push_back(std::make_unique<ModifyHeaderFactory>());
              f.push_back(std::make_unique<ChecksumUpdateFactory>());
              f.push_back(std::make_unique<IfFactory>());
              f.push_back(std::make_unique<ThenFactory>());
              f.push_back(std::make_unique<ElseFactory>());
              f.push_back(std::make_unique<ForwardFactory>());
              f.push_back(std::make_unique<BroadcastFactory>());
              f.push_back(std::make_unique<DropFactory>());
              f.push_back(std::make_unique<TableLookupFactory>());
              f.push_back(std::make_unique<TableUpdateFactory>());
              f.push_back(std::make_unique<TableDeleteFactory>());
              f.push_back(std::make_unique<DchainAllocateNewIndexFactory>());
              f.push_back(std::make_unique<DchainIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<DchainRejuvenateIndexFactory>());
              f.push_back(std::make_unique<DchainFreeIndexFactory>());
              f.push_back(std::make_unique<VectorReadFactory>());
              f.push_back(std::make_unique<VectorWriteFactory>());
              f.push_back(std::make_unique<MapGetFactory>());
              f.push_back(std::make_unique<MapPutFactory>());
              f.push_back(std::make_unique<MapEraseFactory>());
              f.push_back(std::make_unique<ChtFindBackendFactory>());
              f.push_back(std::make_unique<HashObjFactory>());
              f.push_back(std::make_unique<VectorRegisterLookupFactory>());
              f.push_back(std::make_unique<VectorRegisterUpdateFactory>());
              f.push_back(std::make_unique<FCFSCachedTableReadFactory>());
              f.push_back(std::make_unique<FCFSCachedTableWriteFactory>());
              f.push_back(std::make_unique<FCFSCachedTableDeleteFactory>());
              f.push_back(std::make_unique<MapRegisterReadFactory>());
              f.push_back(std::make_unique<MapRegisterWriteFactory>());
              f.push_back(std::make_unique<MapRegisterDeleteFactory>());
              f.push_back(std::make_unique<HHTableReadFactory>());
              f.push_back(std::make_unique<HHTableUpdateFactory>());
              f.push_back(std::make_unique<HHTableConditionalUpdateFactory>());
              f.push_back(std::make_unique<HHTableDeleteFactory>());
              f.push_back(std::make_unique<TBIsTracingFactory>());
              f.push_back(std::make_unique<TBTraceFactory>());
              f.push_back(std::make_unique<TBUpdateAndCheckFactory>());
              f.push_back(std::make_unique<TBExpireFactory>());
              f.push_back(std::make_unique<MeterInsertFactory>());
              f.push_back(std::make_unique<IntegerAllocatorFreeIndexFactory>());
              f.push_back(std::make_unique<CMSUpdateFactory>());
              f.push_back(std::make_unique<CMSQueryFactory>());
              f.push_back(std::make_unique<CMSIncrementFactory>());
              f.push_back(std::make_unique<CMSCountMinFactory>());
              return f;
            }(),
            std::make_unique<TofinoCPUContext>()) {}
};

} // namespace tofino_cpu
