#include "logging/logging.h"

#include <stdio.h>
#include <stdarg.h> // va_start

#include <string.h>
#include <fcntl.h>

#if defined(_POSIX_VERSION) || defined(__APPLE__)
#include <unistd.h>
#include <syslog.h>
#endif

#if defined(_WIN32)
#define PUBLIC
#else
#define PUBLIC   __attribute__((visibility("default")))
#define INTERNAL __attribute__((visibility("internal")))
#endif

static int _log_level = 1;

PUBLIC void log_set_level(int level) {
  _log_level = level;
}

static int _syslog_enabled = 0;

PUBLIC void log_switch_to_syslog(void) {
  _syslog_enabled = 1;
}

static int         _file_log_enabled = 0;
static const char *log_file_path     = NULL;
static int         log_file_fd       = -1;
static FILE *      log_file_stream   = NULL;

PUBLIC void log_switch_to_file(const char *path) {
  _file_log_enabled = 1;
  log_file_path     = strdup(path);

#if 0
  log_file_fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
#endif
  log_file_stream = fopen(log_file_path, "a+");
}

static int check_log_file_available() {
  if (log_file_stream)
    return 1;

  if (log_file_path) {
    log_file_stream = fopen(log_file_path, "a+");
  }

  if (log_file_stream)
    return 1;

  return 0;
}

PUBLIC int log_internal_impl(unsigned int level, const char *fmt, ...) {
  if (level < _log_level)
    return 0;

  va_list ap;
  va_start(ap, fmt);
#pragma clang diagnostic ignored "-Wformat"
#if defined(_POSIX_VERSION) || defined(__APPLE__)
  if (_syslog_enabled) {
    vsyslog(LOG_ERR, fmt, ap);
  }
#endif
  if (_file_log_enabled) {
    if (check_log_file_available()) {
#define MAX_PRINT_BUFFER_SIZE 1024
      char buffer[MAX_PRINT_BUFFER_SIZE] = {0};
      vsnprintf(buffer, MAX_PRINT_BUFFER_SIZE - 1, fmt, ap);
      if (fwrite(buffer, sizeof(char), strlen(buffer) + 1, log_file_stream) == -1) {
        // log_file_fd invalid
        log_file_stream = NULL;
        if (check_log_file_available()) {
          // try again
          fwrite(buffer, sizeof(char), strlen(buffer) + 1, log_file_stream);
        }
      }
      fflush(log_file_stream);
    } else {
      vprintf(fmt, ap);
    }
  }

  if (!_syslog_enabled && !_file_log_enabled) {
#if defined(__ANDROID__)
#define ANDROID_LOG_TAG "Dobby"
#include <android/log.h>
    __android_log_vprint(ANDROID_LOG_INFO, ANDROID_LOG_TAG, fmt, ap);
#else
    vprintf(fmt, ap);
#endif
  }

#pragma clang diagnostic warning "-Wformat"
  va_end(ap);
  return 0;
}
