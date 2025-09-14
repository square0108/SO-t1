#include <string.h>
#define STRP_QUOTE_HANDLING_OFF 0
#define STRP_QUOTE_HANDLING_ON 1
#define STRP_REMOVE_LAST_NEWLINE_OFF 0
#define STRP_REMOVE_LAST_NEWLINE_ON 1

/* !! HEAP RETURN VALUE !! PLEASE FREE() HIM.  
Tokeniza la string `str`, separando tokens segun el string delimitador `delim` y retorna un arreglo NULL-terminado de los tokens extraidos.

input: 
- String (char array terminado en \0, como todos saben)
- _Bool quote_handling: Al pasar 1, encerrará todos los pares de quotes (",') dentro de un solo token, ignorando cualquier caracter especial.
- _Bool del_last_newline: Al pasar 1, siempre eliminará el carácter '\n' del último token procesado, si es que lo hay.
Return val:
- Puntero a memoria dinámica contigua, donde cada miembro del "arreglo" es un puntero a un token (string con NULL después del '\0'). A su vez, el último token almacenado también es un puntero NULL. Los punteros char* son creados con memoria dinámica, y también lo son las strings a las que apuntan. Complejidad espacial es O(N*M), donde N es el búfer de punteros a tokens, y M es el largo del string o token más grande.
*/
char** partition_into_array(char* str, _Bool quote_handling, _Bool del_last_newline);
/*
Debe ser llamado para liberar la memoria del arreglo de tokens exitosamente
*/
void free_tokenized_array(char** tokarray);

