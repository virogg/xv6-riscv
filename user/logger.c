#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(2, "Usage: logger [0|1]\n");
        exit(1);
    }

    int enable = atoi(argv[1]);
    if (logger(enable) < 0) {
        fprintf(2, "logger: failed to set log mode\n");
        exit(1);
    }

    exit(0);
}