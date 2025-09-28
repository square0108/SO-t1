CC=gcc
CFLAGS=-I include/ -Wall -Wextra

myshell: src/strparse.c src/cli.c src/main.c
	$(CC) -o myshell src/strparse.c src/cli.c src/main.c $(CFLAGS)

all: myshell

.PHONY: all clean
clean:
	rm -f myshell *.out
