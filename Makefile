CC=gcc

myshell: src/strparse.c src/cli.c
	$(CC) -o myshell src/strparse.c src/cli.c src/main.c -I include/ -Wall -Wextra
all: myshell
