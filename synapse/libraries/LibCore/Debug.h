#pragma once

#include <string>
#include <stdint.h>
#include <csignal>
#include <iostream>
#include <limits>
#include <assert.h>
#include <filesystem>

namespace LibCore {

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

#define panic(fmt, ...)                                                                                                                    \
  {                                                                                                                                        \
    fprintf(stderr, COLOR_RED_BRIGHT fmt "\n" COLOR_RESET, ##__VA_ARGS__);                                                                 \
    fflush(stderr);                                                                                                                        \
    LibCore::dbg_breakpoint();                                                                                                             \
    exit(1);                                                                                                                               \
  }

inline void dbg_pause() {
  std::cout << "Press Enter to continue ";
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

inline void dbg_breakpoint() { raise(SIGTRAP); }

} // namespace LibCore