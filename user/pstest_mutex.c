#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int mu;
    mutex(&mu);
    if (mu < 0) {
        fprintf(2, "mutex create error\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "fork error\n");
        exit(1);
    }

    char buffer[2];
    buffer[1] = '\0';
    for (int i = 1; i < argc; i++) {
        for (int j = 0; argv[i][j] != '\0'; j++) {
            if (mutex_lock(mu) < 0) {
                fprintf(2, "mutex lock error\n");
                exit(1);
            }

            buffer[0] = argv[i][j];
            printf("%d: arg %d, char '%s'\n", getpid(), i, buffer);

            if (mutex_unlock(mu) < 0) {
                fprintf(2, "mutex unlock error\n");
                exit(1);
            }
        }
    }

    if (pid > 0) {
        wait(0);
        close(mu);
    }
    exit(0);
}