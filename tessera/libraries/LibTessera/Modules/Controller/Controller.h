#pragma once

#include <LibTessera/Target.h>
#include <LibTessera/Modules/ModuleFactory.h>
#include <LibTessera/Modules/Controller/ControllerContext.h>

#include <LibTessera/Modules/Controller/Ignore.h>
#include <LibTessera/Modules/Controller/ParseHeader.h>
#include <LibTessera/Modules/Controller/ModifyHeader.h>
#include <LibTessera/Modules/Controller/If.h>
#include <LibTessera/Modules/Controller/Then.h>
#include <LibTessera/Modules/Controller/Else.h>
#include <LibTessera/Modules/Controller/Forward.h>
#include <LibTessera/Modules/Controller/Broadcast.h>
#include <LibTessera/Modules/Controller/Drop.h>
#include <LibTessera/Modules/Controller/AbortTransaction.h>
#include <LibTessera/Modules/Controller/DataplaneMapTableAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneMapTableLookup.h>
#include <LibTessera/Modules/Controller/DataplaneMapTableUpdate.h>
#include <LibTessera/Modules/Controller/DataplaneMapTableDelete.h>
#include <LibTessera/Modules/Controller/DataplaneMapSetTableAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneMapSetTableLookup.h>
#include <LibTessera/Modules/Controller/DataplaneMapSetTableUpdate.h>
#include <LibTessera/Modules/Controller/DataplaneMapSetTableInsert.h>
#include <LibTessera/Modules/Controller/DataplaneMapSetTableDelete.h>
#include <LibTessera/Modules/Controller/DataplaneGuardedMapTableAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneGuardedMapTableLookup.h>
#include <LibTessera/Modules/Controller/DataplaneGuardedMapTableGuardCheck.h>
#include <LibTessera/Modules/Controller/DataplaneGuardedMapTableUpdate.h>
#include <LibTessera/Modules/Controller/DataplaneGuardedMapTableDelete.h>
#include <LibTessera/Modules/Controller/DataplaneVectorTableAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneVectorTableLookup.h>
#include <LibTessera/Modules/Controller/DataplaneVectorTableUpdate.h>
#include <LibTessera/Modules/Controller/DataplaneDchainTableAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneDchainTableAllocateNewIndex.h>
#include <LibTessera/Modules/Controller/DataplaneDchainTableFreeIndex.h>
#include <LibTessera/Modules/Controller/DataplaneDchainTableIsIndexAllocated.h>
#include <LibTessera/Modules/Controller/DataplaneDchainTableRefreshIndex.h>
#include <LibTessera/Modules/Controller/DchainAllocate.h>
#include <LibTessera/Modules/Controller/DchainAllocateNewIndex.h>
#include <LibTessera/Modules/Controller/DchainIsIndexAllocated.h>
#include <LibTessera/Modules/Controller/DchainRejuvenateIndex.h>
#include <LibTessera/Modules/Controller/DchainFreeIndex.h>
#include <LibTessera/Modules/Controller/VectorAllocate.h>
#include <LibTessera/Modules/Controller/VectorRead.h>
#include <LibTessera/Modules/Controller/VectorWrite.h>
#include <LibTessera/Modules/Controller/MapAllocate.h>
#include <LibTessera/Modules/Controller/MapGet.h>
#include <LibTessera/Modules/Controller/MapPut.h>
#include <LibTessera/Modules/Controller/MapErase.h>
#include <LibTessera/Modules/Controller/ChecksumUpdate.h>
#include <LibTessera/Modules/Controller/ChtAllocate.h>
#include <LibTessera/Modules/Controller/ChtFindBackend.h>
#include <LibTessera/Modules/Controller/HashObj.h>
#include <LibTessera/Modules/Controller/DataplaneVectorRegisterAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneVectorRegisterLookup.h>
#include <LibTessera/Modules/Controller/DataplaneVectorRegisterUpdate.h>
#include <LibTessera/Modules/Controller/DataplaneFCFSCachedTableAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneFCFSCachedTableRead.h>
#include <LibTessera/Modules/Controller/DataplaneFCFSCachedTableWrite.h>
#include <LibTessera/Modules/Controller/DataplaneFCFSCachedTableAllocateAndWrite.h>
#include <LibTessera/Modules/Controller/DataplaneFCFSCachedTableIsIndexAllocated.h>
#include <LibTessera/Modules/Controller/DataplaneFCFSCachedSetAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneFCFSCachedSetRead.h>
#include <LibTessera/Modules/Controller/DataplaneFCFSCachedSetWrite.h>
#include <LibTessera/Modules/Controller/DataplaneFCFSCachedSetAllocateAndWrite.h>
#include <LibTessera/Modules/Controller/DataplaneHHTableAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneHHTableRead.h>
#include <LibTessera/Modules/Controller/DataplaneHHTableUpdate.h>
#include <LibTessera/Modules/Controller/DataplaneHHTableIsIndexAllocated.h>
#include <LibTessera/Modules/Controller/DataplaneHHTableDelete.h>
#include <LibTessera/Modules/Controller/DataplaneHHTableOutOfBandUpdate.h>
#include <LibTessera/Modules/Controller/DataplaneCMSAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneCMSQuery.h>
#include <LibTessera/Modules/Controller/DataplaneBloomFilterAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneBloomFilterQuery.h>
#include <LibTessera/Modules/Controller/DataplaneBloomFilterSet.h>
#include <LibTessera/Modules/Controller/TokenBucketAllocate.h>
#include <LibTessera/Modules/Controller/TokenBucketIsTracing.h>
#include <LibTessera/Modules/Controller/TokenBucketTrace.h>
#include <LibTessera/Modules/Controller/TokenBucketUpdateAndCheck.h>
#include <LibTessera/Modules/Controller/TokenBucketExpire.h>
#include <LibTessera/Modules/Controller/DataplaneMeterAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneMeterInsert.h>
#include <LibTessera/Modules/Controller/DataplaneIntegerAllocatorAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneIntegerAllocatorFreeIndex.h>
#include <LibTessera/Modules/Controller/CMSAllocate.h>
#include <LibTessera/Modules/Controller/CMSUpdate.h>
#include <LibTessera/Modules/Controller/CMSQuery.h>
#include <LibTessera/Modules/Controller/CMSIncrement.h>
#include <LibTessera/Modules/Controller/CMSCountMin.h>
#include <LibTessera/Modules/Controller/BloomFilterAllocate.h>
#include <LibTessera/Modules/Controller/BloomFilterQuery.h>
#include <LibTessera/Modules/Controller/BloomFilterSet.h>
#include <LibTessera/Modules/Controller/DataplaneCuckooHashTableAllocate.h>
#include <LibTessera/Modules/Controller/DataplaneExpireItemsSingleMapIteratively.h>

