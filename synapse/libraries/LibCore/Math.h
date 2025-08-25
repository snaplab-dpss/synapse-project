#pragma once

#include <LibCore/Types.h>

#include <vector>

namespace LibCore {

struct horners_result_t {
  double f;
  double df_dx;
};

horners_result_t poly_calc_horners_method(const std::vector<double> &coefficients, double x);

// Coefficients are in increasing order: x^0, x^1, x^2, ...
// min and max are inclusive
double newton_root_finder(const std::vector<double> &coefficients, u64 min, u64 max);

} // namespace LibCore