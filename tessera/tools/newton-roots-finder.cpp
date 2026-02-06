#include <LibCore/Math.h>

int main() {
  std::vector<double> coefficients = {-1.89e+15, 1.51e+04, 1.00e+00, 6.61e-05, 4.36e-09, 2.88e-13,
                                      1.91e-17,  1.26e-21, 8.32e-26, 5.49e-30, 3.63e-34, 2.40e-38};
  LibCore::newton_root_finder(coefficients, 0, 125000000000);
  return 0;
}