CC=gcc

myshell: src/strparse.c src/cli.c
	$(CC) -o myshell src/strparse.c src/cli.c -I include/ -Wall -Wextra
tests: src/strparse.c src/tests.c
	$(CC) -o tests src/strparse.c src/tests.c -I include/ -Wall -Wextra
all: myshell tests
