#include <errno.h>
#include <string.h>
#include "log.h"

/* for now, always log at trace */
int g_debugging = DEBUG_TRACE;

void debug_level_increase(void) {
    if (g_debugging < DEBUG_TRACE) g_debugging++;
}

void debug_level_decrease(void) {
    if (g_debugging > 0) g_debugging--;
}

void log_failure(char *what) {
    int err = errno;

    syslog(LOG_ERR, "%s failed: errno=%d, %s", what, err, strerror(err));
    if (err == ENODEV) {
        syslog(LOG_EMERG, "bluetooth device disappeared!");
    }
}
