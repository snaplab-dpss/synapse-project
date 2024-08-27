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

  using call_synthesizer_fn =
      std::function<void(coder_t &, const call_t &, symbols_t)>;
  std::unordered_map<std::string, call_synthesizer_fn> call_synthesizers;

  struct var_t {
    std::string name;
    klee::ref<klee::Expr> expr;
    klee::ref<klee::Expr> addr;
  };

  struct stack_frame_t {
    std::vector<var_t> vars;
    std::map<klee::ref<klee::Expr>, var_t> cache;
  };

  std::vector<stack_frame_t> stack;
  std::unordered_map<std::string, int> reserved_var_names;

public:
  BDDSynthesizer(BDDSynthesizerTarget _target, std::ostream &_out);

  void synthesize(const BDD *bdd);

private:
  void init(const BDD *bdd);
  void process(const BDD *bdd);
  void synthesize(const Node *node);

  var_t build_key(klee::ref<klee::Expr> key_addr, klee::ref<klee::Expr> key,
                  coder_t &coder, bool &key_in_stack);

  void synthesize(coder_t &, const call_t &, symbols_t);
  void packet_borrow_next_chunk(coder_t &, const call_t &, symbols_t);
  void packet_return_chunk(coder_t &, const call_t &, symbols_t);
  void nf_set_rte_ipv4_udptcp_checksum(coder_t &, const call_t &, symbols_t);
  void expire_items_single_map(coder_t &, const call_t &, symbols_t);
  void expire_items_single_map_iteratively(coder_t &, const call_t &,
                                           symbols_t);
  void map_allocate(coder_t &, const call_t &, symbols_t);
  void map_get(coder_t &, const call_t &, symbols_t);
  void map_put(coder_t &, const call_t &, symbols_t);
  void map_erase(coder_t &, const call_t &, symbols_t);
  void vector_allocate(coder_t &, const call_t &, symbols_t);
  void vector_borrow(coder_t &, const call_t &, symbols_t);
  void vector_return(coder_t &, const call_t &, symbols_t);
  void dchain_allocate(coder_t &, const call_t &, symbols_t);
  void dchain_allocate_new_index(coder_t &, const call_t &, symbols_t);
  void dchain_rejuvenate_index(coder_t &, const call_t &, symbols_t);
  void dchain_expire_one(coder_t &, const call_t &, symbols_t);
  void dchain_is_index_allocated(coder_t &, const call_t &, symbols_t);
  void dchain_free_index(coder_t &, const call_t &, symbols_t);
  void sketch_allocate(coder_t &, const call_t &, symbols_t);
  void sketch_compute_hashes(coder_t &, const call_t &, symbols_t);
  void sketch_refresh(coder_t &, const call_t &, symbols_t);
  void sketch_fetch(coder_t &, const call_t &, symbols_t);
  void sketch_touch_buckets(coder_t &, const call_t &, symbols_t);
  void sketch_expire(coder_t &, const call_t &, symbols_t);

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
