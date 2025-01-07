#pragma once

#include <filesystem>
#include <stack>
#include <optional>

#include "transpiler.h"
#include "../../synthesizer.h"
#include "../../../targets/tofino/tofino.h"
#include "../../../targets/tofino/data_structures/data_structures.h"

namespace synapse {
namespace tofino {

class EPSynthesizer : public Synthesizer, public EPVisitor {
private:
  struct var_t {
    code_t name;
    klee::ref<klee::Expr> expr;
    std::optional<symbol_t> symbol;
    bool is_bool;

    var_t() {}

    var_t(const code_t &name, const symbol_t &symbol, bool _is_bool = false)
        : name(name), expr(symbol.expr), symbol(symbol), is_bool(_is_bool) {}

    var_t(const code_t &name, klee::ref<klee::Expr> expr, bool _is_bool = false)
        : name(name), expr(expr), is_bool(_is_bool) {}
  };

  typedef std::vector<var_t> vars_t;

private:
  std::unordered_map<code_t, int> var_prefix_usage;
  std::vector<vars_t> var_stacks;
  std::vector<vars_t> hdrs_stacks;

  std::unordered_map<node_id_t, vars_t> parser_vars;
  std::unordered_map<node_id_t, vars_t> parser_hdrs;

  Transpiler transpiler;

public:
  EPSynthesizer(std::ostream &_out, const BDD *bdd);

  virtual void log(const EPNode *ep_node) const override {
    // Don't log anything.
  }

  void visit(const EP *ep) override;
  void visit(const EP *ep, const EPNode *ep_node) override;

  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::SendToController *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Recirculate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::ParserCondition *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::ParserExtraction *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::ParserReject *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::TableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::FCFSCachedTableReadOrWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::FCFSCachedTableWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node,
               const tofino::FCFSCachedTableDelete *node) override final;

private:
  code_t slice_var(const var_t &var, unsigned offset, bits_t size) const;
  code_t type_from_var(const var_t &var) const;

  code_t get_unique_var_name(const code_t &prefix);
  bool get_var(klee::ref<klee::Expr> expr, var_t &out_var) const;

  vars_t get_squashed_vars() const;
  vars_t get_squashed_hdrs() const;

  bool get_hdr_var(node_id_t node_id, klee::ref<klee::Expr> expr, var_t &out_var) const;

  void transpile_parser(const Parser &parser);
  void transpile_table(coder_t &coder, const Table *table,
                       const std::vector<klee::ref<klee::Expr>> &keys,
                       const std::vector<klee::ref<klee::Expr>> &values);
  void transpile_register(coder_t &coder, const Register *reg, klee::ref<klee::Expr> index,
                          klee::ref<klee::Expr> value);
  void transpile_fcfs_cached_table(coder_t &coder, const FCFSCachedTable *fcfs_cached_table,
                                   klee::ref<klee::Expr> key, klee::ref<klee::Expr> value);

  void dbg_vars() const;

  friend class Transpiler;
};

} // namespace tofino
} // namespace synapse