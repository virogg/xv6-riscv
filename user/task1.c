#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static const int SLEEP_TIME = 10;

int
main(int argc, char* argv[])
{
    int use_kill = 0;
    if (argc > 1 && strcmp(argv[1], "-kill") == 0) {
        use_kill = 1;
    }
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "fork error\n");
        exit(2);
    }
    if (pid == 0) {
        sleep(SLEEP_TIME);
        exit(1);
    }
    printf("Parent PID: %d, Child PID: %d\n", getpid(), pid);

    if (use_kill == 1) {
        if (kill(pid) < 0) {
            fprintf(2, "kill error\n");
            exit(1);
        }
    }

    int status;
    int p = wait(&status);
    if (p < 0) {
        fprintf(2, "wait error\n");
        exit(1);
    }

    printf("Process: %d, Status: %d\n", p, status);
    exit(0);
}