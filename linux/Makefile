CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -Wno-deprecated-declarations

TARGET = fifo_echo_server
SRC = fifo_echo_server.c logger.c

.PHONY: all clean rebuild

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $^

clean:
	rm -f $(TARGET)
	rm -f $(FIFO_FILE)

rebuild:
	$(MAKE) clean
	$(MAKE) all