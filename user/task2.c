#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static const int CHUNK_SIZE = 64;

int
write_chunks(int fd, const char *buf, int len)
{
    int written = 0;
    while (written < len) {
        int chunk = len - written;
        if (chunk > CHUNK_SIZE) {
            chunk = CHUNK_SIZE;
        }

        int n = write(fd, buf + written, chunk);
        if (n < 0) {
            return -1;
        }
        written += n;
    }
    return 0;
}

int
main(int argc, char* argv[])
{
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        fprintf(2, "pipe error\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "fork error\n");
        exit(1);
    }
    if (pid == 0) {
        close(pipefd[1]);
        close(0);
        if (dup(pipefd[0]) < 0) {
            fprintf(2, "dup error\n");
            exit(1);
        }
        close(pipefd[0]);

        char *wc_argv[] = {"/wc", 0};
        exec("/wc", wc_argv);

        fprintf(2, "exec error\n");
        exit(1);
    }
    close(pipefd[0]);
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        int len = strlen(arg);
        if (write_chunks(pipefd[1], arg, len) < 0) {
            fprintf(2, "write error\n");
            close(pipefd[1]);
            wait(0);
            exit(1);
        }
        char ndl = '\n';
        if (write_chunks(pipefd[1], &ndl, 1) < 0) {
            fprintf(2, "write '\\n' error\n");
            close(pipefd[1]);
            wait(0);
            exit(1);
        }
    }

    if (close(pipefd[1]) < 0) {
        fprintf(2, "close error\n");
    }
    wait(0);
    exit(0);
}