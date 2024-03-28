#pragma once

#include "node.h"
#include "return_raw.h"

namespace BDD {
class ReturnInit : public Node {
public:
  enum ReturnType { SUCCESS, FAILURE };

private:
  ReturnType value;

  void fill_return_value(calls_t calls) {
    assert(calls.size());

    auto start_time_finder = [](call_t call) -> bool {
      return call.function_name == "start_time";
    };

    auto start_time_it =
        std::find_if(calls.begin(), calls.end(), start_time_finder);
    auto found = (start_time_it != calls.end());

    value = found ? SUCCESS : FAILURE;
  }

public:
  ReturnInit(node_id_t _id, const klee::ConstraintManager &_constraints)
      : Node(_id, Node::NodeType::RETURN_INIT, _constraints), value(SUCCESS) {}

  ReturnInit(node_id_t _id, const ReturnRaw *raw)
      : Node(_id, Node::NodeType::RETURN_INIT, nullptr, nullptr,
             raw->constraints) {
    auto calls_list = raw->get_calls();
    assert(calls_list.size());
    fill_return_value(calls_list[0]);
  }

  ReturnInit(node_id_t _id, const Node_ptr &_prev,
             const klee::ConstraintManager &_constraints, ReturnType _value)
      : Node(_id, Node::NodeType::RETURN_INIT, nullptr, _prev, _constraints),
        value(_value) {}

  ReturnType get_return_value() const { return value; }

  virtual Node_ptr clone(bool recursive = false) const;
  virtual void recursive_update_ids(node_id_t &new_id) override;

  void visit(BDDVisitor &visitor) const override;
  std::string dump(bool one_liner = false) const;
};
} // namespace BDD