#pragma once

#include "../visitor.h"

class PrinterDebug : public BDDVisitor {
private:
  bool traverse;

public:
  PrinterDebug(bool _traverse) : traverse(_traverse) {}
  PrinterDebug() : PrinterDebug(true) {}

  void visit(const BDD *bdd) override;
  void visitRoot(const Node *root) override;

  BDDVisitorAction visitBranch(const Branch *node) override;
  BDDVisitorAction visitCall(const Call *node) override;
  BDDVisitorAction visitRoute(const Route *node) override;

  static void debug(const Node *node);

private:
  void visitConstraints(const Node *node);
};
