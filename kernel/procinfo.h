#define PROC_NAME_LEN 16

struct procinfo {
    int pid;
    char name[PROC_NAME_LEN];
    int state;
    int ppid;
};
