#pragma once

#include <iostream>
#include <sstream>
#include <utility>

// =========================================================================
// Modern Pure C++ Variadic Logger (No cstdio / printf)
// =========================================================================

enum LogLevel {
  LOG_LEVEL_DEBUG = 1,
  LOG_LEVEL_INFO = 2,
  LOG_LEVEL_ERROR = 3,
};

// Error only by default

#ifdef DEBUG
inline LogLevel g_log_level = LOG_LEVEL_DEBUG;
#else
inline LogLevel g_log_level = LOG_LEVEL_INFO;
#endif

inline void set_log_level(LogLevel level) { g_log_level = level; }

inline void build_log_string(std::ostringstream &) {}

template <typename T, typename... Args>
void build_log_string(std::ostringstream &oss, T &&first, Args &&...rest) {
  oss << std::forward<T>(first);
  build_log_string(oss, std::forward<Args>(rest)...);
}

template <LogLevel Priority, typename... Args> void log_output(Args &&...args) {
  if (Priority < g_log_level) {
    return;
  }

  std::ostringstream oss;
  build_log_string(oss, std::forward<Args>(args)...);
  std::string message = oss.str();

  if (Priority == LOG_LEVEL_ERROR) {
    std::cerr << "[ERROR] " << message << std::endl;
  } else if (Priority == LOG_LEVEL_DEBUG) {
    std::cerr << "[DEBUG] " << message << std::endl;
  } else {
    std::cerr << "[INFO] " << message << std::endl;
  }
}

#define LOGD(...) log_output<LOG_LEVEL_DEBUG>(__VA_ARGS__)
#define LOGI(...) log_output<LOG_LEVEL_INFO>(__VA_ARGS__)
#define LOGE(...) log_output<LOG_LEVEL_ERROR>(__VA_ARGS__)