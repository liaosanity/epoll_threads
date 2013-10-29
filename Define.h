#ifndef DEFINE_H
#define DEFINE_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/epoll.h>
#include <deque>

using namespace std;

/**
 * Log levels 
 */
#define MY_DEBUG 0
#define MY_VERBOSE 1
#define MY_NOTICE 2
#define MY_WARNING 3
#define MY_LOG_RAW (1 << 10) // Modifier to log without timestamp

/**
 * ANSI Colors
 */
#define ANSI_BOLD "\033[1m"
#define ANSI_CYAN "\033[36m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_RED "\033[31m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_GREEN "\033[32m"
#define ANSI_WHITE "\033[37m"
#define ANSI_RESET "\033[0m"

#define MIN_BUFSZ 1024
#define MAX_BUFSZ 8192

#define THREAD_POOL_SZ 10
#define LISTEN_PORT 8090
#define LISTEN_POOL_SZ 1024

#define I_EPOLL_READ 0
#define I_EPOLL_WRITE 1
#define I_EPOLL_RW 2

#define I_EPOLL_WAIT_TIMEOUT 1000 // 1 seconds

#define I_EPOLL_LEVEL_TRIGGERED 2 // default
#define I_EPOLL_EDGE_TRIGGERED EPOLLET

#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

#define MY_VERSION "1.0.0"

/**
 * Anti-warning macro... 
 */
#define MY_NOTUSED(V) ((void) V)

/**
 * Static server configuration 
 */
#define MY_MAXIDLETIME 0 // default client timeout: infinite
#define MY_MAX_CLIENTS 10000
#define MY_MAX_LOGMSG_LEN 4096
#define MY_CONFIGLINE_MAX 1024

/**
 * Global server state
 */
typedef struct tagSERVER 
{
    bool shutdown;              // Shutdown need to set 'true'
    char pidfile[MIN_BUFSZ];    // PID file path
    int port;                   // TCP listening port
    char bindaddr[MIN_BUFSZ];   // Bind address or NULL
    int verbosity;              // Loglevel in myepoll.conf
    int maxidletime;            // Client timeout in seconds
    int daemonize;              // True if running as a daemon
    char logfile[MIN_BUFSZ];    // Path of log file
    int maxclients;    // Max number of simultaneous clients
    int maxthreads;    // Max number of worker threads
} SERVER;

/**
 * Extern declarations
 */
extern SERVER g_Server;

/** 
 * Core functions 
 */
extern void myepollLog(int level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#endif
