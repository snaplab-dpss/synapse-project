#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// Asks the meter, in the controller, whether a key is tracked RIGHT NOW. Only
// build_initial_controller_logic builds one of these: it replaces the answer the data plane got
// when it sent the packet up, which by then may be out of date. See the comment there.
class DataplaneMeterIsTracing : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::optional<symbol_t> is_tracing;

public:
  DataplaneMeterIsTracing(const BDDNode *_node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys, std::optional<symbol_t> _is_tracing)
      : ControllerModule(ModuleType::Controller_DataplaneMeterIsTracing, "DataplaneMeterIsTracing", _node), obj(_obj), keys(_keys),
        is_tracing(_is_tracing) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneMeterIsTracing(node, obj, keys, is_tracing);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  std::optional<symbol_t> get_is_tracing() const { return is_tracing; }
};

} // namespace Controller
} // namespace LibSynapse
