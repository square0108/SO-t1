#include "strparse.h"
#include "cli.h"
const _Bool tokenize_settings[] = {STRP_QUOTE_HANDLING_OFF, STRP_REMOVE_LAST_NEWLINE_ON};

// El plan es correr un loop donde el padre esté atento a la línea de comandos,
// y hacer un fork() para ejecutar lo que se ingrese.
int main()
{
	struct CLIthread session;
	char** tokenized_input;
	session.awaiting_input = 1;
	while(1) {
		while (session.awaiting_input) {
			print_prompt();
			size_t buffer_size = CLI_INPUT_BUFSIZE;
			char* user_input = NULL; // getline() asignara memoria automaticamente al pasar NULL
			getline(&user_input,&buffer_size,stdin);
			tokenized_input = partition_into_array(user_input, tokenize_settings[0], tokenize_settings[1]); 
			session.command = tokenized_input[0];
			session.args = tokenized_input;

			// If the resulting input is "\n", it is the equivalent of an empty "enter", or a prompt of only spaces.
			if (strlen(session.command) == 0) {
				break;
			}
			else { 
				session.awaiting_input = 0;
				break;
			}
		}
		while (!session.awaiting_input) {
			fork_n_execp(session.command, session.args, STDOUT_FILENO);
			session.awaiting_input = 1;
		}
		// free heap memory, partition_into_array will reserve memory for this variable regardless of whether the input is empty or not
		free_tokenized_array(tokenized_input);
	}
	printf("This shouldn't print :p\n");
	return 0;
}

