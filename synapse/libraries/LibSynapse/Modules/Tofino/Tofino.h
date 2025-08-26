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
#include <LibSynapse/Modules/Tofino/GuardedMapTableLookup.h>
#include <LibSynapse/Modules/Tofino/GuardedMapTableGuardCheck.h>
#include <LibSynapse/Modules/Tofino/VectorTableLookup.h>
#include <LibSynapse/Modules/Tofino/DchainTableLookup.h>
#include <LibSynapse/Modules/Tofino/VectorRegisterLookup.h>
#include <LibSynapse/Modules/Tofino/VectorRegisterUpdate.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableRead.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableReadWrite.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableWrite.h>
#include <LibSynapse/Modules/Tofino/FCFSCachedTableDelete.h>
#include <LibSynapse/Modules/Tofino/MeterUpdate.h>
#include <LibSynapse/Modules/Tofino/HHTableRead.h>
#include <LibSynapse/Modules/Tofino/HHTableOutOfBandUpdate.h>
#include <LibSynapse/Modules/Tofino/IntegerAllocatorAllocate.h>
#include <LibSynapse/Modules/Tofino/IntegerAllocatorIsAllocated.h>
#include <LibSynapse/Modules/Tofino/IntegerAllocatorRejuvenate.h>
#include <LibSynapse/Modules/Tofino/CMSQuery.h>
#include <LibSynapse/Modules/Tofino/CMSIncrement.h>
#include <LibSynapse/Modules/Tofino/CMSIncAndQuery.h>
#include <LibSynapse/Modules/Tofino/LPMLookup.h>
#include <LibSynapse/Modules/Tofino/CuckooHashTableReadWrite.h>

namespace LibSynapse {
namespace Tofino {

struct TofinoTarget : public Target {
  TofinoTarget(const tna_config_t &tna_config, const std::string &instance_id)
      : Target(
            TargetType(TargetArchitecture::Tofino, instance_id),
            [instance_id]() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<RecirculateFactory>(instance_id));
              f.push_back(std::make_unique<ForwardFactory>(instance_id));
              f.push_back(std::make_unique<DropFactory>(instance_id));
              f.push_back(std::make_unique<BroadcastFactory>(instance_id));
              f.push_back(std::make_unique<IgnoreFactory>(instance_id));
              f.push_back(std::make_unique<IfFactory>(instance_id));
              f.push_back(std::make_unique<ThenFactory>(instance_id));
              f.push_back(std::make_unique<ElseFactory>(instance_id));
              f.push_back(std::make_unique<ParserExtractionFactory>(instance_id));
              f.push_back(std::make_unique<ParserConditionFactory>(instance_id));
              f.push_back(std::make_unique<ParserRejectFactory>(instance_id));
              f.push_back(std::make_unique<ModifyHeaderFactory>(instance_id));
              f.push_back(std::make_unique<MapTableLookupFactory>(instance_id));
              f.push_back(std::make_unique<GuardedMapTableLookupFactory>(instance_id));
              f.push_back(std::make_unique<GuardedMapTableGuardCheckFactory>(instance_id));
              f.push_back(std::make_unique<DchainTableLookupFactory>(instance_id));
              f.push_back(std::make_unique<VectorTableLookupFactory>(instance_id));
              f.push_back(std::make_unique<VectorRegisterLookupFactory>(instance_id));
              f.push_back(std::make_unique<VectorRegisterUpdateFactory>(instance_id));
              f.push_back(std::make_unique<FCFSCachedTableReadFactory>(instance_id));
              f.push_back(std::make_unique<FCFSCachedTableReadWriteFactory>(instance_id));
              f.push_back(std::make_unique<FCFSCachedTableWriteFactory>(instance_id));
              f.push_back(std::make_unique<FCFSCachedTableDeleteFactory>(instance_id));
              f.push_back(std::make_unique<MeterUpdateFactory>(instance_id));
              f.push_back(std::make_unique<HHTableReadFactory>(instance_id));
              f.push_back(std::make_unique<HHTableOutOfBandUpdateFactory>(instance_id));
              // f.push_back(std::make_unique<IntegerAllocatorAllocateFactory>());
              // f.push_back(std::make_unique<IntegerAllocatorIsAllocatedFactory>());
              // f.push_back(std::make_unique<IntegerAllocatorRejuvenateFactory>());
              f.push_back(std::make_unique<CMSQueryFactory>(instance_id));
              f.push_back(std::make_unique<CMSIncrementFactory>(instance_id));
              f.push_back(std::make_unique<CMSIncAndQueryFactory>(instance_id));
              f.push_back(std::make_unique<LPMLookupFactory>(instance_id));
              f.push_back(std::make_unique<CuckooHashTableReadWriteFactory>(instance_id));
              f.push_back(std::make_unique<SendToControllerFactory>(instance_id));
              return f;
            }(),
            std::make_unique<TofinoContext>(tna_config)) {}
};

} // namespace Tofino
} // namespace LibSynapse
