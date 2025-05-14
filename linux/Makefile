CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -pedantic -O2 -D_GNU_SOURCE  -D_FILE_OFFSET_BITS=64
LDFLAGS ?=
LDLIBS  ?=
PROGRAM := getinode

SRC     := getinode.c
OBJ     := $(SRC:.c=.o)

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: clean
clean:
	$(RM) $(OBJ) $(PROGRAM)

# ensure make doesn’t confuse these as files
.PHONY: all