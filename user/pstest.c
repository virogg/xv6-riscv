#include "kernel/types.h"
#include "user/user.h"

void
assert(int s, const char* test_name)
{
    if (s) return;
    fprintf(2, "[%s] failed\n", test_name);
    exit(1);
}

void
test_buffer_size()
{
    const int lim = 2;
    struct procinfo *buf = malloc(lim * sizeof(struct procinfo));
    if (buf == 0) {
        fprintf(2, "[test_buffer_size] failed: malloc error\n");
        free(buf);
        return;
    }
    int pid = fork();

    if (pid < 0) {
        fprintf(2, "[test_buffer_size] failed: fork error\n");
        free(buf);
        return;
    }
    if (pid == 0) {
        sleep(10);
        free(buf);
        exit(1);
    }
    int r = ps_listinfo(buf, lim);
    free(buf);
    assert(r == -2, "test_buffer_size");
    printf("[test_buffer_size]: passed, err = %d\n", r);
}

void
test_incorrect_address()
{
    int r = ps_listinfo((struct procinfo*)0xffffffff, 10);
    assert(r < 0, "test_incorrect_address");
    printf("[test_incorrect_address]: passed, err = %d\n", r);
}

void
test_null_plist()
{
    int r = ps_listinfo(0, 0);
    assert(r >= 0, "test_null_plist");
    printf("[test_null_plist]: passed, total proc = %d\n", r);
}

void
test_correct()
{
    const int lim = 64;
    struct procinfo *buf = malloc(lim * sizeof(struct procinfo));
    if (buf == 0) {
        fprintf(2, "[test_correct] failed: malloc error\n");
        free(buf);
        return;
    }
    int r = ps_listinfo(buf, lim);
    free(buf);
    assert(r > 0 && r <= 64, "test_correct");
    printf("[test_correct]: passed, r = %d\n", r);
}

int
main()
{
    test_buffer_size();
    test_incorrect_address();
    test_null_plist();
    test_correct();
    exit(0);
}