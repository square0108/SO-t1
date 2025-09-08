#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "strparse.h"
#define WORKDIR_BUFSIZE 512
#define CLINPUT_BUFSIZE 512
const _Bool tokenize_settings[] = {STRP_QUOTE_HANDLING_OFF, STRP_REMOVE_LAST_NEWLINE_ON};

void fork_n_execp(char* program, char* args[]);
void print_prompt();

// El plan es correr un loop donde el padre esté atento a la línea de comandos,
// y hacer un fork() para ejecutar lo que se ingrese.
int main()
{
	_Bool is_awaiting_input = 1;
	while (1) {
	while (is_awaiting_input) {
		print_prompt();
		size_t buffer_size = CLINPUT_BUFSIZE;
		char* user_input = NULL; // getline() asignara memoria automaticamente al pasar NULL
		getline(&user_input,&buffer_size,stdin);
		char** tokenized_input = partition_into_array(user_input, tokenize_settings[0], tokenize_settings[1]); 
		char* program = tokenized_input[0];
		char** args = tokenized_input;
		// If the resulting input is "\n", it is the equivalent of an empty "enter", or a prompt of only spaces.
		if (strlen(program) == 0) {
			free_tokenized_array(tokenized_input);
			continue;
		}
		else {
			is_awaiting_input = 0;
			fork_n_execp(program, args);
			is_awaiting_input = 1;
		}
		free_tokenized_array(tokenized_input);
	}
	}
	printf("This shouldn't print :p\n");
	return 0;
}

// Utiliza la system call getcwd(), que esta limitada por un buffer size.
void print_prompt()
{
	char username[] = "USERNAME"; 
	char wd_buffer[WORKDIR_BUFSIZE];
	dprintf(STDOUT_FILENO,"%s:%s $ ",username,getcwd(wd_buffer,(size_t) WORKDIR_BUFSIZE));
}

void fork_n_execp(char* program, char* args[])
{
	int fd[2];
	int pipe_status = pipe(fd);
	if (pipe_status == -1) {
		dprintf(STDERR_FILENO, "Pipe failed during command execution!\n");
		exit(-1);
	}

	int child_pid = fork();
	if (child_pid == -1) {
		dprintf(STDERR_FILENO, "Fork failed during command execution!\n");
		exit(-1);
	}

	// output of the child is sent to the parent's read end of the pipe
	if(child_pid == 0) {
		dup2(fd[1],STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		execvp(program,args);

	}
	else {
		close(fd[1]);
		char buf[2048]; // this should probably be #define'd
		int nbytes = sizeof(buf);
		int bytes_read;
		while ((bytes_read = read(fd[0],buf,nbytes)) > 0 ) {
			write(STDOUT_FILENO, buf, bytes_read);
		}
		close(fd[0]); // if read returned -1, then parent has finished reading
		wait(NULL);
	}
}
