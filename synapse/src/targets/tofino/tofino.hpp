#pragma once

#include <toml++/toml.hpp>

#include "../target.hpp"

#include "send_to_controller.hpp"
#include "recirculate.hpp"
#include "ignore.hpp"
#include "if.hpp"
#include "then.hpp"
#include "else.hpp"
#include "forward.hpp"
#include "drop.hpp"
#include "broadcast.hpp"
#include "parser_extraction.hpp"
#include "parser_condition.hpp"
#include "parser_reject.hpp"
#include "modify_header.hpp"
#include "table_lookup.hpp"
#include "vector_register_lookup.hpp"
#include "vector_register_update.hpp"
#include "fcfs_cached_table_read.hpp"
#include "fcfs_cached_table_read_or_write.hpp"
#include "fcfs_cached_table_write.hpp"
#include "fcfs_cached_table_delete.hpp"
#include "meter_update.hpp"
#include "hh_table_read.hpp"
#include "hh_table_conditional_update.hpp"
#include "int_alloc_allocate.hpp"
#include "int_alloc_is_allocated.hpp"
#include "int_alloc_rejuvenate.hpp"
#include "cms_query.hpp"
#include "cms_increment.hpp"
#include "cms_inc_and_query.hpp"

#include "tofino_context.hpp"
#include "../../profiler.hpp"

namespace synapse {
namespace tofino {

struct TofinoTarget : public Target {
  TofinoTarget(const toml::table &config)
      : Target(
            TargetType::Tofino,
            []() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<SendToControllerFactory>());
              f.push_back(std::make_unique<RecirculateFactory>());
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