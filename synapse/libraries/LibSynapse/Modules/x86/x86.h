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
#include <LibSynapse/Modules/x86/MapGet.h>
#include <LibSynapse/Modules/x86/MapPut.h>
#include <LibSynapse/Modules/x86/MapErase.h>
#include <LibSynapse/Modules/x86/VectorRead.h>
#include <LibSynapse/Modules/x86/VectorWrite.h>
#include <LibSynapse/Modules/x86/DchainRejuvenateIndex.h>
#include <LibSynapse/Modules/x86/DchainAllocateNewIndex.h>
#include <LibSynapse/Modules/x86/DchainIsIndexAllocated.h>
#include <LibSynapse/Modules/x86/DchainFreeIndex.h>
#include <LibSynapse/Modules/x86/CMSCountMin.h>
#include <LibSynapse/Modules/x86/CMSIncrement.h>
#include <LibSynapse/Modules/x86/CMSPeriodicCleanup.h>
#include <LibSynapse/Modules/x86/HashObj.h>
#include <LibSynapse/Modules/x86/ChtFindBackend.h>
#include <LibSynapse/Modules/x86/TokenBucketExpire.h>
#include <LibSynapse/Modules/x86/TokenBucketIsTracing.h>
#include <LibSynapse/Modules/x86/TokenBucketTrace.h>
#include <LibSynapse/Modules/x86/TokenBucketUpdateAndCheck.h>

namespace LibSynapse {
namespace x86 {

struct x86Target : public Target {
  x86Target(const std::string &instance_id)
      : Target(
            TargetType(TargetArchitecture::x86, instance_id),
            [instance_id]() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<IgnoreFactory>(instance_id));
              f.push_back(std::make_unique<ForwardFactory>(instance_id));
              f.push_back(std::make_unique<BroadcastFactory>(instance_id));
              f.push_back(std::make_unique<DropFactory>(instance_id));
              f.push_back(std::make_unique<ParseHeaderFactory>(instance_id));
              f.push_back(std::make_unique<ModifyHeaderFactory>(instance_id));
              f.push_back(std::make_unique<ChecksumUpdateFactory>(instance_id));
              f.push_back(std::make_unique<IfFactory>(instance_id));
              f.push_back(std::make_unique<ThenFactory>(instance_id));
              f.push_back(std::make_unique<ElseFactory>(instance_id));
              f.push_back(std::make_unique<ExpireItemsSingleMapFactory>(instance_id));
              f.push_back(std::make_unique<ExpireItemsSingleMapIterativelyFactory>(instance_id));
              f.push_back(std::make_unique<MapGetFactory>(instance_id));
              f.push_back(std::make_unique<MapPutFactory>(instance_id));
              f.push_back(std::make_unique<MapEraseFactory>(instance_id));
              f.push_back(std::make_unique<VectorReadFactory>(instance_id));
              f.push_back(std::make_unique<VectorWriteFactory>(instance_id));
              f.push_back(std::make_unique<DchainRejuvenateIndexFactory>(instance_id));
              f.push_back(std::make_unique<DchainAllocateNewIndexFactory>(instance_id));
              f.push_back(std::make_unique<DchainIsIndexAllocatedFactory>(instance_id));
              f.push_back(std::make_unique<DchainFreeIndexFactory>(instance_id));
              f.push_back(std::make_unique<CMSCountMinFactory>(instance_id));
              f.push_back(std::make_unique<CMSIncrementFactory>(instance_id));
              f.push_back(std::make_unique<CMSPeriodicCleanupFactory>(instance_id));
              f.push_back(std::make_unique<HashObjFactory>(instance_id));
              f.push_back(std::make_unique<ChtFindBackendFactory>(instance_id));
              f.push_back(std::make_unique<TokenBucketExpireFactory>(instance_id));
              f.push_back(std::make_unique<TokenBucketIsTracingFactory>(instance_id));
              f.push_back(std::make_unique<TokenBucketTraceFactory>(instance_id));
              f.push_back(std::make_unique<TokenBucketUpdateAndCheckFactory>(instance_id));
              return f;
            }(),
            std::make_unique<x86Context>()) {}
};

} // namespace x86
} // namespace LibSynapse