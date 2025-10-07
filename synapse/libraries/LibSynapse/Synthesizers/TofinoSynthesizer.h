#pragma once

#include <LibCore/Template.h>
#include <LibCore/Coder.h>
#include <LibBDD/BDD.h>
#include <LibSynapse/Visitor.h>
#include <LibSynapse/Modules/Tofino/Tofino.h>

#include <klee/util/ExprVisitor.h>

#include <filesystem>
#include <stack>
#include <optional>
#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

using LibCore::code_t;
using LibCore::coder_t;
using LibCore::indent_t;
using LibCore::marker_t;
using LibCore::Template;

class TofinoSynthesizer : public EPVisitor {
public:
  TofinoSynthesizer(const EP *ep, std::filesystem::path _out_file);

  void synthesize();

  using transpiler_opt_t = u32;

  static constexpr const transpiler_opt_t TRANSPILER_OPT_NO_OPTION             = 0b000;
  static constexpr const transpiler_opt_t TRANSPILER_OPT_SWAP_HDR_ENDIANNESS   = 0b001;
  static constexpr const transpiler_opt_t TRANSPILER_OPT_SWAP_CONST_ENDIANNESS = 0b010;
  static constexpr const transpiler_opt_t TRANSPILER_OPT_REVERSE_VAR_BYTES     = 0b100;

private:
  class Transpiler : public klee::ExprVisitor::ExprVisitor {
  private:
    std::stack<coder_t> coders;
    TofinoSynthesizer *synthesizer;
    transpiler_opt_t loaded_opt;

  public:
    Transpiler(TofinoSynthesizer *synthesizer);

    code_t transpile(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION);

    static code_t type_from_size(bits_t size);
    static code_t type_from_expr(klee::ref<klee::Expr> expr);
    static code_t transpile_literal(u64 value, bits_t size, bool hex = false);
    static code_t transpile_constant(klee::ref<klee::Expr> expr, bool swap_endianness);
    static code_t swap_endianness(const code_t &expr, bits_t size);

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
    code_t original_name;
    klee::ref<klee::Expr> original_expr;
    bits_t original_size;
    code_t name;
    klee::ref<klee::Expr> expr;
    bits_t size;
    bool force_bool;
    bool is_header_field;
    bool is_buffer;

    var_t() = default;
    var_t(const code_t &_name, klee::ref<klee::Expr> _expr, bits_t _size, bool _force_bool, bool _is_header_field, bool _is_buffer)
        : original_name(_name), original_expr(_expr), original_size(_size), name(_name), expr(_expr), size(_size), force_bool(_force_bool),
          is_header_field(_is_header_field), is_buffer(_is_buffer) {}
    var_t(const code_t &_original_name, klee::ref<klee::Expr> _original_expr, bits_t _original_size, const code_t &_name, klee::ref<klee::Expr> _expr,
          bits_t _size, bool _force_bool, bool _is_header_field, bool _is_buffer)
        : original_name(_original_name), original_expr(_original_expr), original_size(_original_size), name(_name), expr(_expr), size(_size),
          force_bool(_force_bool), is_header_field(_is_header_field), is_buffer(_is_buffer) {}

    var_t(const var_t &other)            = default;
    var_t(var_t &&other)                 = default;
    var_t &operator=(const var_t &other) = default;

    code_t get_type() const;
    bool is_bool() const;
    var_t get_slice(bits_t offset, bits_t size, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION) const;
    code_t get_stem() const;
    void declare(coder_t &coder, std::optional<code_t> assignment = std::nullopt) const;
    bool is_slice() const { return original_name != name || original_expr != expr || original_size != size; }
    std::vector<code_t> split_by_dot() const;
    std::string to_string() const;
    code_t flatten_name() const;
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

    std::optional<var_t> get(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION) const;
    std::optional<var_t> get_exact(klee::ref<klee::Expr> expr) const;
    std::optional<var_t> get_hdr(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION) const;
    std::optional<var_t> get_exact_hdr(klee::ref<klee::Expr> expr) const;
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
    Stack squash_hdrs_only() const;
    std::optional<var_t> get(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION) const;
    std::optional<var_t> get_hdr(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION) const;
    std::vector<Stack> get_all() const;
    Stack get_first_stack() const { return stacks.front(); }
  };

  using alloc_opt_t = u32;

