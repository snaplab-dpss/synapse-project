#pragma once

#include <LibCore/Synthesizer.h>
#include <LibSynapse/Visitor.h>
#include <LibBDD/BDD.h>
#include <LibSynapse/Modules/Tofino/Tofino.h>

#include <klee/util/ExprVisitor.h>

#include <filesystem>
#include <stack>
#include <optional>
#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

class TofinoSynthesizer : public LibCore::Synthesizer, public EPVisitor {
public:
  TofinoSynthesizer(const EP *ep, std::filesystem::path _out_path);

  virtual void synthesize() override final;

private:
  class Transpiler : public klee::ExprVisitor::ExprVisitor {
  private:
    std::stack<coder_t> coders;
    const TofinoSynthesizer *synthesizer;

  public:
    Transpiler(const TofinoSynthesizer *synthesizer);

    code_t transpile(klee::ref<klee::Expr> expr);

    static code_t type_from_size(bits_t size);
    static code_t type_from_expr(klee::ref<klee::Expr> expr);
    static code_t transpile_literal(u64 value, bits_t size);
    static code_t transpile_constant(klee::ref<klee::Expr> expr);

    Action visitNotOptimized(const klee::NotOptimizedExpr &e);
    Action visitRead(const klee::ReadExpr &e);
    Action visitSelect(const klee::SelectExpr &e);
    Action visitConcat(const klee::ConcatExpr &e);
    Action visitExtract(const klee::ExtractExpr &e);
    Action visitZExt(const klee::ZExtExpr &e);
    Action visitSExt(const klee::SExtExpr &e);
    Action visitAdd(const klee::AddExpr &e);
    Action visitSub(const klee::SubExpr &e);
    Action visitMul(const klee::MulExpr &e);
    Action visitUDiv(const klee::UDivExpr &e);
    Action visitSDiv(const klee::SDivExpr &e);
    Action visitURem(const klee::URemExpr &e);
    Action visitSRem(const klee::SRemExpr &e);
    Action visitNot(const klee::NotExpr &e);
    Action visitAnd(const klee::AndExpr &e);
    Action visitOr(const klee::OrExpr &e);
    Action visitXor(const klee::XorExpr &e);
    Action visitShl(const klee::ShlExpr &e);
    Action visitLShr(const klee::LShrExpr &e);
    Action visitAShr(const klee::AShrExpr &e);
    Action visitEq(const klee::EqExpr &e);
    Action visitNe(const klee::NeExpr &e);
    Action visitUlt(const klee::UltExpr &e);
    Action visitUle(const klee::UleExpr &e);
    Action visitUgt(const klee::UgtExpr &e);
    Action visitUge(const klee::UgeExpr &e);
    Action visitSlt(const klee::SltExpr &e);
    Action visitSle(const klee::SleExpr &e);
    Action visitSgt(const klee::SgtExpr &e);
    Action visitSge(const klee::SgeExpr &e);
  };

  struct var_t {
    code_t name;
    klee::ref<klee::Expr> expr;
    bool force_bool;
    bool is_header_field;

    var_t() = default;
    var_t(const code_t &_name, klee::ref<klee::Expr> _expr, bool _force_bool = false, bool _is_header_field = false)
        : name(_name), expr(_expr), force_bool(_force_bool), is_header_field(_is_header_field) {}

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
    std::vector<var_t> frames;
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
    std::vector<var_t> get_all() const;
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
    std::vector<Stack> get_all() const;
  };

  using alloc_opt_t = u32;

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
  std::unordered_map<LibBDD::node_id_t, Stack> parser_vars;
  std::unordered_map<LibBDD::node_id_t, Stack> parser_hdrs;
  std::vector<coder_t> recirc_coders;
  std::optional<code_path_t> active_recirc_code_path;

  const EP *ep;
  Transpiler transpiler;

  void visit(const EP *ep, const EPNode *ep_node) override final;

  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToController *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Recirculate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserCondition *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserExtraction *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ParserReject *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::MapTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::DchainTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableReadWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::LPMLookup *node) override final;

  coder_t &get(const std::string &marker) override final;
  code_t transpile(klee::ref<klee::Expr> expr);

  code_t create_unique_name(const code_t &name);
  var_t alloc_var(const code_t &name, klee::ref<klee::Expr> expr, alloc_opt_t option = 0);
  code_path_t alloc_recirc_coder();

  code_t build_register_action_name(const EPNode *node, const Register *reg, RegisterActionType action) const;
  void transpile_parser(const Parser &parser);
  void transpile_action_decl(coder_t &coder, const std::string action_name, const std::vector<klee::ref<klee::Expr>> &params);
  void transpile_table_decl(coder_t &coder, const Table *table, const std::vector<klee::ref<klee::Expr>> &keys,
                            const std::vector<klee::ref<klee::Expr>> &values);
  void transpile_register_decl(coder_t &coder, const Register *reg, klee::ref<klee::Expr> index, klee::ref<klee::Expr> value);
  void transpile_register_read_action_decl(coder_t &coder, const Register *reg, const code_t &name);
  void transpile_register_write_action_decl(coder_t &coder, const Register *reg, const code_t &name, const var_t &write_value);
  void transpile_fcfs_cached_table_decl(coder_t &coder, const FCFSCachedTable *fcfs_cached_table, klee::ref<klee::Expr> key,
                                        klee::ref<klee::Expr> value);
  void transpile_lpm_decl(coder_t &coder, const LPM *lpm, klee::ref<klee::Expr> addr, klee::ref<klee::Expr> device);

  void dbg_vars() const;

  static code_t get_parser_state_name(const ParserState *state, bool state_init);

  friend class Transpiler;
};

} // namespace Tofino
} // namespace LibSynapse