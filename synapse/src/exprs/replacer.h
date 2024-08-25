#pragma once

#include <vector>

#include <klee/Expr.h>

klee::ref<klee::Expr>
replace_symbols(klee::ref<klee::Expr> expr,
                const std::vector<klee::ref<klee::ReadExpr>> &reads);