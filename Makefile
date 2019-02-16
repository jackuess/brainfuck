CC = gcc
CFLAGS = -pedantic -std=c99 -O3 -fstrict-aliasing -fsanitize=undefined
CFLAGS += -Werror -Wextra -Wall -Wconversion -Wno-sign-conversion -Wstrict-aliasing
DEBUG = 1

ifeq ($(DEBUG), 1)
    CFLAGS += -g
else
    CFLAGS += -DNDEBUG
endif

brainfuck: brainfuck.c
	$(CC) $(CFLAGS) $< -o$@
