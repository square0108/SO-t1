#include <bits/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "strparse.h"
#include "cli.h"
#define FD_NOPIPE -1



// Comandos builtin de la shell */

CLICommand builtins[] = {
    {"gato", builtin_gato, 1},
    {"miprof", builtin_miprof, 1},
	{"exit", builtin_exit, 0},
    {NULL, NULL, 0}
};

int cli_global_fgpgid = CLI_FGPGID_DEFAULT;

int miprof_child_pid = -1;
int miprof_child_timeout = 0;

// Utiliza la system call getcwd(), que esta limitada por un buffer size.
void print_prompt()
{
	char username[] = "USERNAME"; 
	char wd_buffer[CLI_WORKDIR_BUFSIZE];
	dprintf(STDOUT_FILENO,"%s:%s $ ",username,getcwd(wd_buffer,(size_t) CLI_WORKDIR_BUFSIZE));
}

int errcheck_pipe(int status);

void exec_userprompt(char* tokenized_input[])
{
	int tok_idx = 0, num_cmds = 0, arg_idx = 0;
	char* cmds[CLI_MAX_CMDS][CLI_MAX_ARGS]; // x = N-prompt segment to be piped ; y = argument(s) associated to the n-ary command
	// offload all the tokenized commands into the cmds[][] array, grouping them for a pipeline
	while (tokenized_input[tok_idx] != NULL) {
		if (strcmp(tokenized_input[tok_idx], "|") == 0) { 
			cmds[num_cmds][arg_idx] = NULL; // null-terminate this command's array of arguments, '|' MUST NOT be written to args
			num_cmds++;
			arg_idx = 0;
		}
		else {
			cmds[num_cmds][arg_idx] = tokenized_input[tok_idx];
			arg_idx++;
		}
		tok_idx++;
	}
	cmds[num_cmds][arg_idx] = NULL; // null-terminate the last command in the pipeline
	num_cmds++;
	pipeline_exec(cmds, num_cmds);
}

/*
Con 'comandos' nos referimos a toda una seccion del prompt entre dos caracteres pipe ('|'), o entre el principio del prompt y un pipe, o entre un pipe y el final del prompt, o el prompt completo si no hay algun pipe.

input: 
- arreglo multidimensional cmds[][]: Cada par (i,j) tiene el siguiente significado: La posición i-ésima denota al comando i-ésimo comando a ejecutar (indexando desde 0), y los strings desde 0 a j_max son los argumentos de cada comando.
- num_cmds: la extension maxima de `i` descrita anteriormente, el numero total de comandos a ejecutar.
*/
void pipeline_exec(char *cmds[][CLI_MAX_ARGS], int num_cmds)
{
	int fd_read_prev = FD_NOPIPE;
	if (num_cmds > 1) {
		for (int i = 0; i < num_cmds; i++) {
			int fd_pipe[2];
			pipe(fd_pipe);
			int pid = fork();
			if (pid == 0) {
				// --- Hijo ---
//				printf("CHILD | i: %i ; pipes: %i %i ; fd_read_prev: %i\n",i,fd_pipe[0],fd_pipe[1],fd_read_prev);
				// Asigna a hijo al grupo "foreground" (comparten el mismo pgid que el primer comando)
				if (i == 0) {
					setpgid(0,0);
					cli_global_fgpgid = getpid();
				}
				else
					setpgid(0,cli_global_fgpgid);

				if (i != num_cmds-1) {
					// conectar stdout con pipe de escritura
					dup2(fd_pipe[1],STDOUT_FILENO);
				}
				// hijo no lee desde pipe[0], si no que desde fd_read_prev (descriptor que recibio escritura del hijo en i-1). Lee hasta alcanzar EOF en fd_read_prev.
				close(fd_pipe[1]);
				close(fd_pipe[0]);
				if (fd_read_prev != FD_NOPIPE) {
					dup2(fd_read_prev,STDIN_FILENO);
					close(fd_read_prev);
				}

                CLICommand command = handle_alias(cmds[i][0]);
                int rc = exec(command, cmds[i]);
                if (rc == -1) {
                    _exit(127);
                } else if (rc == 0) {
                    _exit(0);
                }
			}
			// --- Padre ---
			else if (pid > 0) {
				// Asigna a hijos al grupo "foreground" (redundante para evitar condicion de carrera)
				if (i == 0) {
					setpgid(pid,pid);
					cli_global_fgpgid = pid;
					if (tcsetpgrp(STDIN_FILENO,pid) < 0) {
						fprintf(stderr,"Error: No se pudo setear foreground process group en pipeline.\n");
						exit(-1);
					}
				}
				else
					setpgid(pid,cli_global_fgpgid);

				// Cierra pipe del proceso hijo anterior, pues la copia de este descriptor ya la tiene el hijo actual
				if (fd_read_prev != FD_NOPIPE) {
					close(fd_read_prev);
				}
//				printf("PARENT | i: %i ; pipes: %i %i ; fd_read_prev (Closed): %i ",i,fd_pipe[0],fd_pipe[1],fd_read_prev);
				// fd_read_prev es actualizado y copiado a la memoria del hijo creado en el siguiente ciclo. El hijo actual escribe a esta pipe, así su output se transfiere al siguiente hijo.
				if (i != num_cmds-1) {
					fd_read_prev = fd_pipe[0];
				}
				else close(fd_pipe[0]);
//				printf("fd_read_prev (New): %i\n", fd_read_prev);
				close(fd_pipe[1]);
			}
		}
	}
	// Ejecucion sin pipes
	else {
        int pid = -1;
        CLICommand command = handle_alias(cmds[0][0]);
        if (command.must_fork) {
		    pid = fork();
        }

		if (pid == 0) {
            // Hijo
			setpgid(0,0);
			cli_global_fgpgid = getpid();
            int rc = exec(command, cmds[0]);

            // Esto solo corre si se trataba de un comando builtin, o si es que no se encontró
            // el comando.
            if (rc == -1) {
                _exit(127);
            } else if (rc == 0) {
                _exit(0);
            }
        }
		else if (pid > 0) {
            // Padre
			setpgid(pid,pid);
			cli_global_fgpgid = pid;
			if (tcsetpgrp(STDIN_FILENO,pid) < 0) {
				fprintf(stderr,"Error: No se pudo setear foreground process group en ejec sin pipes.\n");
				exit(-1);
			}
		} else {
            // caso no fork, no hacer exit, estamos en el padre!
            int rc = exec(command, cmds[0]);
        }
	}		
    
	for (int i = 0; i < num_cmds; i++) wait(NULL);
	// Entrega control del shell de vuelta a proceso shell
	if (tcsetpgrp(STDIN_FILENO,getpid()) < 0) {
		fprintf(stderr,"Error: No se pudo devolver estado foreground al shell.\n");
		exit(-1);
	}
}

