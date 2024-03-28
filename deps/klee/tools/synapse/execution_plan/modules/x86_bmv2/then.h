#pragma once

#include "../module.h"

#include "else.h"

namespace synapse {
namespace targets {
namespace x86_bmv2 {

class Then : public Module {
public:
  Then() : Module(ModuleType::x86_BMv2_Then, TargetType::x86_BMv2, "Then") {}

  Then(BDD::Node_ptr node)
      : Module(ModuleType::x86_BMv2_Then, TargetType::x86_BMv2, "Then", node) {}

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
    auto cloned = new Then(node);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    return other->get_type() == type;
  }
};
} // namespace x86_bmv2
} // namespace targets
} // namespace synapse
