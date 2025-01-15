#pragma once

#include <filesystem>
#include <stack>
#include <optional>

#include "transpiler.hpp"
#include "../../synthesizer.hpp"
#include "../../../targets/tofino/tofino.hpp"
#include "../../../targets/tofino/data_structures/data_structures.hpp"

namespace synapse {
namespace tofino {

class EPSynthesizer : public Synthesizer, public EPVisitor {
private:
  struct var_t {
    code_t name;
    klee::ref<klee::Expr> expr;
    bool force_bool;
    bool is_header_field;

    var_t() = default;
    var_t(const code_t &name, klee::ref<klee::Expr> expr, bool _force_bool = false, bool _is_header_field = false)
        : name(name), expr(expr), force_bool(_force_bool), is_header_field(_is_header_field) {}

    var_t(const var_t &other)            = default;
    var_t(var_t &&other)                 = default;
    var_t &operator=(const var_t &other) = default;

    code_t get_type() const;
    bool is_bool() const;
    var_t get_slice(bits_t offset, bits_t size) const;
    code_t get_stem() const;
    void declare(coder_t &coder, std::optional<code_t> assignment = std::nullopt) const;
  };

  class Stack {
  private:
    std::vector<var_t> vars;
    std::unordered_set<code_t> names;

  public:
    Stack()                              = default;
    Stack(const Stack &other)            = default;
    Stack(Stack &&other)                 = default;
    Stack &operator=(const Stack &other) = default;

    void push(const var_t &var);
    void push(const Stack &stack);
    void clear();

    std::optional<var_t> get(klee::ref<klee::Expr> expr) const;
    std::optional<var_t> get_exact(klee::ref<klee::Expr> expr) const;
    const std::vector<var_t> &get_all() const;
  };

  class Stacks {
  private:
    std::vector<Stack> stacks;

  public:
    Stacks() : stacks(1) {}
    Stacks(const Stacks &other)            = default;
    Stacks(Stacks &&other)                 = default;
    Stacks &operator=(const Stacks &other) = default;

    void push();
    void pop();
    void clear();

    void insert_front(const var_t &var);
    void insert_front(const Stack &stack);

    void insert_back(const var_t &var);
    void insert_back(const Stack &stack);

    Stack squash() const;
    std::optional<var_t> get(klee::ref<klee::Expr> expr) const;
    const std::vector<Stack> &get_all() const;
  };

  typedef u32 alloc_opt_t;

  static constexpr const alloc_opt_t LOCAL        = 0b000001;
  static constexpr const alloc_opt_t GLOBAL       = 0b000010;
  static constexpr const alloc_opt_t EXACT_NAME   = 0b000100;
  static constexpr const alloc_opt_t HEADER       = 0b001000;
  static constexpr const alloc_opt_t HEADER_FIELD = 0b010000;
  static constexpr const alloc_opt_t FORCE_BOOL   = 0b100000;

  std::unordered_map<code_t, int> var_prefix_usage;

  Stacks ingress_vars;
  Stacks hdrs_stacks;
  Stack cpu_hdr_vars;
  Stack recirc_hdr_vars;

  std::unordered_set<DS_ID> declared_ds;
  std::unordered_map<node_id_t, Stack> parser_vars;
  std::unordered_map<node_id_t, Stack> parser_hdrs;
  std::vector<coder_t> recirc_coders;
  std::optional<code_path_t> active_recirc_code_path;

  Transpiler transpiler;

public:
  EPSynthesizer(std::ostream &out, const BDD *bdd);

  coder_t &get(const std::string &marker) override final;

  void visit(const EP *ep) override final;
  void visit(const EP *ep, const EPNode *ep_node) override final;

  Action visit(const EP *ep, const EPNode *ep_node, const tofino::SendToController *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Recirculate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::ParserCondition *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::ParserExtraction *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::ParserReject *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::TableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableReadOrWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const tofino::FCFSCachedTableDelete *node) override final;

private:
  code_t transpile(klee::ref<klee::Expr> expr);

  code_t create_unique_name(const code_t &name);
  var_t alloc_var(const code_t &name, klee::ref<klee::Expr> expr, alloc_opt_t option);
  code_path_t alloc_recirc_coder();

  code_t build_register_action_name(const EPNode *node, const Register *reg, RegisterActionType action) const;
  void transpile_parser(const Parser &parser);
  void transpile_table_decl(coder_t &coder, const Table *table, const std::vector<klee::ref<klee::Expr>> &keys,
                            const std::vector<klee::ref<klee::Expr>> &values);
  void transpile_register_decl(coder_t &coder, const Register *reg, klee::ref<klee::Expr> index,
                               klee::ref<klee::Expr> value);
  void transpile_register_read_action_decl(coder_t &coder, const Register *reg, const code_t &name);
  void transpile_register_write_action_decl(coder_t &coder, const Register *reg, const code_t &name,
                                            const var_t &write_value);
  void transpile_fcfs_cached_table_decl(coder_t &coder, const FCFSCachedTable *fcfs_cached_table,
                                        klee::ref<klee::Expr> key, klee::ref<klee::Expr> value);

  void dbg_vars() const;

  friend class Transpiler;
};

} // namespace tofino
} // namespace synapse