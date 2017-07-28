/********************************************************************************
 *
 * Copyright (c) 2016 Afero, Inc.
 *
 * Licensed under the MIT license (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License
 * at
 *
 * https://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *******************************************************************************/

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <syslog.h>
#include <getopt.h>

#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <bluetooth/bluetooth.h>

#include "beetle.h"
#include "command.h"
#include "devicelist.h"
#include "connlist.h"
#include "hci_beetle.h"
#include "utils.h"
#include "central.h"
#include "peripheral.h"

#include "build_info.h"

#define CENTRAL_NAME "cen"
#define PERIPHERAL_NAME "per"
#define DEFAULT_PORT 6969

static session_type_t s_defaultSessionType = centralSession;
static session_type_t s_sessionType;

session_type_t get_session_type(void)
{
    return s_sessionType;
}

static int s_clientFd  = -1;   /* client socket                    */
static int s_signal    = 0;    /* the signal that occurred         */
static int s_port = DEFAULT_PORT;

int g_debug = 2;        /* enable debug messages by default */

#define SET_SCAN_ENABLE_TIMEOUT         5000
#define SET_ADVERTISE_ENABLE_TIMEOUT    5000
#define CONNECT_TIMEOUT     5000
#define NO_SOCKET_FD        0xffff
#define OPEN_ATTEMPT_DELAY  20
#define MAX_OPEN_ATTEMPTS   10

#define CMD_BUF_SIZE 1024
int send_cmd(char *fmt, ...)
{
    char buf[CMD_BUF_SIZE];
    va_list va;
    int len;

    if (s_clientFd < 0) {
        errno = EBADF;
        return -1;
    }

    va_start(va, fmt);
    len = vsnprintf (buf, sizeof(buf), fmt, va);
    va_end(va);

    /* don't log advertisements */
    if ((buf[0] != 'a' || s_sessionType == peripheralSession) && g_debug >= 1) {
        syslog(LOG_DEBUG, "<%s", buf);
    }

    int res = write (s_clientFd, buf, len);
    if (res < 0) {
        return res;
    }
    return write (s_clientFd, "\n", 1);
}

/* handle debug command */
int cmd_debug(void *param1, void *param2, void *param3, void *context)
{
    g_debug = *(int *)param1;
    send_cmd("deb %04x", STATUS_OK);
    return 0;
}

/* handle central command */
int cmd_mode(void *param1, void *param2, void *param3, void *context)
{
    int retVal = 0;
    char *mode = (char *)param1;
    if (!strncasecmp(mode, PERIPHERAL_NAME, sizeof(PERIPHERAL_NAME) - 1)) {
        if (s_sessionType != peripheralSession) {
            s_sessionType = peripheralSession;
            retVal = SESSION_SWITCH_SESSION; /* we're switching sessions */
            send_cmd("mod %04x %04x", STATUS_OK, s_sessionType);
        } else {
            send_cmd("mod %04x", STATUS_ALREADY_IN_MODE);
        }
    } else if (!strncasecmp(mode, CENTRAL_NAME, sizeof(CENTRAL_NAME) - 1)) {
        if (s_sessionType != centralSession) {
            s_sessionType = centralSession;
            retVal = SESSION_SWITCH_SESSION;
            send_cmd("mod %04x %04x", STATUS_OK, s_sessionType);
        } else {
            send_cmd("mod %04x", STATUS_ALREADY_IN_MODE);
        }
    } else {
        send_cmd("mod %04x", STATUS_BAD_PARAM);
    }
    return retVal;
}

static int set_up_listener(void)
{
    struct sockaddr_in servaddr;

    /* create the socket */
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        log_failure("echo server socket");
        return -1;
    }

    int optval = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0) {
        log_failure("echo server setsockopt");
        close(listenFd);
        return -1;
    }

    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(s_port);

    if (bind(listenFd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        log_failure("bind");
        close(listenFd);
        return -1;
    }

    if (listen(listenFd, 1) < 0) {
        log_failure("listen");
        close(listenFd);
        return -1;
    }
    syslog(LOG_INFO,"listening...");
    return listenFd;
}

static int accept_connection(int listenFd)
{
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);

    int clientFd = accept(listenFd, (struct sockaddr *)&clientaddr, &len);
    if (clientFd < 0) {
        log_failure("accept");
        return -1;
    }

    syslog(LOG_INFO, "Connection from %s", inet_ntoa(clientaddr.sin_addr));

    return clientFd;
}

int dying(void) {
    return s_signal == SIGINT || s_signal == SIGTERM;
}

/* return & clear any non-fatal signal */
int signaled(void) {
    if (dying()) return s_signal;
    int rv = s_signal;
    s_signal = 0;
    return rv;
}

