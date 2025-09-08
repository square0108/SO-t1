#include <unistd.h>
#include "strparse.h"
#include <stdio.h>
#include <stdlib.h>
#define GETLINE_BUFSIZE 4096

// unused for now
const char* available_tests[] = {
	"strparse with getline",
	0
};

int strparse_print_tokens(char** str);
void strparse_getline();

int main(int argc, char* argv[])
{
	if (argc != 2) {
		int temp = 0;
		dprintf(STDERR_FILENO, "Execute with the number of the test to run:\n");
		while (available_tests[temp] != 0) {
			printf("%s: %i\n", available_tests[temp], temp);
			exit(-1);
		}
	}
	int testchoice = atoi(argv[1]);
	// surely this is temporary
	switch (testchoice) {
		case 0:
			strparse_getline();
			break;
		default:
			break;
	};
}

void strparse_getline()
{
	size_t buffer_size = GETLINE_BUFSIZE;
	char* userin = NULL;
	getline(&userin,&buffer_size,stdin);
	char** asd = partition_into_array(userin, STRP_QUOTE_HANDLING_OFF, STRP_REMOVE_LAST_NEWLINE_ON);
	strparse_print_tokens(asd);
	free_tokenized_array(asd);
}

int strparse_print_tokens(char** str)
{	
	int idx = 0;
	printf("Tokens:\n");
	while (str[idx] != NULL) {
		printf("{%s}\n", str[idx]);
		idx++;
	}
 	idx = 0;
	printf("Addresses: \n");
	while (str[idx] != NULL) {
		printf("{%p}\n", &(str[idx]));
		idx++;
	}
	printf("last token: {%s}\n", str[idx]);
	printf("last pointer: {%p}\n", &str[idx]);
	printf("sizeof token array: {%li}\n", sizeof(str));
	return 0;
}