  static constexpr const alloc_opt_t SKIP_STACK_ALLOC    = 0b0000001;
  static constexpr const alloc_opt_t EXACT_NAME          = 0b0000010;
  static constexpr const alloc_opt_t HEADER              = 0b0000100;
  static constexpr const alloc_opt_t HEADER_FIELD        = 0b0001000;
  static constexpr const alloc_opt_t BUFFER              = 0b0010000;
  static constexpr const alloc_opt_t FORCE_BOOL          = 0b0100000;
  static constexpr const alloc_opt_t IS_INGRESS_METADATA = 0b1000000;

  const std::filesystem::path out_file;
  Template code_template;

  std::unordered_map<code_t, int> var_prefix_usage;

  Stacks ingress_vars;
  Stack hdr_vars;
  Stack cpu_hdr_vars;
  Stack recirc_hdr_vars;

  std::unordered_set<DS_ID> declared_ds;
  std::unordered_map<bdd_node_id_t, Stack> parser_vars;
  std::vector<coder_t> recirc_coders;
  std::optional<code_path_t> active_recirc_code_path;
  std::unordered_set<code_t> ingress_metadata_var_names;

  const EP *target_ep;
  Transpiler transpiler;

  void visit(const EP *ep, const EPNode *ep_node) override final;
  void log(const EPNode *node) const override final;

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
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableGuardCheck *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::DchainTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableReadWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableOutOfBandUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::LPMLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncAndQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CuckooHashTableReadWrite *node) override final;

  coder_t &get(const std::string &marker);

  code_t create_unique_name(const code_t &name);
  var_t alloc_var(const code_t &name, klee::ref<klee::Expr> expr, alloc_opt_t option = 0);
  code_path_t alloc_recirc_coder();

  code_t build_register_action_name(const Register *reg, RegisterActionType action, const EPNode *node = nullptr) const;

  std::unordered_map<RegisterActionType, std::vector<code_t>> cms_get_rows_reg_actions(const CountMinSketch *cms);
  std::unordered_map<RegisterActionType, std::vector<code_t>> cms_get_rows_actions(const CountMinSketch *cms);
  std::unordered_map<RegisterActionType, std::vector<code_t>> cms_get_rows_values(const CountMinSketch *cms);

  void transpile_parser(const Parser &parser);
  void transpile_action_decl(const code_t &action_name, const std::vector<code_t> &body);
  void transpile_action_decl(const code_t &action_name, const std::vector<klee::ref<klee::Expr>> &params, bool params_are_buffers);
  void transpile_table_decl(const Table *table, const std::vector<klee::ref<klee::Expr>> &keys, const std::vector<klee::ref<klee::Expr>> &values,
                            bool values_are_buffers, std::vector<var_t> &keys_vars);
  void transpile_register_decl(const Register *reg);
  void transpile_register_action_decl(const Register *reg, const code_t &action_name, RegisterActionType type, std::optional<var_t> write_value);
  void transpile_hash_decl(const Hash *hash);
  void transpile_digest_decl(const Digest *digest, const std::vector<klee::ref<klee::Expr>> &keys);
  void transpile_fcfs_cached_table_decl(const FCFSCachedTable *fcfs_cached_table, const std::vector<klee::ref<klee::Expr>> &keys,
                                        klee::ref<klee::Expr> value);
  void transpile_lpm_decl(const LPM *lpm, klee::ref<klee::Expr> addr, klee::ref<klee::Expr> device);
  std::vector<code_t> cms_get_hashes_values(const CountMinSketch *cms);
  std::vector<code_t> cms_get_hashes_calculators(const CountMinSketch *cms, const EPNode *ep_node);
  void transpile_cms_hash_calculator_decl(const CountMinSketch *cms, const EPNode *ep_node, const std::vector<var_t> &keys_vars);
  void transpile_cms_decl(const CountMinSketch *cms, const EPNode *ep_node);
  void transpile_cuckoo_hash_table_decl(const CuckooHashTable *cuckoo_hash_table);
  void transpile_if_condition(const If::condition_t &condition);

  void declare_var_in_ingress_metadata(const var_t &var);
  void dbg_vars() const;

  static code_t get_parser_state_name(const ParserState *state, bool state_init);

  friend class Transpiler;
};

} // namespace Tofino
} // namespace LibSynapse