#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

static const int CHUNK_SIZE = 4096;

int
main(int argc, char *argv[])
{
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        if (close(pipefd[1]) < 0) {
            perror("close write-end in child");
            exit(EXIT_FAILURE);
        }

        char buffer[CHUNK_SIZE];
        ssize_t len;
        while (1) {
            len = read(pipefd[0], buffer, CHUNK_SIZE);
            if (len < 0) {
                perror("read error");
                exit(EXIT_FAILURE);
            }
            if (len == 0) {
                break;
            }
            ssize_t written = 0;
            while (written < len) {
                ssize_t w = write(STDOUT_FILENO, buffer + written, len - written);
                if (w < 0) {
                    perror("write error");
                    exit(EXIT_FAILURE);
                }
                written += w;
            }
        }

        if (close(pipefd[0]) < 0) {
            perror("close read-end in child");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    if (close(pipefd[0]) < 0) {
        perror("close read-end in parent");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        size_t len = strlen(arg);
        size_t written = 0;
        while (written < len) {
            size_t chunk = len - written;
            if (chunk > CHUNK_SIZE) {
                chunk = CHUNK_SIZE;
            }
            ssize_t w = write(pipefd[1], arg + written, chunk);
            if (w < 0) {
                perror("write error");
                close(pipefd[1]);
                wait(NULL);
                exit(EXIT_FAILURE);
            }
            written += w;
        }
        char ndl = '\n';
        if (write(pipefd[1], &ndl, 1) < 0) {
            perror("write '\\n' error");
            close(pipefd[1]);
            wait(NULL);
            exit(EXIT_FAILURE);
        }
    }
    if (close(pipefd[1]) < 0) {
        perror("close error");
        wait(NULL);
        exit(EXIT_FAILURE);
    }
    wait(NULL);
    exit(EXIT_SUCCESS);
}