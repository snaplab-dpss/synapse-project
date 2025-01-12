#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define UNW_LOCAL_ONLY

#include "system.h"

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
#include <libunwind.h>
#include <cxxabi.h>
#include <dlfcn.h>

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
    unw_word_t offset, pc;
    std::array<char, 256> fname;

    if (std::string(fname.data()) == "main") {
      break;
    }

    unw_get_reg(&cursor, UNW_REG_IP, &pc);
    if (pc == 0) {
      break;
    }

    if (unw_get_proc_name(&cursor, fname.data(), sizeof(fname), &offset) == 0) {
      int demangle_status  = 0;
      char *demangled_name = abi::__cxa_demangle(fname.data(), nullptr, nullptr, &demangle_status);
      fprintf(stderr, "  -> %s ", (demangle_status == 0 ? demangled_name : fname.data()));
      if (demangle_status == 0) {
        std::free(demangled_name);
      }
    } else {
      fprintf(stderr, "  -> [unknown] ");
    }

    Dl_info info;
    if (dladdr((void *)pc, &info) && info.dli_fname) {
      uintptr_t load_base         = reinterpret_cast<uintptr_t>(info.dli_fbase);
      uintptr_t relative_pc       = pc - load_base;
      const std::string line_info = resolve_line_info(info.dli_fname, relative_pc);
      fprintf(stderr, " (in %s %s)\n", info.dli_fname, line_info.c_str());
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