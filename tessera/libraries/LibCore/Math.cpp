#include <LibCore/Math.h>
#include <LibCore/Debug.h>

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
  //   std::cerr << "Min: " << min << "\n";
  //   std::cerr << "Max: " << max << "\n";
  //   if (coefficients.size() > 10) {
  //     std::cerr << "Coefficients:\n";
  //     for (const auto &coef : coefficients) {
  //       std::cerr << std::scientific << coef << "\n";
  //     }
  //   }

  double x = min;

  std::set<double> tested;
  for (int i = 0; i < NEWTON_MAX_ITERATIONS; i++) {
    const horners_result_t poly_calc = poly_calc_horners_method(coefficients, x);
    if (std::abs(poly_calc.f) <= NEWTON_PRECISION) {
      break;
    }

    // std::cerr << " x " << std::scientific << x;
    // std::cerr << " f(x) " << std::scientific << poly_calc.f;
    // std::cerr << " df/dx " << std::scientific << poly_calc.df_dx << "\n";

    tested.insert(x);
    assert_or_panic(!std::isinf(poly_calc.f), "Precision issues. This is a bug.");

    x -= poly_calc.f / poly_calc.df_dx;

    if (tested.find(x) != tested.end()) {
      // We are stuck in a loop. Let's just return the best effort.
      break;
    }
  }

  assert_or_panic(x >= min, "Root is below minimum (%lf < %lu)", x, min);
  assert_or_panic(x <= max, "Root is above maximum (%lf > %lu)", x, max);

  return x;
}

} // namespace LibCore