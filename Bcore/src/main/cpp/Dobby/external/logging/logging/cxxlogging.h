#ifndef CXXLOGGING_H_
#define CXXLOGGING_H_

#include "logging.h"

typedef enum {
  LOG_LEVEL_FATAL = 0,
  LOG_LEVEL_ERROR = 1,
  LOG_LEVEL_WARNING = 2,
  LOG_LEVEL_DEBUG = 3,
  LOG_LEVEL_VERBOSE = 4
} LogLevel;

class Logger {
public:
  void setLogLevel(LogLevel level);

  void log(LogLevel level, const char *tag, const char *fmt, ...);

  void LogFatal(const char *fmt, ...);

private:
  LogLevel log_level_;
};

#endif
