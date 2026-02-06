#pragma once

#include <LibTessera/Target.h>
#include <LibTessera/Modules/x86/x86Context.h>

#include <LibTessera/Modules/x86/Ignore.h>
#include <LibTessera/Modules/x86/Forward.h>
#include <LibTessera/Modules/x86/Broadcast.h>
#include <LibTessera/Modules/x86/Drop.h>
#include <LibTessera/Modules/x86/ParseHeader.h>
#include <LibTessera/Modules/x86/ModifyHeader.h>
#include <LibTessera/Modules/x86/ChecksumUpdate.h>
#include <LibTessera/Modules/x86/If.h>
#include <LibTessera/Modules/x86/Then.h>
#include <LibTessera/Modules/x86/Else.h>
#include <LibTessera/Modules/x86/ExpireItemsSingleMap.h>
#include <LibTessera/Modules/x86/ExpireItemsSingleMapIteratively.h>
#include <LibTessera/Modules/x86/MapGet.h>
#include <LibTessera/Modules/x86/MapPut.h>
#include <LibTessera/Modules/x86/MapErase.h>
#include <LibTessera/Modules/x86/VectorRead.h>
#include <LibTessera/Modules/x86/VectorWrite.h>
#include <LibTessera/Modules/x86/DchainRejuvenateIndex.h>
#include <LibTessera/Modules/x86/DchainAllocateNewIndex.h>
#include <LibTessera/Modules/x86/DchainIsIndexAllocated.h>
#include <LibTessera/Modules/x86/DchainFreeIndex.h>
#include <LibTessera/Modules/x86/CMSCountMin.h>
#include <LibTessera/Modules/x86/CMSIncrement.h>
#include <LibTessera/Modules/x86/CMSPeriodicCleanup.h>
#include <LibTessera/Modules/x86/HashObj.h>
#include <LibTessera/Modules/x86/ChtFindBackend.h>
#include <LibTessera/Modules/x86/TokenBucketExpire.h>
#include <LibTessera/Modules/x86/TokenBucketIsTracing.h>
#include <LibTessera/Modules/x86/TokenBucketTrace.h>
#include <LibTessera/Modules/x86/TokenBucketUpdateAndCheck.h>

namespace LibTessera {
namespace x86 {

struct x86Target : public Target {
  x86Target()
      : Target(
            TargetType::x86,
            []() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<IgnoreFactory>());
              f.push_back(std::make_unique<ForwardFactory>());
              f.push_back(std::make_unique<BroadcastFactory>());
              f.push_back(std::make_unique<DropFactory>());
              f.push_back(std::make_unique<ParseHeaderFactory>());
              f.push_back(std::make_unique<ModifyHeaderFactory>());
              f.push_back(std::make_unique<ChecksumUpdateFactory>());
              f.push_back(std::make_unique<IfFactory>());
              f.push_back(std::make_unique<ThenFactory>());
              f.push_back(std::make_unique<ElseFactory>());
              f.push_back(std::make_unique<ExpireItemsSingleMapFactory>());
              f.push_back(std::make_unique<ExpireItemsSingleMapIterativelyFactory>());
              f.push_back(std::make_unique<MapGetFactory>());
              f.push_back(std::make_unique<MapPutFactory>());
              f.push_back(std::make_unique<MapEraseFactory>());
              f.push_back(std::make_unique<VectorReadFactory>());
              f.push_back(std::make_unique<VectorWriteFactory>());
              f.push_back(std::make_unique<DchainRejuvenateIndexFactory>());
              f.push_back(std::make_unique<DchainAllocateNewIndexFactory>());
              f.push_back(std::make_unique<DchainIsIndexAllocatedFactory>());
              f.push_back(std::make_unique<DchainFreeIndexFactory>());
              f.push_back(std::make_unique<CMSCountMinFactory>());
              f.push_back(std::make_unique<CMSIncrementFactory>());
              f.push_back(std::make_unique<CMSPeriodicCleanupFactory>());
              f.push_back(std::make_unique<HashObjFactory>());
              f.push_back(std::make_unique<ChtFindBackendFactory>());
              f.push_back(std::make_unique<TokenBucketExpireFactory>());
              f.push_back(std::make_unique<TokenBucketIsTracingFactory>());
              f.push_back(std::make_unique<TokenBucketTraceFactory>());
              f.push_back(std::make_unique<TokenBucketUpdateAndCheckFactory>());
              return f;
            }(),
            std::make_unique<x86Context>()) {}
};

} // namespace x86
} // namespace LibTessera