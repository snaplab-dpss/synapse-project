#pragma once

#include <LibBDD/Visitors/Visitor.h>

namespace LibBDD {

class PrinterDebug : public BDDVisitor {
private:
  bool traverse;

public:
  PrinterDebug(bool _traverse) : traverse(_traverse) {}
  PrinterDebug() : PrinterDebug(true) {}

  void visit(const BDD *bdd) override;
  void visitRoot(const BDDNode *root) override;

  BDDVisitor::Action visit(const Branch *node) override;
  BDDVisitor::Action visit(const Call *node) override;
  BDDVisitor::Action visit(const Route *node) override;

  static void debug(const BDDNode *node);

private:
  void visitConstraints(const BDDNode *node);
};

} // namespace LibBDD