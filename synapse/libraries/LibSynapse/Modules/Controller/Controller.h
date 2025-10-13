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
  ControllerTarget(const InstanceId _instance_id)
      : Target(
            TargetType(TargetArchitecture::Controller, instance_id),
            [instance_id]() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<IgnoreFactory>(_instance_id));
              f.push_back(std::make_unique<ParseHeaderFactory>(_instance_id));
              f.push_back(std::make_unique<ModifyHeaderFactory>(_instance_id));
              f.push_back(std::make_unique<ChecksumUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<IfFactory>(_instance_id));
              f.push_back(std::make_unique<ThenFactory>(_instance_id));
              f.push_back(std::make_unique<ElseFactory>(_instance_id));
              f.push_back(std::make_unique<ForwardFactory>(_instance_id));
              f.push_back(std::make_unique<BroadcastFactory>(_instance_id));
              f.push_back(std::make_unique<DropFactory>(_instance_id));
              f.push_back(std::make_unique<AbortTransactionFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneMapTableAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneMapTableLookupFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneMapTableUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneMapTableDeleteFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableLookupFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableGuardCheckFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableDeleteFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneVectorTableAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneVectorTableLookupFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneVectorTableUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableAllocateNewIndexFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableFreeIndexFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableIsIndexAllocatedFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableRefreshIndexFactory>(_instance_id));
              f.push_back(std::make_unique<DchainAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DchainAllocateNewIndexFactory>(_instance_id));
              f.push_back(std::make_unique<DchainIsIndexAllocatedFactory>(_instance_id));
              f.push_back(std::make_unique<DchainRejuvenateIndexFactory>(_instance_id));
              f.push_back(std::make_unique<DchainFreeIndexFactory>(_instance_id));
              f.push_back(std::make_unique<VectorAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<VectorReadFactory>(_instance_id));
              f.push_back(std::make_unique<VectorWriteFactory>(_instance_id));
              f.push_back(std::make_unique<MapAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<MapGetFactory>(_instance_id));
              f.push_back(std::make_unique<MapPutFactory>(_instance_id));
              f.push_back(std::make_unique<MapEraseFactory>(_instance_id));
              f.push_back(std::make_unique<ChtAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<ChtFindBackendFactory>(_instance_id));
              f.push_back(std::make_unique<HashObjFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneVectorRegisterAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneVectorRegisterLookupFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneVectorRegisterUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneFCFSCachedTableAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneFCFSCachedTableReadFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneFCFSCachedTableWriteFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneHHTableAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneHHTableReadFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneHHTableUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneHHTableIsIndexAllocatedFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneHHTableDeleteFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneHHTableOutOfBandUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneCMSAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneCMSQueryFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketIsTracingFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketTraceFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketUpdateAndCheckFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketExpireFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneMeterAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneMeterInsertFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneIntegerAllocatorAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneIntegerAllocatorFreeIndexFactory>(_instance_id));
              f.push_back(std::make_unique<CMSAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<CMSUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<CMSQueryFactory>(_instance_id));
              f.push_back(std::make_unique<CMSIncrementFactory>(_instance_id));
              f.push_back(std::make_unique<CMSCountMinFactory>(_instance_id));
              f.push_back(std::make_unique<DataplaneCuckooHashTableAllocateFactory>(_instance_id));
              return f;
            }(),
            std::make_unique<ControllerContext>()) {}
};

} // namespace Controller
} // namespace LibSynapse
