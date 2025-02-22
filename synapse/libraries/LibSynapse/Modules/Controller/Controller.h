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
#include <LibSynapse/Modules/Controller/MapTableAllocate.h>
#include <LibSynapse/Modules/Controller/MapTableLookup.h>
#include <LibSynapse/Modules/Controller/MapTableUpdate.h>
#include <LibSynapse/Modules/Controller/MapTableDelete.h>
#include <LibSynapse/Modules/Controller/VectorTableAllocate.h>
#include <LibSynapse/Modules/Controller/VectorTableLookup.h>
#include <LibSynapse/Modules/Controller/VectorTableUpdate.h>
#include <LibSynapse/Modules/Controller/DchainTableAllocate.h>
#include <LibSynapse/Modules/Controller/DchainTableLookup.h>
#include <LibSynapse/Modules/Controller/DchainTableUpdate.h>
#include <LibSynapse/Modules/Controller/DchainTableDelete.h>
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
#include <LibSynapse/Modules/Controller/VectorRegisterAllocate.h>
#include <LibSynapse/Modules/Controller/VectorRegisterLookup.h>
#include <LibSynapse/Modules/Controller/VectorRegisterUpdate.h>
#include <LibSynapse/Modules/Controller/FCFSCachedTableAllocate.h>
#include <LibSynapse/Modules/Controller/FCFSCachedTableRead.h>
#include <LibSynapse/Modules/Controller/FCFSCachedTableWrite.h>
#include <LibSynapse/Modules/Controller/FCFSCachedTableDelete.h>
#include <LibSynapse/Modules/Controller/HHTableAllocate.h>
#include <LibSynapse/Modules/Controller/HHTableRead.h>
#include <LibSynapse/Modules/Controller/HHTableUpdate.h>
#include <LibSynapse/Modules/Controller/HHTableConditionalUpdate.h>
#include <LibSynapse/Modules/Controller/HHTableDelete.h>
#include <LibSynapse/Modules/Controller/TokenBucketAllocate.h>
#include <LibSynapse/Modules/Controller/TokenBucketIsTracing.h>
#include <LibSynapse/Modules/Controller/TokenBucketTrace.h>
#include <LibSynapse/Modules/Controller/TokenBucketUpdateAndCheck.h>
#include <LibSynapse/Modules/Controller/TokenBucketExpire.h>
#include <LibSynapse/Modules/Controller/MeterAllocate.h>
#include <LibSynapse/Modules/Controller/MeterInsert.h>
#include <LibSynapse/Modules/Controller/IntegerAllocatorAllocate.h>
#include <LibSynapse/Modules/Controller/IntegerAllocatorFreeIndex.h>
#include <LibSynapse/Modules/Controller/CMSAllocate.h>
#include <LibSynapse/Modules/Controller/CMSUpdate.h>
#include <LibSynapse/Modules/Controller/CMSQuery.h>
#include <LibSynapse/Modules/Controller/CMSIncrement.h>
#include <LibSynapse/Modules/Controller/CMSCountMin.h>

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
              f.push_back(std::make_unique<MapTableAllocateFactory>());
              f.push_back(std::make_unique<MapTableLookupFactory>());
              f.push_back(std::make_unique<MapTableUpdateFactory>());
              f.push_back(std::make_unique<MapTableDeleteFactory>());
              f.push_back(std::make_unique<VectorTableAllocateFactory>());
              f.push_back(std::make_unique<VectorTableLookupFactory>());
              f.push_back(std::make_unique<VectorTableUpdateFactory>());
              f.push_back(std::make_unique<DchainTableAllocateFactory>());
              f.push_back(std::make_unique<DchainTableLookupFactory>());
              f.push_back(std::make_unique<DchainTableUpdateFactory>());
              f.push_back(std::make_unique<DchainTableDeleteFactory>());
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
              f.push_back(std::make_unique<VectorRegisterAllocateFactory>());
              f.push_back(std::make_unique<VectorRegisterLookupFactory>());
              f.push_back(std::make_unique<VectorRegisterUpdateFactory>());
              f.push_back(std::make_unique<FCFSCachedTableAllocateFactory>());
              f.push_back(std::make_unique<FCFSCachedTableReadFactory>());
              f.push_back(std::make_unique<FCFSCachedTableWriteFactory>());
              f.push_back(std::make_unique<FCFSCachedTableDeleteFactory>());
              f.push_back(std::make_unique<HHTableAllocateFactory>());
              f.push_back(std::make_unique<HHTableReadFactory>());
              f.push_back(std::make_unique<HHTableUpdateFactory>());
              f.push_back(std::make_unique<HHTableConditionalUpdateFactory>());
              f.push_back(std::make_unique<HHTableDeleteFactory>());
              f.push_back(std::make_unique<TokenBucketAllocateFactory>());
              f.push_back(std::make_unique<TokenBucketIsTracingFactory>());
              f.push_back(std::make_unique<TokenBucketTraceFactory>());
              f.push_back(std::make_unique<TokenBucketUpdateAndCheckFactory>());
              f.push_back(std::make_unique<TokenBucketExpireFactory>());
              f.push_back(std::make_unique<MeterAllocateFactory>());
              f.push_back(std::make_unique<MeterInsertFactory>());
              f.push_back(std::make_unique<IntegerAllocatorAllocateFactory>());
              f.push_back(std::make_unique<IntegerAllocatorFreeIndexFactory>());
              f.push_back(std::make_unique<CMSAllocateFactory>());
              f.push_back(std::make_unique<CMSUpdateFactory>());
              f.push_back(std::make_unique<CMSQueryFactory>());
              f.push_back(std::make_unique<CMSIncrementFactory>());
              f.push_back(std::make_unique<CMSCountMinFactory>());
              return f;
            }(),
            std::make_unique<ControllerContext>()) {}
};

} // namespace Controller
} // namespace LibSynapse