#pragma once

#include <LibTessera/Target.h>
#include <LibTessera/Modules/ModuleFactory.h>
#include <LibTessera/Modules/Tofino/TofinoContext.h>

#include <LibTessera/Modules/Tofino/SendToController.h>
#include <LibTessera/Modules/Tofino/Recirculate.h>
#include <LibTessera/Modules/Tofino/Forward.h>
#include <LibTessera/Modules/Tofino/Drop.h>
#include <LibTessera/Modules/Tofino/Broadcast.h>
#include <LibTessera/Modules/Tofino/Ignore.h>
#include <LibTessera/Modules/Tofino/If.h>
#include <LibTessera/Modules/Tofino/Then.h>
#include <LibTessera/Modules/Tofino/Else.h>
#include <LibTessera/Modules/Tofino/ParserExtraction.h>
#include <LibTessera/Modules/Tofino/ParserCondition.h>
#include <LibTessera/Modules/Tofino/ParserReject.h>
#include <LibTessera/Modules/Tofino/ModifyHeader.h>
#include <LibTessera/Modules/Tofino/MapTableLookup.h>
#include <LibTessera/Modules/Tofino/MapSetTableLookup.h>
#include <LibTessera/Modules/Tofino/GuardedMapTableLookup.h>
#include <LibTessera/Modules/Tofino/GuardedMapTableGuardCheck.h>
#include <LibTessera/Modules/Tofino/VectorTableLookup.h>
#include <LibTessera/Modules/Tofino/DchainTableLookup.h>
#include <LibTessera/Modules/Tofino/VectorRegisterLookup.h>
#include <LibTessera/Modules/Tofino/VectorRegisterUpdate.h>
#include <LibTessera/Modules/Tofino/VectorRegisterReadConditionalUpdate.h>
#include <LibTessera/Modules/Tofino/VectorRegisterReadConditionalUpdateSingleAction.h>
#include <LibTessera/Modules/Tofino/FCFSCachedTableRead.h>
#include <LibTessera/Modules/Tofino/FCFSCachedTableReadInsert.h>
#include <LibTessera/Modules/Tofino/FCFSCachedTableInsert.h>
#include <LibTessera/Modules/Tofino/FCFSCachedTableIsIndexAllocated.h>
#include <LibTessera/Modules/Tofino/FCFSCachedSetRead.h>
#include <LibTessera/Modules/Tofino/FCFSCachedSetReadInsert.h>
#include <LibTessera/Modules/Tofino/FCFSCachedSetInsert.h>
#include <LibTessera/Modules/Tofino/MeterUpdate.h>
#include <LibTessera/Modules/Tofino/HHTableRead.h>
#include <LibTessera/Modules/Tofino/HHTableOutOfBandUpdate.h>
#include <LibTessera/Modules/Tofino/IntegerAllocatorAllocate.h>
#include <LibTessera/Modules/Tofino/IntegerAllocatorIsAllocated.h>
#include <LibTessera/Modules/Tofino/IntegerAllocatorRejuvenate.h>
#include <LibTessera/Modules/Tofino/CMSQuery.h>
#include <LibTessera/Modules/Tofino/CMSIncrement.h>
#include <LibTessera/Modules/Tofino/CMSIncAndQuery.h>
#include <LibTessera/Modules/Tofino/BloomFilterSet.h>
#include <LibTessera/Modules/Tofino/BloomFilterQuery.h>
#include <LibTessera/Modules/Tofino/BloomFilterQueryAndSet.h>
#include <LibTessera/Modules/Tofino/LPMLookup.h>
#include <LibTessera/Modules/Tofino/CuckooHashTableReadWrite.h>

namespace LibTessera {
namespace Tofino {

struct TofinoTarget : public Target {
  TofinoTarget(const tna_config_t &tna_config)
      : Target(
            TargetType::Tofino,
            []() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
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
              f.push_back(std::make_unique<MapTableLookupFactory>());
              f.push_back(std::make_unique<MapSetTableLookupFactory>());
              f.push_back(std::make_unique<GuardedMapTableLookupFactory>());
              f.push_back(std::make_unique<GuardedMapTableGuardCheckFactory>());
              f.push_back(std::make_unique<DchainTableLookupFactory>());
              f.push_back(std::make_unique<VectorRegisterReadConditionalUpdateFactory>());
              f.push_back(std::make_unique<VectorRegisterReadConditionalUpdateSingleActionFactory>());
              f.push_back(std::make_unique<VectorRegisterLookupFactory>());
              f.push_back(std::make_unique<VectorRegisterUpdateFactory>());
              f.push_back(std::make_unique<VectorTableLookupFactory>());
              f.push_back(std::make_unique<FCFSCachedSetReadFactory>());
              f.push_back(std::make_unique<FCFSCachedSetInsertFactory>());
              f.push_back(std::make_unique<FCFSCachedSetReadInsertFactory>());
              f.push_back(std::make_unique<FCFSCachedTableReadFactory>());
              f.push_back(std::make_unique<FCFSCachedTableReadInsertFactory>());
              f.push_back(std::make_unique<FCFSCachedTableInsertFactory>());
              f.push_back(std::make_unique<FCFSCachedTableIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<MeterUpdateFactory>());
              f.push_back(std::make_unique<HHTableReadFactory>());
              f.push_back(std::make_unique<HHTableOutOfBandUpdateFactory>());
              f.push_back(std::make_unique<CMSQueryFactory>());
              f.push_back(std::make_unique<CMSIncrementFactory>());
              f.push_back(std::make_unique<CMSIncAndQueryFactory>());
              f.push_back(std::make_unique<BloomFilterSetFactory>());
              f.push_back(std::make_unique<BloomFilterQueryFactory>());
              f.push_back(std::make_unique<BloomFilterQueryAndSetFactory>());
              f.push_back(std::make_unique<LPMLookupFactory>());
              f.push_back(std::make_unique<CuckooHashTableReadWriteFactory>());
              f.push_back(std::make_unique<SendToControllerFactory>());
              return f;
            }(),
            std::make_unique<TofinoContext>(tna_config)) {}
};

} // namespace Tofino
} // namespace LibTessera