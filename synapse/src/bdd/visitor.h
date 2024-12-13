#pragma once

class BDD;
class Node;
class Branch;
class Call;
class Route;

class BDDVisitor {
public:
  enum class Action { Continue, Stop };

  virtual void visit(const BDD *bdd);
  void visit(const Node *node);

protected:
  virtual Action visit(const Branch *node) = 0;
  virtual Action visit(const Call *node) = 0;
  virtual Action visit(const Route *node) = 0;
  virtual void visitRoot(const Node *root);
};
