CC=gcc
CFLAGS=-I include/ -Wall -Wextra

gatosh: src/strparse.c src/cli.c src/main.c
	$(CC) -o gatosh src/strparse.c src/cli.c src/main.c $(CFLAGS)

all: gatosh

.PHONY: all clean
clean:
	rm -f gatosh *.out

