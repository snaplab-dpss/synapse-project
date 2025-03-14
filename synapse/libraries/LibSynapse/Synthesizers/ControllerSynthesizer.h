#pragma once

#include <LibCore/Synthesizer.h>
#include <LibSynapse/Visitor.h>
#include <LibBDD/BDD.h>
#include <LibSynapse/Modules/Controller/Controller.h>
#include <LibSynapse/Modules/Tofino/Tofino.h>

#include <klee/util/ExprVisitor.h>

#include <filesystem>
#include <stack>
#include <optional>
#include <unordered_set>

namespace LibSynapse {
namespace Controller {

class ControllerSynthesizer : public LibCore::Synthesizer, public EPVisitor {
public:
  ControllerSynthesizer(const EP *ep, std::filesystem::path out_file);

  virtual void synthesize() override final;

  using transpiler_opt_t = u32;

  static constexpr const transpiler_opt_t TRANSPILER_OPT_NO_OPTION      = 0b0;
  static constexpr const transpiler_opt_t TRANSPILER_OPT_INVERT_HEADERS = 0b1;

private:
  class Transpiler : public klee::ExprVisitor::ExprVisitor {
  private:
    std::stack<coder_t> coders;
    const ControllerSynthesizer *synthesizer;
    transpiler_opt_t loaded_opt;

  public:
    Transpiler(const ControllerSynthesizer *synthesizer);

    code_t transpile(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION);

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
    std::optional<addr_t> addr;
    bool is_ptr;
    bool is_buffer;
    bool is_header;

    code_t get_slice(bits_t offset, bits_t size, transpiler_opt_t opt) const;
    code_t get_stem() const;
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

    std::optional<var_t> get(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const;
    std::optional<var_t> get_by_addr(addr_t addr) const;
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
    std::optional<var_t> get(klee::ref<klee::Expr> expr, transpiler_opt_t opt) const;
    std::optional<var_t> get_by_addr(addr_t addr) const;
    std::vector<Stack> get_all() const;
  };

  Stacks vars;
  std::unordered_map<std::string, int> reserved_var_names;
  std::vector<code_t> state_member_init_list;
  std::vector<ep_node_id_t> code_paths;

  const EP *ep;
  Transpiler transpiler;

  void synthesize_nf_init();
  void synthesize_nf_process();
  void synthesize_state_member_init_list();

  void visit(const EP *ep, const EPNode *ep_node) override final;

  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToController *node) override final;

  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::ParseHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::ChecksumUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapTableAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapTableUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorTableAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorTableUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainTableAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainTableAllocateNewIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainTableFreeIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainTableIsIndexAllocated *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainTableRefreshIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainAllocateNewIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainRejuvenateIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainIsIndexAllocated *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::DchainFreeIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapGet *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapPut *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MapErase *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::ChtAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::ChtFindBackend *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HashObj *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::FCFSCachedTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableConditionalUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::HHTableDelete *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketIsTracing *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketTrace *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketUpdateAndCheck *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::TokenBucketExpire *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MeterAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::MeterInsert *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::IntegerAllocatorAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::IntegerAllocatorFreeIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Controller::CMSCountMin *node) override final;

  bool in_nf_init{true};
  void change_to_process_coder() { in_nf_init = false; }
  coder_t &get_current_coder();
  coder_t &get(const std::string &marker) override final;

  code_t create_unique_name(const code_t &name);
  code_t assert_unique_name(const code_t &name);

  using var_alloc_opt_t = u32;

  static constexpr const var_alloc_opt_t NO_OPTION        = 0b0000000;
  static constexpr const var_alloc_opt_t EXACT_NAME       = 0b0000001;
  static constexpr const var_alloc_opt_t IS_CPU_HDR       = 0b0000010;
  static constexpr const var_alloc_opt_t IS_CPU_HDR_EXTRA = 0b0000100;
  static constexpr const var_alloc_opt_t IS_PTR           = 0b0001000;
  static constexpr const var_alloc_opt_t IS_BUFFER        = 0b0010000;
  static constexpr const var_alloc_opt_t IS_HEADER        = 0b0100000;
  static constexpr const var_alloc_opt_t SKIP_ALLOC       = 0b1000000;

  var_t alloc_var(const code_t &name, klee::ref<klee::Expr> expr, std::optional<addr_t> addr, var_alloc_opt_t opt);
  code_path_t alloc_recirc_coder();

  var_t transpile_buffer_decl_and_set(coder_t &coder, const code_t &proposed_name, klee::ref<klee::Expr> expr, bool skip_alloc);
  void transpile_table_decl(const Tofino::Table *table);
  void transpile_register_decl(const Tofino::Register *reg);
  void transpile_map_table_decl(const Tofino::MapTable *map_table);
  void transpile_vector_table_decl(const Tofino::VectorTable *vector_table);
  void transpile_dchain_table_decl(const Tofino::DchainTable *dchain_table, time_ns_t expiration_time);
  void transpile_vector_register_decl(const Tofino::VectorRegister *vector_register);

  void dbg_vars() const;

  friend class Transpiler;
};

} // namespace Controller
} // namespace LibSynapse