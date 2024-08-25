#include "synthesizer.h"

#define NF_TEMPLATE_FILENAME "nf.template.cpp"
#define PROFILER_TEMPLATE_FILENAME "profiler.template.cpp"

#define MARKER_NF_STATE "NF_STATE"
#define MARKER_NF_INIT "NF_INIT"
#define MARKER_NF_PROCESS "NF_PROCESS"

static std::string template_from_type(BDDSynthesizerTarget target) {
  std::string template_filename;

  switch (target) {
  case BDDSynthesizerTarget::NF:
    template_filename = NF_TEMPLATE_FILENAME;
    break;
  case BDDSynthesizerTarget::PROFILER:
    template_filename = PROFILER_TEMPLATE_FILENAME;
    break;
  }

  return template_filename;
}

BDDSynthesizer::BDDSynthesizer(BDDSynthesizerTarget _target, std::ostream &_out)
    : Synthesizer(template_from_type(_target),
                  {
                      {MARKER_NF_STATE, 0},
                      {MARKER_NF_INIT, 0},
                      {MARKER_NF_PROCESS, 0},
                  },
                  _out),
      target(_target) {}

void BDDSynthesizer::visit(const BDD *bdd) { Synthesizer::dump(); }

BDDVisitorAction BDDSynthesizer::visitBranch(const Branch *node) {
  return BDDVisitorAction::VISIT_CHILDREN;
}
BDDVisitorAction BDDSynthesizer::visitCall(const Call *node) {
  return BDDVisitorAction::VISIT_CHILDREN;
}
BDDVisitorAction BDDSynthesizer::visitRoute(const Route *node) {
  return BDDVisitorAction::VISIT_CHILDREN;
}