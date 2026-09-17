#pragma once

#include <LibCore/Template.h>
#include <LibCore/Coder.h>
#include <LibBDD/BDD.h>
#include <LibSynapse/Synthesizers/HandoffLayout.h>
#include <LibSynapse/Visitor.h>
#include <LibSynapse/Modules/Tofino/Tofino.h>

#include <klee/util/ExprVisitor.h>

#include <deque>
#include <map>
#include <tuple>
#include <filesystem>
#include <set>
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
  const handoff_layout_t &get_handoff_layout() const { return handoff_layout; }

  using transpiler_opt_t = u32;

  static constexpr const transpiler_opt_t TRANSPILER_OPT_NO_OPTION             = 0b000;
  static constexpr const transpiler_opt_t TRANSPILER_OPT_SWAP_CONST_ENDIANNESS = 0b001;
  static constexpr const transpiler_opt_t TRANSPILER_OPT_REVERSE_VAR_BYTES     = 0b010;

private:
  class Transpiler : public klee::ExprVisitor::ExprVisitor {
  private:
    std::stack<coder_t> coders;
    TofinoSynthesizer *synthesizer;
    transpiler_opt_t loaded_opt;
    std::map<klee::ref<klee::Expr>, code_t> temporary_transpilations;

  public:
    Transpiler(TofinoSynthesizer *synthesizer);

    code_t transpile(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION,
                     std::map<klee::ref<klee::Expr>, code_t> temporary_transpilations = std::map<klee::ref<klee::Expr>, code_t>());

    static code_t type_from_size(bits_t size);
    static code_t type_from_expr(klee::ref<klee::Expr> expr);
    static code_t type_from_register_out_value(RegisterActionOutValueSize out_value_size, bits_t stored_value_size);
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
    bool transient = false; // A step's own temporary (an operand, a shift half): never carried past a cut.

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

    void push(const var_t &var, bool allow_duplicates = false);
    void push(const Stack &stack);
    void clear();

    std::optional<var_t> get(const code_t &name) const;
    std::optional<var_t> get(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION) const;
    std::optional<var_t> get_exact(klee::ref<klee::Expr> expr) const;
    std::optional<var_t> compose_hdr_fields(klee::ref<klee::Expr> expr) const;
    std::optional<var_t> get_hdr(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION) const;
    std::optional<var_t> get_exact_hdr(klee::ref<klee::Expr> expr) const;
    std::vector<var_t> get_all() const;

    bool set_var_expr(const code_t &name, klee::ref<klee::Expr> expr);
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

    void insert_front(const var_t &var, bool allow_duplicates = false);
    void insert_front(const Stack &stack);

    void insert_back(const var_t &var, bool allow_duplicates = false);
    void insert_back(const Stack &stack);

    Stack squash() const;
    Stack squash_hdrs_only() const;
    std::optional<var_t> get(const code_t &name) const;
    std::optional<var_t> get(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION) const;
    std::optional<var_t> get_hdr(klee::ref<klee::Expr> expr, transpiler_opt_t opt = TRANSPILER_OPT_NO_OPTION) const;
    std::vector<Stack> get_all() const;
    Stack get_first_stack() const { return stacks.front(); }

    bool set_var_expr(const code_t &name, klee::ref<klee::Expr> expr);
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

  // The chunks nf_set_rte_ipv4_udptcp_checksum covers, gathered before emission: a deparser
  // Checksum reads and writes whole fields only, so their headers are laid out with the checksum,
  // and the fields the L4 pseudo-header reads, as fields of their own (checksum_boundaries). A
  // chunk is known by its expression (chunk_key): the call names its headers by address, and the
  // L4 address is the TCP header's on one path and the UDP header's on another.
  struct checksummed_chunk_t {
    bool is_ip;
    bytes_t length;
  };
  static code_t chunk_key(klee::ref<klee::Expr> hdr) { return expr_to_string(hdr, true); }
  std::unordered_map<code_t, checksummed_chunk_t> checksummed_chunks;
  // The fields a chunk's header came out with, in header order.
  struct hdr_field_t {
    bytes_t offset;
    bits_t width;
    code_t name;
  };
  std::unordered_map<code_t, std::vector<hdr_field_t>> hdr_fields_by_hdr;
  // The bytes of each chunk (chunk_key) a header rewrite writes from a computed value, gathered
  // before emission: a chain statement reading such a field on the ALU would tie the field, and
  // then the rewrite's source, to the chain's sliced cluster (the SYN-ACK's sequence word).
  std::unordered_map<code_t, std::vector<std::pair<bytes_t, bytes_t>>> rewritten_packet_bytes;
  // The headers a ChecksumUpdate covers, by its EP node, and per gress the pair whose checksums
  // the deparser recomputes when the path set the flag.
  struct checksum_site_t {
    code_t ip_hdr;
    code_t l4_hdr;
  };
  std::unordered_map<ep_node_id_t, checksum_site_t> checksum_headers_of;
  std::optional<checksum_site_t> ingress_checksum_site;
  std::optional<checksum_site_t> egress_checksum_site;
  // The headers a ChecksumUpdate above the node being visited covers: the flag is set where the
  // packet leaves, which can be another gress or pass (flag_pending_checksum).
  std::optional<checksum_site_t> pending_checksum;
  static std::set<bytes_t> checksum_boundaries(const checksummed_chunk_t &chunk);
  void flag_pending_checksum();
  void emit_deparser_checksums(bool egress);
  Stack cpu_hdr_vars;
  Stack recirc_hdr_vars;
  // Recirculation passes are mutually exclusive and self-identifying: build_recirc_hdr stamps the
  // code path and the next pass dispatches on it, so the reader of a slot is always the writer's
  // counterpart. That lets passes share slots instead of each contributing its own fields, which
  // matters because every field of the header has to sit in PHV at once to be deparsed.
  std::map<code_t, size_t> recirc_slots_used;
  // A value carried by a recirculation keeps its slot on every path that carries it: a shared
  // action placed after the recirculation reads the slot the first path gave the value, and a
  // second path sharing that action has to put its own copy of the value there. Slots are
  // remembered by the carried variable's name, which is the same on both paths for a shared
  // value; a name never seen before takes a slot no remembered name owns.
  std::map<code_t, std::pair<code_t, size_t>> recirc_slot_by_name; // name -> (slot kind, slot)
  std::map<code_t, std::set<size_t>> recirc_slots_owned;           // slot kind -> slots some name owns
  std::map<code_t, size_t> recirc_slot_next;                       // slot kind -> next slot to try, per pass

  // Every computed value of a byte width, by the canonical op id of its producer: a slot of the
  // state header `hdr.st`, allocated per path by live range (plan_value_homes). A header field is
  // an exact container, which the allocator cannot slice: in metadata, the rotates' odd-bit cuts
  // spread through every value they touch until an action needs one PHV source per slice (the
  // ground truth keeps its hash state in `recirc_state` for the same reason). Slots, reused once
  // a value is dead, because header fields are not overlaid: one per op would not fit the PHV,
  // and bf-p4c gives up on a cluster of a few dozen sliced fields long before the PHV is full.
  // The header crosses every cut with the packet, so a value live past one keeps its slot.
  std::unordered_map<std::string, var_t> slot_fields;
  std::map<bits_t, size_t> state_slots_used;                           // width -> slots
  std::map<std::tuple<bool, code_t, code_t>, unsigned> slot_rotations; // (egress, word, source word) -> the one rotation the word takes it at
  std::unordered_map<std::string, std::vector<std::tuple<std::string, unsigned, bool>>> slot_readers; // op -> (reader op, rotation, in egress)
  std::unordered_map<std::string, std::vector<code_t>> planned_sources;    // op -> the words the planner expects its statement to read
  std::unordered_map<std::string, std::vector<code_t>> planned_source_ops; // the same, by the op the planner credited
  std::set<std::pair<bool, code_t>> words_used;                            // (egress, word): the words each gress writes or reads, over every path
  Stack egress_state_hdr_vars;

  std::unordered_set<DS_ID> declared_ds;
  std::unordered_set<const EPNode *> emitted_compute_steps; // Steps emit_compute_run already emitted, as part of a run.
  // A chain of shared compute actions two paths call from two sites of one pass is called once:
  // bf-p4c makes a table, with its own hash-distribution units, per call site (plan_shared_runs).
  // Sites of one ingress pass set a flag where their calls were, and the calls follow the closing
  // brace of the If the sites diverge at. Sites in different egress blocks leave a marker, and the
  // blocks come out as one arm of the code-path ladder, each block's own statements nested under
  // its code path around the one call sequence (synthesize).
  struct shared_run_t {
    std::vector<DS_ID> actions;               // In stage order.
    std::unordered_set<const EPNode *> sites; // The runs' first steps.
    const EPNode *join = nullptr;             // Ingress: the If whose closing brace the calls follow.
    code_t flag;                              // Ingress: the metadata flag every site sets.
    std::vector<const EPNode *> egress_cuts;  // Egress: the crossings whose blocks merge.
    // Ingress, sites in several passes (a loop's iterations, loop_key_t): the calls follow the
    // passes' whole if / else chain, under the flag.
    bool after_passes = false;
    // Egress, blocks calling different parts of the run: by action, the crossings of the blocks
    // calling it (every block when absent).
    std::unordered_map<DS_ID, std::vector<const EPNode *>> callers;
  };
  std::vector<shared_run_t> shared_runs;
  std::unordered_map<const EPNode *, std::vector<size_t>> shared_runs_by_site;
  std::unordered_map<const EPNode *, std::vector<size_t>> shared_runs_by_join;
  std::unordered_map<const EPNode *, code_path_t> egress_code_path_of; // Crossing -> the egress block it opens.
  // A hand-off to the controller sends the state header whole, after the cpu header: copying its
  // words into cpu fields costs what the chain cannot spare (hash-distribution units, or the
  // sliced cluster on the ALU). Which word holds which symbol at each hand-off, for the
  // controller's synthesizer (HandoffLayout.h).
  handoff_layout_t handoff_layout;
  // A path calls a shared action for the ops it reuses, and the action's other ops are another
  // path's: their statements must not run there. A site whose path runs a part of an action calls
  // a variant holding that part, declared once per part after the plan is walked
  // (emit_action_variants) from the statements the declaring path emitted (action_statements).
  struct action_statement_t {
    code_t statement;
    bool in_hash;
  };
  std::unordered_map<DS_ID, std::unordered_map<std::string, action_statement_t>> action_statements; // Action -> op -> its statement.
  mutable std::optional<std::unordered_set<std::string>> loop_step_ops;                             // The op ids of every loop's steps.
  std::unordered_map<DS_ID, bool> action_in_egress;                                                 // Action -> the control declaring it.
  std::unordered_map<DS_ID, std::vector<std::vector<std::string>>> action_variants;                 // Action -> the parts called, in order.
  // Actions called before their declaring path was emitted: whether they fold away is only known
  // once every path is, so their calls go out and the folded ones are dropped at the end.
  std::unordered_set<DS_ID> calls_before_declaration;
  std::unordered_map<bdd_node_id_t, Stack> parser_vars;
  // One coder per recirculation pass, assembled into an if / else-if chain at the end of
  // synthesis. A deque, not a vector: coder_t's copy constructor does not carry the stream
  // over, so a reallocation would silently drop code already emitted, and references handed
  // out by get() must survive later allocations.
  std::deque<coder_t> recirc_coders;
  std::optional<code_path_t> active_recirc_code_path;
  // One code block per crossing, like the recirculation's: the egress reads which crossing the
  // packet took from hdr.egress_state.code_path and runs only that block. Without it every
  // packet in the egress ran every crossing's work in sequence.
  std::deque<coder_t> egress_coders;
  std::optional<code_path_t> active_egress_code_path;

  // Set once a SendToEgress is emitted. Until then the ingress bypasses the egress pipeline
  // exactly as it always has, so a solution that stays in ingress is emitted unchanged.
  bool uses_egress = false;

  // While set, everything the emitter writes goes to the egress pipeline's coders instead of
  // the ingress ones, so the existing emission code works unchanged on the far side of the cut.
  bool in_egress = false;

  // The ingress apply coder that was open where the plan crossed. A pass-ending module reached
  // in egress writes its port there: ucast_egress_port is an ingress field, and plan order is not
  // emission order, so the write lands in the ingress even though the node sits later.
  coder_t *ingress_coder_at_cut = nullptr;

  // The packet headers that are valid where the plan crossed into egress, in extraction order.
  // Only packets that crossed reach the egress, and they all took the one path that led to the
  // cut, so the egress parser can extract exactly these, linearly.
  std::vector<code_t> egress_parser_hdrs;

  // Values a concat rotate has cut, by variable name. Copying one into a deparsed header field
  // needs the hash unit (the field cannot be split, so each piece would be its own PHV source);
  // copying an uncut one does not, and wrapping it anyway spends hash-distribution units, of
  // which only three 32-bit ops fit per stage.
  std::unordered_set<code_t> cut_values;
  std::unordered_set<code_t> ingress_metadata_var_names;

  const EP *target_ep;
  Transpiler transpiler;

  void visit(const EP *ep, const EPNode *ep_node) override final;
  void log(const EPNode *node) const override final;

  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToController *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Recirculate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::SendToEgress *node) override final;
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
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ChecksumUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::MapTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::MapSetTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::GuardedMapTableGuardCheck *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::DchainTableLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterReadConditionalUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterReadConditionalUpdateSingleAction *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::VectorRegisterReadConditionalIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableReadInsert *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableInsert *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedTableIsIndexAllocated *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedSetRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedSetReadInsert *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FCFSCachedSetInsert *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableRead *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::HHTableOutOfBandUpdate *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::LPMLookup *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncrement *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSIncAndQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CMSQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::BloomFilterQueryAndSet *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::BloomFilterQuery *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::BloomFilterSet *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CuckooHashTableReadWrite *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::HashObj *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::CountTrailingZeros *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::FindFirstSetBit *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::PowerOfTwo *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Ln *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::Divide *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::ArithmeticOp *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::RotateLeft *node) override final;
  Action visit(const EP *ep, const EPNode *ep_node, const Tofino::RotateLeftShifts *node) override final;

  void emit_compute_table(const EP *ep, DS_ID table_id, klee::ref<klee::Expr> in, klee::ref<klee::Expr> out);
  // Code for `operand` usable inside an action (metadata, header fields and constants as they
  // are; anything else staged into `meta.<table_id><suffix>` first, or always if `force_stage`).
  // A keyless table whose only action assigns `computation` to `out_var`: one stage, as the
  // placer charged for it. `in_hash` computes the assignment in the hash unit (@in_hash).
  // Emits the run of consecutive compute steps starting at `first` (see the definition).
  code_path_t alloc_egress_coder();
  void emit_compute_run(const EP *ep, const EPNode *first);
  // Before the plan is walked: every computed value of a byte width gets a slot of the state
  // header, reused once the value is dead (slot_fields).
  void plan_value_homes(const EP *ep);
  // Before the plan is walked: the shared runs (shared_run_t), their sites, joins and calls.
  void plan_shared_runs(const EP *ep);
  // The steps of the compute run starting at `first`: consecutive compute modules (an ignored
  // node between them is transparent) and an If whose condition operands had to be computed,
  // which ends the run.
  std::vector<const EPNode *> compute_run_steps(const EPNode *first) const;
  // The statements calling a compute action: the action, its one-@in_hash companions and its
  // action-data companion, as its declaration spills them (emit_compute_run).
  std::vector<code_t> compute_action_calls(const TofinoContext *tofino_ctx, const DS_ID &action_id) const;
  // The statements calling the action `name` holding `ops`, companions included.
  std::vector<code_t> action_calls(const code_t &name, const std::vector<compute_op_t> &ops) const;
  // Declares the action `name` holding `ops`, with the statements it has of them, spilling every
  // @in_hash op past the first into a `_hN` companion and, next to a hash op, the statements
  // carrying action data into a `_k` one; the companions are the ones action_calls calls.
  void declare_compute_action(coder_t &coder, const code_t &name, const std::vector<compute_op_t> &ops,
                              const std::unordered_map<std::string, action_statement_t> &statements) const;
  // After the plan is walked: every variant a site called (action_variants), declared in its control.
  void emit_action_variants(const TofinoContext *tofino_ctx);
  // The shared runs joining at `join`, an If just closed: their calls, each under its flag.
  void emit_shared_runs_after(const EPNode *join);
  // The calls of a shared run's actions, companions included, at the time the join is written:
  // an action whose every statement folded into another (an exit xor into its reader) is not
  // declared, so it is not called either.
  std::vector<code_t> shared_run_calls(const shared_run_t &run) const;
  // Every op of this action emitted an empty statement, so emit_compute_run declared nothing:
  // calling it would name a declaration the program does not have.
  bool compute_action_folded(const DS_ID &action) const;
  // `action` holds a loop's step: the op that changes a state value between two iterations, which
  // writes the word of the state it changes (the words are by role). An op of the iteration before
  // it can still read that word in the step's own stage -- the hardware hands both the old value --
  // but P4 statements run in order, so the step's call goes after the stage's other calls.
  bool overwrites_loop_state(const DS_ID &action) const;
  // Compute actions in the order their calls are written: by stage, a stage's loop steps last.
  bool called_before(const DS_ID &a, const DS_ID &b) const;
  // The symbols anything past the cut at `cut_node` still uses: what a BDD node reachable from it
  // reads, plus what a later hand-off to the controller ships. `next` is the cut's continuation.
  std::unordered_set<std::string> live_symbols_past(const EP *ep, const BDDNode *cut_node, const EPNode *next) const;
  // The recirculation header slot for the carried variable `name`: the slot it had on another
  // pass, or a fresh one no remembered name owns. The pass's slot counters restart at
  // begin_recirc_slots.
  size_t recirc_slot_for(const code_t &name, const code_t &slot_kind);
  void begin_recirc_slots() { recirc_slot_next.clear(); }

  coder_t &get(const std::string &marker);

  code_t create_unique_name(const code_t &name);
  var_t alloc_var(const code_t &name, klee::ref<klee::Expr> expr, alloc_opt_t option = 0);
  var_t alloc_var(const code_t &name, bits_t size, alloc_opt_t option = 0);
  code_path_t alloc_recirc_coder();

  code_t build_register_action_name(const Register *reg, RegisterActionType action, const EPNode *node = nullptr) const;

  struct fcfs_cs_internals_t {
    code_t liveness_query;
    code_t liveness_query_and_refresh;
    std::vector<var_t> keys;
    std::map<std::pair<DS_ID, RegisterActionType>, code_t> keys_reg_actions;
  };

  fcfs_cs_internals_t fcfs_cs_get_internals(const FCFSCachedSet *fcfs_cs);

  struct fcfs_ct_internals_t {
    code_t liveness_query;
    code_t liveness_query_and_refresh;
    std::vector<var_t> keys;
    std::map<std::pair<DS_ID, RegisterActionType>, code_t> keys_reg_actions;
    std::map<std::pair<DS_ID, RegisterActionType>, code_t> index_to_keys_reg_actions;
  };

  fcfs_ct_internals_t fcfs_ct_get_internals(const FCFSCachedTable *fcfs_ct);

  std::unordered_map<RegisterActionType, std::vector<code_t>> cms_get_rows_reg_actions(const CountMinSketch *cms);
  std::unordered_map<RegisterActionType, std::vector<code_t>> cms_get_rows_actions(const CountMinSketch *cms);
  std::unordered_map<RegisterActionType, std::vector<code_t>> cms_get_rows_values(const CountMinSketch *cms);
  std::vector<code_t> cms_get_hashes_values(const CountMinSketch *cms);
  std::vector<code_t> cms_get_hashes_calculators(const CountMinSketch *cms, const EPNode *ep_node);

  std::unordered_map<RegisterActionType, std::vector<code_t>> bf_get_rows_reg_actions(const BloomFilter *bf);
  std::unordered_map<RegisterActionType, std::vector<code_t>> bf_get_rows_actions(const BloomFilter *bf);
  std::unordered_map<RegisterActionType, std::vector<code_t>> bf_get_rows_values(const BloomFilter *bf);
  var_t bf_get_estimate_value(const BloomFilter *bf);

  void transpile_parser(const Parser &parser);
  void transpile_action_decl(const code_t &action_name, const std::vector<code_t> &body);
  void transpile_action_decl(const code_t &action_name, const std::vector<klee::ref<klee::Expr>> &params, bool params_are_buffers);
  void transpile_table_decl(const Table *table, const std::vector<klee::ref<klee::Expr>> &keys, const std::vector<klee::ref<klee::Expr>> &values,
                            bool values_are_buffers, std::vector<var_t> &keys_vars);
  void transpile_table_decl(const Table *table, const std::vector<var_t> &keys_vars, const std::vector<klee::ref<klee::Expr>> &values,
                            bool values_are_buffers);
  void transpile_register_decl(const Register *reg);

  struct register_action_extras_t {
    std::optional<code_t> external_var;
    std::optional<u64> extra_constant;
    std::optional<klee::ref<klee::Expr>> extra_condition;
    std::optional<klee::ref<klee::Expr>> write_value;
    std::map<klee::ref<klee::Expr>, code_t> temporary_transpilations;
  };

  void transpile_register_action_decl(const Register *reg, const code_t &action_name, RegisterActionType type,
                                      std::optional<register_action_extras_t> extras = std::nullopt);
  // Emit a register `.execute()`. When the index is a computed (non-constant) value the
  // register is placed by bf-p4c as a keyless `hash_action` table (hash distribution for
  // addressing), which is legal only if its action carries NO action data -- and bf-p4c
  // fuses the following data-independent, constant-bearing statement into it (which BDD
  // reorder can place right after the execute), breaking that. So for a computed index we
  // wrap the execute in its own named action (nothing can fuse in; mirrors the expert's
  // regexec_* actions). For a constant index we emit it inline, unchanged. `lhs` is the
  // (metadata) destination of the returned value, or empty for a value-less execute.
  void emit_register_execute(const code_t &lhs, const code_t &action_name, const klee::ref<klee::Expr> &index, const code_t &index_code,
                             const EPNode *ep_node);
  void transpile_hash_decl(const Hash *hash);
  void transpile_hash_calculation(const Hash *hash, const std::vector<code_t> &inputs, code_t &hash_calculator, code_t &output_hash);
  void transpile_digest_decl(const Digest *digest);
  void transpile_fcfs_ct_decl(const FCFSCachedTable *fcfs_ct, const EPNode *ep_node);
  void transpile_fcfs_ct_hash_calculation(const Hash *hash, const std::vector<code_t> &inputs, const var_t &fcfs_ct_value, code_t &hash_calculator,
                                          code_t &output_hash);
  void transpile_fcfs_cs_decl(const FCFSCachedSet *fcfs_cs, const EPNode *ep_node);
  void transpile_lpm_decl(const LPM *lpm, klee::ref<klee::Expr> addr, klee::ref<klee::Expr> device);

  void transpile_cms_hash_calculator_decl(const CountMinSketch *cms, const EPNode *ep_node, const std::vector<var_t> &keys_vars);
  void transpile_cms_decl(const CountMinSketch *cms, const EPNode *ep_node);

  // The bloom filter's rows, and the site's actions on them of `action_type`, one per row: each
  // hashes the key inside the register's execute, from the key's fields as they are (an ALU copy
  // into metadata would tie the fields to the chain's cluster), with the site's own Hash
  // instances declared ahead. An action declared at an earlier site with the same inputs is
  // reused; with other inputs the site gets its own. Returns the site's actions, in row order.
  std::vector<code_t> transpile_bf_decl(const BloomFilter *bf, const EPNode *ep_node, const std::vector<code_t> &key_inputs,
                                        RegisterActionType action_type);
  std::unordered_map<code_t, std::vector<code_t>> bf_action_inputs; // A row action -> the key inputs it hashes.
  void transpile_cuckoo_hash_table_decl(const CuckooHashTable *cuckoo_hash_table);
  void transpile_if_condition(const If::condition_t &condition);
  void transpile_digest(const Digest &digest, const std::vector<code_t> &fields);

  void declare_var_in_ingress_metadata(const var_t &var);
  void dbg_vars() const;

  static code_t get_parser_state_name(const ParserState *state, bool state_init);

  friend class Transpiler;
};

} // namespace Tofino
} // namespace LibSynapse