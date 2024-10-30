#pragma once

#include <iostream>
#include <sstream>
#include <csignal>

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

#define DEBUG_PAUSE                                                            \
  {                                                                            \
    std::cout << "Press Enter to continue ";                                   \
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');        \
  }

#define PANIC(fmt, ...)                                                        \
  {                                                                            \
    fprintf(stderr,                                                            \
            COLOR_RED_BRIGHT "\n"                                              \
                             "******************\n"                            \
                             "*     PANIC!     *\n"                            \
                             "******************"                              \
                             "\n" COLOR_RESET fmt "\n",                        \
            ##__VA_ARGS__);                                                    \
    fflush(stderr);                                                            \
    assert(false && "Panic");                                                  \
    exit(1);                                                                   \
  }

#define ASSERT_OR_PANIC(stmt, fmt, ...)                                        \
  {                                                                            \
    if (!(stmt)) {                                                             \
      PANIC(fmt, ##__VA_ARGS__);                                               \
    }                                                                          \
  }

#define BREAKPOINT raise(SIGTRAP);

class Log {

public:
  enum Level { DEBUG = 0, LOG = 1, WARNING = 2, ERROR = 3 };

  std::ostream stream;

  static Level MINIMUM_LOG_LEVEL;
  static bool is_debug_active() {
    return Log::MINIMUM_LOG_LEVEL >= Level::DEBUG;
  }

  static Log log();
  static Log dbg();
  static Log wrn();
  static Log err();

private:
  std::string color;
  Level level;

private:
  Log(const Level &_level) : stream(nullptr), level(_level) {
    switch (_level) {
    case Level::LOG:
      stream.rdbuf(std::cout.rdbuf());
      color = COLOR_WHITE;
      break;
    case Level::DEBUG:
      stream.rdbuf(std::cerr.rdbuf());
      color = COLOR_CYAN;
      break;
    case Level::WARNING:
      stream.rdbuf(std::cerr.rdbuf());
      color = COLOR_RED;
      break;
    case Level::ERROR:
      stream.rdbuf(std::cerr.rdbuf());
      color = COLOR_RED_BRIGHT;
      break;
    default:
      stream.rdbuf(std::cerr.rdbuf());
      color = COLOR_WHITE;
    }
  }

  Log(const Log &log) : Log(log.level) {}

  template <typename T> friend Log &operator<<(Log &log, T &&t);
  template <typename T> friend Log &operator<<(Log &&log, T &&t);
};

template <typename T> Log &operator<<(Log &log, T &&t) {
  if (log.level < Log::MINIMUM_LOG_LEVEL)
    return log;

  // Use a temporary stringstream to handle formatting
  std::ostringstream temp_stream;
  temp_stream << std::forward<T>(t);
  std::string output = temp_stream.str();

  // Now output to the main log stream with color
  log.stream << log.color << output << COLOR_RESET;

  return log;
}

template <typename T> Log &operator<<(Log &&log, T &&t) {
  return log << std::forward<T>(t);
}