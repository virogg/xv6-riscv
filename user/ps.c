#include "kernel/types.h"
#include "user/user.h"

#define BUFFER_SIZE 64

static char*
state_to_string(int st)
{
    switch(st){
        case 0: return "UNUSED";
        case 1: return "SLEEP";
        case 2: return "RUNNABLE";
        case 3: return "RUNNING\t";
        case 4: return "ZOMBIE\t";
        default: return "???\t";
    }
}

int
main()
{
    int lim, n;
    struct procinfo *buf = 0;

    n = ps_listinfo(0, 0);
    if (n < 0) {
        printf("ps: error getting process count, err=%d\n", n);
        exit(1);
    }
    if (n == 0) {
        fprintf(2, "ps: no processes\n");
        exit(1);
    }
    lim = n;

    while(1) {
        buf = malloc(lim * sizeof(struct procinfo));
        if (buf == 0) {
            printf("ps: malloc error\n");
            exit(1);
        }
        n = ps_listinfo(buf, lim);
        if (n == -2){
            free(buf);
            lim *= 2;
        } else {
            break;
        }
    }
    printf("Got %d processes (buffer size %d)\n", n, lim);
    printf("id\tname\tstate\t\tppid\tpname\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%s\t%s\t%d\t%s\n", buf[i].pid, buf[i].name, state_to_string(buf[i].state), buf[i].ppid, buf[i].pname);
    }
    free(buf);
    exit(0);
}