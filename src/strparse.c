#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include "strparse.h"
#define NTOKENS_BUFSIZE 256
#define ARGLEN_BUFSIZE 512

// Cualquier caracter presente en la sig. string será delimitador
const char token_delimiters[] = " \n";
const char special_tokens[] = {
	'|','\0'
};
size_t max_tokens = NTOKENS_BUFSIZE;
size_t max_strlen = ARGLEN_BUFSIZE;

int is_specialtok(char ch);
int is_delimiter(char ch);
void free_tokenized_array(char** tok_array);
void check_ntok_resize(char** tok_array, int used_buckets, size_t max_tokens);
void check_arglen_resize(char** tok_array, int arg_idx, int arg_len, size_t max_len);

void free_tokenized_array(char** tok_array)
{
	int idx = 0;
	while (tok_array[idx] != NULL) {
		free(tok_array[idx]);
		idx++;
	}
	free(tok_array);
}

// Funcion de tokenizacion que utiliza buffer tok_array en vez de heap, y ademas omite el uso de strtok_r
// Con ayuda de https://stackoverflow.com/questions/21918728/how-do-i-make-this-shell-to-parse-the-statement-with-quotes-around-them-in-c
/*
Inputs:
	- char* prompt: string del input completo hecho por el usuario
	- char* tok_array[]: arreglo buffer que será modificado
*/
char** tokenize_into_array(const char* prompt)
{
	char** tok_array;
	int n_tokens = 0;
	const char* prompt_ptr = prompt;
	
	// Reserva de espacio inicial
	tok_array = calloc(max_tokens, sizeof(char*));
	for (size_t i = 0; i < max_tokens; i++) tok_array[i] = malloc(max_strlen*sizeof(char));

	while (*prompt_ptr != '\0') {
		// Saltarse delimitadores (espacios y newlines fuera de quote)
		if (is_delimiter(*prompt_ptr)) {
			prompt_ptr++;
			continue;
		}
		// Si es un caracter "normal" (no pipe ni delim.) comenzar a registrar token
		if ((!is_delimiter(*prompt_ptr) && !is_specialtok(*prompt_ptr))) {
			int idx = 0;

			while ((!is_delimiter(*prompt_ptr) && !is_specialtok(*prompt_ptr)) && *prompt_ptr != '\0') {
				// Manejo de quotes. Caracteres especiales (e.j. '|','>') pierden significado dentro de un quote.
				if (*prompt_ptr == '\'' || *prompt_ptr == '"') {
					const char quote = *prompt_ptr;
					prompt_ptr++;
					
					// No se escriben quotes cerradas al token
					while (*prompt_ptr != quote) {
						if (*prompt_ptr == '\0') {
							dprintf(STDERR_FILENO, "Error: Su comando tiene una comilla no cerrada.\n");
							tok_array[0][0] = '\0';
							return tok_array;
						}
						tok_array[n_tokens][idx++] = *prompt_ptr;
						prompt_ptr++;
					}
					prompt_ptr++;
				}
				// Manejo de caracteres "regulares".
				else {
					tok_array[n_tokens][idx++] = *prompt_ptr;
					prompt_ptr++;
				}
			}
			tok_array[n_tokens++][idx] = '\0';
		}
		// pipe token
		if (*prompt_ptr == '|') {
			tok_array[n_tokens][0] = '|';
			tok_array[n_tokens++][1] = '\0';
			prompt_ptr++;
		}
 	}
	tok_array[n_tokens] = NULL;
	return tok_array;
}

int is_specialtok(char ch)
{
	if (strchr(special_tokens, ch) != NULL) return 1;
	else return 0;
}

int is_delimiter(char ch)
{
	if ((strchr(token_delimiters, ch)) != NULL) return 1;
	else return 0;
}

void _print_tokens(char** tokarray)
{
	int idx = 0;
	while (tokarray[idx] != NULL) printf("%s\n", tokarray[idx++]);
}
