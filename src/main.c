#include "strparse.h"
#include "cli.h"
#include <signal.h>
const _Bool tokenize_settings[] = {STRP_QUOTE_HANDLING_OFF, STRP_REMOVE_LAST_NEWLINE_ON};

extern int cli_global_fgpgid;

void sigint_handler(int sig);

// El plan es correr un loop donde el padre esté atento a la línea de comandos,
// y hacer un fork() para ejecutar lo que se ingrese.
int main()
{
	struct CLIthread session;
	char** tokenized_input;
	session.awaiting_input = 1;
	signal(SIGINT, sigint_handler);
	
	// Para manejar procesos foreground
/*
	int shell_pid = getpid();
	setpgid(shell_pid, shell_pid);
	tcsetpgrp(STDIN_FILENO, shell_pid); // for moving child processes to foreground later
	signal(SIGTTOU,SIG_IGN);
//		signal(SIGTTIN,SIG_IGN);
//		signal(SIGTSTP,SIG_IGN);
*/

	while(1) {
		while (session.awaiting_input) {
			cli_global_fgpgid = CLI_FGPGID_DEFAULT; // Esto lo resetea tras cada comando exitoso o interrupción
			print_prompt();
			size_t buffer_size = CLI_INPUT_BUFSIZE;
			char* user_input = NULL; // getline() asignara memoria automaticamente al pasar NULL
			getline(&user_input,&buffer_size,stdin);
			tokenized_input = tokenize_into_array(user_input);
//			_print_tokens(tokenized_input);
			session.command = tokenized_input[0];
			session.args = tokenized_input;

			// Ver tokenize_into_array(). Este siempre coloca NULL como ultimo token del arreglo.
			if (session.command == NULL) {
				break;
			}
			else { 
				session.awaiting_input = 0;
			}
		}
		while (!session.awaiting_input) {
			// Start the child (or child pipeline) to handle program execution for the shell, moving it into the foreground
			exec_userprompt((&session)->args);
			session.awaiting_input = 1;
		}
		// liberar memoria heap
		free_tokenized_array(tokenized_input);
	}
	printf("This shouldn't print :p\n");
	return 0;
}

void sigint_handler(int sig)
{
	if (sig == SIGINT) {
		if (cli_global_fgpgid == CLI_FGPGID_DEFAULT) {
			printf("\nType 'exit' to exit the shell\n");
			print_prompt();
		}
		else {
			// kill all foreground processes
			killpg(cli_global_fgpgid, SIGINT);
			cli_global_fgpgid = CLI_FGPGID_DEFAULT;
			printf("\n");
		}
	}
}
