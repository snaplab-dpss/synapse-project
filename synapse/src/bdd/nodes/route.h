#pragma once

#include "node.h"
#include "../../system.h"

namespace synapse {
enum class RouteOp { Forward, Drop, Broadcast };

class Route : public Node {
private:
  RouteOp operation;
  int dst_device;

public:
  Route(node_id_t _id, const klee::ConstraintManager &_constraints, SymbolManager *_symbol_manager, RouteOp _operation, int _dst_device)
      : Node(_id, NodeType::Route, _constraints, _symbol_manager), operation(_operation), dst_device(_dst_device) {}

  Route(node_id_t _id, const klee::ConstraintManager &_constraints, SymbolManager *_symbol_manager, RouteOp _operation)
      : Node(_id, NodeType::Route, _constraints, _symbol_manager), operation(_operation) {
    assert((operation == RouteOp::Drop || operation == RouteOp::Broadcast) && "Invalid operation");
  }

  Route(node_id_t _id, Node *_next, Node *_prev, const klee::ConstraintManager &_constraints, SymbolManager *_symbol_manager,
        RouteOp _operation, int _dst_device)
      : Node(_id, NodeType::Route, _next, _prev, _constraints, _symbol_manager), operation(_operation), dst_device(_dst_device) {}

  int get_dst_device() const { return dst_device; }
  RouteOp get_operation() const { return operation; }

  virtual Node *clone(NodeManager &manager, bool recursive = false) const override;

  std::string dump(bool one_liner = false, bool id_name_only = false) const;
};
} // namespace synapse