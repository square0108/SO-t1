#include "strparse.h"
#include "cli.h"
#include <signal.h>

const _Bool tokenize_settings[] = {STRP_QUOTE_HANDLING_OFF, STRP_REMOVE_LAST_NEWLINE_ON};
struct CLIthread session = {.awaiting_input = 1};

void sigint_handler(int sig);

// El plan es correr un loop donde el padre esté atento a la línea de comandos,
// y hacer un fork() para ejecutar lo que se ingrese.
int main()
{
	char** tokenized_input;

	// Para manejar procesos foreground
	setpgid(0,0);
	signal(SIGTTOU,SIG_IGN); // si no hago esto, muere el shell. Puede desbloquearse despues de tcsetpgrp
	tcsetpgrp(STDIN_FILENO, getpid()); // for moving child processes to foreground later

    CLICommand cmd = handle_alias("gato");
    char* args[] = {"gato", NULL};
    exec(cmd,  args);
	while(1) {
		while (session.awaiting_input) {
			signal(SIGINT, sigint_handler); // Ignorar SIGINT si no hay comando ejecutandose
			print_prompt();
			size_t buffer_size = CLI_INPUT_BUFSIZE;
			char* user_input = NULL; // getline() asignara memoria automaticamente al pasar NULL
			getline(&user_input,&buffer_size,stdin);
			tokenized_input = tokenize_into_array(user_input);
			session.command = tokenized_input[0];
			session.args = tokenized_input;

			// Ver tokenize_into_array(). Este siempre coloca NULL como ultimo token del arreglo.
			if (session.command == NULL || strlen(session.command) == 0) {
				break;
			}
			else { 
				session.awaiting_input = 0;
			}
		}
		while (!session.awaiting_input) {
			signal(SIGINT,SIG_DFL);
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
	if (sig == SIGINT && session.awaiting_input == 1) {
		printf("\nType 'exit' to exit the shell\n");
		print_prompt();
	}
}
