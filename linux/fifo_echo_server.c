#include "logger.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define DEFAULT_FIFO "/tmp/echo_server_fifo"
#define DEFAULT_LOG "/tmp/echo_server.log"
#define ALARM_INTERVAL 5

typedef struct {
    unsigned long messages;
    unsigned long bytes;
    unsigned long alarms;
} Statistics;

typedef struct {
    char  *fifo_name;
    char  *log_file;
    bool fifo_created;
    bool is_daemon;
    Statistics stats;
} ServerContext;

ServerContext ctx = {
        .fifo_name    = (char*)DEFAULT_FIFO,
        .log_file     = NULL,
        .fifo_created = false,
        .is_daemon = false,
        .stats = {0, 0, 0}
};

static volatile sig_atomic_t need_exit_now     = 0; // SIGTERM
static volatile sig_atomic_t need_exit_grace   = 0; // SIGINT
static volatile sig_atomic_t alarm_fired       = 0; // SIGALRM
static volatile sig_atomic_t stats_requested   = 0; // SIGUSR1
static volatile sig_atomic_t need_daemonize    = 0; // SIGHUP
static volatile sig_atomic_t last_signal       = 0;

static volatile sig_atomic_t fifo_fd = -1;

static void handle_signal(int sig) {
    last_signal = sig;
    switch(sig) {
        case SIGTERM:
            LOG_INFO("Received SIGTERM, immediate shutdown");
            need_exit_now   = 1;
            break;
        case SIGINT:
            LOG_INFO("Received SIGINT, graceful shutdown");
            need_exit_grace = 1;
            break;
        case SIGALRM: alarm_fired     = 1; break;
        case SIGUSR1: stats_requested = 1; break;
        case SIGHUP:  need_daemonize  = 1; break;
        // case SIGQUIT: break; // ignore SIGQUIT
    }
}

static void setup_signals() {
    struct sigaction sa = {0};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_signal;
    sa.sa_flags   = 0;

    const int signals[] = {SIGTERM, SIGINT, SIGALRM, SIGUSR1, SIGHUP};
    for (size_t i = 0; i < sizeof(signals)/sizeof(*signals); i++) {
        if (sigaction(signals[i], &sa, NULL) == -1) {
            LOG_FATAL("sigaction(%d): %s", signals[i], strerror(errno));
        }
    }

    struct sigaction ign = { .sa_handler = SIG_IGN };
    sigemptyset(&ign.sa_mask);
    if (sigaction(SIGQUIT, &ign, NULL) == -1) {
        LOG_FATAL("sigaction(SIGQUIT): %s", strerror(errno));
    }
}

static void print_stats() {
    LOG_INFO("Statistics: Messages=%lu Bytes=%lu Alarms=%lu",
             ctx.stats.messages, ctx.stats.bytes, ctx.stats.alarms);
}

static void daemonize() {
    if (ctx.is_daemon) {
        LOG_ERROR("Is already a daemon!");
        return;
    }

    Statistics pre_daemon_stats = ctx.stats;

    pid_t pid = fork();
    if (pid < 0) {
        LOG_FATAL("First fork failed: %s", strerror(errno));
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() == -1) {
        LOG_FATAL("setsid failed: %s", strerror(errno));
    }

    int fd = open(ctx.log_file ? ctx.log_file : DEFAULT_LOG, O_WRONLY|O_CREAT|O_APPEND, 0600);
    if (fd == -1) LOG_FATAL("open log: %s", strerror(errno));

    if (dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1) {
        LOG_FATAL("dup2: %s", strerror(errno));
    }
    if (fd > STDERR_FILENO) close(fd);

    ctx.is_daemon = true;
    log_reopen();

    LOG_INFO("Daemonization completed");
    LOG_INFO("Daemon PID: %d", getpid());
    LOG_INFO("Statistics at daemonization:");
    ctx.stats = pre_daemon_stats;
    print_stats();
}

