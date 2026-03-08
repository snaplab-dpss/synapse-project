#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>
#include <klee/util/Ref.h>

namespace LibSynapse {
namespace x86 {
class SendToDevice : public x86Module {
private:
  klee::ref<klee::Expr> outgoing_port;
  klee::ref<klee::Expr> code_path;
  Symbols symbols;

public:
  SendToDevice(const BDDNode *_node, klee::ref<klee::Expr> _outgoing_port, klee::ref<klee::Expr> _code_path, Symbols _symbols)
      : x86Module(ModuleType::x86_SendToDevice, "SendToDevice", _node), outgoing_port(_outgoing_port), code_path(_code_path), symbols(_symbols) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new SendToDevice(node, outgoing_port, code_path, symbols); }

  const klee::ref<klee::Expr> get_outgoing_port() const { return outgoing_port; }
  const klee::ref<klee::Expr> get_code_path() const { return code_path; }
  const Symbols &get_symbols() const { return symbols; }
};

class SendToDeviceFactory : public x86ModuleFactory {
public:
  SendToDeviceFactory() : x86ModuleFactory(ModuleType::x86_SendToDevice, "SendToDevice") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
