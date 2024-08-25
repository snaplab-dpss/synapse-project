#pragma once

#include <filesystem>
#include <stack>
#include <optional>

#include "../../bdd/bdd.h"
#include "../synthesizer.h"

enum class BDDSynthesizerTarget { NF, PROFILER };

class BDDSynthesizer : public Synthesizer, public BDDVisitor {
private:
  BDDSynthesizerTarget target;

public:
  BDDSynthesizer(BDDSynthesizerTarget _target, std::ostream &_out);

  void visit(const BDD *bdd) override;

  BDDVisitorAction visitBranch(const Branch *node) override;
  BDDVisitorAction visitCall(const Call *node) override;
  BDDVisitorAction visitRoute(const Route *node) override;
};
