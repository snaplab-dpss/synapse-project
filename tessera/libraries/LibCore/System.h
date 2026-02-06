#pragma once

#include <string>
#include <filesystem>

namespace LibCore {

uintptr_t get_base_addr();
std::string get_exec_path();
std::string exec_cmd(const std::string &cmd);
long get_file_size(const char *fname);
std::filesystem::path create_random_file(const std::string &extension);

} // namespace LibCore