static void on_signal(int signal) {
    s_signal = signal;
}

static void setup_signals(void) {
    // Set signal handlers
    sigset_t sigset;
    sigemptyset(&sigset);
    struct sigaction siginfo = {
        .sa_handler = on_signal,
        .sa_mask = sigset,
        .sa_flags = 0,
    };

    sigaction(SIGINT, &siginfo, NULL);
    sigaction(SIGTERM, &siginfo, NULL);
    sigaction(SIGUSR1, &siginfo, NULL);
}

#define PID_FILE_PATH "/var/run/beetle.pid"

static void usage(void)
{
    fprintf(stderr, "usage -- beetle [options][-p <port>][-d][-v]\n");
    fprintf(stderr, "  -v               show version info and exit\n");
    fprintf(stderr, "  -i <interface>   set hci interface (example: hci0)\n");
    fprintf(stderr, "  -m {cen|per}     set start up mode (central or peripheral)\n");
    fprintf(stderr, "  -p <port#>       set listen port\n");
    fprintf(stderr, "  -d               run in background (daemon)\n");
}

static int do_session(session_function_t sessionFn, int clientFd, bhci_t *bhci)
{
    int i;
    for (i = 0; i < MAX_OPEN_ATTEMPTS; i++) {
        int ret = (sessionFn)(clientFd, bhci);

        syslog(LOG_DEBUG, "ret=%d", ret);
        if (ret != SESSION_FAILED_NONFATAL) {
            return ret;
        }

        sleep(OPEN_ATTEMPT_DELAY);
    }
    syslog(LOG_ERR, "Unable to start session after %d tries; giving up", i);
    return SESSION_FAILED_FATAL;
}

int main(int argc, char* const argv[])
{
    int daemonize = 0;
    int opt;
    char *interface = NULL;
    int dev = -1;

    while ((opt = getopt(argc, argv, "vdm:i:p:")) != -1) {
        switch (opt) {
            case 'd' :
                daemonize = 1;
                break;
            case 'm' :
                if (!strncasecmp(optarg, CENTRAL_NAME, sizeof(CENTRAL_NAME) - 1)) {
                    s_defaultSessionType = centralSession;
                } else if (!strncasecmp(optarg, PERIPHERAL_NAME, sizeof(PERIPHERAL_NAME) - 1)) {
                    s_defaultSessionType = peripheralSession;
                } else {
                    usage();
                    exit(1);
                }
                break;
            case 'p' :
                s_port = atoi(optarg);
                if (s_port < 1024 || s_port > 65535) {
                    fprintf(stderr, "port must be between 1024 and 65535; falling back to default %d\n", DEFAULT_PORT);
                }
                break;
            case 'i' :
                interface = optarg;
                break;
            case 'v':
                fprintf(stderr, "beetle %s %s\n", BUILD_DATE, REVISION);
                exit(0);
                break;
            default :
                usage();
                exit(1);
                break;
        }
    }

    /* TODO set up pid file to guard against multiple invocations */
    if (daemonize) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "fork failed errno=%d", errno);
            exit(1);
        } else if (pid != 0) {
            exit(0);
        }
    }

    openlog("beetle", LOG_PID | LOG_NDELAY, LOG_USER);
    syslog(LOG_INFO, "beetle starting up: %s %s", BUILD_DATE, REVISION);

    setup_signals();

    int listenFd = set_up_listener();
    if (listenFd < 0) {
        goto exit;
    }

    while (1) {
        bhci_t bhci;
        if (bhci_open(&bhci, interface) < 0) {
            log_failure("bhci_open");
            exit(1);
        }

        s_clientFd = accept_connection(listenFd);

        if (s_clientFd >= 0) {

            /* go to default session type */
            s_sessionType = s_defaultSessionType;

            while (1) {
                int ret;

                if (s_sessionType == centralSession) {
                    ret = do_session(central_session, s_clientFd, &bhci);
                } else if (s_sessionType == peripheralSession) {
                    ret = do_session(peripheral_session, s_clientFd, &bhci);
                } else {
                    syslog(LOG_ERR, "unknown session type %d", s_sessionType);
                    ret = SESSION_FAILED_FATAL;
                    break;
                }

                if (ret != SESSION_SWITCH_SESSION) {
                    break;
                }
            }

            syslog(LOG_INFO, "Disconnecting");
            close(s_clientFd);
        }

        /* we can safely exit without BlueZ issues here */
        if (s_signal) {
            close(listenFd);
            closelog();
            exit(128 + s_signal);
        }

        bhci_close(&bhci);
    }

    close(listenFd);

exit:
    closelog();
    return 0;
}