static void cleanup() {
    if (fifo_fd != -1) close((int)fifo_fd);

    if (ctx.fifo_created) {
        LOG_INFO("Removing FIFO %s", ctx.fifo_name);
        if (unlink(ctx.fifo_name) == -1) {
            LOG_ERROR("FIFO %s removal failed: %s", ctx.fifo_name, strerror(errno));
        } else {
            LOG_INFO("FIFO %s removed", ctx.fifo_name);
        }
    }
    print_stats();
    LOG_INFO("Server shutdown");
    log_close();
}

static void exit_immediately(int ec) {
    cleanup();
    exit(ec);
}

static void handle_flags(void) {
    if (alarm_fired) {
        ctx.stats.alarms++;
        LOG_INFO("Server active, waiting for data");
        alarm_fired = 0;
        alarm(ALARM_INTERVAL);
    }
    if (stats_requested) {
        print_stats();
        stats_requested = 0;
    }
    if (need_daemonize) {
        LOG_INFO("Daemon started");
        need_daemonize = 0;
        daemonize();
    }
    if (need_exit_now) {
        exit_immediately(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "df:l:")) != -1) {
        switch(opt) {
            case 'd': ctx.is_daemon = true; break;
            case 'f': ctx.fifo_name = optarg; break;
            case 'l': ctx.log_file = optarg; break;
            default:
                fprintf(stderr, "Usage: %s [-d] [-f FIFO] [-l LOG_FILE]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (ctx.is_daemon && !ctx.log_file) {
        fprintf(stderr, "Error: daemon mode requires -l LOG_FILE.\n");
        exit(EXIT_FAILURE);
    }

    log_init(ctx.log_file);
    LOG_INFO("Starting server (PID: %d)", getpid());

    struct stat st;
    if (mkfifo(ctx.fifo_name, 0600) == -1) {
        if (errno != EEXIST) LOG_FATAL("mkfifo(%s) failed: %s", ctx.fifo_name, strerror(errno));
        if (stat(ctx.fifo_name, &st) == -1) LOG_FATAL("stat(%s) failed: %s", ctx.fifo_name, strerror(errno));
        if (!S_ISFIFO(st.st_mode)) LOG_FATAL("%s is not a FIFO", ctx.fifo_name);
        LOG_INFO("Using existing FIFO");
    } else {
        ctx.fifo_created = true;
        LOG_INFO("FIFO(%s) created", ctx.fifo_name);
    }

    if (ctx.is_daemon) {
        daemonize();
    }

    setup_signals();
    alarm(ALARM_INTERVAL);

    char buf[4096 + 1];

    for (;;) {
        handle_flags();
        if (need_exit_grace && fifo_fd == -1) {
            LOG_INFO("Graceful exit (fifo not opened yet)");
            exit_immediately(EXIT_SUCCESS);
        }

        int fd;
        while ((fd = open(ctx.fifo_name, O_RDONLY)) == -1) {
            if (errno == EINTR) {
                handle_flags();
                if (need_exit_grace || need_exit_now) break;
                continue;
            }
            LOG_FATAL("open(%s): %s", ctx.fifo_name, strerror(errno));
        }
        if (fd == -1) continue;
        fifo_fd = fd;
        LOG_INFO("FIFO opened (fd=%d)", fd);

        ssize_t n;
        while ( (n = read(fd, buf, sizeof(buf)-1)) != 0 ) {
            if (n == -1) {
                if (errno == EINTR) {
                    handle_flags();
                    if (need_exit_now) {
                        close(fd); fifo_fd = -1;
                        exit_immediately(EXIT_SUCCESS);
                    }
                    continue;
                }
                LOG_ERROR("read: %s", strerror(errno));
                break;
            }
            buf[n] = '\0';
            LOG_INFO("Received %zd bytes", n);
            fwrite(buf, 1, (size_t)n, log_stream);
            if (buf[n-1] != '\n') fputc('\n', log_stream);
            ctx.stats.bytes += (unsigned long)n;
        }
        close(fd); fifo_fd = -1;
        ctx.stats.messages++;
        LOG_INFO("Session #%lu ended", ctx.stats.messages);

        if (need_exit_grace) {
            LOG_INFO("Graceful exit (after reading)");
            exit_immediately(EXIT_SUCCESS);
        }
    }

    return 0;
}