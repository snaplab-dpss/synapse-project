#include <LibBDD/BDD.h>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <CLI/CLI.hpp>

using namespace LibCore;
using namespace LibBDD;

namespace {

std::string operand_to_string(const loop_operand_t &operand) {
  switch (operand.kind) {
  case LoopOperandKind::Body:
    return "b" + std::to_string(operand.index);
  case LoopOperandKind::State:
    return "prev.b" + std::to_string(operand.index);
  case LoopOperandKind::Outside:
    return expr_to_string(operand.expr, true);
  }
  return "?";
}

// One line per body op; also what tells two loops with the same body apart from the rest.
std::string body_to_string(const loop_t &loop) {
  std::stringstream ss;
  for (size_t i = 0; i < loop.body.size(); i++) {
    const loop_body_op_t &op = loop.body[i];
    ss << "    b" << i << " = " << op.fn << "(";
    for (size_t j = 0; j < op.operands.size(); j++) {
      ss << (j ? ", " : "") << operand_to_string(op.operands[j]);
    }
    ss << ") : w" << op.width << "\n";
  }
  return ss.str();
}

} // namespace

int main(int argc, char **argv) {
  CLI::App app{"Detect the loops symbolic execution unrolled into a BDD"};

  std::filesystem::path input_bdd_file;
  std::filesystem::path dot_file;
  bool show_nodes{false};
  bool trace{false};

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")->required();
  app.add_option("--dot", dot_file, "Write the BDD as a dot file, the detected loops highlighted.");
  app.add_flag("--nodes", show_nodes, "Print the node computing every body op in every iteration.");
  app.add_flag("--trace", trace, "Print what the detection tried and why each attempt failed.");

  CLI11_PARSE(app, argc, argv);

  SymbolManager manager;
  const BDD bdd(input_bdd_file, &manager);
  const std::vector<loop_t> loops = bdd.detect_loops(trace ? &std::cerr : nullptr);

  std::cout << loops.size() << " loop(s) in " << input_bdd_file.filename().string() << "\n";

  std::vector<std::string> bodies;
  for (size_t l = 0; l < loops.size(); l++) {
    const loop_t &loop     = loops[l];
    const std::string body = body_to_string(loop);

    std::cout << "\nLoop " << l << ": " << loop.iterations.size() << " iterations, body of " << loop.body.size() << " ops, state:";
    for (size_t s : loop.state) {
      std::cout << " b" << s;
    }
    std::cout << "\n";

    for (size_t other = 0; other < bodies.size(); other++) {
      if (bodies[other] == body) {
        std::cout << "  same body as loop " << other << "\n";
        break;
      }
    }
    bodies.push_back(body);

    std::cout << "  body:\n" << body;

    std::cout << "  iterations:\n";
    for (size_t k = 0; k < loop.iterations.size(); k++) {
      const std::vector<std::optional<bdd_node_id_t>> &iteration = loop.iterations[k];
      size_t present                                             = 0;
      for (const std::optional<bdd_node_id_t> &node : iteration) {
        present += node.has_value();
      }
      std::cout << "    " << k << ": " << present << "/" << iteration.size() << " ops";
      if (present < iteration.size()) {
        std::cout << " (missing:";
        for (size_t i = 0; i < iteration.size(); i++) {
          if (!iteration[i]) {
            std::cout << " b" << i;
          }
        }
        std::cout << ")";
      }
      if (show_nodes) {
        std::cout << " nodes:";
        for (const std::optional<bdd_node_id_t> &node : iteration) {
          std::cout << " " << (node ? std::to_string(*node) : "-");
        }
      }
      std::cout << "\n";
    }

    std::cout << "  steps:\n";
    for (const loop_step_t &step : loop.steps) {
      std::cout << "    after iteration " << step.after_iteration << ": node " << step.node << " changes b" << step.state << "\n";
    }

    std::cout << "  entry:";
    for (bdd_node_id_t node : loop.entry) {
      std::cout << " " << node;
    }
    std::cout << "\n";
  }

  if (!dot_file.empty()) {
    // Consecutive iterations alternate between two colours, so their boundary shows; steps and the
    // entry have one colour each. A node carrying several body ops (a rotate and the operation it
    // holds inline) lists them all.
    const Color even_iteration(173, 216, 230);
    const Color odd_iteration(144, 238, 144);
    bdd_visualizer_opts_t opts;
    opts.fname          = dot_file;
    opts.default_color  = Color(Color::Literal::White);
    const auto annotate = [&](bdd_node_id_t node, const std::string &note) {
      std::string &annotation = opts.annotations_per_node[node];
      annotation += (annotation.empty() ? "" : ", ") + note;
    };
    for (size_t l = 0; l < loops.size(); l++) {
      const loop_t &loop = loops[l];
      for (size_t k = 0; k < loop.iterations.size(); k++) {
        for (size_t i = 0; i < loop.iterations[k].size(); i++) {
          if (const std::optional<bdd_node_id_t> node = loop.iterations[k][i]) {
            opts.colors_per_node[*node] = k % 2 ? odd_iteration : even_iteration;
            annotate(*node, "loop " + std::to_string(l) + " it " + std::to_string(k) + " b" + std::to_string(i));
          }
        }
      }
      for (const loop_step_t &step : loop.steps) {
        opts.colors_per_node[step.node] = Color(Color::Literal::Orange);
        annotate(step.node,
                 "loop " + std::to_string(l) + " step after it " + std::to_string(step.after_iteration) + " into b" + std::to_string(step.state));
      }
      for (bdd_node_id_t node : loop.entry) {
        opts.colors_per_node[node] = Color(Color::Literal::Yellow);
        annotate(node, "loop " + std::to_string(l) + " entry");
      }
    }
    BDDViz::dump_to_file(&bdd, opts);
    std::cout << "\nWrote " << dot_file.string() << "\n";
  }

  return 0;
}
