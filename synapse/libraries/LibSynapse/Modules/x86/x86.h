#pragma once

#include <LibSynapse/Target.h>
#include <LibSynapse/Modules/x86/x86Context.h>

#include <LibSynapse/Modules/x86/Ignore.h>
#include <LibSynapse/Modules/x86/Forward.h>
#include <LibSynapse/Modules/x86/Broadcast.h>
#include <LibSynapse/Modules/x86/Drop.h>
#include <LibSynapse/Modules/x86/ParseHeader.h>
#include <LibSynapse/Modules/x86/ModifyHeader.h>
#include <LibSynapse/Modules/x86/ChecksumUpdate.h>
#include <LibSynapse/Modules/x86/If.h>
#include <LibSynapse/Modules/x86/Then.h>
#include <LibSynapse/Modules/x86/Else.h>
#include <LibSynapse/Modules/x86/ExpireItemsSingleMap.h>
#include <LibSynapse/Modules/x86/ExpireItemsSingleMapIteratively.h>
#include <LibSynapse/Modules/x86/MapAllocate.h>
#include <LibSynapse/Modules/x86/MapAllocate.h>
#include <LibSynapse/Modules/x86/MapGet.h>
#include <LibSynapse/Modules/x86/MapPut.h>
#include <LibSynapse/Modules/x86/MapErase.h>
#include <LibSynapse/Modules/x86/VectorAllocate.h>
#include <LibSynapse/Modules/x86/VectorAllocate.h>
#include <LibSynapse/Modules/x86/VectorRead.h>
#include <LibSynapse/Modules/x86/VectorWrite.h>
#include <LibSynapse/Modules/x86/DchainAllocate.h>
#include <LibSynapse/Modules/x86/DchainAllocate.h>
#include <LibSynapse/Modules/x86/DchainRejuvenateIndex.h>
#include <LibSynapse/Modules/x86/DchainAllocateNewIndex.h>
#include <LibSynapse/Modules/x86/DchainIsIndexAllocated.h>
#include <LibSynapse/Modules/x86/DchainFreeIndex.h>
#include <LibSynapse/Modules/x86/CMSAllocate.h>
#include <LibSynapse/Modules/x86/CMSAllocate.h>
#include <LibSynapse/Modules/x86/CMSCountMin.h>
#include <LibSynapse/Modules/x86/CMSIncrement.h>
#include <LibSynapse/Modules/x86/CMSPeriodicCleanup.h>
#include <LibSynapse/Modules/x86/HashObj.h>
#include <LibSynapse/Modules/x86/ChtFindBackend.h>
#include <LibSynapse/Modules/x86/SendToDevice.h>
#include <LibSynapse/Modules/x86/TokenBucketAllocate.h>
#include <LibSynapse/Modules/x86/SendToDevice.h>
#include <LibSynapse/Modules/x86/TokenBucketAllocate.h>
#include <LibSynapse/Modules/x86/TokenBucketExpire.h>
#include <LibSynapse/Modules/x86/TokenBucketIsTracing.h>
#include <LibSynapse/Modules/x86/TokenBucketTrace.h>
#include <LibSynapse/Modules/x86/TokenBucketUpdateAndCheck.h>
#include <LibSynapse/Modules/x86/LPMAllocate.h>
#include <LibSynapse/Modules/x86/LPMAllocate.h>

namespace LibSynapse {
namespace x86 {

struct x86Target : public Target {
  x86Target(const InstanceId _instance_id)
      : Target(
            TargetType(TargetArchitecture::x86, _instance_id),
            [_instance_id]() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<IgnoreFactory>(_instance_id));
              f.push_back(std::make_unique<ForwardFactory>(_instance_id));
              f.push_back(std::make_unique<BroadcastFactory>(_instance_id));
              f.push_back(std::make_unique<DropFactory>(_instance_id));
              f.push_back(std::make_unique<ParseHeaderFactory>(_instance_id));
              f.push_back(std::make_unique<ModifyHeaderFactory>(_instance_id));
              f.push_back(std::make_unique<ChecksumUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<IfFactory>(_instance_id));
              f.push_back(std::make_unique<ThenFactory>(_instance_id));
              f.push_back(std::make_unique<ElseFactory>(_instance_id));
              f.push_back(std::make_unique<ExpireItemsSingleMapFactory>(_instance_id));
              f.push_back(std::make_unique<ExpireItemsSingleMapIterativelyFactory>(_instance_id));
              f.push_back(std::make_unique<MapAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<MapGetFactory>(_instance_id));
              f.push_back(std::make_unique<MapPutFactory>(_instance_id));
              f.push_back(std::make_unique<MapEraseFactory>(_instance_id));
              f.push_back(std::make_unique<VectorAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<VectorReadFactory>(_instance_id));
              f.push_back(std::make_unique<VectorWriteFactory>(_instance_id));
              f.push_back(std::make_unique<DchainAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<DchainRejuvenateIndexFactory>(_instance_id));
              f.push_back(std::make_unique<DchainAllocateNewIndexFactory>(_instance_id));
              f.push_back(std::make_unique<DchainIsIndexAllocatedFactory>(_instance_id));
              f.push_back(std::make_unique<DchainFreeIndexFactory>(_instance_id));
              f.push_back(std::make_unique<CMSAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<CMSCountMinFactory>(_instance_id));
              f.push_back(std::make_unique<CMSIncrementFactory>(_instance_id));
              f.push_back(std::make_unique<CMSPeriodicCleanupFactory>(_instance_id));
              f.push_back(std::make_unique<HashObjFactory>(_instance_id));
              f.push_back(std::make_unique<ChtFindBackendFactory>(_instance_id));
              f.push_back(std::make_unique<SendToDeviceFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketAllocateFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketExpireFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketIsTracingFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketTraceFactory>(_instance_id));
              f.push_back(std::make_unique<TokenBucketUpdateAndCheckFactory>(_instance_id));
              f.push_back(std::make_unique<LPMAllocateFactory>(_instance_id));
              return f;
            }(),
            std::make_unique<x86Context>()) {}
};

} // namespace x86
} // namespace LibSynapse
