#include <string.h>
#define STRP_QUOTE_HANDLING_OFF 0
#define STRP_QUOTE_HANDLING_ON 1
#define STRP_REMOVE_LAST_NEWLINE_OFF 0
#define STRP_REMOVE_LAST_NEWLINE_ON 1

/* !! HEAP RETURN VALUE !! PLEASE FREE() HIM.  
Tokeniza la string `str`, separando tokens segun el string delimitador `delim` y retorna un arreglo NULL-terminado de los tokens extraidos.
*/
char** partition_into_array(char* str, _Bool quote_handling, _Bool del_last_newline);
void free_tokenized_array(char** tokarray);

