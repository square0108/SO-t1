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

void exec_userprompt(char* tokenized_input[])
{
	/* TODO: section below to be moved into strparse.c (?)*/
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
	/* end section */
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
	int fd_prev = FD_NOPIPE, pipeline_pgid = 0;
	for (int cmd_idx = 0; cmd_idx < num_cmds; cmd_idx++) {
		int pipe_fd[2];
		int status = pipe(pipe_fd);
		errcheck_pipe(status); 

		int pid = fork(); // En cada loop, el hijo de la iteracion actual muere, y la memoria del padre es heredada al hijo del ciclo cmd_idx+1.
		/* --- Child --- */
		if (pid == 0) {
			if (cmd_idx == 0) { // Primer comando del pipeline
				pipeline_pgid = getpid();
				setpgid(0,pipeline_pgid);
				cli_global_fgpgid = pipeline_pgid; // Global var
			}
			else { // if not first command, join existing proc group
				setpgid(0, pipeline_pgid);
			}

			/* --- Pipe setup y ejecucion de comandos --- */
			// El primer hijo no lee, pero todos desde [1,cmd_idx-1] leen input desde fd_prev
			if (cmd_idx != 0) {
				dup2(fd_prev, STDIN_FILENO);
				close(fd_prev);
			}
			// El ultimo hijo no debe escribir a un pipe, por defecto escribirá a STDOUT.
			if (cmd_idx != num_cmds-1) {
				close(pipe_fd[0]);
				dup2(pipe_fd[1],STDOUT_FILENO);
				close(pipe_fd[1]);
			}
			// Si execvp retorna exitosamente, llama exit(0), esto previene que el hijo haga fork() en la siguiente iteración del for loop
			execvp(cmds[cmd_idx][0],cmds[cmd_idx]);
			// Manejo de errores para comando no reconocido
			dprintf(STDERR_FILENO, "Command not found: \"%s\"\n", cmds[cmd_idx][0]);
			_exit(127);
		} 
		/* --- Parent --- */
		else {
			// Esto es redundante, pero evita condiciones de carrera al encadenar procesos hijos al grupo de procesos pipeline
			if (cmd_idx == 0) {
				pipeline_pgid = pid;
				setpgid(pid,pipeline_pgid);
				cli_global_fgpgid = pipeline_pgid; // Global var
			}
			else {
				setpgid(pid,pipeline_pgid);
			}

			/* --- Pipe management --- */ 
			// El padre debe cerrar el descriptor de lectura anterior, pues heredará fd_prev a su hijo en el siguiente ciclo for.
			if (fd_prev != FD_NOPIPE) {
				close(fd_prev);
			}
			if (cmd_idx != num_cmds-1) { // if parent is not the last command
				// Estos descriptores y sus estados son heredados por el siguiente hijo en la pipeline
				close(pipe_fd[1]);
				fd_prev = pipe_fd[0];
			}
		}
	/*
		// nota: esto escribe inmediatamente, sin esperar al termino de ejecucion del proceso hijo
		while ((bytes_read = read(fd[0],buf,nbytes)) > 0 ) {
			write(fildes_dest, buf, bytes_read);
		}
		close(fd[0]); // if read returned -1, then parent has finished reading
		wait(NULL);
	*/
	}
	// Esperar a que termine la pipeline
	for (int i = 0; i < num_cmds; i++) wait(NULL);
}

int errcheck_pipe(int status)
{
	if (status >= 0) return 0;
	else {
		dprintf(STDERR_FILENO, "Pipe creation failed!\n");
		exit(-1);
	}
}
