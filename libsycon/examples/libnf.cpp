// Example: use libnf's CPU data structures + math, which are embedded in (and
// namespaced under) libsycon. Demonstrates that linking libsycon is enough to reach
// libnf -- no separate libnf link, and the symbols live under `libnf::`.
//
// This is a standalone demo (its own main), so it defines empty sycon framework hooks
// just to satisfy libsycon's symbols; it does not talk to the switch.

#include <sycon/sycon.h>
#include <sycon/libnf.h>

#include <cstdint>
#include <iostream>

using namespace sycon;

// Unused framework hooks (this example doesn't run the controller loop).
void sycon::nf_init() {}
void sycon::nf_exit() {}
void sycon::nf_user_signal_handler() {}
void sycon::nf_args(CLI::App &app) { (void)app; }
nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  (void)now;
  (void)pkt;
  (void)size;
  return nf_process_result_t();
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  // --- math helpers ---
  std::cout << "find_first_set_bit(0x00080000) = " << libnf::find_first_set_bit(0x00080000u) << " (expect 20)\n";
  std::cout << "power_of_two(5)                = " << libnf::power_of_two(5) << " (expect 32)\n";
  std::cout << "min(3, 7)                      = " << libnf::min(3, 7) << " (expect 3)\n";
  std::cout << "divide(20, 4)                  = " << libnf::divide(20, 4) << " (expect 5)\n";
  std::cout << "ln(100, 64)                    = " << libnf::ln(100, 64) << " (expect 294)\n";

  // --- a CPU vector (like the estimators an offloaded HLL would keep) ---
  libnf::Vector *estimators = nullptr;
  if (!libnf::vector_allocate(sizeof(uint32_t), 64, &estimators)) {
    std::cerr << "vector_allocate failed\n";
    return 1;
  }
  uint32_t *cell = nullptr;
  libnf::vector_borrow(estimators, 3, (void **)&cell);
  *cell = 42;
  libnf::vector_return(estimators, 3, cell);

  libnf::vector_borrow(estimators, 3, (void **)&cell);
  std::cout << "estimators[3]                  = " << *cell << " (expect 42)\n";
  libnf::vector_return(estimators, 3, cell);

  // --- a CPU map (key -> value) ---
  libnf::Map *m = nullptr;
  if (!libnf::map_allocate(16, sizeof(uint32_t), &m)) {
    std::cerr << "map_allocate failed\n";
    return 1;
  }
  uint32_t key = 0xdeadbeef;
  libnf::map_put(m, &key, 7);
  int value_out = -1;
  int hit = libnf::map_get(m, &key, &value_out);
  std::cout << "map_get(0xdeadbeef)            = hit:" << hit << " value:" << value_out << " (expect hit:1 value:7)\n";

  std::cout << "libnf reached through libsycon successfully.\n";
  return 0;
}
