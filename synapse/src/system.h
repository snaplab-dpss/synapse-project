#pragma once

#include <string>
#include <stdint.h>
#include <csignal>
#include <iostream>
#include <limits>

#define COLOR_RESET "\033[0m"
#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_RED_BRIGHT "\u001b[31;1m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE ""
#define COLOR_BOLD "\033[1m"

#define SYNAPSE_PANIC(fmt, ...)                                                                    \
  {                                                                                                \
    fprintf(stderr, COLOR_RED_BRIGHT "\n");                                                        \
    fprintf(stderr, "PANIC: " fmt "\n", ##__VA_ARGS__);                                            \
    fprintf(stderr, COLOR_RESET "\n");                                                             \
    fflush(stderr);                                                                                \
    dbg_breakpoint();                                                                              \
    exit(1);                                                                                       \
  }

#define SYNAPSE_ASSERT(stmt, ...)                                                                  \
  {                                                                                                \
    if (!(stmt)) {                                                                                 \
      fprintf(stderr, COLOR_RED_BRIGHT "\n");                                                      \
      fprintf(stderr, "ASSERTION FAILED: ");                                                       \
      fprintf(stderr, ##__VA_ARGS__);                                                              \
      fprintf(stderr, "\n");                                                                       \
      fprintf(stderr, "Backtrace:\n");                                                             \
      backtrace();                                                                                 \
      fprintf(stderr, COLOR_RESET);                                                                \
      fflush(stderr);                                                                              \
      dbg_breakpoint();                                                                            \
      exit(1);                                                                                     \
    }                                                                                              \
  }

namespace synapse {

inline void dbg_pause() {
  std::cout << "Press Enter to continue ";
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

inline void dbg_breakpoint() { raise(SIGTRAP); }

uintptr_t get_base_addr();
std::string get_exec_path();
std::string exec_cmd(const std::string &cmd);
void backtrace();
long get_file_size(const char *fname);

} // namespace synapse