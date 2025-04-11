#include "logger.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_FIFO "/tmp/echo_server_fifo"
#define DEFAULT_LOG "/tmp/echo_server.log"
#define ALARM_INTERVAL 5

typedef struct {
    unsigned long messages;
    unsigned long bytes;
    unsigned long alarms;
} Statistics;

typedef struct {
    char* fifo_name;
    char* log_file;
    bool fifo_created;
    bool is_daemon;
    Statistics stats;
} ServerContext;

ServerContext ctx = {
        .fifo_name = DEFAULT_FIFO,
        .log_file = DEFAULT_LOG,
        .fifo_created = false,
        .is_daemon = false,
        .stats = {0, 0, 0}
};

volatile sig_atomic_t should_exit = 0;
volatile sig_atomic_t exit_after_read = 0;
volatile sig_atomic_t alarm_pending = 0;
volatile sig_atomic_t stats_pending = 0;
volatile sig_atomic_t daemonize_flag = 0;
volatile sig_atomic_t current_fd = -1;

void handle_signal(int sig) {
    switch(sig) {
        case SIGTERM:
            LOG_INFO("Received SIGTERM, immediate shutdown");
            should_exit = 1;
            break;
        case SIGINT:
            LOG_INFO("Received SIGINT, graceful shutdown");
            exit_after_read = 1;
            break;
        case SIGALRM:
            alarm_pending = 1;
            break;
        case SIGUSR1:
            stats_pending = 1;
            break;
        case SIGHUP:
            if (!ctx.is_daemon) {
                LOG_INFO("Received SIGHUP, daemonizing");
                daemonize_flag = 1;
            }
            break;
        case SIGQUIT: break; //ignore SIGQUIT
    }
}

void setup_signals() {
    struct sigaction sa = {
            .sa_handler = handle_signal,
            .sa_flags = 0
    };
    sigemptyset(&sa.sa_mask);

    const int signals[] = {SIGTERM, SIGINT, SIGALRM, SIGUSR1, SIGHUP};
    for (size_t i = 0; i < sizeof(signals)/sizeof(*signals); i++) {
        if (sigaction(signals[i], &sa, 0) == -1) {
            LOG_FATAL("Signal handler setup failed: %s", strerror(errno));
        }
    }

    sa.sa_handler = SIG_IGN;
    sigaction(SIGQUIT, &sa, NULL);
}

void print_stats() {
    LOG_INFO("Statistics: Messages=%lu Bytes=%lu Alarms=%lu",
             ctx.stats.messages, ctx.stats.bytes, ctx.stats.alarms);
}

void process_interrupt(int fd);

void daemonize() {
    Statistics pre_daemon_stats = ctx.stats;

    pid_t pid = fork();
    if (pid < 0) {
        LOG_FATAL("First fork failed: %s", strerror(errno));
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        LOG_FATAL("setsid failed: %s", strerror(errno));
    }

    if (current_fd != -1) {
        close(current_fd);
        current_fd = -1;
    }

    int log_fd = open(ctx.log_file, O_WRONLY|O_CREAT|O_APPEND, 0600);
    dup2(log_fd, STDOUT_FILENO);
    dup2(log_fd, STDERR_FILENO);
    close(log_fd);

    ctx.is_daemon = true;
    ctx.stats = pre_daemon_stats;

    LOG_INFO("Daemonization completed");
    LOG_INFO("Daemon PID: %d", getpid());
    LOG_INFO("Statistics at daemonization:");
    print_stats();
}

void cleanup() {
    if (ctx.fifo_created) {
        LOG_INFO("Removing FIFO %s", ctx.fifo_name);
        if (unlink(ctx.fifo_name)) {
            LOG_ERROR("FIFO removal failed: %s", strerror(errno));
        }
    }
    LOG_INFO("Server shutdown");
    fflush(log_stream);
    log_close();
}

void process_interrupt(int fd) {
    if (should_exit) {
        LOG_INFO("Immediate shutdown requested");
        if (fd != -1) close(fd);
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (exit_after_read) {
        LOG_INFO("Graceful shutdown after EOF");
    }

    if (alarm_pending) {
        LOG_INFO("Server active, waiting for data");
        ctx.stats.alarms++;
        alarm_pending = 0;
        alarm(ALARM_INTERVAL);
    }

    if (stats_pending) {
        print_stats();
        stats_pending = 0;
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
    if (mkfifo(ctx.fifo_name, 0600)) {
        if (errno != EEXIST) LOG_FATAL("mkfifo failed: %s", strerror(errno));

        if (stat(ctx.fifo_name, &st)) LOG_FATAL("stat failed: %s", strerror(errno));

        if (!S_ISFIFO(st.st_mode)) LOG_FATAL("%s is not a FIFO", ctx.fifo_name);

        LOG_INFO("Using existing FIFO");
    } else {
        ctx.fifo_created = true;
        LOG_INFO("FIFO created");
    }

    setup_signals();
    alarm(ALARM_INTERVAL);

    while (!should_exit && !exit_after_read) {
        if (daemonize_flag) {
            daemonize();
            daemonize_flag = 0;
            close(current_fd);
            current_fd = -1;
            continue;
        }

        process_interrupt(current_fd);

        int fd;
        while ((fd = open(ctx.fifo_name, O_RDONLY)) == -1) {
            if (errno == EINTR) {
                process_interrupt(current_fd);
                if (should_exit) break;
                continue;
            }
            LOG_FATAL("open failed: %s", strerror(errno));
        }
        current_fd = fd;

        LOG_INFO("FIFO opened (fd: %d)", fd);

        char buf[4096 + 1];
        ssize_t nread;
        while ((nread = read(fd, buf, sizeof(buf)))) {
            if (nread == -1) {
                if (errno == EINTR) {
                    process_interrupt(current_fd);
                    if (should_exit) break;
                    continue;
                }
                LOG_ERROR("read error: %s", strerror(errno));
                break;
            }

            if (nread > 0) {
                buf[nread] = '\0';
                LOG_INFO("Received %zd bytes: %s", nread, buf);
                ctx.stats.bytes += nread;
            }
        }

        close(fd);
        current_fd = -1;
        ctx.stats.messages++;
        if (!should_exit) alarm(ALARM_INTERVAL);
        LOG_INFO("FIFO closed");
    }

    print_stats();
    cleanup();
    return 0;
}