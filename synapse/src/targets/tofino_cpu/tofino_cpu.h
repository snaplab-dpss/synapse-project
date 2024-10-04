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
#include "sketch_compute_hashes.h"
#include "sketch_expire.h"
#include "sketch_fetch.h"
#include "sketch_refresh.h"
#include "sketch_touch_buckets.h"
#include "vector_register_lookup.h"
#include "vector_register_update.h"
#include "fcfs_cached_table_read.h"
#include "fcfs_cached_table_write.h"
#include "fcfs_cached_table_delete.h"
#include "tb_is_tracing.h"
#include "tb_trace.h"
#include "tb_update_and_check.h"
#include "tb_expire.h"
#include "meter_insert.h"

#include "tofino_cpu_context.h"

namespace tofino_cpu {

struct TofinoCPUTarget : public Target {
  TofinoCPUTarget()
      : Target(TargetType::TofinoCPU,
               {
                   new IgnoreGenerator(),
                   new ParseHeaderGenerator(),
                   new ModifyHeaderGenerator(),
                   new ChecksumUpdateGenerator(),
                   new IfGenerator(),
                   new ThenGenerator(),
                   new ElseGenerator(),
                   new ForwardGenerator(),
                   new BroadcastGenerator(),
                   new DropGenerator(),
                   new TableLookupGenerator(),
                   new TableUpdateGenerator(),
                   new TableDeleteGenerator(),
                   new DchainAllocateNewIndexGenerator(),
                   new DchainIsIndexAllocatedGenerator(),
                   new DchainRejuvenateIndexGenerator(),
                   new DchainFreeIndexGenerator(),
                   new VectorReadGenerator(),
                   new VectorWriteGenerator(),
                   new MapGetGenerator(),
                   new MapPutGenerator(),
                   new MapEraseGenerator(),
                   new ChtFindBackendGenerator(),
                   new HashObjGenerator(),
                   new SketchComputeHashesGenerator(),
                   new SketchExpireGenerator(),
                   new SketchFetchGenerator(),
                   new SketchRefreshGenerator(),
                   new SketchTouchBucketsGenerator(),
                   new VectorRegisterLookupGenerator(),
                   new VectorRegisterUpdateGenerator(),
                   new FCFSCachedTableReadGenerator(),
                   new FCFSCachedTableWriteGenerator(),
                   new FCFSCachedTableDeleteGenerator(),
                   new TBIsTracingGenerator(),
                   new TBTraceGenerator(),
                   new TBUpdateAndCheckGenerator(),
                   new TBExpireGenerator(),
                   new MeterInsertGenerator(),
               },
               new TofinoCPUContext()) {}
};

} // namespace tofino_cpu
