#pragma once

#include "system.h"

#include <iostream>
#include <sstream>
#include <csignal>
#include <iomanip>

namespace synapse {

class Log {
public:
  enum class Level { DEBUG = 0, LOG = 1, WARNING = 2, ERROR = 3 };

  std::ostream stream;

  static Level min_log_level;
  static bool is_dbg_active();
  static Log log();
  static Log dbg();
  static Log wrn();
  static Log err();

private:
  std::string color;
  Level level;

private:
  Log(Level level);
  Log(const Log &log);

  template <typename T> friend Log &operator<<(Log &log, T &&t);
  template <typename T> friend Log &operator<<(Log &&log, T &&t);
};

template <typename T> Log &operator<<(Log &log, T &&t) {
  if (log.level < Log::min_log_level)
    return log;

  // Use a temporary stringstream to handle formatting
  std::ostringstream temp_stream;
  temp_stream << std::forward<T>(t);
  std::string output = temp_stream.str();

  // Now output to the main log stream with color
  log.stream << log.color << output << COLOR_RESET;

  return log;
}

template <typename T> Log &operator<<(Log &&log, T &&t) { return log << std::forward<T>(t); }
} // namespace synapse