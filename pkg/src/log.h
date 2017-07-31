#pragma once

#include <syslog.h>

enum {
    DEBUG_OFF = 0,
    DEBUG_ON = 1,
    DEBUG_TRACE = 2
};

extern int g_debugging;

#define _LOG(level, format, ...) do { \
  if (g_debugging >= level) syslog(LOG_DEBUG, format, ## __VA_ARGS__); \
} while (0)

#define ERROR(format, ...) syslog(LOG_ERR, format, ## __VA_ARGS__)
#define WARNING(format, ...)  syslog(LOG_WARNING, format, ## __VA_ARGS__)
#define INFO(format, ...)  syslog(LOG_INFO, format, ## __VA_ARGS__)
#define DEBUG(format, ...) _LOG(DEBUG_ON, format, ## __VA_ARGS__)
#define TRACE(format, ...) _LOG(DEBUG_TRACE, format, ## __VA_ARGS__)

void debug_level_increase(void);
void debug_level_decrease(void);
void log_failure(char *what);
