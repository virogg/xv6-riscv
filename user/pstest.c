#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "fork error\n");
        exit(1);
    }

    char buffer[2];
    buffer[1] = '\0';
    for (int i = 1; i < argc; i++) {
        for (int j = 0; argv[i][j] != '\0'; j++) {
            buffer[0] = argv[i][j];
            printf("%d: arg %d, char '%s'\n", getpid(), i, buffer);
        }
    }

    if (pid > 0) {
        wait(0);
    }
    exit(0);
}