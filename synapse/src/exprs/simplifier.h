#pragma once

#include <klee/Expr.h>

klee::ref<klee::Expr> simplify(klee::ref<klee::Expr> expr);
bool simplify_extract(klee::ref<klee::Expr> extract_expr, klee::ref<klee::Expr> &out);