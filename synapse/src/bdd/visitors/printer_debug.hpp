#pragma once

#include "../visitor.hpp"

namespace synapse {
class PrinterDebug : public BDDVisitor {
private:
  bool traverse;

public:
  PrinterDebug(bool _traverse) : traverse(_traverse) {}
  PrinterDebug() : PrinterDebug(true) {}

  void visit(const BDD *bdd) override;
  void visitRoot(const Node *root) override;

  BDDVisitor::Action visit(const Branch *node) override;
  BDDVisitor::Action visit(const Call *node) override;
  BDDVisitor::Action visit(const Route *node) override;

  static void debug(const Node *node);

private:
  void visitConstraints(const Node *node);
};
} // namespace synapse