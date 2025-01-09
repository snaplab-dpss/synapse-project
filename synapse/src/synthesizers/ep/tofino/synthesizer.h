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

    var_t(const var_t &other) = default;
    var_t(var_t &&other) = default;
    var_t &operator=(const var_t &other) = default;
  };

  typedef std::vector<var_t> vars_t;

private:
  std::unordered_map<code_t, int> var_prefix_usage;
  std::vector<vars_t> var_stacks;
  std::vector<vars_t> hdrs_stacks;

  std::unordered_set<DS_ID> declared_ds;
  std::unordered_map<node_id_t, vars_t> parser_vars;
  std::unordered_map<node_id_t, vars_t> parser_hdrs;

  Transpiler transpiler;

public:
  EPSynthesizer(std::ostream &out, const BDD *bdd);

  void visit(const EP *ep) override final;
  void visit(const EP *ep, const EPNode *ep_node) override final;

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

  code_t get_unique_name(const code_t &prefix);
  bool get_var(klee::ref<klee::Expr> expr, var_t &out_var) const;

  vars_t get_squashed_vars() const;
  vars_t get_squashed_hdrs() const;

  bool get_hdr_var(node_id_t node_id, klee::ref<klee::Expr> expr, var_t &out_var) const;
  code_t build_register_action_name(const EPNode *node, const Register *reg,
                                    RegisterActionType action) const;
  void transpile_parser(const Parser &parser);
  code_t transpile_table_decl(indent_t lvl, const Table *table,
                              const std::vector<klee::ref<klee::Expr>> &keys,
                              const std::vector<klee::ref<klee::Expr>> &values);
  code_t transpile_register_decl(indent_t lvl, const Register *reg, klee::ref<klee::Expr> index,
                                 klee::ref<klee::Expr> value);
  code_t transpile_register_read_action_decl(indent_t lvl, const Register *reg, const code_t &name);
  code_t transpile_register_write_action_decl(indent_t lvl, const Register *reg, const code_t &name,
                                              const var_t &write_value);
  code_t transpile_fcfs_cached_table_decl(indent_t lvl, const FCFSCachedTable *fcfs_cached_table,
                                          klee::ref<klee::Expr> key, klee::ref<klee::Expr> value);

  void dbg_vars() const;

  friend class Transpiler;
};

} // namespace tofino
} // namespace synapse