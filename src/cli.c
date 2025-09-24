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
CLIBuiltinCommand builtins[] = {
    {"gato", builtin_gato},
    {"miprof", builtin_miprof},
    {NULL, NULL}
};

// Esta variable global es reseteada cada vez que la shell entra a 'awaiting_input', pero es responsabilidad de la funcion llamada setearlo correctamente al process group que la shell ejecuta.
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

int make_proc_fgp_leader(int pid);

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
				if (i == 0) 
					make_proc_fgp_leader(0);
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

                handle_alias_exec(cmds[i][0], cmds[i]);
                _exit(127);
			}
			else if (pid > 0) {
				// --- Padre ---
				// Asigna a hijos al grupo "foreground" (redundante para evitar condicion de carrera)
				if (i == 0)
					make_proc_fgp_leader(pid);
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
	else {
		int pid = fork();
		if (pid == 0) {
			setpgid(0,0);
			cli_global_fgpgid = getpid();

            handle_alias_exec(cmds[0][0], cmds[0]);
            _exit(127);
			//execvp(cmds[0][0],cmds[0]);
			//printf("Error. Command not found: %s\n", cmds[0][0]);
		}
		else if (pid > 0) {
			setpgid(pid,pid);
			cli_global_fgpgid = pid;
		}
	}		

	for (int i = 0; i < num_cmds; i++) wait(NULL);
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


void handle_alias_exec(char* alias, char* argv[]) {
    // contar argc
    int argc = 0; 
    while (argv[argc] != NULL) { argc++; }

    int k = 0;
    int is_builtin = 0;
    while (builtins[k].alias != NULL) {
        if (strcmp(builtins[k].alias, alias) == 0) {
            int rc = (*builtins[k].func)(argc, argv);
            is_builtin = 1;
            break;
        }
        k += 1;
    }

    if (!is_builtin) {
        execvp(alias, argv);
        printf("Error. Command not found: %s\n", alias);
    }
	_exit(127);
}


int builtin_gato(int argc, char** argv) {
    if (argc > 1) {
        printf("gato takes no arguments\n");
        return -1;
    }
    
    char* gato = "    /\\_____/\\\n"
                 "   /  o   o  \\\n"
                 "  ( ==  ^  == )\n"
                 "   )         (\n"
                 "  (           )\n"
                 " ( (  )   (  ) )\n"
                 "(__(__)___(__)__)\n";


    printf("%s", gato);
    return 1;
}

int miprof_ejec(char** argv, int max_time, char* file_out) {

    int fd;
    if (file_out != NULL) {
        fd = open(file_out, O_WRONLY | O_CREAT | O_APPEND, 0644); 

        if (file_out != NULL) {
            if (fd == -1) {
                perror("Error opening file");
                _exit(1);
            }

            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("Error redirecting stdout");
                close(fd);
                _exit(1);
            }

            close(fd);
            setvbuf(stdout, NULL, _IONBF, 0);
        }
    }


    struct timespec ts_start, ts_end;
    int time_start_rc;
    int pid = fork();
    if (pid == 0) {
        // Hijo

        setpgid(0,0);
        cli_global_fgpgid = getpid();

        handle_alias_exec(argv[0], argv);
        _exit(127);
    }
    else if (pid > 0) {
        // Padre
        miprof_child_pid = pid;

        time_start_rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
        if (max_time > 0) {
            struct sigaction sa;
            sa.sa_handler = alarm_handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            if (sigaction(SIGALRM, &sa, NULL) == -1) {
                perror("sigaction");
                return 1;
            }

            alarm(max_time);

        }

        while (wait(NULL) == -1) {
            if (errno == EINTR) continue;
            else break;
        }
        int time_end_rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);

        struct rusage stats;
        int rusage_rc = getrusage(RUSAGE_CHILDREN, &stats);


        char report_buffer[1000];
        char* report_format = 
                "######################################################\n"
                "#################### MIPROF REPORT ###################\n"
                "######################################################\n\n"
                "timed out                 : %d\n"
                "maximum resident set size : %ld [kilobytes]\n"
                "user CPU time             : %ld [nanoseconds]\n"
                "system CPU time           : %ld [microseconds]\n"
                "real time                 : %ld [seconds]\n";


        long maxrss, utime, stime, rtime;
        if (!rusage_rc) {
            maxrss = stats.ru_maxrss;
            utime = stats.ru_utime.tv_usec;
            stime = stats.ru_stime.tv_usec;
        }
        if (!time_start_rc && !time_end_rc) {
            time_t start_nsec = ts_start.tv_nsec;
            time_t end_nsec = ts_end.tv_nsec;

            time_t start_sec = ts_start.tv_sec;
            time_t end_sec = ts_end.tv_sec;

            rtime = end_sec - start_sec;
        }

        sprintf(report_buffer, report_format, miprof_child_timeout, maxrss, utime, stime, rtime);
        printf("\n%s\n", report_buffer);
        fflush(stdout);
    }

    return 1;
}


int builtin_miprof(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: \n");
        return 1;
    } else if (strcmp("ejec", argv[1]) == 0) {
        miprof_ejec(argv + 2, -1, NULL);
    } else if (strcmp("ejecutar", argv[1]) == 0) {
        if (argc < 4) {
            printf("Usage: \n");
            return 1;
        }

        char* endptr;
        errno = 0;
        long max_time = strtol(argv[2], &endptr, 10);
        if (errno != 0) {
            perror("strtol");
            return 1;
        }
        if (*endptr != '\0') {
            printf("Usage: \n");
            return 1;
        }
        miprof_ejec(argv + 3, (int) max_time, NULL);
    } else if (strcmp("ejecsave", argv[1]) == 0) {
        if (argc < 4) {
            printf("Usage: \n");
            return 1;
        }

        miprof_ejec(argv + 3, -1, argv[2]);
    } else {
        printf("Usage: \n");
        return 1;
    }


    return 1;
}

void alarm_handler(int signum) {
    if (miprof_child_pid > 0) {
        kill(miprof_child_pid, SIGKILL);
        miprof_child_timeout = 1;
    }
}
