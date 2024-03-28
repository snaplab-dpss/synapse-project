#pragma once

#include "../module.h"

namespace synapse {
namespace targets {
namespace x86_bmv2 {

class PacketGetMetadata : public Module {
private:
  klee::ref<klee::Expr> metadata;

public:
  PacketGetMetadata()
      : Module(ModuleType::x86_BMv2_PacketGetMetadata, TargetType::x86_BMv2,
               "PacketGetMetadata") {}

  PacketGetMetadata(BDD::Node_ptr node, klee::ref<klee::Expr> _metadata)
      : Module(ModuleType::x86_BMv2_PacketGetMetadata, TargetType::x86_BMv2,
               "PacketGetMetadata", node),
        metadata(_metadata) {}

private:
  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    return processing_result_t();
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor,
                     const ExecutionPlanNode *ep_node) const override {
    visitor.visit(ep_node, this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new PacketGetMetadata(node, metadata);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const PacketGetMetadata *>(other);

    if (!kutil::solver_toolbox.are_exprs_always_equal(metadata,
                                                      other_cast->metadata)) {
      return false;
    }

    return true;
  }

  const klee::ref<klee::Expr> &get_metadata() const { return metadata; }
};
} // namespace x86_bmv2
} // namespace targets
} // namespace synapse
