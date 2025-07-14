#pragma once

#include <LibBDD/Nodes/Node.h>

namespace LibBDD {

enum class RouteOp { Forward, Drop, Broadcast };

class Route : public BDDNode {
private:
  RouteOp operation;
  klee::ref<klee::Expr> dst_device;

public:
  Route(bdd_node_id_t _id, const klee::ConstraintManager &_constraints, SymbolManager *_symbol_manager, RouteOp _operation,
        klee::ref<klee::Expr> _dst_device)
      : BDDNode(_id, BDDNodeType::Route, _constraints, _symbol_manager), operation(_operation), dst_device(_dst_device) {}

  Route(bdd_node_id_t _id, const klee::ConstraintManager &_constraints, SymbolManager *_symbol_manager, RouteOp _operation)
      : BDDNode(_id, BDDNodeType::Route, _constraints, _symbol_manager), operation(_operation) {
    assert((operation == RouteOp::Drop || operation == RouteOp::Broadcast) && "Invalid operation");
  }

  Route(bdd_node_id_t _id, BDDNode *_next, BDDNode *_prev, const klee::ConstraintManager &_constraints, SymbolManager *_symbol_manager,
        RouteOp _operation, klee::ref<klee::Expr> _dst_device)
      : BDDNode(_id, BDDNodeType::Route, _next, _prev, _constraints, _symbol_manager), operation(_operation), dst_device(_dst_device) {}

  RouteOp get_operation() const { return operation; }
  klee::ref<klee::Expr> get_dst_device() const { return dst_device; }

  void set_dst_device(klee::ref<klee::Expr> _dst_device) { dst_device = _dst_device; }

  virtual BDDNode *clone(BDDNodeManager &manager, bool recursive = false) const override;
  std::string dump(bool one_liner = false, bool id_name_only = false) const;
};

} // namespace LibBDD