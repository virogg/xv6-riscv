#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define BUFSIZE 512

void test_null_device(int fd);
void test_zero_device(int fd);
void test_urandom_device(int fd);
void test_nullstat_device(int fd);
void validate_device(int fd, const char *name);

int main() {
    printf("\n=== STARTING TESTS ===\n");

    int fd_null = open("dev_null", O_RDWR);
    int fd_zero = open("dev_zero", O_RDWR);
    int fd_urandom = open("dev_urandom", O_RDWR);
    int fd_nullstat = open("dev_nullstat", O_RDWR);

    validate_device(fd_null, "dev_null");
    validate_device(fd_zero, "dev_zero");
    validate_device(fd_urandom, "dev_urandom");
    validate_device(fd_nullstat, "dev_nullstat");

    test_null_device(fd_null);
    test_zero_device(fd_zero);
    test_urandom_device(fd_urandom);
    test_nullstat_device(fd_nullstat);

    close(fd_null);
    close(fd_zero);
    close(fd_urandom);
    close(fd_nullstat);

    printf("\n=== TESTS PASSED ===\n");
    exit(0);
}

void validate_device(int fd, const char *name) {
    if(fd < 0) {
        fprintf(2, "Error: device %s has not been opened\n", name);
        exit(1);
    }
}

void test_null_device(int fd) {
    printf("\n=== Test dev_null ===\n");

    char data[] = "test_data";
    int write_result = write(fd, data, sizeof(data));
    printf("Writing %ld bytes: %s\n", sizeof(data), write_result == sizeof(data) ? "PASS" : "FAIL");

    char buf[BUFSIZE];
    int read_result = read(fd, buf, sizeof(data));
    printf("Trying to read from dev_null: got %d bytes (expected 0)\n", read_result);
}

void test_zero_device(int fd) {
    printf("\n=== Test dev_zero ====\n");

    int write_result = write(fd, "invalid", 6);
    printf("Writing: %s (got %d)\n", write_result == -1 ? "PASS (Got error)" : "FAIL", write_result);

    char buf[4];
    read(fd, buf, sizeof(buf));
    printf("Read 4 bytes from dev_zero: [%d][%d][%d][%d] %s\n", buf[0], buf[1], buf[2], buf[3], (buf[0] | buf[1] | buf[2] | buf[3]) == 0 ? "PASS" : "FAIL");
}

void test_urandom_device(int fd) {
    printf("\n=== Test dev_urandom ===\n");

    char initial[4], updated[4];

    read(fd, initial, sizeof(initial));
    printf("Read 4 bytes from initial dev_urand: [%d][%d][%d][%d]\n", initial[0], initial[1], initial[2], initial[3]);

    read(fd, initial, sizeof(initial));
    printf("Read another 4 bytes from initial dev_urand: [%d][%d][%d][%d]\n", initial[0], initial[1], initial[2], initial[3]);

    printf("Write not sizeof(seed) bytes: %s (got %d)\n", write(fd, "test", 5) == -1 ? "PASS (Got error)" : "FAIL", write(fd, "test", 5));

    uint32 new_seed = 0x12345678;
    int write_result = write(fd, &new_seed, sizeof(new_seed));
    printf("Write sizeof(seed) bytes: %s\n", write_result == sizeof(new_seed) ? "PASS" : "FAIL");

    read(fd, updated, sizeof(updated));
    printf("Read 4 bytes from updated dev_urand: [%d][%d][%d][%d]\n", updated[0], updated[1], updated[2], updated[3]);
}

void test_nullstat_device(int fd) {
    printf("\n=== Test dev_nullstat ===\n");

    uint64 counter;
    const char *test_data = "test123";

    int write_result = write(fd, test_data, 7);
    printf("Write 7 bytes to nullstat: %s (consumed %d bytes)\n",
           write_result == 7 ? "PASS" : "FAIL",
           write_result);

    char buf[6];
    int read_result = read(fd, buf, sizeof(buf));
    printf("Read less than 8 bytes: %s (expected -1, got %d)\n",
           read_result == -1 ? "PASS" : "FAIL",
           read_result);

    read_result = read(fd, &counter, sizeof(counter));
    if(read_result == sizeof(counter)) {
        printf("Read 8 bytes: PASS\n");
        printf("Total bytes collected: %lu (expected 7)\n", counter);
    } else {
        printf("Read 8 bytes: FAIL (got %d bytes)\n", read_result);
    }
}
