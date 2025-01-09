#include <fstream>
#include <iostream>

#include "bdd.h"

#include "nodes/call.h"
#include "nodes/branch.h"
#include "nodes/route.h"
#include "../util/exprs.h"
#include "../system.h"
#include "../util/kQuery.h"

namespace synapse {
namespace {
constexpr const char *const MAGIC_SIGNATURE = "===== BDD =====";
constexpr const char *const KQUERY_DELIMITER = ";;-- kQuery --";
constexpr const char *const SYMBOLS_DELIMITER = ";;-- Symbols --";
constexpr const char *const INIT_DELIMITER = ";;-- Init --";
constexpr const char *const NODES_DELIMITER = ";; -- Nodes --";
constexpr const char *const EDGES_DELIMITER = ";; -- Edges --";
constexpr const char *const ROOT_DELIMITER = ";; -- Root --";

std::string serialize_expr(klee::ref<klee::Expr> expr, kQuery_t &kQuery) {
  assert(!expr.isNull() && "Null expr");
  auto expr_str = expr_to_string(expr);
  kQuery.values.push_back(expr);
  return expr_str;
}

std::string serialize_call(const call_t &call, kQuery_t &kQuery) {
  std::stringstream call_stream;
  std::string expr_str;

  call_stream << call.function_name;
  call_stream << "(";

  bool first = true;
  for (const auto &arg_pair : call.args) {
    const std::string &arg_name = arg_pair.first;
    const arg_t &arg = arg_pair.second;

    if (first) {
      first = false;
    } else {
      call_stream << ",";
    }

    call_stream << arg_name;
    call_stream << ":";

    expr_str = serialize_expr(arg.expr, kQuery);
    call_stream << expr_str;

    if (arg.fn_ptr_name.first) {
      call_stream << "&";
      call_stream << arg.fn_ptr_name.second;
      continue;
    }

    if (arg.in.isNull() && arg.out.isNull()) {
      continue;
    }

    call_stream << "&[";

    if (!arg.in.isNull()) {
      expr_str = serialize_expr(arg.in, kQuery);
      call_stream << expr_str;
    }

    call_stream << "->";

    if (!arg.out.isNull()) {
      expr_str = serialize_expr(arg.out, kQuery);
      call_stream << expr_str;
    }

    call_stream << "]";

    call_stream << "[";
    for (auto i = 0u; i < arg.meta.size(); i++) {
      const meta_t &meta = arg.meta[i];

      if (i != 0) {
        call_stream << ",";
      }

      call_stream << "{";
      call_stream << meta.symbol;
      call_stream << ",";
      call_stream << meta.offset;
      call_stream << ",";
      call_stream << meta.size;
      call_stream << "}";
    }
    call_stream << "]";
  }

  call_stream << ")";

  if (call.extra_vars.size()) {
    bool first = true;
    call_stream << "*{";

    for (const auto &extra_var_pair : call.extra_vars) {
      const std::string &extra_var_name = extra_var_pair.first;
      const extra_var_t &extra_var = extra_var_pair.second;

      if (first) {
        first = false;
      } else {
        call_stream << ",";
      }

      call_stream << extra_var_name;
      call_stream << ":";

      klee::ref<klee::Expr> in = extra_var.first;
      klee::ref<klee::Expr> out = extra_var.second;

      call_stream << "[";

      if (!in.isNull()) {
        expr_str = serialize_expr(in, kQuery);
        call_stream << expr_str;
      }

      call_stream << "->";

      if (!out.isNull()) {
        expr_str = serialize_expr(out, kQuery);
        call_stream << expr_str;
      }

      call_stream << "]";
    }

    call_stream << "}*";
  }

  call_stream << "->";

  if (call.ret.isNull()) {
    call_stream << "[]";
  } else {
    expr_str = serialize_expr(call.ret, kQuery);
    call_stream << expr_str;
  }

  return call_stream.str();
}

std::string serialize_symbols(const Symbols &symbols, kQuery_t &kQuery) {
  std::stringstream symbols_stream;

  symbols_stream << "=>";
  symbols_stream << "<";

  bool first = true;
  for (const symbol_t &symbol : symbols.get()) {
    if (!first) {
      symbols_stream << ",";
    } else {
      first = false;
    }

    symbols_stream << "{";
    symbols_stream << symbol.base;
    symbols_stream << ":";
    symbols_stream << symbol.name;
    symbols_stream << ":";
    symbols_stream << serialize_expr(symbol.expr, kQuery);
    symbols_stream << "}";
  }
  symbols_stream << ">";

  return symbols_stream.str();
}

void serialize_init(const std::vector<call_t> &calls, std::stringstream &init_stream, kQuery_t &kQuery) {
  for (const call_t &call : calls) {
    init_stream << serialize_call(call, kQuery);
    init_stream << "\n";
  }
}

klee::ref<klee::Expr> pop_expr(std::vector<klee::ref<klee::Expr>> &exprs) {
  klee::ref<klee::Expr> expr = exprs[0];
  exprs.erase(exprs.begin());
  return expr;
}

std::vector<meta_t> parse_meta(const std::string &meta_str) {
  std::vector<meta_t> meta;
  std::vector<std::string> elems(3);

  auto lvl = 0;
  auto curr_el = 0;

  for (auto c : meta_str) {
    if (c == '{') {
      lvl++;
      continue;
    }

    if (c == '}') {
      lvl--;

      auto symbol = elems[0];
      auto offset = static_cast<bits_t>(std::stoi(elems[1]));
      auto size = static_cast<bits_t>(std::stoi(elems[2]));

      auto m = meta_t{symbol, offset, size};
      meta.push_back(m);

      continue;
    }

    if (c == ',' && lvl == 0) {
      for (auto &el : elems) {
        el.clear();
      }

      curr_el = 0;
      continue;
    }

    else if (c == ',') {
      curr_el++;
      continue;
    }

    elems[curr_el] += c;
  }

  return meta;
}

std::pair<std::string, arg_t> parse_arg(std::string serialized_arg, std::vector<klee::ref<klee::Expr>> &exprs) {
  std::string arg_name;
  arg_t arg;

  size_t delim = serialized_arg.find(":");
  assert(delim != std::string::npos && "Invalid arg");

  arg_name = serialized_arg.substr(0, delim);
  serialized_arg = serialized_arg.substr(delim + 1);

  std::string expr_str;
  std::string in_str;
  std::string out_str;
  std::string fn_ptr_name;
  std::string meta_str;

  delim = serialized_arg.find("&");

  if (delim == std::string::npos) {
    expr_str = serialized_arg;
  } else {
    expr_str = serialized_arg.substr(0, delim);
    serialized_arg = serialized_arg.substr(delim + 1);

    delim = serialized_arg.find("[");

    if (delim == std::string::npos) {
      fn_ptr_name = serialized_arg;
    } else {
      serialized_arg = serialized_arg.substr(delim + 1);

      delim = serialized_arg.find("->");
      assert(delim != std::string::npos && "Invalid arg");

      in_str = serialized_arg.substr(0, delim);

      serialized_arg = serialized_arg.substr(delim + 2);

      delim = serialized_arg.find("]");
      assert(delim != std::string::npos && "Invalid arg");

      out_str = serialized_arg.substr(0, delim);
      meta_str = serialized_arg.substr(delim + 1);

      auto meta_start = meta_str.find("[");
      auto meta_end = meta_str.find("]");

      assert(meta_start != std::string::npos && "Invalid arg");
      assert(meta_end != std::string::npos && "Invalid arg");

      meta_str = meta_str.substr(meta_start + 1, meta_end - 1);
      arg.meta = parse_meta(meta_str);
    }
  }

  if (expr_str.size()) {
    arg.expr = pop_expr(exprs);
  }

  if (fn_ptr_name.size()) {
    arg.fn_ptr_name = std::make_pair(true, fn_ptr_name);
  }

  if (in_str.size()) {
    arg.in = pop_expr(exprs);
  }

  if (out_str.size()) {
    arg.out = pop_expr(exprs);
  }

  return std::make_pair(arg_name, arg);
}

std::pair<std::string, extra_var_t> parse_extra_var(std::string serialized_extra_var, std::vector<klee::ref<klee::Expr>> &exprs) {
  std::string extra_var_name;
  klee::ref<klee::Expr> in;
  klee::ref<klee::Expr> out;

  size_t delim = serialized_extra_var.find(":");
  assert(delim != std::string::npos && "Invalid extra var");

  extra_var_name = serialized_extra_var.substr(0, delim);
  serialized_extra_var = serialized_extra_var.substr(delim + 1);

  std::string in_str;
  std::string out_str;

  delim = serialized_extra_var.find("[");
  assert(delim != std::string::npos && "Invalid extra var");

  serialized_extra_var = serialized_extra_var.substr(delim + 1);

  delim = serialized_extra_var.find("->");
  assert(delim != std::string::npos && "Invalid extra var");

  in_str = serialized_extra_var.substr(0, delim);
  out_str = serialized_extra_var.substr(delim + 2);

  delim = out_str.find("]");
  assert(delim != std::string::npos && "Invalid extra var");

  out_str = out_str.substr(0, delim);

  if (in_str.size()) {
    in = pop_expr(exprs);
  }

  if (out_str.size()) {
    out = pop_expr(exprs);
  }

  return std::make_pair(extra_var_name, std::make_pair(in, out));
}

call_t parse_call(std::string serialized_call, std::vector<klee::ref<klee::Expr>> &exprs) {

  call_t call;

  // Cleanup by removing duplicated spaces
  auto new_end = std::unique(serialized_call.begin(), serialized_call.end(), [](char lhs, char rhs) { return lhs == rhs && lhs == ' '; });
  serialized_call.erase(new_end, serialized_call.end());

  size_t delim = serialized_call.find("(");
  assert(delim != std::string::npos && "Invalid call");

  call.function_name = serialized_call.substr(0, delim);
  serialized_call = serialized_call.substr(delim + 1);

  std::vector<std::string> args_str;

  int parenthesis_lvl = 1;
  delim = 0;
  std::string arg_str;
  for (auto c : serialized_call) {
    delim++;
    if (c == '(' || c == '[') {
      parenthesis_lvl++;
    } else if (c == ')' || c == ']') {
      parenthesis_lvl--;

      if (parenthesis_lvl == 0) {
        if (arg_str.size()) {
          args_str.push_back(arg_str);
          arg_str.clear();
        }
        break;
      }
    } else if (c == ',' && parenthesis_lvl == 1) {
      args_str.push_back(arg_str);
      arg_str.clear();

      continue;
    }

    arg_str += c;
  }

  for (auto arg_str : args_str) {
    auto arg_pair = parse_arg(arg_str, exprs);
    call.args[arg_pair.first] = arg_pair.second;
  }

  serialized_call = serialized_call.substr(delim);
  delim = serialized_call.find("*{");

  if (delim != std::string::npos) {
    serialized_call = serialized_call.substr(2);

    delim = serialized_call.find("}*");
    assert(delim != std::string::npos && "Invalid call");

    std::string extra_vars_str = serialized_call.substr(0, delim);
    serialized_call = serialized_call.substr(delim + 2);

    while (extra_vars_str.size()) {
      delim = extra_vars_str.find(",");

      std::string extra_var_str;

      if (delim == std::string::npos) {
        extra_var_str = extra_vars_str;
      } else {
        extra_var_str = extra_vars_str.substr(0, delim);
        extra_vars_str = extra_vars_str.substr(delim + 1);
      }

      auto extra_var_pair = parse_extra_var(extra_var_str, exprs);
      call.extra_vars[extra_var_pair.first] = extra_var_pair.second;

      if (delim == std::string::npos) {
        break;
      }
    }
  }

  delim = serialized_call.find("->");
  assert(delim != std::string::npos && "Invalid call");

  serialized_call = serialized_call.substr(delim + 2);

  if (serialized_call != "[]") {
    call.ret = pop_expr(exprs);
  }

  return call;
}

symbol_t parse_call_symbol(std::string serialized_symbol, std::vector<klee::ref<klee::Expr>> &exprs) {
  size_t delim = serialized_symbol.find(":");
  assert(delim != std::string::npos && "Invalid symbol");

  std::string base = serialized_symbol.substr(0, delim);
  serialized_symbol = serialized_symbol.substr(delim + 1);

  delim = serialized_symbol.find(":");
  assert(delim != std::string::npos && "Invalid symbol");

  std::string name = serialized_symbol.substr(0, delim);
  serialized_symbol = serialized_symbol.substr(delim + 1);

  klee::ref<klee::Expr> expr = pop_expr(exprs);
  symbol_t symbol(expr);

  return symbol;
}

Symbols parse_call_symbols(std::string serialized_symbols, std::vector<klee::ref<klee::Expr>> &exprs) {
  Symbols symbols;

  assert(serialized_symbols[0] == '<' && "Invalid symbols");

  if (serialized_symbols == "<>") {
    return symbols;
  }

  serialized_symbols = serialized_symbols.substr(1);

  std::string symbol_str;
  for (char c : serialized_symbols) {
    if (c == '{') {
      symbol_str.clear();
    } else if (c == '}' && symbol_str.size()) {
      symbol_t symbol = parse_call_symbol(symbol_str, exprs);
      symbols.add(symbol);
    } else if (c == '>') {
      break;
    } else {
      symbol_str += c;
    }
  }

  return symbols;
}

Node *parse_node_call(node_id_t id, const klee::ConstraintManager &constraints, SymbolManager *symbol_manager, std::string serialized,
                      std::vector<klee::ref<klee::Expr>> &exprs, NodeManager &manager) {
  size_t delim = serialized.find("=>");
  assert(delim != std::string::npos && "Invalid call");

  std::string call_str = serialized.substr(0, delim);
  std::string symbols_str = serialized.substr(delim + 2);

  call_t call = parse_call(call_str, exprs);
  Symbols symbols = parse_call_symbols(symbols_str, exprs);

  Call *call_node = new Call(id, constraints, symbol_manager, call, symbols);
  manager.add_node(call_node);
  return call_node;
}

Node *parse_node_branch(node_id_t id, const klee::ConstraintManager &constraints, SymbolManager *symbol_manager, std::string serialized,
                        std::vector<klee::ref<klee::Expr>> &exprs, NodeManager &manager) {
  klee::ref<klee::Expr> condition = pop_expr(exprs);
  Branch *branch_node = new Branch(id, constraints, symbol_manager, condition);
  manager.add_node(branch_node);
  return branch_node;
}

Node *parse_node_route(node_id_t id, const klee::ConstraintManager &constraints, SymbolManager *symbol_manager, std::string serialized,
                       std::vector<klee::ref<klee::Expr>> &exprs, NodeManager &manager) {
  size_t delim = serialized.find(" ");
  assert(delim != std::string::npos && "Invalid route");

  auto route_operation_str = serialized.substr(0, delim);
  auto dst_device_str = serialized.substr(delim + 1);

  Route *route_node;

  if (route_operation_str == "FWD") {
    int dst_device = std::stoi(dst_device_str);
    route_node = new Route(id, constraints, symbol_manager, RouteOp::Forward, dst_device);
  } else if (route_operation_str == "DROP") {
    route_node = new Route(id, constraints, symbol_manager, RouteOp::Drop);
  } else if (route_operation_str == "BCAST") {
    route_node = new Route(id, constraints, symbol_manager, RouteOp::Broadcast);
  } else {
    panic("Unknown route operation");
  }

  manager.add_node(route_node);

  return route_node;
}

Node *parse_node(std::string serialized_node, std::vector<klee::ref<klee::Expr>> &exprs, NodeManager &manager,
                 SymbolManager *symbol_manager) {
  Node *node;

  size_t delim = serialized_node.find(":");
  assert(delim != std::string::npos && "Invalid node");

  node_id_t id = std::stoull(serialized_node.substr(0, delim));
  serialized_node = serialized_node.substr(delim + 1);

  assert(serialized_node[0] == '(' && "Invalid node");
  serialized_node = serialized_node.substr(1);

  delim = serialized_node.find(" ");
  assert(delim != std::string::npos && "Invalid node");

  std::string serialized_constraints_num = serialized_node.substr(0, delim);
  serialized_node = serialized_node.substr(delim + 1);
  int constraints_num = std::atoi(serialized_constraints_num.c_str());
  assert(constraints_num >= 0 && "Invalid node");

  klee::ConstraintManager constraints;
  for (int i = 0; i < constraints_num; i++) {
    klee::ref<klee::Expr> constraint = pop_expr(exprs);
    constraints.addConstraint(constraint);
  }

  delim = serialized_node.find(" ");
  assert(delim != std::string::npos && "Invalid node");

  std::string node_type_str = serialized_node.substr(0, delim);

  serialized_node = serialized_node.substr(delim + 1);
  serialized_node = serialized_node.substr(0, serialized_node.size() - 1);

  if (node_type_str == "CALL") {
    node = parse_node_call(id, constraints, symbol_manager, serialized_node, exprs, manager);
  } else if (node_type_str == "BRANCH") {
    node = parse_node_branch(id, constraints, symbol_manager, serialized_node, exprs, manager);
  } else if (node_type_str == "ROUTE") {
    node = parse_node_route(id, constraints, symbol_manager, serialized_node, exprs, manager);
  } else {
    panic("Unknown node type");
  }

  assert(node && "Invalid node");

  return node;
}
} // namespace

void BDD::serialize(const std::filesystem::path &fpath) const {
  std::ofstream out(fpath.string());

  assert(out && "Could not open file");
  assert(out.is_open() && "File is not open");

  kQuery_t kQuery;

  std::stringstream symbols_stream;
  std::stringstream init_stream;
  std::stringstream nodes_stream;
  std::stringstream edges_stream;

  symbols_stream << serialize_expr(device.expr, kQuery);
  symbols_stream << "\n";
  symbols_stream << serialize_expr(packet_len.expr, kQuery);
  symbols_stream << "\n";
  symbols_stream << serialize_expr(time.expr, kQuery);
  symbols_stream << "\n";

  serialize_init(init, init_stream, kQuery);

  std::vector<const Node *> nodes{root};

  while (nodes.size()) {
    const Node *node = nodes[0];
    nodes.erase(nodes.begin());

    nodes_stream << node->get_id();
    nodes_stream << ":(";

    const klee::ConstraintManager &constraints = node->get_constraints();

    nodes_stream << constraints.size();
    nodes_stream << " ";

    for (klee::ref<klee::Expr> constraint : constraints) {
      serialize_expr(constraint, kQuery);
    }

    switch (node->get_type()) {
    case NodeType::Call: {
      const Call *call_node = dynamic_cast<const Call *>(node);
      const Node *next = node->get_next();
      const Symbols &symbols = call_node->get_local_symbols();

      nodes_stream << "CALL";
      nodes_stream << " ";
      nodes_stream << serialize_call(call_node->get_call(), kQuery);
      nodes_stream << serialize_symbols(symbols, kQuery);

      if (next) {
        edges_stream << "(";
        edges_stream << node->get_id();
        edges_stream << "->";
        edges_stream << next->get_id();
        edges_stream << ")";
        edges_stream << "\n";

        nodes.push_back(next);
      }
    } break;
    case NodeType::Branch: {
      const Branch *branch_node = dynamic_cast<const Branch *>(node);
      klee::ref<klee::Expr> condition = branch_node->get_condition();
      const Node *on_true = branch_node->get_on_true();
      const Node *on_false = branch_node->get_on_false();

      assert(!condition.isNull() && "Null condition");

      nodes_stream << "BRANCH";
      nodes_stream << " ";
      nodes_stream << serialize_expr(condition, kQuery);

      edges_stream << "(";
      edges_stream << node->get_id();
      edges_stream << "->";
      if (on_true) {
        edges_stream << on_true->get_id();
        nodes.push_back(on_true);
      }
      edges_stream << "->";
      if (on_false) {
        edges_stream << on_false->get_id();
        nodes.push_back(on_false);
      }
      edges_stream << ")";
      edges_stream << "\n";
    } break;
    case NodeType::Route: {
      const Route *route_node = dynamic_cast<const Route *>(node);
      const Node *next = node->get_next();

      nodes_stream << "ROUTE";
      nodes_stream << " ";

      switch (route_node->get_operation()) {
      case RouteOp::Forward:
        nodes_stream << "FWD";
        break;
      case RouteOp::Drop:
        nodes_stream << "DROP";
        break;
      case RouteOp::Broadcast:
        nodes_stream << "BCAST";
        break;
      }

      nodes_stream << " ";
      nodes_stream << route_node->get_dst_device();

      if (next) {
        edges_stream << "(";
        edges_stream << node->get_id();
        edges_stream << "->";
        edges_stream << next->get_id();
        edges_stream << ")";
        edges_stream << "\n";

        nodes.push_back(next);
      }
    } break;
    }

    nodes_stream << ")";
    nodes_stream << "\n";
  }

  out << MAGIC_SIGNATURE << "\n";

  out << KQUERY_DELIMITER << "\n";
  out << kQuery.dump(symbol_manager);

  out << SYMBOLS_DELIMITER << "\n";
  out << symbols_stream.str();

  out << INIT_DELIMITER << "\n";
  out << init_stream.str();

  out << NODES_DELIMITER << "\n";
  out << nodes_stream.str();

  out << EDGES_DELIMITER << "\n";
  out << edges_stream.str();

  out << ROOT_DELIMITER << "\n";
  out << root->get_id() << "\n";

  out.close();
}

void process_edge(std::string serialized_edge, std::map<node_id_t, Node *> &nodes) {
  size_t delim = serialized_edge.find("(");
  assert(delim != std::string::npos && "Invalid edge");

  serialized_edge = serialized_edge.substr(delim + 1);

  delim = serialized_edge.find(")");
  assert(delim != std::string::npos && "Invalid edge");

  serialized_edge = serialized_edge.substr(0, delim);

  delim = serialized_edge.find("->");
  assert(delim != std::string::npos && "Invalid edge");

  std::string prev_id_str = serialized_edge.substr(0, delim);
  node_id_t prev_id = std::stoi(prev_id_str);

  assert(nodes.find(prev_id) != nodes.end() && "Invalid edge");
  Node *prev = nodes[prev_id];

  serialized_edge = serialized_edge.substr(delim + 2);

  delim = serialized_edge.find("->");

  if (delim != std::string::npos) {
    assert(prev->get_type() == NodeType::Branch && "Invalid edge");

    std::string on_true_id_str = serialized_edge.substr(0, delim);
    std::string on_false_id_str = serialized_edge.substr(delim + 2);

    Branch *branch_node = dynamic_cast<Branch *>(prev);

    if (on_true_id_str.size()) {
      node_id_t on_true_id = std::stoi(on_true_id_str);
      assert(nodes.find(on_true_id) != nodes.end() && "Invalid edge");
      Node *on_true = nodes[on_true_id];
      branch_node->set_on_true(on_true);
      on_true->set_prev(prev);
    }

    if (on_false_id_str.size()) {
      node_id_t on_false_id = std::stoi(on_false_id_str);
      assert(nodes.find(on_false_id) != nodes.end() && "Invalid edge");
      Node *on_false = nodes[on_false_id];
      branch_node->set_on_false(on_false);
      on_false->set_prev(prev);
    }
  } else {
    std::string next_id_str = serialized_edge;
    node_id_t next_id = std::stoi(next_id_str);

    assert(nodes.find(next_id) != nodes.end() && "Invalid edge");
    Node *next = nodes[next_id];

    prev->set_next(next);
    next->set_prev(prev);
  }
}

void BDD::deserialize(const std::filesystem::path &fpath) {
  std::ifstream bdd_file(fpath.string());

  if (!bdd_file) {
    panic("Unable to open BDD file \"%s\"", fpath.c_str());
  }

  enum class state_t {
    STATE_START,
    STATE_KQUERY,
    STATE_SYMBOLS,
    STATE_INIT,
    STATE_NODES,
    STATE_EDGES,
    STATE_ROOT,
  } state = state_t::STATE_START;

  auto get_next_state = [](state_t state, const std::string &line) {
    if (line == KQUERY_DELIMITER)
      return state_t::STATE_KQUERY;
    if (line == SYMBOLS_DELIMITER)
      return state_t::STATE_SYMBOLS;
    if (line == INIT_DELIMITER)
      return state_t::STATE_INIT;
    if (line == NODES_DELIMITER)
      return state_t::STATE_NODES;
    if (line == EDGES_DELIMITER)
      return state_t::STATE_EDGES;
    if (line == ROOT_DELIMITER)
      return state_t::STATE_ROOT;
    return state;
  };

  std::string kQueryStr;
  std::vector<klee::ref<klee::Expr>> exprs;
  std::map<node_id_t, Node *> nodes;
  std::string current_node;

  bool magic_check = false;
  int parenthesis_level = 0;

  while (!bdd_file.eof()) {
    std::string line;
    std::getline(bdd_file, line);

    if (line.empty()) {
      continue;
    }

    switch (state) {
    case state_t::STATE_START: {
      magic_check |= line == MAGIC_SIGNATURE;
      assert(magic_check && "Not a BDD file (missing magic signature)");
    } break;

    case state_t::STATE_KQUERY: {
      kQueryStr += line + "\n";

      if (get_next_state(state, line) != state) {
        kQueryParser parser(symbol_manager);
        kQuery_t kQuery = parser.parse(kQueryStr);
        assert(kQuery.constraints.empty() && "Invalid kQuery");
        exprs = kQuery.values;
      }
    } break;

    case state_t::STATE_SYMBOLS: {
      if (get_next_state(state, line) != state)
        break;

      device = symbol_manager->get_symbol("DEVICE");
      pop_expr(exprs);
      std::getline(bdd_file, line);

      packet_len = symbol_manager->get_symbol("pkt_len");
      pop_expr(exprs);
      std::getline(bdd_file, line);

      time = symbol_manager->get_symbol("next_time");
      pop_expr(exprs);
      std::getline(bdd_file, line);

    } break;

    case state_t::STATE_INIT: {
      if (get_next_state(state, line) != state)
        break;

      call_t call = parse_call(line, exprs);
      init.push_back(call);
    } break;

    case state_t::STATE_NODES: {
      if (get_next_state(state, line) != state)
        break;

      current_node += line;

      for (auto c : line) {
        if (c == '(') {
          parenthesis_level++;
        } else if (c == ')') {
          parenthesis_level--;
        }
      }

      if (parenthesis_level == 0) {
        Node *node = parse_node(current_node, exprs, manager, symbol_manager);

        assert(node && "Invalid node");
        assert(nodes.find(node->get_id()) == nodes.end() && "Duplicate node");

        id = std::max(id, node->get_id()) + 1;

        nodes[node->get_id()] = node;
        current_node.clear();
      }
    } break;

    case state_t::STATE_EDGES: {
      if (get_next_state(state, line) != state)
        break;
      process_edge(line, nodes);
    } break;

    case state_t::STATE_ROOT: {
      if (get_next_state(state, line) != state)
        break;

      node_id_t root_id = std::stoll(line);
      assert(nodes.find(root_id) != nodes.end() && "Invalid root node");

      root = nodes[root_id];
    } break;
    }

    state = get_next_state(state, line);
  }
}
} // namespace synapse