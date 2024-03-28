#pragma once

#include "node.h"

namespace BDD {

class ReturnRaw : public Node {
private:
  std::vector<calls_t> calls_list;

public:
  ReturnRaw(node_id_t _id, const klee::ConstraintManager &_constraints,
            std::vector<calls_t> _calls_list)
      : Node(_id, Node::NodeType::RETURN_RAW, _constraints),
        calls_list(_calls_list) {}

  ReturnRaw(node_id_t _id, const Node_ptr &_prev,
            const klee::ConstraintManager &_constraints,
            std::vector<calls_t> _calls_list)
      : Node(_id, Node::NodeType::RETURN_RAW, nullptr, _prev, _constraints),
        calls_list(_calls_list) {}

  std::vector<calls_t> get_calls() const { return calls_list; }

  virtual Node_ptr clone(bool recursive = false) const;
  virtual void recursive_update_ids(node_id_t &new_id) override;

  void visit(BDDVisitor &visitor) const override;
  std::string dump(bool one_liner = false) const;
};

} // namespace BDD