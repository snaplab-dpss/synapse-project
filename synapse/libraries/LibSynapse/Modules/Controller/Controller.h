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
#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableDelete.h>
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
  ControllerTarget(const std::string &instance_id)
      : Target(
            TargetType(TargetArchitecture::Controller, instance_id),
            [instance_id]() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<IgnoreFactory>(instance_id));
              f.push_back(std::make_unique<ParseHeaderFactory>(instance_id));
              f.push_back(std::make_unique<ModifyHeaderFactory>(instance_id));
              f.push_back(std::make_unique<ChecksumUpdateFactory>(instance_id));
              f.push_back(std::make_unique<IfFactory>(instance_id));
              f.push_back(std::make_unique<ThenFactory>(instance_id));
              f.push_back(std::make_unique<ElseFactory>(instance_id));
              f.push_back(std::make_unique<ForwardFactory>(instance_id));
              f.push_back(std::make_unique<BroadcastFactory>(instance_id));
              f.push_back(std::make_unique<DropFactory>(instance_id));
              f.push_back(std::make_unique<AbortTransactionFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneMapTableAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneMapTableLookupFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneMapTableUpdateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneMapTableDeleteFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableLookupFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableGuardCheckFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableUpdateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneGuardedMapTableDeleteFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneVectorTableAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneVectorTableLookupFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneVectorTableUpdateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableAllocateNewIndexFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableFreeIndexFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableIsIndexAllocatedFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneDchainTableRefreshIndexFactory>(instance_id));
              f.push_back(std::make_unique<DchainAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DchainAllocateNewIndexFactory>(instance_id));
              f.push_back(std::make_unique<DchainIsIndexAllocatedFactory>(instance_id));
              f.push_back(std::make_unique<DchainRejuvenateIndexFactory>(instance_id));
              f.push_back(std::make_unique<DchainFreeIndexFactory>(instance_id));
              f.push_back(std::make_unique<VectorAllocateFactory>(instance_id));
              f.push_back(std::make_unique<VectorReadFactory>(instance_id));
              f.push_back(std::make_unique<VectorWriteFactory>(instance_id));
              f.push_back(std::make_unique<MapAllocateFactory>(instance_id));
              f.push_back(std::make_unique<MapGetFactory>(instance_id));
              f.push_back(std::make_unique<MapPutFactory>(instance_id));
              f.push_back(std::make_unique<MapEraseFactory>(instance_id));
              f.push_back(std::make_unique<ChtAllocateFactory>(instance_id));
              f.push_back(std::make_unique<ChtFindBackendFactory>(instance_id));
              f.push_back(std::make_unique<HashObjFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneVectorRegisterAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneVectorRegisterLookupFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneVectorRegisterUpdateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneFCFSCachedTableAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneFCFSCachedTableReadFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneFCFSCachedTableWriteFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneFCFSCachedTableDeleteFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneHHTableAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneHHTableReadFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneHHTableUpdateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneHHTableIsIndexAllocatedFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneHHTableDeleteFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneHHTableOutOfBandUpdateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneCMSAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneCMSQueryFactory>(instance_id));
              f.push_back(std::make_unique<TokenBucketAllocateFactory>(instance_id));
              f.push_back(std::make_unique<TokenBucketIsTracingFactory>(instance_id));
              f.push_back(std::make_unique<TokenBucketTraceFactory>(instance_id));
              f.push_back(std::make_unique<TokenBucketUpdateAndCheckFactory>(instance_id));
              f.push_back(std::make_unique<TokenBucketExpireFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneMeterAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneMeterInsertFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneIntegerAllocatorAllocateFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneIntegerAllocatorFreeIndexFactory>(instance_id));
              f.push_back(std::make_unique<CMSAllocateFactory>(instance_id));
              f.push_back(std::make_unique<CMSUpdateFactory>(instance_id));
              f.push_back(std::make_unique<CMSQueryFactory>(instance_id));
              f.push_back(std::make_unique<CMSIncrementFactory>(instance_id));
              f.push_back(std::make_unique<CMSCountMinFactory>(instance_id));
              f.push_back(std::make_unique<DataplaneCuckooHashTableAllocateFactory>(instance_id));
              return f;
            }(),
            std::make_unique<ControllerContext>()) {}
};

} // namespace Controller
} // namespace LibSynapse
