#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define BUFSIZE 512

static char hex_table[] = "0123456789abcdef";

void
print_hex_byte(uint byte)
{
    char buf[3];
    buf[0] = hex_table[(byte >> 4) & 0xF];
    buf[1] = hex_table[byte & 0xF];
    buf[2] = ' ';
    write(1, buf, 3);
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(2, "Usage: hexdump <file> <bytes>\n");
        exit(1);
    }

    int fd = open(argv[1], 0);
    if(fd < 0) {
        fprintf(2, "Error opening %s\n", argv[1]);
        exit(1);
    }

    int count = atoi(argv[2]);
    char buf[BUFSIZE];
    int total = 0;
    int line_pos = 0;

    while(total < count) {
        int to_read = (count - total > BUFSIZE) ? BUFSIZE : count - total;
        int n = read(fd, buf, to_read);
        if(n <= 0) break;

        for(int i = 0; i < n; i++) {
            print_hex_byte(buf[i]);

            if(++line_pos % 16 == 0) write(1, "\n", 1);
        }
        total += n;
    }
    if(line_pos % 16 != 0)
        write(1, "\n", 1);

    printf("\nRead %d bytes\n", total);
    close(fd);
    exit(0);
}