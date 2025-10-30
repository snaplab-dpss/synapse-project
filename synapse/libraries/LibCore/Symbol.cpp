#include <LibCore/Symbol.h>
#include <LibCore/Expr.h>
#include <LibCore/Debug.h>

#include <klee/util/ExprVisitor.h>

#include <regex>

namespace LibCore {

namespace {
class SymbolNamesRetriever : public klee::ExprVisitor::ExprVisitor {
private:
  std::unordered_set<std::string> names;

public:
  SymbolNamesRetriever() {}

  Action visitRead(const klee::ReadExpr &e) {
    klee::ref<klee::ReadExpr> expr = const_cast<klee::ReadExpr *>(&e);
    const klee::UpdateList &ul     = e.updates;
    const klee::Array *root        = ul.root;
    names.insert(root->name);
    return Action::doChildren();
  }

  const std::unordered_set<std::string> &get_names() const { return names; }
};

} // namespace

symbol_t::symbol_t(const std::string &_base, const std::string &_name, klee::ref<klee::Expr> _expr) : base(_base), name(_name), expr(_expr) {}

symbol_t::symbol_t(klee::ref<klee::Expr> _expr) : expr(_expr) {
  SymbolNamesRetriever retriever;
  retriever.visit(expr);
  const std::unordered_set<std::string> &names = retriever.get_names();
  assert(names.size() == 1 && "Invalid number of symbols");
  name = *names.begin();
  base = base_from_name(name);
}

bool symbol_t::is_symbol(klee::ref<klee::Expr> expr) {
  if (expr.isNull()) {
    return false;
  }

  SymbolNamesRetriever retriever;
  retriever.visit(expr);
  return retriever.get_names().size() == 1;
}

std::unordered_set<std::string> symbol_t::get_symbols_names(klee::ref<klee::Expr> expr) {
  if (expr.isNull()) {
    return {};
  }

  SymbolNamesRetriever retriever;
  retriever.visit(expr);
  return retriever.get_names();
}
std::size_t Symbols::symbol_hash_t::operator()(const symbol_t &s) const noexcept {
  std::hash<std::string> hash_fn;
  return hash_fn(s.name);
}

std::string symbol_t::base_from_name(std::string name) {
  assert(name.size() && "Empty name");

  // Regex to capture the base name before any suffixes like _123 or __456
  // ([a-zA-Z_]*[a-zA-Z]) captures the base name
  // (?:_\\d+)? non-capturing group for optional _123 suffix
  // (?:__\\d+)? non-capturing group for optional __456 suffix
  const std::regex pattern("^([a-zA-Z_]*[a-zA-Z])(?:_\\d+)?(?:__\\d+)?$");
  std::smatch match_results;
  if (std::regex_match(name, match_results, pattern) && match_results.size() > 1) {
    return match_results[1];
  }

  return name;
}

bool Symbols::symbol_equal_t::operator()(const symbol_t &a, const symbol_t &b) const noexcept { return a.name == b.name; }

Symbols::Symbols(const std::vector<symbol_t> &symbols) {
  for (const symbol_t &symbol : symbols) {
    add(symbol);
  }
}

void Symbols::add(const symbol_t &symbol) { data.insert(symbol); }

void Symbols::add(const Symbols &symbols) {
  for (const symbol_t &symbol : symbols.data) {
    add(symbol);
  }
}

std::vector<symbol_t> Symbols::get() const {
  std::vector<symbol_t> result;
  result.insert(result.end(), data.begin(), data.end());
  return result;
}

bool Symbols::has(const std::string &name) const {
  auto is_target = [name](const symbol_t &s) { return s.name == name || (name == "packet_chunks" && s.base == "packet_chunks"); };
  return std::find_if(data.begin(), data.end(), is_target) != data.end();
}

size_t Symbols::size() const { return data.size(); }

bool Symbols::empty() const { return data.empty(); }

symbol_t Symbols::get(const std::string &name) const {
  for (const symbol_t &symbol : data) {
    if (symbol.name == name) {
      return symbol;
    }
  }
  panic("Symbol not found");
}

bool Symbols::remove(const std::string &name) {
  auto is_target = [name](const symbol_t &s) { return s.name == name; };
  return std::erase_if(data, is_target) > 0;
}

Symbols Symbols::filter_by_base(const std::string &base) const {
  Symbols result;
  for (const symbol_t &symbol : data) {
    if (symbol.base == base) {
      result.data.insert(symbol);
    }
  }
  return result;
}

Symbols Symbols::intersect(const Symbols &symbols) const {
  Symbols result;
  for (const symbol_t &symbol : data) {
    if (symbols.has(symbol.name)) {
      result.data.insert(symbol);
    }
  }
  return result;
}

std::ostream &operator<<(std::ostream &os, const symbol_t &symbol) {
  os << "symbol{";
  os << "base=" << symbol.base << ", ";
  os << "name=" << symbol.name << ", ";
  os << "expr=" << expr_to_string(symbol.expr, true) << "}";
  return os;
}

std::ostream &operator<<(std::ostream &os, const Symbols &symbols) {
  os << "Symbols:\n";
  for (const symbol_t &symbol : symbols.data) {
    os << "  " << symbol << "\n";
  }
  os << "\n";
  return os;
}

} // namespace LibCore