int errcheck_pipe(int status)
{
	if (status >= 0) return 0;
	else {
		dprintf(STDERR_FILENO, "Pipe creation failed!\n");
		exit(-1);
	}
}

int make_proc_fgp_leader(int pid)
{
	setpgid(pid,pid);
	cli_global_fgpgid = pid;
	return 0;
}


CLICommand handle_alias(char* alias) {
    int k = 0;
    while (builtins[k].alias != NULL) {
        if (strcmp(builtins[k].alias, alias) == 0) {
            // Quizás handlear esto algún día
            // int rc = (*builtins[k].func)(argc, argv);
            // is_builtin = 1;
            return builtins[k];
        }
        k += 1;
    }

    return (CLICommand){alias, NULL, 1};
}

int exec(CLICommand cmd, char** argv) {
    // En este caso, se trata de un programa, usar execvp
    if (cmd.func == NULL) {
        execvp(cmd.alias, argv);
        fprintf(stderr, "Error. Command not found: %s\n", cmd.alias);
        return -1;
    } 

    // En caso contrario, es un builtin, contamos argumentos y usamos la función correspondiente.
    int argc = 0; 
    while (argv[argc] != NULL) { argc++; }
    int rc = (*cmd.func)(argc, argv);
    //printf("command: %s, rc: %d\n", cmd.alias, rc);
    return rc;
}


int builtin_gato(int argc, char** argv) {
    if (argc > 1) {
        printf("gato takes no arguments\n");
        return -1;
    }

    char* gatosh =
    "         _._     _,-'\"\"`-._\n"
    "        (,-.`._,'(       |\\`-/|\n"
    "            `-.-' \\ )-`( , o o)\n"
    "                  `-    \\`_`\"'-\n"
    "  ________        __          _________.__     \n"
    " /  _____/_____ _/  |_  ____ /   _____/|  |__  \n"
    "/   \\  ___\\__  \\\\   __\\/  _ \\\\_____  \\ |  |  \\ \n"
    "\\    \\_\\  \\/ __ \\|  | (  <_> )        \\|   Y  \\\n"
    " \\______  (____  /__|  \\____/_______  /|___|  /\n"
    "        \\/     \\/                   \\/      \\/ \n";
     
    printf("%s\n", gatosh);
    return 0;
}

