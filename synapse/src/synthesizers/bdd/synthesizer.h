#pragma once

#include <filesystem>
#include <stack>
#include <optional>

#include "../../bdd/bdd.h"
#include "../synthesizer.h"
#include "transpiler.h"

enum class BDDSynthesizerTarget { NF, PROFILER };

class BDDSynthesizer : public Synthesizer {
private:
  BDDSynthesizerTarget target;
  BDDTranspiler transpiler;

  using init_synthesizer_fn = std::function<void(coder_t &, const call_t &)>;
  std::unordered_map<std::string, init_synthesizer_fn> init_synthesizers;

  using process_synthesizer_fn = std::function<void(coder_t &, const Call *)>;
  std::unordered_map<std::string, process_synthesizer_fn> process_synthesizers;

  struct var_t {
    std::string name;
    klee::ref<klee::Expr> expr;
    klee::ref<klee::Expr> addr;
  };

  struct stack_frame_t {
    std::vector<var_t> vars;
  };

  std::vector<stack_frame_t> stack;
  std::unordered_map<std::string, int> reserved_var_names;

  // Relevant for profiling
  std::unordered_set<node_id_t> map_stats_nodes;

public:
  BDDSynthesizer(BDDSynthesizerTarget _target, std::ostream &_out);

  void synthesize(const BDD *bdd);

private:
  void init_pre_process(const BDD *bdd);
  void process(const BDD *bdd);
  void init_post_process();
  void synthesize(const Node *node);

  var_t build_key(klee::ref<klee::Expr> key_addr, klee::ref<klee::Expr> key,
                  coder_t &coder, bool &key_in_stack);

  void synthesize_init(coder_t &, const call_t &);
  void synthesize_process(coder_t &, const Call *);

  void map_allocate(coder_t &, const call_t &);
  void vector_allocate(coder_t &, const call_t &);
  void dchain_allocate(coder_t &, const call_t &);
  void cms_allocate(coder_t &, const call_t &);
  void tb_allocate(coder_t &, const call_t &);

  void packet_borrow_next_chunk(coder_t &, const Call *);
  void packet_return_chunk(coder_t &, const Call *);
  void nf_set_rte_ipv4_udptcp_checksum(coder_t &, const Call *);
  void expire_items_single_map(coder_t &, const Call *);
  void expire_items_single_map_iteratively(coder_t &, const Call *);
  void map_get(coder_t &, const Call *);
  void map_put(coder_t &, const Call *);
  void map_erase(coder_t &, const Call *);
  void vector_borrow(coder_t &, const Call *);
  void vector_return(coder_t &, const Call *);
  void dchain_allocate_new_index(coder_t &, const Call *);
  void dchain_rejuvenate_index(coder_t &, const Call *);
  void dchain_expire_one(coder_t &, const Call *);
  void dchain_is_index_allocated(coder_t &, const Call *);
  void dchain_free_index(coder_t &, const Call *);
  void cms_compute_hashes(coder_t &, const Call *);
  void cms_refresh(coder_t &, const Call *);
  void cms_fetch(coder_t &, const Call *);
  void cms_touch_buckets(coder_t &, const Call *);
  void cms_expire(coder_t &, const Call *);
  void tb_is_tracing(coder_t &, const Call *);
  void tb_trace(coder_t &, const Call *);
  void tb_update_and_check(coder_t &, const Call *);
  void tb_expire(coder_t &, const Call *);

  void stack_dbg() const;
  void stack_push();
  void stack_pop();
  var_t stack_get(const std::string &name) const;
  var_t stack_get(klee::ref<klee::Expr> expr);
  bool stack_find(klee::ref<klee::Expr> expr, var_t &var);
  void stack_add(const var_t &var);
  void stack_replace(const var_t &var, klee::ref<klee::Expr> new_expr);

  var_t build_var(const std::string &name, klee::ref<klee::Expr> expr);
  var_t build_var(const std::string &name, klee::ref<klee::Expr> expr,
                  klee::ref<klee::Expr> addr);
  code_t slice_var(const var_t &var, unsigned offset, bits_t size) const;

  friend class BDDTranspiler;
};
