#include <LibSynapse/Modules/Tofino/TNA/TNA.h>

namespace LibSynapse {
namespace Tofino {

using LibCore::get_unique_symbolic_reads;
using LibCore::symbolic_read_t;
using LibCore::symbolic_reads_t;

bool TNA::condition_meets_phv_limit(klee::ref<klee::Expr> expr) const {
  bytes_t total_packet_bytes_read = 0U;
  symbolic_reads_t bytes_read     = get_unique_symbolic_reads(expr);
  for (const symbolic_read_t &byte_read : bytes_read) {
    if (byte_read.symbol == "packet_chunks") {
      total_packet_bytes_read += 1;
    }
  }
  return total_packet_bytes_read <= tna_config.properties.max_packet_bytes_in_condition;
}

void TNA::debug() const { pipeline.debug(); }

std::vector<tofino_port_t> TNA::plausible_ingress_ports_in_bdd_node(const BDD *bdd, const BDDNode *node) const {
  std::vector<tofino_port_t> plausible_ports;

  const klee::ConstraintManager constraints = bdd->get_constraints(node);
  for (const tofino_port_t port : tna_config.ports) {
    klee::ref<klee::Expr> handles_port = solver_toolbox.exprBuilder->Eq(
        bdd->get_device().expr, solver_toolbox.exprBuilder->Constant(port.nf_device, bdd->get_device().expr->getWidth()));
    if (solver_toolbox.is_expr_maybe_true(constraints, handles_port)) {
      plausible_ports.push_back(port);
    }
  }

  return plausible_ports;
}

} // namespace Tofino
} // namespace LibSynapse