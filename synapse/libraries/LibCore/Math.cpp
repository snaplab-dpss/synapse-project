#include <LibCore/Math.h>
#include <LibCore/Debug.h>

#include <algorithm>
#include <cmath>

namespace LibCore {

namespace {
constexpr const int NEWTON_MAX_ITERATIONS{100};
constexpr const double NEWTON_PRECISION{1e-3};
} // namespace

horners_result_t poly_calc_horners_method(const std::vector<double> &coefficients, double x) {
  horners_result_t result;
  result.f     = 0;
  result.df_dx = 0;
  for (int i = coefficients.size() - 1; i >= 0; --i) {
    if (coefficients[i] == 0) {
      continue;
    }
    if (i > 0) {
      result.df_dx = result.df_dx * x + i * coefficients[i];
    }
    result.f = result.f * x + coefficients[i];
  }
  return result;
}

double newton_root_finder(const std::vector<double> &coefficients, u64 min, u64 max) {
  // Solved in y = x / max so the iteration works near unit scale (the callers' polynomials
  // have degrees up to the recirculation depth and coefficients spanning tens of orders of
  // magnitude). Newton steps are kept inside a bracket that always holds a sign change; a step
  // leaving it, or not shrinking it, bisects instead. Falls back to plain Newton when the
  // bracket ends don't differ in sign.
  const double scale = static_cast<double>(max) > 0 ? static_cast<double>(max) : 1.0;
  std::vector<double> scaled(coefficients.size());
  double power = 1.0;
  for (size_t i = 0; i < coefficients.size(); i++) {
    scaled[i] = coefficients[i] * power;
    power *= scale;
  }

  double lo   = static_cast<double>(min) / scale;
  double hi   = static_cast<double>(max) / scale;
  double f_lo = poly_calc_horners_method(scaled, lo).f;
  double f_hi = poly_calc_horners_method(scaled, hi).f;

  if (f_lo == 0) {
    return lo * scale;
  }
  if (f_hi == 0) {
    return hi * scale;
  }

  const bool bracketed = (f_lo < 0) != (f_hi < 0);
  double x             = lo;

  for (int i = 0; i < NEWTON_MAX_ITERATIONS; i++) {
    const horners_result_t poly_calc = poly_calc_horners_method(scaled, x);
    assert_or_panic(!std::isinf(poly_calc.f) && !std::isnan(poly_calc.f), "Precision issues. This is a bug.");
    if (std::abs(poly_calc.f) <= NEWTON_PRECISION) {
      break;
    }

    if (bracketed) {
      if ((poly_calc.f < 0) == (f_lo < 0)) {
        lo   = x;
        f_lo = poly_calc.f;
      } else {
        hi   = x;
        f_hi = poly_calc.f;
      }
      if (hi - lo <= 1e-12 * std::max(1.0, hi)) {
        break;
      }
    }

    double next = x - poly_calc.f / poly_calc.df_dx;
    if (bracketed && (!(next > lo && next < hi) || std::isnan(next))) {
      next = (lo + hi) / 2;
    }
    if (next == x) {
      break;
    }
    x = next;
  }

  if (bracketed) {
    x = std::min(std::max(x, lo), hi);
  }
  x *= scale;

  assert_or_panic(x >= static_cast<double>(min), "Root is below minimum (%lf < %lu)", x, min);
  assert_or_panic(x <= static_cast<double>(max), "Root is above maximum (%lf > %lu)", x, max);

  return x;
}

} // namespace LibCore