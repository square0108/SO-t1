#include <sys/wait.h>
#include "strparse.h"
#include "cli.h"
#define FD_NOPIPE -1

// just one more global variable bro
int cli_global_fgpgid = CLI_FGPGID_DEFAULT;

// Utiliza la system call getcwd(), que esta limitada por un buffer size.
void print_prompt()
{
	char username[] = "USERNAME"; 
	char wd_buffer[CLI_WORKDIR_BUFSIZE];
	dprintf(STDOUT_FILENO,"%s:%s $ ",username,getcwd(wd_buffer,(size_t) CLI_WORKDIR_BUFSIZE));
}

int errcheck_pipe(int status);

int make_proc_fgp_leader(int pid);

void exec_userprompt(char* tokenized_input[])
{
	int tok_idx = 0, num_cmds = 0, arg_idx = 0;
	char* cmds[CLI_MAX_CMDS][CLI_MAX_ARGS]; // x = N-prompt segment to be piped ; y = argument(s) associated to the n-ary command
	// offload all the tokenized commands into the cmds[][] array, grouping them for a pipeline
	while (tokenized_input[tok_idx] != NULL) {
		if (strcmp(tokenized_input[tok_idx], "|") == 0) { 
			cmds[num_cmds][arg_idx] = NULL; // null-terminate this command's array of arguments, '|' MUST NOT be written to args
			num_cmds++;
			arg_idx = 0;
		}
		else {
			cmds[num_cmds][arg_idx] = tokenized_input[tok_idx];
			arg_idx++;
		}
		tok_idx++;
	}
	cmds[num_cmds][arg_idx] = NULL; // null-terminate the last command in the pipeline
	num_cmds++;
	pipeline_exec(cmds, num_cmds);
}

/*
Con 'comandos' nos referimos a toda una seccion del prompt entre dos caracteres pipe ('|'), o entre el principio del prompt y un pipe, o entre un pipe y el final del prompt, o el prompt completo si no hay algun pipe.

input: 
- arreglo multidimensional cmds[][]: Cada par (i,j) tiene el siguiente significado: La posición i-ésima denota al comando i-ésimo comando a ejecutar (indexando desde 0), y los strings desde 0 a j_max son los argumentos de cada comando.
- num_cmds: la extension maxima de `i` descrita anteriormente, el numero total de comandos a ejecutar.
*/
void pipeline_exec(char *cmds[][CLI_MAX_ARGS], int num_cmds)
{
	int fd_read_prev = FD_NOPIPE;
	if (num_cmds > 1) {
		for (int i = 0; i < num_cmds; i++) {
			int fd_pipe[2];
			pipe(fd_pipe);
			int pid = fork();
			if (pid == 0) {
				// --- Hijo ---
//				printf("CHILD | i: %i ; pipes: %i %i ; fd_read_prev: %i\n",i,fd_pipe[0],fd_pipe[1],fd_read_prev);
				// Asigna a hijo al grupo "foreground" (comparten el mismo pgid que el primer comando)
				if (i == 0) 
					make_proc_fgp_leader(0);
				else
					setpgid(0,cli_global_fgpgid);

				if (i != num_cmds-1) {
					// conectar stdout con pipe de escritura
					dup2(fd_pipe[1],STDOUT_FILENO);
				}
				// hijo no lee desde pipe[0], si no que desde fd_read_prev (descriptor que recibio escritura del hijo en i-1). Lee hasta alcanzar EOF en fd_read_prev.
				close(fd_pipe[1]);
				close(fd_pipe[0]);
				if (fd_read_prev != FD_NOPIPE) {
					dup2(fd_read_prev,STDIN_FILENO);
					close(fd_read_prev);
				}
				execvp(cmds[i][0],cmds[i]);
				printf("Error. Command not found: %s\n",cmds[i][0]);
				_exit(127);
			}
			else if (pid > 0) {
				// --- Padre ---
				// Asigna a hijos al grupo "foreground" (redundante para evitar condicion de carrera)
				if (i == 0)
					make_proc_fgp_leader(pid);
				else
					setpgid(pid,cli_global_fgpgid);

				// Cierra pipe del proceso hijo anterior, pues la copia de este descriptor ya la tiene el hijo actual
				if (fd_read_prev != FD_NOPIPE) {
					close(fd_read_prev);
				}
//				printf("PARENT | i: %i ; pipes: %i %i ; fd_read_prev (Closed): %i ",i,fd_pipe[0],fd_pipe[1],fd_read_prev);
				// fd_read_prev es actualizado y copiado a la memoria del hijo creado en el siguiente ciclo. El hijo actual escribe a esta pipe, así su output se transfiere al siguiente hijo.
				if (i != num_cmds-1) {
					fd_read_prev = fd_pipe[0];
				}
				else close(fd_pipe[0]);
//				printf("fd_read_prev (New): %i\n", fd_read_prev);
				close(fd_pipe[1]);
			}
		}
		
	}
	else {
		int pid = fork();
		if (pid == 0) {
			setpgid(0,0);
			cli_global_fgpgid = getpid();
			execvp(cmds[0][0],cmds[0]);
			printf("Error. Command not found: %s\n", cmds[0][0]);
			_exit(127);
		}
		else if (pid > 0) {
			setpgid(pid,pid);
			cli_global_fgpgid = pid;
		}
	}		

	for (int i = 0; i < num_cmds; i++) wait(NULL);
	// Si ya terminaron todos los hijos, entonces terminó todo el foreground process group
	cli_global_fgpgid = CLI_FGPGID_DEFAULT;
}

int errcheck_pipe(int status)
{
	if (status >= 0) return 0;
	else {
		dprintf(STDERR_FILENO, "Pipe creation failed!\n");
		exit(-1);
	}
}

int make_proc_fgp_leader(int pid)
{
	setpgid(pid,pid);
	cli_global_fgpgid = pid;
	return 0;
}
