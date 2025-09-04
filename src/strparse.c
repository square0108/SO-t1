#include <string.h>
#include <stdlib.h>
#define NTOK_GUESS 64

// TODO: Manejar caso de quotes y doble quotes: en la shell de bash, las quotes encierran a un solo token
//			i.e. "Matias del Rio" deberia ser un unico token en vez de {"Matias ; del ; Rio"}
// Espera lo pensé un poco más y esto ni siquiera importa... Los programas son los que ven cómo procesar quotes
char** partition_into_array(char* str, char* delim)
{
	int n_tokens = 0;
	int realloc_mult = 1;
	char* saveptr = NULL; // utilizado internamente por strtok_r para mantener sincronizacion en hebras	
	char** ret_array = (char**) calloc((size_t)NTOK_GUESS,sizeof(char*));
	char* token;
	// First call to strtok passes the string to be parsed, subsequent calls must pass NULL. (wtf?)
	token = strtok_r(str,delim,&saveptr);
	if (token == NULL) return NULL;
	n_tokens +=1;
	ret_array[n_tokens-1] = token;

	while ((token = strtok_r(NULL,delim,&saveptr)) != NULL) {
		n_tokens +=1;
		// Resizea a ret_array si es que se llena, y reserva 1 direccion adicional para terminar el array en NULL
		if (n_tokens > (NTOK_GUESS*realloc_mult)-1) {
			realloc_mult +=1;
			ret_array = (char**) realloc(ret_array, sizeof(char**)*NTOK_GUESS*realloc_mult);	
		}

		ret_array[n_tokens-1] = token;
	}
	ret_array[n_tokens] = NULL;
	return ret_array;
}
