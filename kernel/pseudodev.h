enum { DEV_NULL = 0, DEV_ZERO, DEV_URANDOM, DEV_NULLSTAT };

void pseudodevinit(void);
int  pseudodevread(int user_dst, uint64 dst, int n, short minor);
int  pseudodevwrite(int user_src, uint64 src, int n, short minor);