#pragma once

#include <toml++/toml.hpp>

#include "../target.h"

#include "send_to_controller.h"
#include "recirculate.h"
#include "ignore.h"
#include "if.h"
#include "then.h"
#include "else.h"
#include "forward.h"
#include "drop.h"
#include "broadcast.h"
#include "parser_extraction.h"
#include "parser_condition.h"
#include "parser_reject.h"
#include "modify_header.h"
#include "table_lookup.h"
#include "vector_register_lookup.h"
#include "vector_register_update.h"
#include "fcfs_cached_table_read.h"
#include "fcfs_cached_table_read_or_write.h"
#include "fcfs_cached_table_write.h"
#include "fcfs_cached_table_delete.h"
#include "meter_update.h"
#include "hh_table_read.h"
#include "hh_table_conditional_update.h"
#include "int_alloc_allocate.h"
#include "int_alloc_is_allocated.h"
#include "int_alloc_rejuvenate.h"
#include "cms_query.h"
#include "cms_increment.h"
#include "cms_inc_and_query.h"

#include "tofino_context.h"
#include "../../profiler.h"

namespace tofino {

struct TofinoTarget : public Target {
  TofinoTarget(const toml::table &config)
      : Target(TargetType::Tofino,
               {
                   new SendToControllerGenerator(),
                   new RecirculateGenerator(),
                   new IgnoreGenerator(),
                   new IfGenerator(),
                   new ThenGenerator(),
                   new ElseGenerator(),
                   new ForwardGenerator(),
                   new DropGenerator(),
                   new BroadcastGenerator(),
                   new ParserExtractionGenerator(),
                   new ParserConditionGenerator(),
                   new ParserRejectGenerator(),
                   new ModifyHeaderGenerator(),
                   new TableLookupGenerator(),
                   new VectorRegisterLookupGenerator(),
                   new VectorRegisterUpdateGenerator(),
                   new FCFSCachedTableReadGenerator(),
                   new FCFSCachedTableReadOrWriteGenerator(),
                   new FCFSCachedTableWriteGenerator(),
                   new FCFSCachedTableDeleteGenerator(),
                   new MeterUpdateGenerator(),
                   new HHTableReadGenerator(),
                   new HHTableConditionalUpdateGenerator(),
                   new IntegerAllocatorAllocateGenerator(),
                   new IntegerAllocatorIsAllocatedGenerator(),
                   new IntegerAllocatorRejuvenateGenerator(),
                   new CMSQueryGenerator(),
                   new CMSIncrementGenerator(),
                   new CMSIncAndQueryGenerator(),
               },
               new TofinoContext(config)) {}
};

} // namespace tofino
