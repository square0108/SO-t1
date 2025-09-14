#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#define CLI_AWAITING_INPUT 1
#define CLI_PROCESSING_INPUT 0
#define CLI_WORKDIR_BUFSIZE 512
#define CLI_INPUT_BUFSIZE 512
#define CLI_FGPGID_DEFAULT -2
#define CLI_MAX_ARGS 1024
#define CLI_MAX_CMDS 256

extern int cli_global_fgpgid;

struct CLIthread {
	_Bool awaiting_input;
	char* command;
	char** args;
};

void pipeline_exec(char* cmds[][CLI_MAX_ARGS], int num_cmds);
void print_prompt();
void sig_handler(int sig);
void exec_userprompt(char* tokenized_input[]);
