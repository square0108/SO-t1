```
         _._     _,-'""`-._
        (,-.`._,'(       |\`-/|
            `-.-' \ )-`( , o o)
                  `-    \`_`"'-
  ________        __          _________.__     
 /  _____/_____ _/  |_  ____ /   _____/|  |__  
/   \  ___\__  \\   __\/  _ \\_____  \ |  |  \ 
\    \_\  \/ __ \|  | (  <_> )        \|   Y  \
 \______  (____  /__|  \____/_______  /|___|  /
``` 
Intérprete de comandos Linux básico, programado en C. 
Desarrollado para la Tarea 1 del curso Sistemas Operativos, Universidad de Concepción.

# Compilación y ejecución
```bash
make clean gatosh
make all
./gatosh
```
# Comandos integrados
GatoSh cuenta con tres comandos integrados:
* exit: Como su nombre lo indica, termina la shell.
* gato: Despliega el logo de GatoSh en la salida estandar.
* miprof: Comando para realizar profiling de otros comandos/programas:
    ```
    Uso:
    miprof ejec <comando> <args>...
    miprof ejecutar <maxtiempo> <comando> <args>...
    miprof ejecsave <archivo> <comando> <args>...
    miprof -h | --help
    
    Opciones:
    -h --help            Muestra este texto.
    <maxtiempo>          Tiempo de ejecución máximo en segundos para el
                         profiling.
    <archivo>            Archivo output en el que guardar los
                         resultados del profiling.
    <comando> <args>...  El comando y todos sus argumentos se ejecutan
                         bajo profiling.
    ```
