#pragma once

#include <LibSynapse/Target.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/Controller/ControllerContext.h>

#include <LibSynapse/Modules/Controller/Ignore.h>
#include <LibSynapse/Modules/Controller/ParseHeader.h>
#include <LibSynapse/Modules/Controller/ModifyHeader.h>
#include <LibSynapse/Modules/Controller/If.h>
#include <LibSynapse/Modules/Controller/Then.h>
#include <LibSynapse/Modules/Controller/Else.h>
#include <LibSynapse/Modules/Controller/Forward.h>
#include <LibSynapse/Modules/Controller/Broadcast.h>
#include <LibSynapse/Modules/Controller/Drop.h>
#include <LibSynapse/Modules/Controller/AbortTransaction.h>
#include <LibSynapse/Modules/Controller/DataplaneMapTableAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneMapTableLookup.h>
#include <LibSynapse/Modules/Controller/DataplaneMapTableUpdate.h>
#include <LibSynapse/Modules/Controller/DataplaneMapTableDelete.h>
#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableLookup.h>
#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableGuardCheck.h>
#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableUpdate.h>
#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableDelete.h>
#include <LibSynapse/Modules/Controller/DataplaneVectorTableAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneVectorTableLookup.h>
#include <LibSynapse/Modules/Controller/DataplaneVectorTableUpdate.h>
#include <LibSynapse/Modules/Controller/DataplaneDchainTableAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneDchainTableAllocateNewIndex.h>
#include <LibSynapse/Modules/Controller/DataplaneDchainTableFreeIndex.h>
#include <LibSynapse/Modules/Controller/DataplaneDchainTableIsIndexAllocated.h>
#include <LibSynapse/Modules/Controller/DataplaneDchainTableRefreshIndex.h>
#include <LibSynapse/Modules/Controller/DchainAllocate.h>
#include <LibSynapse/Modules/Controller/DchainAllocateNewIndex.h>
#include <LibSynapse/Modules/Controller/DchainIsIndexAllocated.h>
#include <LibSynapse/Modules/Controller/DchainRejuvenateIndex.h>
#include <LibSynapse/Modules/Controller/DchainFreeIndex.h>
#include <LibSynapse/Modules/Controller/VectorAllocate.h>
#include <LibSynapse/Modules/Controller/VectorRead.h>
#include <LibSynapse/Modules/Controller/VectorWrite.h>
#include <LibSynapse/Modules/Controller/MapAllocate.h>
#include <LibSynapse/Modules/Controller/MapGet.h>
#include <LibSynapse/Modules/Controller/MapPut.h>
#include <LibSynapse/Modules/Controller/MapErase.h>
#include <LibSynapse/Modules/Controller/ChecksumUpdate.h>
#include <LibSynapse/Modules/Controller/ChtAllocate.h>
#include <LibSynapse/Modules/Controller/ChtFindBackend.h>
#include <LibSynapse/Modules/Controller/HashObj.h>
#include <LibSynapse/Modules/Controller/DataplaneVectorRegisterAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneVectorRegisterLookup.h>
#include <LibSynapse/Modules/Controller/DataplaneVectorRegisterUpdate.h>
#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableRead.h>
#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableWrite.h>
#include <LibSynapse/Modules/Controller/DataplaneHHTableAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneHHTableRead.h>
#include <LibSynapse/Modules/Controller/DataplaneHHTableUpdate.h>
#include <LibSynapse/Modules/Controller/DataplaneHHTableIsIndexAllocated.h>
#include <LibSynapse/Modules/Controller/DataplaneHHTableDelete.h>
#include <LibSynapse/Modules/Controller/DataplaneHHTableOutOfBandUpdate.h>
#include <LibSynapse/Modules/Controller/DataplaneCMSAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneCMSQuery.h>
#include <LibSynapse/Modules/Controller/TokenBucketAllocate.h>
#include <LibSynapse/Modules/Controller/TokenBucketIsTracing.h>
#include <LibSynapse/Modules/Controller/TokenBucketTrace.h>
#include <LibSynapse/Modules/Controller/TokenBucketUpdateAndCheck.h>
#include <LibSynapse/Modules/Controller/TokenBucketExpire.h>
#include <LibSynapse/Modules/Controller/DataplaneMeterAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneMeterInsert.h>
#include <LibSynapse/Modules/Controller/DataplaneIntegerAllocatorAllocate.h>
#include <LibSynapse/Modules/Controller/DataplaneIntegerAllocatorFreeIndex.h>
#include <LibSynapse/Modules/Controller/CMSAllocate.h>
#include <LibSynapse/Modules/Controller/CMSUpdate.h>
#include <LibSynapse/Modules/Controller/CMSQuery.h>
#include <LibSynapse/Modules/Controller/CMSIncrement.h>
#include <LibSynapse/Modules/Controller/CMSCountMin.h>
#include <LibSynapse/Modules/Controller/DataplaneCuckooHashTableAllocate.h>

namespace LibSynapse {
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
              f.push_back(std::make_unique<DataplaneHHTableAllocateFactory>());
              f.push_back(std::make_unique<DataplaneHHTableReadFactory>());
              f.push_back(std::make_unique<DataplaneHHTableUpdateFactory>());
              f.push_back(std::make_unique<DataplaneHHTableIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<DataplaneHHTableDeleteFactory>());
              f.push_back(std::make_unique<DataplaneHHTableOutOfBandUpdateFactory>());
              f.push_back(std::make_unique<DataplaneCMSAllocateFactory>());
              f.push_back(std::make_unique<DataplaneCMSQueryFactory>());
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
              f.push_back(std::make_unique<DataplaneCuckooHashTableAllocateFactory>());
              return f;
            }(),
            std::make_unique<ControllerContext>()) {}
};

} // namespace Controller
} // namespace LibSynapse