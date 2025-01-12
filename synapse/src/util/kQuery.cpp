#include "kQuery.h"

#include "../system.h"
#include "../util/exprs.h"

#include <unordered_map>

#include <llvm/Support/MemoryBuffer.h>

#include <klee/ExprBuilder.h>
#include <klee/perf-contracts.h>
#include <klee/Constraints.h>
#include <klee/Solver.h>
#include <expr/Parser.h>
#include <klee/util/ExprVisitor.h>

namespace synapse {

namespace {
class ArrayReplacer : public klee::ExprVisitor::ExprVisitor {
private:
  std::unique_ptr<klee::ExprBuilder> builder;
  std::unordered_map<std::string, const klee::Array *> name_to_array;
  std::unordered_map<const klee::Expr *, klee::ref<klee::Expr>> replacements;

public:
  ArrayReplacer(const std::vector<const klee::Array *> &arrays)
      : klee::ExprVisitor::ExprVisitor(true), builder(klee::createDefaultExprBuilder()) {
    for (const klee::Array *array : arrays) {
      name_to_array.insert({array->name, array});
    }
  }

  Action visitExprPost(const klee::Expr &e) override final {
    auto it = replacements.find(&e);

    if (it != replacements.end()) {
      return Action::changeTo(it->second);
    } else {
      return Action::doChildren();
    }
  }

  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e) override final {
    klee::UpdateList updates = e.updates;
    assert(updates.getSize() == 0 && "TODO: handle updates");

    auto name_to_array_it = name_to_array.find(updates.root->name);

    if (name_to_array_it != name_to_array.end()) {
      klee::UpdateList new_updates(name_to_array_it->second, nullptr);
      klee::ref<klee::Expr> new_read = builder->Read(new_updates, e.index);

      if (replacements.find(&e) == replacements.end()) {
        replacements.insert({&e, new_read});
      }

      return Action::doChildren();
    }

    return Action::doChildren();
  }
};

// The LLVM MemoryBufferMem is private, and using MemoryBuffer::getMemBuffer causes memory
// leaks. This is a workaround to create a MemoryBufferMem.
class MemoryBufferMem : public llvm::MemoryBuffer {
public:
  MemoryBufferMem(llvm::StringRef InputData, bool RequiresNullTerminator) {
    init(InputData.begin(), InputData.end(), RequiresNullTerminator);
  }

  virtual const char *getBufferIdentifier() const override {
    // The name is stored after the class itself.
    return reinterpret_cast<const char *>(this + 1);
  }

  virtual BufferKind getBufferKind() const override { return MemoryBuffer_Malloc; }
};

} // namespace

std::string kQuery_t::dump(const SymbolManager *manager) const {
  std::stringstream kQuery_builder;

  for (const klee::Array *array : manager->get_arrays()) {
    kQuery_builder << "array " << array->getName();
    kQuery_builder << "[" << array->getSize() << "]";
    kQuery_builder << " : ";
    kQuery_builder << "w" << array->getDomain();
    kQuery_builder << " -> ";
    kQuery_builder << "w" << array->getRange();
    kQuery_builder << " = ";
    if (array->isSymbolicArray()) {
      kQuery_builder << "symbolic";
    } else {
      kQuery_builder << "[" << array->getSize() << "]";
    }
    kQuery_builder << "\n";
  }

  kQuery_builder << "(query [\n";
  for (const klee::ref<klee::Expr> &constraint : constraints) {
    kQuery_builder << "    " << expr_to_string(constraint) << "\n";
  }
  kQuery_builder << "] false [\n";
  for (const klee::ref<klee::Expr> &value : values) {
    kQuery_builder << "    " << expr_to_string(value) << "\n";
  }
  kQuery_builder << "])";
  kQuery_builder << "\n";

  return kQuery_builder.str();
}

kQueryParser::kQueryParser(SymbolManager *_manager) : manager(_manager), builder(klee::createDefaultExprBuilder()) {
  assert(manager && "SymbolManager is null");
}

kQuery_t kQueryParser::parse(const std::string &kQueryStr) {
  kQuery_t kQuery;
  std::vector<std::unique_ptr<klee::expr::Decl>> decls;

  std::unique_ptr<MemoryBufferMem> mb = std::make_unique<MemoryBufferMem>(kQueryStr, false);
  std::unique_ptr<klee::expr::Parser> parser =
      std::unique_ptr<klee::expr::Parser>(klee::expr::Parser::Create("kQueryParser", mb.get(), builder.get(), false));

  while (klee::expr::Decl *decl = parser->ParseTopLevelDecl()) {
    assert(!parser->GetNumErrors() && "Error parsing kquery in call path file.");
    decls.emplace_back(decl);

    if (klee::expr::ArrayDecl *array_decl = dyn_cast<klee::expr::ArrayDecl>(decl)) {
      symbol_t symbol = manager->store_clone(array_decl->Root);
      kQuery.symbols.add(symbol);
    } else if (klee::expr::QueryCommand *qc = dyn_cast<klee::expr::QueryCommand>(decl)) {
      for (klee::ref<klee::Expr> expr : qc->Constraints)
        kQuery.constraints.push_back(expr);
      for (klee::ref<klee::Expr> value : qc->Values)
        kQuery.values.push_back(value);
      break;
    }
  }

  ArrayReplacer array_replacer(manager->get_arrays());
  for (klee::ref<klee::Expr> &expr : kQuery.values)
    expr = array_replacer.visit(expr);
  for (klee::ref<klee::Expr> &expr : kQuery.constraints)
    expr = array_replacer.visit(expr);

  return kQuery;
}

klee::ref<klee::Expr> kQueryParser::parse_expr(const std::string &expr_str) {
  std::stringstream kQuery_builder;

  for (const klee::Array *array : manager->get_arrays()) {
    kQuery_builder << "array " << array->getName();
    kQuery_builder << "[" << array->getSize() << "]";
    kQuery_builder << " : ";
    kQuery_builder << "w" << array->getDomain();
    kQuery_builder << " -> ";
    kQuery_builder << "w" << array->getRange();
    kQuery_builder << " = ";
    if (array->isSymbolicArray()) {
      kQuery_builder << "symbolic";
    } else {
      kQuery_builder << "[" << array->getSize() << "]";
    }
    kQuery_builder << "\n";
  }

  kQuery_builder << "(query [] false [";
  kQuery_builder << "(" << expr_str << ")";
  kQuery_builder << "])";

  std::string kQueryStr = kQuery_builder.str();
  kQuery_t query        = parse(kQueryStr);
  assert(query.values.size() == 1 && "Error parsing expr");

  return query.values[0];
}

} // namespace synapse
