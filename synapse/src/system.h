#include <string>
#include <stdint.h>

namespace synapse {
uintptr_t get_base_addr();
std::string get_exec_path();
std::string exec_cmd(const std::string &cmd);
void backtrace();
long get_file_size(const char *fname);
} // namespace synapse