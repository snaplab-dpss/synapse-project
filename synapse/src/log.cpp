#include "log.h"

#include <iostream>
#include <sstream>

namespace synapse {

Log::Level Log::min_log_level = Log::Level::WARNING;

Log::Log(Level _level) : stream(nullptr), level(_level) {
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

Log::Log(const Log &log) : Log(log.level) {}

bool Log::is_dbg_active() { return min_log_level >= Level::DEBUG; }

Log Log::log() { return Log(Log::Level::LOG); }
Log Log::dbg() { return Log(Log::Level::DEBUG); }
Log Log::wrn() { return Log(Log::Level::WARNING) << "[WARNING] "; }
Log Log::err() { return Log(Log::Level::ERROR) << "[ERROR] "; }

} // namespace synapse