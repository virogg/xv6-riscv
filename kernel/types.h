typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

#define PROC_NAME_LEN 16

struct procinfo {
    int pid;
    char name[PROC_NAME_LEN];
    int state;
    int ppid;
    char pname[PROC_NAME_LEN];
};
