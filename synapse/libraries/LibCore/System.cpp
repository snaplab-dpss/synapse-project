#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <LibCore/System.h>

#include <array>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <limits.h>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <cassert>

namespace LibCore {

uintptr_t get_base_addr() {
  std::ifstream maps("/proc/self/maps");
  if (!maps.is_open()) {
    perror("open");
    exit(1);
  }

  std::string line;
  if (std::getline(maps, line)) {
    std::size_t dashPos = line.find('-');
    if (dashPos != std::string::npos) {
      return std::stoul(line.substr(0, dashPos), nullptr, 16);
    }
  }

  return 0;
}

std::string get_exec_path() {
  std::array<char, PATH_MAX> buffer;
  ssize_t len = readlink("/proc/self/exe", buffer.data(), sizeof(buffer.data()) - 1);

  if (len == -1) {
    perror("readlink");
    exit(1);
  }

  buffer[len] = '\0';
  return std::string(buffer.data());
}

std::string exec_cmd(const std::string &cmd) {
  std::array<char, 128> buffer;
  std::string result;

  FILE *pipe = popen(cmd.c_str(), "r");

  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    result += buffer.data();
  }

  pclose(pipe);

  return result;
}

std::string resolve_line_info(const char *executable, uintptr_t address) {
  std::array<char, PATH_MAX> buffer;
  const char *resolved_path = realpath(executable, buffer.data());

  std::ostringstream cmd;
  cmd << "addr2line -e " << resolved_path << " " << std::hex << address;

  std::string result = exec_cmd(cmd.str());
  result.erase(result.find_last_not_of("\n") + 1);

  const std::string discriminator_marker = " (discriminator";
  size_t marker_pos                      = result.find(discriminator_marker);
  if (marker_pos != std::string::npos) {
    result = result.substr(0, marker_pos);
  }

  return result;
}

long get_file_size(const char *fname) {
  FILE *fp = fopen(fname, "r");

  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }

  fseek(fp, 0L, SEEK_END);
  long res = ftell(fp);
  fclose(fp);

  return res;
}

std::filesystem::path create_random_file(const std::string &extension) {
  char filename[] = "/tmp/XXXXXX";
  int fd          = mkstemp(filename);
  assert(fd != -1 && "Failed to create temporary file");
  const std::string fpath = std::string(filename) + extension;
  return fpath;
}

} // namespace LibCore