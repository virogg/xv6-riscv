#include "kernel/types.h"
#include "user/user.h"

static const int BUFFER_SIZE = 512;

int
read_line(int fd, char* buffer, int buffer_size)
{
    int i = 0;
    while (1){
        char c;
        int n = read(fd, &c, 1);
        if (n < 0) {
            fprintf(2, "add: read error\n");
            exit(1);
        }
        if (n == 0) {
            break;
        }
        if (c == '\n') {
            buffer[i] = '\0';
            return i;
        }
        if (i < buffer_size - 1) {
            buffer[i++] = c;
        } else {
            buffer[buffer_size - 1]= '\0';
            fprintf(2, "add: buffer overflow\n");
            exit(1);
        }
    }
    buffer[i] = '\0';
    return i;
}

int
main(int argc, char *argv[])
{
    char buffer[BUFFER_SIZE];
    int l = read_line(0, buffer, BUFFER_SIZE);
    if (l < 0) exit(1);
    printf("|%s|\n", buffer);

    char *space = 0;
    space = strchr(buffer, ' ');
    if (!space) {
        printf("add: invalid input format\n");
        exit(1);
    }

    *space = '\0';
    char *first_ptr = buffer;
    int first_sign = 1;
    if (*first_ptr == '-') {
        first_sign = -1;
        first_ptr++;
    }

    char *second_ptr = space + 1;
    int second_sign = 1;
    if (*second_ptr == '-') {
        second_sign = -1;
        second_ptr++;
    }

    if (*first_ptr == '\0' || *second_ptr == '\0') {
        printf("add: invalid input format\n");
        exit(1);
    }

    int a = atoi(first_ptr) * first_sign;
    int b = atoi(second_ptr) * second_sign;

    int sum = add(a, b); // do syscall

    printf("%d\n", sum);

    exit(0);
}