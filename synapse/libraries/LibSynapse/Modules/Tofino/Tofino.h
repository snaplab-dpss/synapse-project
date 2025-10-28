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
  TofinoTarget(const tna_config_t &tna_config, const InstanceId _instance_id)
      : Target(
            TargetType(TargetArchitecture::Tofino, _instance_id),
            [_instance_id]() -> std::vector<std::unique_ptr<ModuleFactory>> {
              std::vector<std::unique_ptr<ModuleFactory>> f;
              f.push_back(std::make_unique<RecirculateFactory>(_instance_id));
              f.push_back(std::make_unique<ForwardFactory>(_instance_id));
              f.push_back(std::make_unique<DropFactory>(_instance_id));
              f.push_back(std::make_unique<BroadcastFactory>(_instance_id));
              f.push_back(std::make_unique<IgnoreFactory>(_instance_id));
              f.push_back(std::make_unique<IfFactory>(_instance_id));
              f.push_back(std::make_unique<ThenFactory>(_instance_id));
              f.push_back(std::make_unique<ElseFactory>(_instance_id));
              f.push_back(std::make_unique<ParserExtractionFactory>(_instance_id));
              f.push_back(std::make_unique<ParserConditionFactory>(_instance_id));
              f.push_back(std::make_unique<ParserRejectFactory>(_instance_id));
              f.push_back(std::make_unique<ModifyHeaderFactory>(_instance_id));
              f.push_back(std::make_unique<MapTableLookupFactory>(_instance_id));
              f.push_back(std::make_unique<GuardedMapTableLookupFactory>(_instance_id));
              f.push_back(std::make_unique<GuardedMapTableGuardCheckFactory>(_instance_id));
              f.push_back(std::make_unique<DchainTableLookupFactory>(_instance_id));
              f.push_back(std::make_unique<VectorTableLookupFactory>(_instance_id));
              f.push_back(std::make_unique<VectorRegisterLookupFactory>(_instance_id));
              f.push_back(std::make_unique<VectorRegisterUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<FCFSCachedTableReadFactory>(_instance_id));
              f.push_back(std::make_unique<FCFSCachedTableReadWriteFactory>(_instance_id));
              f.push_back(std::make_unique<FCFSCachedTableWriteFactory>(_instance_id));
              f.push_back(std::make_unique<MeterUpdateFactory>(_instance_id));
              f.push_back(std::make_unique<HHTableReadFactory>(_instance_id));
              f.push_back(std::make_unique<HHTableOutOfBandUpdateFactory>(_instance_id));
              // f.push_back(std::make_unique<IntegerAllocatorAllocateFactory>(_instance_id));
              // f.push_back(std::make_unique<IntegerAllocatorIsAllocatedFactory>(_instance_id));
              // f.push_back(std::make_unique<IntegerAllocatorRejuvenateFactory>(_instance_id));
              f.push_back(std::make_unique<CMSQueryFactory>(_instance_id));
              f.push_back(std::make_unique<CMSIncrementFactory>(_instance_id));
              f.push_back(std::make_unique<CMSIncAndQueryFactory>(_instance_id));
              f.push_back(std::make_unique<LPMLookupFactory>(_instance_id));
              f.push_back(std::make_unique<CuckooHashTableReadWriteFactory>(_instance_id));
              f.push_back(std::make_unique<SendToControllerFactory>(_instance_id));
              return f;
            }(),
            std::make_unique<TofinoContext>(tna_config)) {}
};

} // namespace Tofino
} // namespace LibSynapse
