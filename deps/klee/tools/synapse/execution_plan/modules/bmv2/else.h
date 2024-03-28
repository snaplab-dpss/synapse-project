#pragma once

#include "../module.h"

namespace synapse {
namespace targets {
namespace bmv2 {

class Else : public Module {
public:
  Else() : Module(ModuleType::BMv2_Else, TargetType::BMv2, "Else") {}
  Else(BDD::Node_ptr node)
      : Module(ModuleType::BMv2_Else, TargetType::BMv2, "Else", node) {}

private:
  processing_result_t process(const ExecutionPlan &ep,
                              BDD::Node_ptr node) override {
    processing_result_t result;
    return result;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor,
                     const ExecutionPlanNode *ep_node) const override {
    visitor.visit(ep_node, this);
  }

  virtual Module_ptr clone() const override {
    auto cloned = new Else(node);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    return other->get_type() == type;
  }
};
} // namespace bmv2
} // namespace targets
} // namespace synapse