namespace LibTessera {
namespace Controller {

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
              f.push_back(std::make_unique<AbortTransactionFactory>());
              f.push_back(std::make_unique<DataplaneMapTableAllocateFactory>());
              f.push_back(std::make_unique<DataplaneMapTableLookupFactory>());
              f.push_back(std::make_unique<DataplaneMapTableUpdateFactory>());
              f.push_back(std::make_unique<DataplaneMapTableDeleteFactory>());
              f.push_back(std::make_unique<DataplaneMapSetTableAllocateFactory>());
              f.push_back(std::make_unique<DataplaneMapSetTableLookupFactory>());
              f.push_back(std::make_unique<DataplaneMapSetTableUpdateFactory>());
              f.push_back(std::make_unique<DataplaneMapSetTableInsertFactory>());
              f.push_back(std::make_unique<DataplaneMapSetTableDeleteFactory>());
              f.push_back(std::make_unique<DataplaneGuardedMapTableAllocateFactory>());
              f.push_back(std::make_unique<DataplaneGuardedMapTableLookupFactory>());
              f.push_back(std::make_unique<DataplaneGuardedMapTableGuardCheckFactory>());
              f.push_back(std::make_unique<DataplaneGuardedMapTableUpdateFactory>());
              f.push_back(std::make_unique<DataplaneGuardedMapTableDeleteFactory>());
              f.push_back(std::make_unique<DataplaneVectorTableAllocateFactory>());
              f.push_back(std::make_unique<DataplaneVectorTableLookupFactory>());
              f.push_back(std::make_unique<DataplaneVectorTableUpdateFactory>());
              f.push_back(std::make_unique<DataplaneDchainTableAllocateFactory>());
              f.push_back(std::make_unique<DataplaneDchainTableAllocateNewIndexFactory>());
              f.push_back(std::make_unique<DataplaneDchainTableFreeIndexFactory>());
              f.push_back(std::make_unique<DataplaneDchainTableIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<DataplaneDchainTableRefreshIndexFactory>());
              f.push_back(std::make_unique<DchainAllocateFactory>());
              f.push_back(std::make_unique<DchainAllocateNewIndexFactory>());
              f.push_back(std::make_unique<DchainIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<DchainRejuvenateIndexFactory>());
              f.push_back(std::make_unique<DchainFreeIndexFactory>());
              f.push_back(std::make_unique<VectorAllocateFactory>());
              f.push_back(std::make_unique<VectorReadFactory>());
              f.push_back(std::make_unique<VectorWriteFactory>());
              f.push_back(std::make_unique<MapAllocateFactory>());
              f.push_back(std::make_unique<MapGetFactory>());
              f.push_back(std::make_unique<MapPutFactory>());
              f.push_back(std::make_unique<MapEraseFactory>());
              f.push_back(std::make_unique<ChtAllocateFactory>());
              f.push_back(std::make_unique<ChtFindBackendFactory>());
              f.push_back(std::make_unique<HashObjFactory>());
              f.push_back(std::make_unique<DataplaneVectorRegisterAllocateFactory>());
              f.push_back(std::make_unique<DataplaneVectorRegisterLookupFactory>());
              f.push_back(std::make_unique<DataplaneVectorRegisterUpdateFactory>());
              f.push_back(std::make_unique<DataplaneFCFSCachedTableAllocateFactory>());
              f.push_back(std::make_unique<DataplaneFCFSCachedTableReadFactory>());
              f.push_back(std::make_unique<DataplaneFCFSCachedTableWriteFactory>());
              f.push_back(std::make_unique<DataplaneFCFSCachedTableAllocateAndWriteFactory>());
              f.push_back(std::make_unique<DataplaneFCFSCachedTableIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<DataplaneFCFSCachedSetAllocateFactory>());
              f.push_back(std::make_unique<DataplaneFCFSCachedSetReadFactory>());
              f.push_back(std::make_unique<DataplaneFCFSCachedSetWriteFactory>());
              f.push_back(std::make_unique<DataplaneFCFSCachedSetAllocateAndWriteFactory>());
              f.push_back(std::make_unique<DataplaneHHTableAllocateFactory>());
              f.push_back(std::make_unique<DataplaneHHTableReadFactory>());
              f.push_back(std::make_unique<DataplaneHHTableUpdateFactory>());
              f.push_back(std::make_unique<DataplaneHHTableIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<DataplaneHHTableDeleteFactory>());
              f.push_back(std::make_unique<DataplaneHHTableOutOfBandUpdateFactory>());
              f.push_back(std::make_unique<DataplaneCMSAllocateFactory>());
              f.push_back(std::make_unique<DataplaneCMSQueryFactory>());
              f.push_back(std::make_unique<DataplaneBloomFilterAllocateFactory>());
              f.push_back(std::make_unique<DataplaneBloomFilterQueryFactory>());
              f.push_back(std::make_unique<DataplaneBloomFilterSetFactory>());
              f.push_back(std::make_unique<TokenBucketAllocateFactory>());
              f.push_back(std::make_unique<TokenBucketIsTracingFactory>());
              f.push_back(std::make_unique<TokenBucketTraceFactory>());
              f.push_back(std::make_unique<TokenBucketUpdateAndCheckFactory>());
              f.push_back(std::make_unique<TokenBucketExpireFactory>());
              f.push_back(std::make_unique<DataplaneMeterAllocateFactory>());
              f.push_back(std::make_unique<DataplaneMeterInsertFactory>());
              f.push_back(std::make_unique<DataplaneIntegerAllocatorAllocateFactory>());
              f.push_back(std::make_unique<DataplaneIntegerAllocatorFreeIndexFactory>());
              f.push_back(std::make_unique<CMSAllocateFactory>());
              f.push_back(std::make_unique<CMSUpdateFactory>());
              f.push_back(std::make_unique<CMSQueryFactory>());
              f.push_back(std::make_unique<CMSIncrementFactory>());
              f.push_back(std::make_unique<CMSCountMinFactory>());
              f.push_back(std::make_unique<BloomFilterAllocateFactory>());
              f.push_back(std::make_unique<BloomFilterQueryFactory>());
              f.push_back(std::make_unique<BloomFilterSetFactory>());
              f.push_back(std::make_unique<DataplaneCuckooHashTableAllocateFactory>());
              f.push_back(std::make_unique<DataplaneExpireItemsSingleMapIterativelyFactory>());
              return f;
            }(),
            std::make_unique<ControllerContext>()) {}
};

} // namespace Controller
} // namespace LibTessera