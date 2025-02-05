#include <LibSynapse/Target.h>
#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Modules/x86/x86.h>
#include <LibSynapse/Modules/Controller/Controller.h>
#include <LibSynapse/Modules/Tofino/Tofino.h>

#include <sstream>

namespace LibSynapse {

TargetView::TargetView(TargetType _type, std::vector<const ModuleFactory *> _module_factories, const TargetContext *_base_ctx)
    : type(_type), module_factories(_module_factories), base_ctx(_base_ctx) {}

Target::Target(TargetType _type, std::vector<std::unique_ptr<ModuleFactory>> _module_factories, std::unique_ptr<TargetContext> _base_ctx)
    : type(_type), module_factories(std::move(_module_factories)), base_ctx(std::move(_base_ctx)) {}

TargetView Target::get_view() const {
  std::vector<const ModuleFactory *> factories;
  for (const std::unique_ptr<ModuleFactory> &factory : module_factories) {
    factories.push_back(factory.get());
  }
  return TargetView(type, factories, base_ctx.get());
}

std::ostream &operator<<(std::ostream &os, TargetType target) {
  switch (target) {
  case TargetType::Controller:
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

TargetsView::TargetsView(const std::vector<TargetView> &_elements) : elements(_elements) {}

TargetView TargetsView::get_initial_target() const {
  assert(!elements.empty() && "No targets to get the initial target from.");
  return elements[0];
}

Targets::Targets(const toml::table &config) {
  elements.push_back(std::move(std::make_unique<Tofino::TofinoTarget>(config)));
  // elements.push_back(std::move(std::make_unique<Controller::ControllerTarget>()));
  elements.push_back(std::move(std::make_unique<x86::x86Target>()));
}

TargetsView Targets::get_view() const {
  std::vector<TargetView> views;
  for (const std::unique_ptr<Target> &element : elements) {
    views.push_back(element->get_view());
  }
  return TargetsView(views);
}

} // namespace LibSynapse