#include "call.h"
#include "manager.h"
#include "../bdd.h"
#include "../../exprs/exprs.h"

namespace synapse {
Node *Call::clone(NodeManager &manager, bool recursive) const {
  Call *clone;

  if (recursive && next) {
    Node *next_clone = next->clone(manager, true);
    clone = new Call(id, next_clone, nullptr, constraints, call, generated_symbols);
    next_clone->set_prev(clone);
  } else {
    clone = new Call(id, constraints, call, generated_symbols);
  }

  manager.add_node(clone);
  return clone;
}

std::string Call::dump(bool one_liner, bool id_name_only) const {
  std::stringstream ss;
  ss << id << ":";
  ss << call.function_name;

  if (id_name_only) {
    return ss.str();
  }

  ss << "(";

  bool first = true;
  for (auto &arg : call.args) {
    if (!first)
      ss << ", ";
    ss << arg.first << ":";
    ss << expr_to_string(arg.second.expr, one_liner);
    if (!arg.second.in.isNull() || !arg.second.out.isNull()) {
      ss << "[";
    }
    if (!arg.second.in.isNull()) {
      ss << expr_to_string(arg.second.in, one_liner);
    }
    if (!arg.second.out.isNull()) {
      ss << " -> ";
      ss << expr_to_string(arg.second.out, one_liner);
    }
    if (!arg.second.in.isNull() || !arg.second.out.isNull()) {
      ss << "]";
    }
    first = false;
  }
  ss << ")";
  return ss.str();
}

const symbols_t &Call::get_local_symbols() const { return generated_symbols; }

symbol_t Call::get_local_symbol(const std::string &base) const {
  SYNAPSE_ASSERT(!base.empty(), "Empty base");
  SYNAPSE_ASSERT(!generated_symbols.empty(), "No symbols");

  for (const symbol_t &symbol : generated_symbols) {
    if (symbol.base == base) {
      return symbol;
    }
  }

  SYNAPSE_PANIC("Symbol %s not found", base.c_str());
}

bool Call::has_local_symbol(const std::string &base) const {
  for (const symbol_t &symbol : generated_symbols) {
    if (symbol.base == base) {
      return true;
    }
  }

  return false;
}

} // namespace synapse