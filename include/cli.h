#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#define CLI_AWAITING_INPUT 1
#define CLI_PROCESSING_INPUT 0
#define CLI_WORKDIR_BUFSIZE 512
#define CLI_INPUT_BUFSIZE 512

struct CLIthread {
	_Bool awaiting_input;
	char* command;
	char** args;
};

/* Ejecuta el programa con nombre de archivo `program`, pasando argumentos con `args[]` (el cual debe estar terminado con NULL), y dirige la salida del programa al descriptor de archivo fildes_dest  */
void fork_n_execp(char* program, char* args[], int fildes_dest);
void print_prompt();
void sigint_handler(int sig);
