#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "strparse.h"
#define WORKDIR_BUFSIZE 512
#define CLINPUT_BUFSIZE 512

void print_prompt();
/* Debugging functions */

// El plan es correr un loop donde el padre esté atento a la línea de comandos,
// y hacer un fork() para ejecutar lo que se ingrese.
int main()
{

	_Bool is_awaiting_input = 1;
	char* program;
	char** args;
	while (is_awaiting_input) {
		print_prompt();
		size_t buffer_size = CLINPUT_BUFSIZE;
		char* user_input = NULL; // getline() asignara memoria automaticamente al pasar NULL
		getline(&user_input,&buffer_size,stdin);
		// parsing time baybee
			
		dprintf(STDIN_FILENO,"%s",user_input);
	}	

	return 0;
}

// Utiliza la system call getcwd(), que esta limitada por un buffer size.
void print_prompt() // suena como pling plong
{
	char username[] = "USERNAME"; 
	char wd_buffer[WORKDIR_BUFSIZE];
	dprintf(STDOUT_FILENO,"%s:%s $ ",username,getcwd(wd_buffer,(size_t) WORKDIR_BUFSIZE));
}
