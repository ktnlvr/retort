#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "utils.hpp"

namespace retort {

enum LogLevel {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERR,
};

struct LoggerMessage {
  LogLevel level;
  std::string message;
};

struct Logger {
  template <typename... Ts> void log(LogLevel level, Ts &&...vs) {
    std::stringstream ss;
    (ss << ... << std::forward<Ts>(vs));
    messages.push_back(LoggerMessage{.level = level, .message = ss.str()});
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
