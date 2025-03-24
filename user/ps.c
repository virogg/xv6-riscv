#include "kernel/types.h"
#include "user/user.h"

static char*
state_to_string(int st)
{
    switch(st){
        case 0:  return "UNUSED";
        case 1:  return "USED";
        case 2:  return "SLEEP\t";
        case 3:  return "RUNNABLE\t";
        case 4:  return "RUNNING\t";
        case 5:  return "ZOMBIE\t";
        default: return "???";
    }
}

int
main()
{
    int lim;
    int r;
    struct procinfo *buf = 0;

    r = ps_listinfo(0, 0);
    if (r < 0) {
        printf("ps: error getting process count, err=%d\n", r);
        exit(1);
    }
    if (r == 0) {
        fprintf(2, "ps: no processes\n");
        exit(1);
    }
    lim = r;

    while(1) {
        buf = malloc(lim * sizeof(struct procinfo));
        if (buf == 0) {
            printf("ps: malloc error\n");
            exit(1);
        }
        r = ps_listinfo(buf, lim);
        if (r == -2) {
            free(buf);
            lim *= 2;
        } else {
            break;
        }
    }
    printf("Got %d processes (buffer size %d)\n", r, lim);
    printf("id\tname\tstate\t\tppid\tpname\n");
    for (int i = 0; i < r; i++) {
        printf("%d\t%s\t%s\t%d\t%s\n", buf[i].pid, buf[i].name, state_to_string(buf[i].state), buf[i].ppid, buf[i].pname);
    }
    free(buf);
    exit(0);
}