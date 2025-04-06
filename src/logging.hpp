#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "utils.hpp"

namespace retort {

enum struct LogLevel {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERR,
  max_value,
};

auto log_level_to_str(LogLevel lvl) -> const char * {
  if (lvl >= LogLevel::max_value)
    return "???";

  const char *table[uint32_t(LogLevel::max_value)] = {"Trace", "Debug", "Info",
                                                      "Warn", "Err"};

  return table[uint32_t(lvl)];
}

struct LoggerMessage {
  LogLevel level;
  // TODO: optimize this, they are repeated so many times
  std::string logger_name;
  std::string message;
};

struct Logger {
  template <typename... Ts>
  void log(const char *logger_name, LogLevel level, Ts &&...vs) {
    std::stringstream ss;
    (ss << ... << std::forward<Ts>(vs));

    messages.push_back(LoggerMessage{
        .level = level, .logger_name = logger_name, .message = ss.str()});
  }

  std::vector<LoggerMessage> messages;
};

static std::optional<Logger> _global_logger;

static auto global_logger() -> Logger & {
  if (!_global_logger.has_value())
    _global_logger = Logger();
  return _global_logger.value();
}

} // namespace retort
