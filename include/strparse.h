#include <string.h>
#define NTOK_GUESS 64
#define STRP_GROUP_QUOTES 0
#define STRP_IGNORE_QUOTES 1

/* !! HEAP RETURN VALUE !! PLEASE FREE() HIM.  
Tokeniza la string `str`, separando tokens segun el string delimitador `delim` y retorna un arreglo NULL-terminado de los tokens extraidos.
*/
char** partition_into_array(char* str, char* delim);

