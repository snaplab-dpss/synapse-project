#pragma once

#include <LibCore/Template.h>
#include <LibCore/Coder.h>
#include <LibSynapse/Visitor.h>
#include <LibBDD/BDD.h>
#include <LibSynapse/Modules/x86/x86.h>

#include <klee/util/ExprVisitor.h>

#include <filesystem>
#include <stack>
#include <optional>
#include <unordered_set>

namespace LibSynapse {
namespace x86 {

using LibCore::code_t;
using LibCore::coder_t;
using LibCore::indent_t;
using LibCore::marker_t;
using LibCore::Template;

enum class x86SynthesizerTarget { NF, Profiler };

class x86Synthesizer : public EPVisitor {
public:
  x86Synthesizer(const EP *ep, x86SynthesizerTarget target, std::filesystem::path out_path, const std::string &instance_id);

  virtual void synthesize();

private:
  class Transpiler : public klee::ExprVisitor::ExprVisitor {
  private:
    std::stack<coder_t> coders;
    x86Synthesizer *synthesizer;

  public:
    Transpiler(x86Synthesizer *synthesizer);
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
    klee::ref<klee::Expr> addr;

    code_t get_slice(bits_t offset, bits_t size) const;
    std::string to_string() const;
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
    std::optional<var_t> get_by_addr(klee::ref<klee::Expr> addr) const;
    std::optional<var_t> get_exact(klee::ref<klee::Expr> expr) const;
    const std::vector<var_t> &get_all() const;
    std::vector<var_t> &get_all_mutable();
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
    std::optional<var_t> get_by_addr(klee::ref<klee::Expr> addr) const;
    const std::vector<Stack> &get_all() const;
    void replace(const var_t &var, klee::ref<klee::Expr> new_expr);
  };

  const std::filesystem::path out_file;
  Template code_template;

  Stacks vars;
  std::unordered_map<std::string, int> reserved_var_names;

  std::unordered_map<bdd_node_id_t, klee::ref<klee::Expr>> nodes_to_map;
  std::unordered_set<bdd_node_id_t> route_nodes;
  std::unordered_set<bdd_node_id_t> process_nodes;

  const EP *target_ep;
  x86SynthesizerTarget target;
  Transpiler transpiler;
  const std::string instance_id;

  bool in_nf_init{true};
  void change_to_process_coder() { in_nf_init = false; }
  void change_to_nf_init_coder() { in_nf_init = true; }
  coder_t &get_current_coder();
  coder_t &get(const std::string &marker);

  void synthesize_nf_init();
  void synthesize_nf_process();
  void synthesize_nf_init_post_process();

  void visit(const EP *ep, const EPNode *ep_node) override final;
  void log(const EPNode *node) const override final;

  // Visitor Declarations
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Ignore *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::If *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Then *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Else *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Forward *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Broadcast *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::Drop *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ParseHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ModifyHeader *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::MapAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::MapGet *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMap *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ExpireItemsSingleMapIteratively *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainRejuvenateIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::VectorAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::VectorRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::VectorWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainAllocateNewIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::MapPut *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ChecksumUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainIsIndexAllocated *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSCountMin *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::CMSPeriodicCleanup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::MapErase *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::DchainFreeIndex *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::ChtFindBackend *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::HashObj *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketAllocate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketIsTracing *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketTrace *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketUpdateAndCheck *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::TokenBucketExpire *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const x86::LPMAllocate *node) override final;

  var_t build_var_ptr(const std::string &base_name, klee::ref<klee::Expr> addr_expr, klee::ref<klee::Expr> value, coder_t &coder,
                      bool &found_in_stack);
  var_t build_var(const std::string &name, klee::ref<klee::Expr> expr);
  var_t build_var(const std::string &name, klee::ref<klee::Expr> expr, klee::ref<klee::Expr> addr);

  code_t create_unique_name(const std::string &base_name);

  bool find_or_create_tmp_slice_var(klee::ref<klee::Expr> expr, coder_t &coder, var_t &out_var);

  void dbg_vars() const;
  friend class Transpiler;
};
} // namespace x86
} // namespace LibSynapse
