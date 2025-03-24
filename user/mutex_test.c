#include "kernel/types.h"
#include "user/user.h"

void test_read_write() {
    int mu = mutex();
    if (mu < 0) {
        fprintf(2, "[test_read_write] failed: mutex create error\n");
        exit(1);
    }

    char buf[10];
    if (read(mu, buf, 10) != -1) {
        fprintf(2, "[test_read_write] failed: read succeeded\n");
    }
    if (write(mu, "test", 4) != -1) {
        fprintf(2, "[test_read_write] failed: write succeeded\n");
    }

    close(mu);
    printf("[test_read_write] PASS\n");
}

void test_close_self_locked() {
    int mu = mutex();
    if (mu < 0) {
        fprintf(2, "[test_close_self_locked] failed: mutex create error\n");
        exit(1);
    }

    if (mutex_lock(mu) < 0) {
        fprintf(2, "[test_close_self_locked] failed: mutex lock error\n");
        exit(1);
    }

    if (close(mu) != 0) {
        fprintf(2, "[test_close_self_locked] failed: close error\n");
    } else {
        printf("[test_close_self_locked] PASS\n");
    }
}

void test_close_by_other() {
    int mu = mutex();
    if (mu < 0) {
        fprintf(2, "[test_close_by_other] failed: mutex create error\n");
        exit(1);
    }
    if (mutex_lock(mu) < 0) {
        fprintf(2, "[test_close_by_other] failed: mutex lock error\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "[test_close_by_other] failed: fork error\n");
        exit(1);
    }

    if (pid == 0) {
        if (close(mu) < 0) {
            fprintf(2, "[test_close_by_other] failed: child mutex close failed\n");
            exit(1);
        } else {
            fprintf(2, "[test_close_by_other]: if there are no kfree's in LOGGER before this message, then OK\n");
        }
        exit(0);
    } else {
        sleep(20);
        if (mutex_unlock(mu) < 0) {
            fprintf(2, "[test_close_by_other] failed: parent mutex unlock error\n");
            exit(1);
        }
        if (close(mu) < 0) {
            fprintf(2, "[test_close_by_other] failed: parent mutex close failed\n");
            exit(1);
        }
        wait(0);
        printf("[test_close_by_other] PASS\n");
    }
}

void test_exit_with_mutex() {
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "[test_exit_with_mutex] failed: fork error\n");
        exit(1);
    }

    if (pid == 0) {
        int mu = mutex();
        if (mu < 0) {
            fprintf(2, "[test_exit_with_mutex] failed: child mutex create error\n");
            exit(1);
        }

        if (mutex_lock(mu) < 0) {
            fprintf(2, "[test_exit_with_mutex] failed: mutex lock error\n");
            exit(1);
        }
        exit(0);
    } else {
        wait(0);
        int mu = mutex();
        if (mu < 0) {
            fprintf(2, "[test_exit_with_mutex] failed: parent mutex create error\n");
            exit(1);
        }

        if (mutex_lock(mu) < 0) {
            fprintf(2, "[test_exit_with_mutex] failed: parent mutex lock failed\n");
        } else {
            printf("[test_exit_with_mutex] PASS\n");
            mutex_unlock(mu);
        }
        close(mu);
    }
}

void test_unlock_foreign() {
    int mu = mutex();
    if (mu < 0) {
        fprintf(2, "[test_unlock_foreign] failed: mutex create error\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        printf("[test_unlock_foreign] failed: fork error\n");
        exit(1);
    }

    if (pid == 0) {
        if (mutex_lock(mu) < 0) {
            fprintf(2, "[test_unlock_foreign] failed: mutex lock error\n");
            exit(1);
        }
        sleep(50);
        exit(0);
    } else {
        sleep(10);
        if (mutex_unlock(mu) == 0) {
            fprintf(2, "[test_unlock_foreign] failed: unlock succeeded\n");
        }
        else {
            printf("[test_unlock_foreign] PASS\n");
        }
        close(mu);
        wait(0);
    }
}

int main() {
    test_read_write();
    test_close_self_locked();
    test_close_by_other();
    test_exit_with_mutex();
    test_unlock_foreign();
    exit(0);
}