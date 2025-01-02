#include "system.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <limits.h>
#include <execinfo.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <cxxabi.h>

namespace synapse {
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

void backtrace() {
  unw_cursor_t cursor;
  unw_context_t context;

  if (unw_getcontext(&context) != 0) {
    fprintf(stderr, "Error getting context\n");
    exit(1);
  }

  if (unw_init_local(&cursor, &context) != 0) {
    fprintf(stderr, "Error initializing cursor\n");
    exit(1);
  }

  while (unw_step(&cursor) > 0) {
    unw_word_t offset;
    std::array<char, 256> fname;

    if (unw_get_proc_name(&cursor, fname.data(), sizeof(fname), &offset) == 0) {
      int demangle_status;
      char *demangled_name =
          abi::__cxa_demangle(fname.data(), nullptr, nullptr, &demangle_status);
      fprintf(stderr, "  -> %s\n",
              (demangle_status == 0 ? demangled_name : fname.data()));

      if (demangle_status == 0) {
        free(demangled_name);
      }
    } else {
      fprintf(stderr, "  -> [unknown]\n");
    }
  }
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
} // namespace synapse