int miprof_ejec(int argc, char** argv, int max_time, char* file_out) {
    struct timespec ts_start, ts_end;
    int time_start_rc;
    // miprof siempre forkea
    int pid = fork();
    if (pid == 0) {
        // Hijo

        setpgid(0,cli_global_fgpgid);

        CLICommand command = handle_alias(argv[0]);
        int rc = exec(command, argv);
        // A este punto o estamos en un builtin, o no se pudo ejecutar el comando.
        if (rc == -1) {
            _exit(127);
        } else if (rc == 0) {
            _exit(0);
        }
    }
    else if (pid > 0) {
		setpgid(0,cli_global_fgpgid);
        miprof_child_pid = pid;
        time_start_rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);

        int fd, saved_stdout;
        if (file_out != NULL) {
            saved_stdout = dup(STDOUT_FILENO);
            fd = open(file_out, O_WRONLY | O_CREAT | O_APPEND, 0644); 

            if (file_out != NULL) {
                if (fd == -1) {
                    perror("Error opening file");
                    return -1;
                }

                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("Error redirecting stdout");
                    close(fd);
                    return -1;
                }

                close(fd);
                setvbuf(stdout, NULL, _IONBF, 0);
            }
        }

        if (max_time > 0) {
            struct sigaction sa;
            sa.sa_handler = alarm_handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            if (sigaction(SIGALRM, &sa, NULL) == -1) {
                perror("sigaction");
                return 0;
            }
            alarm(max_time);
        }
        
        int status;
        wait(&status);
        // Tener cuidado con el status code al matar el proces.
        if (WEXITSTATUS(status) && !miprof_child_timeout) {
            fprintf(stderr, "miprof could not execute the given command\n");
            return -1;
        }

        int time_end_rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);

        struct rusage stats;
        int rusage_rc = getrusage(RUSAGE_CHILDREN, &stats);


        char report_buffer[1000];
        char* report_format = 
                "#####################################################\n"
                "################# MIPROF USAGE REPORT ###############\n"
                "#####################################################\n"
                "command                   : %s\n"
                "timed out                 : %s\n"
                "maximum resident set size : %ld [kilobytes]\n"
                "user CPU time             : %ld [microseconds]\n"
                "system CPU time           : %ld [microseconds]\n"
                "real time                 : %ld [%s]\n";


        long maxrss, utime, stime, rtime, rtime_n;
        if (!rusage_rc) {
            maxrss = stats.ru_maxrss;
            utime = stats.ru_utime.tv_usec;
            stime = stats.ru_stime.tv_usec;
        }
        if (!time_start_rc && !time_end_rc) {
            time_t start_sec = ts_start.tv_sec;
            time_t end_sec = ts_end.tv_sec;

            time_t start_nsec = ts_start.tv_nsec;
            time_t end_nsec = ts_end.tv_nsec;

            rtime = end_sec - start_sec;
            rtime_n = end_nsec - start_nsec;
        }

        sprintf(report_buffer, report_format, argv[0], miprof_child_timeout ? "true" : "false", maxrss, utime, stime, rtime ? rtime : rtime_n, rtime ? "seconds" : "nanoseconds");
        printf("\n%s\n", report_buffer);
        fflush(stdout);

        if (file_out != NULL) {
            dup2(saved_stdout, STDOUT_FILENO);
            printf("miprof report saved to %s\n", file_out);
        }
    }

    return 0;
}


int builtin_miprof(int argc, char** argv) {

    char* help_message =
        "Uso:\n"
        "  miprof ejec <comando> <args>...\n"
        "  miprof ejecutar <maxtiempo> <comando> <args>...\n"
        "  miprof ejecsave <archivo> <comando> <args>...\n"
        "  miprof -h | --help\n\n"
        "Opciones:\n"
        "  -h --help            Muestra este texto.\n"
        "  <maxtiempo>          Tiempo de ejecución máximo en segundos para el\n"
        "                       profiling.\n"
        "  <archivo>            Archivo output en el que guardar los\n"
        "                       resultados del profiling.\n"
        "  <comando> <args>...  El comando y todos sus argumentos se ejecutan\n"
        "                       bajo profiling.";

    if (argc < 2) {
        fprintf(stderr, "Missing arguments.\n%s\n", help_message);
        return -1;
    }

    if (strcmp("--help", argv[1]) == 0 || strcmp("-h", argv[1]) == 0) {
        printf("MiProf.\n\n%s\n", help_message);
        
    } else if (strcmp("ejec", argv[1]) == 0) {
        if (argc < 3) {
            fprintf(stderr, "Missing arguments.\n%s\n", help_message);
            return -1;
        }
        miprof_ejec(argc - 2, argv + 2, -1, NULL);
    } else if (strcmp("ejecutar", argv[1]) == 0) {
        if (argc < 4) {
            fprintf(stderr, "Missing arguments.\n%s\n", help_message);
            return -1;
        }

        char* endptr;
        errno = 0;
        long max_time = strtol(argv[2], &endptr, 10);
        if (errno != 0) {
            perror("strtol");
            return -1;
        }
        if (*endptr != '\0') {
            fprintf(stderr, "Invalid argument maxtiempo.\n%s\n", help_message);
            return -1;
        }
        miprof_ejec(argc - 3, argv + 3, (int) max_time, NULL);
    } else if (strcmp("ejecsave", argv[1]) == 0) {
        if (argc < 4) {
            fprintf(stderr, "Missing arguments.\n%s\n", help_message);
            return -1;
        }
        miprof_ejec(argc - 3, argv + 3, -1, argv[2]);
    } else {
        fprintf(stderr, "Unknown argument.\n%s\n", help_message);
        return -1;
    }
    return 0;
}

int builtin_exit(int argc, char** argv)
{
	if (argc > 1) {
		dprintf(STDERR_FILENO, "exit: too many arguments\n");
		return -1;
	}
	else {
		//kill(getppid(), SIGKILL);
		//kill(getpgid(0), SIGKILL);
        _exit(0);
		return 0; // shouldnt return on success
	}
}

void alarm_handler(int signum) {
    if (miprof_child_pid > 0) {
        kill(miprof_child_pid, SIGKILL);
        miprof_child_timeout = 1;
    }
}
