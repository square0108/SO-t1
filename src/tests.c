#include <unistd.h>
#include "strparse.h"
#include <stdio.h>
#include <stdlib.h>

// unused for now
const char* available_tests[] = {
	"strparse_in"
};

int strparse_in(char* str, char* delim);

int main(int argc, char* argv[])
{
	/*
	if (argc != 2) {
		dprintf(STDERR_FILENO, "usage: ./tests runall OR ./tests list\n");
		exit(-1);
	}
	if (strcmp(argv[1],"list")) {
		
	}
	*/
	char** asd = partition_into_array(argv[1], argv[2]);
	int idx = 0;
	printf("Tokens:\n");
	while (asd[idx] != NULL) {
		printf("{%s} ", asd[idx]);
		idx++;
	}
	printf("\n");	
}
