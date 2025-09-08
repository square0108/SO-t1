#include <string.h>
#include <stdlib.h>
#include "strparse.h"
#define NTOK_GUESS 64
#define TOK_MAX_LEN 256

// Cualquier caracter presente en la sig. string será delimitador
const char token_delimiters[] = " ";

// TODO: terminar funcionalidad de quote parsing
// Maquina de estados para parsear quotes y doble-quotes
enum QuoteStatus {
	NO_QUOTE,
	OPEN_SQUOTE,
	OPEN_DQUOTE
};
enum QuoteStatus quote_status = NO_QUOTE;
void update_quote_status(char incoming_char);
void update_qstat_from_str(char* str);

char** partition_into_array(char* str, _Bool quote_handling, _Bool del_last_newline)
{
	int n_tokens = 0;
	int realloc_mult = 1;
	char* saveptr = NULL; // utilizado internamente por strtok_r para mantener sincronizacion en hebras	
	char** ret_array = (char**) calloc((size_t)NTOK_GUESS,sizeof(char*));

	// Extrae primer token (llamadas subsiguientes a strtok deben pasar NULL)
	char* unprocessed_token = strtok_r(str,token_delimiters,&saveptr);
	if (unprocessed_token == NULL) return NULL;
	char* first_token = (char*) calloc((size_t)TOK_MAX_LEN, sizeof(char));
	strcpy(first_token,unprocessed_token);
	ret_array[n_tokens] = first_token;
	n_tokens++;

	while ((unprocessed_token = strtok_r(NULL,token_delimiters,&saveptr)) != NULL) {
		// Resizea a ret_array si es que se llena, y reserva 1 direccion adicional para terminar el array en NULL
		if (n_tokens > (NTOK_GUESS*realloc_mult)-1) {
			realloc_mult +=1;
			ret_array = (char**) realloc(ret_array, sizeof(char**)*NTOK_GUESS*realloc_mult);	
		}
		// ** WIP ** hay que ignorar espacios y caracteres especiales dentro de todo un quote, y al final sacarle los quotes
		if (quote_handling == STRP_QUOTE_HANDLING_ON) {
			if (quote_status != NO_QUOTE) { 
				// Si una quote queda abierta desde un token, se combinarán todos los tokens subsiguientes hasta cerrar la quote.
				strcat(ret_array[n_tokens-1], unprocessed_token);
				update_qstat_from_str(unprocessed_token);
				continue; // n_tokens no cambia
			}
			update_qstat_from_str(unprocessed_token);
		}
		char* token = (char*) calloc((size_t)TOK_MAX_LEN, sizeof(char));
		char* end_of_token = strcpy(token, unprocessed_token);
		ret_array[n_tokens] = token;
		n_tokens++;
	}
	char* ptr_to_newl;
	if (STRP_REMOVE_LAST_NEWLINE_ON && (ptr_to_newl = strrchr(ret_array[n_tokens-1],'\n')) != NULL)
		*ptr_to_newl = '\0';
	ret_array[n_tokens] = NULL;
	return ret_array;
}

// The only deciding factor is whether the first quote that appeared has been closed or not
// Note that newlines MUST be included in the quotes, but can be removed if they are the last character and outside of a quote
void update_quote_status(char incoming_char)
{
	switch (incoming_char) {
		case '\'':
			if (quote_status == NO_QUOTE) quote_status = OPEN_SQUOTE;
			else if (quote_status == OPEN_SQUOTE) quote_status = NO_QUOTE;
			break;
		case '\"':
			if (quote_status == NO_QUOTE) quote_status = OPEN_DQUOTE;
			else if (quote_status == OPEN_DQUOTE) quote_status = NO_QUOTE;
			break;
		default:
			break;
	};
}

void update_qstat_from_str(char* str)
{
	int idx = 0;
	while (str[idx] != '\0') {
		char cchar = str[idx];
		if (cchar == '\'' || cchar == '\"') update_quote_status(cchar);
		idx++;
	}
}

void free_tokenized_array(char** tokarray)
{
	int idx = 0;
	while (tokarray[idx] != NULL) {
		free(tokarray[idx]);
		idx++;
	}
	free(tokarray);
}
