#pragma once

#include "../target.hpp"

#include "ignore.hpp"
#include "parse_header.hpp"
#include "modify_header.hpp"
#include "if.hpp"
#include "then.hpp"
#include "else.hpp"
#include "forward.hpp"
#include "broadcast.hpp"
#include "drop.hpp"
#include "table_lookup.hpp"
#include "table_update.hpp"
#include "table_delete.hpp"
#include "dchain_allocate_new_index.hpp"
#include "dchain_is_index_allocated.hpp"
#include "dchain_rejuvenate_index.hpp"
#include "dchain_free_index.hpp"
#include "vector_read.hpp"
#include "vector_write.hpp"
#include "map_get.hpp"
#include "map_put.hpp"
#include "map_erase.hpp"
#include "checksum_update.hpp"
#include "cht_find_backend.hpp"
#include "hash_obj.hpp"
#include "vector_register_lookup.hpp"
#include "vector_register_update.hpp"
#include "fcfs_cached_table_read.hpp"
#include "fcfs_cached_table_write.hpp"
#include "fcfs_cached_table_delete.hpp"
#include "hh_table_read.hpp"
#include "hh_table_update.hpp"
#include "hh_table_conditional_update.hpp"
#include "hh_table_delete.hpp"
#include "tb_is_tracing.hpp"
#include "tb_trace.hpp"
#include "tb_update_and_check.hpp"
#include "tb_expire.hpp"
#include "meter_insert.hpp"
#include "int_alloc_free_index.hpp"
#include "cms_update.hpp"
#include "cms_query.hpp"
#include "cms_increment.hpp"
#include "cms_count_min.hpp"

#include "controller_context.hpp"

namespace synapse {
namespace ctrl {

struct ControllerTarget : public Target {
  ControllerTarget()
      : Target(
            TargetType::Controller,
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
            std::make_unique<ControllerContext>()) {}
};

} // namespace ctrl
} // namespace synapse