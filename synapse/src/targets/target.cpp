#include <assert.h>
#include <sstream>

#include "target.h"
#include "module.h"
#include "module_generator.h"

Target::Target(TargetType _type,
               const std::vector<ModuleGenerator *> &_module_generators,
               const TargetContext *_ctx)
    : type(_type), module_generators(_module_generators), ctx(_ctx) {}

Target::~Target() {
  for (ModuleGenerator *mg : module_generators) {
    if (mg) {
      delete mg;
      mg = nullptr;
    }
  }

  if (ctx) {
    delete ctx;
    ctx = nullptr;
  }
}

std::ostream &operator<<(std::ostream &os, TargetType target) {
  switch (target) {
  case TargetType::TofinoCPU:
    os << "Ctrl";
    break;
  case TargetType::Tofino:
    os << "Tofino";
    break;
  case TargetType::x86:
    os << "x86";
    break;
  }
  return os;
}

std::string to_string(TargetType target) {
  std::stringstream ss;
  ss << target;
  return ss.str();
}