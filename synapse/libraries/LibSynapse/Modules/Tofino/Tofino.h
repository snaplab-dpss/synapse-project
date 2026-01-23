#pragma once

#include <LibSynapse/Target.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>

#include <LibSynapse/Modules/Tofino/SendToController.h>
#include <LibSynapse/Modules/Tofino/Recirculate.h>
#include <LibSynapse/Modules/Tofino/Forward.h>
#include <LibSynapse/Modules/Tofino/Drop.h>
#include <LibSynapse/Modules/Tofino/Broadcast.h>
#include <LibSynapse/Modules/Tofino/Ignore.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/Modules/Tofino/ParserExtraction.h>
#include <LibSynapse/Modules/Tofino/ParserCondition.h>
#include <LibSynapse/Modules/Tofino/ParserReject.h>
#include <LibSynapse/Modules/Tofino/ModifyHeader.h>
#include <LibSynapse/Modules/Tofino/MapTableLookup.h>
#include <LibSynapse/Modules/Tofino/MapSetTableLookup.h>
#include <LibSynapse/Modules/Tofino/GuardedMapTableLookup.h>
#include <LibSynapse/Modules/Tofino/GuardedMapTableGuardCheck.h>
#include <LibSynapse/Modules/Tofino/VectorTableLookup.h>
#include <LibSynapse/Modules/Tofino/DchainTableLookup.h>
#include <LibSynapse/Modules/Tofino/VectorRegisterLookup.h>
#include <LibSynapse/Modules/Tofino/VectorRegisterUpdate.h>
#include <LibSynapse/Modules/Tofino/VectorRegisterConditionalUpdate.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableRead.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableReadInsert.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableInsert.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableIsIndexAllocated.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedSetRead.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedSetReadInsert.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedSetInsert.h>
#include <LibSynapse/Modules/Tofino/MeterUpdate.h>
#include <LibSynapse/Modules/Tofino/HHTableRead.h>
#include <LibSynapse/Modules/Tofino/HHTableOutOfBandUpdate.h>
#include <LibSynapse/Modules/Tofino/IntegerAllocatorAllocate.h>
#include <LibSynapse/Modules/Tofino/IntegerAllocatorIsAllocated.h>
#include <LibSynapse/Modules/Tofino/IntegerAllocatorRejuvenate.h>
#include <LibSynapse/Modules/Tofino/CMSQuery.h>
#include <LibSynapse/Modules/Tofino/CMSIncrement.h>
#include <LibSynapse/Modules/Tofino/CMSIncAndQuery.h>
#include <LibSynapse/Modules/Tofino/BloomFilterSet.h>
#include <LibSynapse/Modules/Tofino/BloomFilterQuery.h>
#include <LibSynapse/Modules/Tofino/BloomFilterQueryAndSet.h>
#include <LibSynapse/Modules/Tofino/LPMLookup.h>
#include <LibSynapse/Modules/Tofino/CuckooHashTableReadWrite.h>

namespace LibSynapse {
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
              f.push_back(std::make_unique<VectorTableLookupFactory>());
              f.push_back(std::make_unique<VectorRegisterLookupFactory>());
              f.push_back(std::make_unique<VectorRegisterUpdateFactory>());
              f.push_back(std::make_unique<VectorRegisterConditionalUpdateFactory>());
              // f.push_back(std::make_unique<FCFSCachedSetReadFactory>());
              // f.push_back(std::make_unique<FCFSCachedSetInsertFactory>());
              // f.push_back(std::make_unique<FCFSCachedSetReadInsertFactory>());
              f.push_back(std::make_unique<FCFSCachedTableReadFactory>());
              f.push_back(std::make_unique<FCFSCachedTableReadInsertFactory>());
              f.push_back(std::make_unique<FCFSCachedTableInsertFactory>());
              f.push_back(std::make_unique<FCFSCachedTableIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<MeterUpdateFactory>());
              f.push_back(std::make_unique<HHTableReadFactory>());
              f.push_back(std::make_unique<HHTableOutOfBandUpdateFactory>());
              // f.push_back(std::make_unique<IntegerAllocatorAllocateFactory>());
              // f.push_back(std::make_unique<IntegerAllocatorIsAllocatedFactory>());
              // f.push_back(std::make_unique<IntegerAllocatorRejuvenateFactory>());
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
} // namespace LibSynapse