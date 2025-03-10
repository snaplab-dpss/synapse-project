#include <sycon/sycon.h>

using namespace sycon;

int main(int argc, char **argv) {
  buffer_t a(4);

  std::cerr << "a: " << a << " (0x" << std::hex << a.get(0, 4) << ")"
            << "\n";

  a.set(0, 4, 0xcafebabe);
  std::cerr << "a: " << a << " (0x" << std::hex << a.get(0, 4) << ")"
            << "\n";

  a = a.reverse();
  std::cerr << "reversed: " << a << " (0x" << std::hex << a.get(0, 4) << ")"
            << "\n";

  a[0] = 1;
  a[1] = 0;
  a[2] = 0;
  a[3] = 0;
  std::cerr << "a: " << a << " (0x" << std::hex << a.get(0, 4) << ")"
            << "\n";

  a.set(0, 4, 1);
  std::cerr << "a: " << a << " (0x" << std::hex << a.get(0, 4) << ")"
            << "\n";

  return 0;
}
