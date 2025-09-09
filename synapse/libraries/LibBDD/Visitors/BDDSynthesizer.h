#pragma once

#include <LibCore/Coder.h>
#include <LibCore/Template.h>
#include <LibBDD/Nodes/Node.h>

#include <klee/util/ExprVisitor.h>

#include <filesystem>
#include <stack>
#include <optional>

namespace LibBDD {

using LibCore::code_t;
using LibCore::coder_t;
using LibCore::indent_t;
using LibCore::marker_t;
using LibCore::Template;

class BDD;
class Call;

enum class BDDSynthesizerTarget { NF, Profiler };

class BDDSynthesizer {
public:
  BDDSynthesizer(const BDD *_bdd, BDDSynthesizerTarget _target, std::filesystem::path _out_file);

  void synthesize();

private:
  class Transpiler : public klee::ExprVisitor::ExprVisitor {
  private:
    std::stack<coder_t> coders;
    BDDSynthesizer *synthesizer;

  public:
    Transpiler(BDDSynthesizer *synthesizer);

    code_t transpile(klee::ref<klee::Expr> expr);

    static bool is_primitive_type(bits_t size);
    static code_t type_from_size(bits_t size);
    static code_t type_from_expr(klee::ref<klee::Expr> expr);

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

  const std::filesystem::path out_file;
  const BDD *bdd;
  BDDSynthesizerTarget target;
  Template code_template;
  Transpiler transpiler;

  struct var_t {
    std::string name;
    klee::ref<klee::Expr> expr;
    klee::ref<klee::Expr> addr;
  };

  using success_condition_t    = std::optional<var_t>;
  using process_synthesizer_fn = std::function<success_condition_t(coder_t &, const Call *)>;
  std::unordered_map<std::string, process_synthesizer_fn> function_synthesizers;

  struct stack_frame_t {
    std::vector<var_t> vars;
  };

  std::vector<stack_frame_t> stack;
  std::unordered_map<std::string, int> reserved_var_names;

  // Relevant for profiling
  std::unordered_map<bdd_node_id_t, klee::ref<klee::Expr>> nodes_to_map;
  std::unordered_set<bdd_node_id_t> route_nodes;
  std::unordered_set<bdd_node_id_t> process_nodes;

  void init_pre_process();
  void process();
  void init_post_process();
  void synthesize(const BDDNode *node);

  success_condition_t synthesize_function(coder_t &, const Call *);

  success_condition_t map_allocate(coder_t &, const Call *);
  success_condition_t vector_allocate(coder_t &, const Call *);
  success_condition_t dchain_allocate(coder_t &, const Call *);
  success_condition_t cms_allocate(coder_t &, const Call *);
  success_condition_t tb_allocate(coder_t &, const Call *);
  success_condition_t lpm_allocate(coder_t &, const Call *);

  success_condition_t packet_borrow_next_chunk(coder_t &, const Call *);
  success_condition_t packet_return_chunk(coder_t &, const Call *);
  success_condition_t nf_set_rte_ipv4_udptcp_checksum(coder_t &, const Call *);
  success_condition_t expire_items_single_map(coder_t &, const Call *);
  success_condition_t expire_items_single_map_iteratively(coder_t &, const Call *);
  success_condition_t map_get(coder_t &, const Call *);
  success_condition_t map_put(coder_t &, const Call *);
  success_condition_t map_erase(coder_t &, const Call *);
  success_condition_t map_size(coder_t &, const Call *);
  success_condition_t vector_borrow(coder_t &, const Call *);
  success_condition_t vector_return(coder_t &, const Call *);
  success_condition_t vector_clear(coder_t &, const Call *);
  success_condition_t vector_sample_lt(coder_t &, const Call *);
  success_condition_t dchain_allocate_new_index(coder_t &, const Call *);
  success_condition_t dchain_rejuvenate_index(coder_t &, const Call *);
  success_condition_t dchain_expire_one(coder_t &, const Call *);
  success_condition_t dchain_is_index_allocated(coder_t &, const Call *);
  success_condition_t dchain_free_index(coder_t &, const Call *);
  success_condition_t cms_increment(coder_t &, const Call *);
  success_condition_t cms_count_min(coder_t &, const Call *);
  success_condition_t cms_periodic_cleanup(coder_t &, const Call *);
  success_condition_t tb_is_tracing(coder_t &, const Call *);
  success_condition_t tb_trace(coder_t &, const Call *);
  success_condition_t tb_update_and_check(coder_t &, const Call *);
  success_condition_t tb_expire(coder_t &, const Call *);
  success_condition_t lpm_lookup(coder_t &, const Call *);
  success_condition_t lpm_update(coder_t &, const Call *);
  success_condition_t lpm_from_file(coder_t &, const Call *);

  void stack_dbg() const;
  void stack_push();
  void stack_pop();
  var_t stack_get(const std::string &name) const;
  var_t stack_get(klee::ref<klee::Expr> expr);
  bool stack_find(klee::ref<klee::Expr> expr, var_t &var);
  bool stack_find_or_create_tmp_slice_var(klee::ref<klee::Expr> expr, coder_t &coder, var_t &var);
  void stack_add(const var_t &var);
  void stack_replace(const var_t &var, klee::ref<klee::Expr> new_expr);

  var_t build_var_ptr(const std::string &base_name, klee::ref<klee::Expr> addr, klee::ref<klee::Expr> value, coder_t &coder, bool &found_in_stack);
  var_t build_var(const std::string &name, klee::ref<klee::Expr> expr);
  var_t build_var(const std::string &name, klee::ref<klee::Expr> expr, klee::ref<klee::Expr> addr);
  code_t slice_var(const var_t &var, unsigned offset, bits_t size) const;

  friend class BDDTranspiler;
};

} // namespace LibBDD