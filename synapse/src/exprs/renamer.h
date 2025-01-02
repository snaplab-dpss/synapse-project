#pragma once

#include <unordered_map>

#include <klee/Expr.h>
#include <klee/Constraints.h>

namespace synapse {
klee::ref<klee::Expr>
rename_symbols(klee::ref<klee::Expr> expr,
               const std::unordered_map<std::string, std::string> &translations);
klee::ConstraintManager
rename_symbols(const klee::ConstraintManager &constraints,
               const std::unordered_map<std::string, std::string> &translations);
} // namespace synapse