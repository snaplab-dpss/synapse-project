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

namespace synapse {
namespace tofino {

struct TofinoTarget : public Target {
  TofinoTarget(const toml::table &config)
      : Target(
            TargetType::Tofino,
            []() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<SendToControllerFactory>());
              // f.push_back(std::make_unique<RecirculateFactory>());
              f.push_back(std::make_unique<ForwardFactory>());
              f.push_back(std::make_unique<DropFactory>());
              f.push_back(std::make_unique<BroadcastFactory>());
              f.push_back(std::make_unique<IgnoreFactory>());
              f.push_back(std::make_unique<IfFactory>());
              f.push_back(std::make_unique<ThenFactory>());
              f.push_back(std::make_unique<ElseFactory>());
              f.push_back(std::make_unique<ParserExtractionFactory>());
              f.push_back(std::make_unique<ParserConditionFactory>());
              f.push_back(std::make_unique<ParserRejectFactory>());
              f.push_back(std::make_unique<ModifyHeaderFactory>());
              f.push_back(std::make_unique<TableLookupFactory>());
              f.push_back(std::make_unique<VectorRegisterLookupFactory>());
              f.push_back(std::make_unique<VectorRegisterUpdateFactory>());
              // f.push_back(std::make_unique<FCFSCachedTableReadFactory>());
              // f.push_back(std::make_unique<FCFSCachedTableReadOrWriteFactory>());
              // f.push_back(std::make_unique<FCFSCachedTableWriteFactory>());
              // f.push_back(std::make_unique<FCFSCachedTableDeleteFactory>());
              f.push_back(std::make_unique<MeterUpdateFactory>());
              // f.push_back(std::make_unique<HHTableReadFactory>());
              // f.push_back(std::make_unique<HHTableConditionalUpdateFactory>());
              // f.push_back(std::make_unique<IntegerAllocatorAllocateFactory>());
              // f.push_back(std::make_unique<IntegerAllocatorIsAllocatedFactory>());
              // f.push_back(std::make_unique<IntegerAllocatorRejuvenateFactory>());
              f.push_back(std::make_unique<CMSQueryFactory>());
              f.push_back(std::make_unique<CMSIncrementFactory>());
              f.push_back(std::make_unique<CMSIncAndQueryFactory>());
              return f;
            }(),
            std::make_unique<TofinoContext>(config)) {}
};

} // namespace tofino
} // namespace synapse