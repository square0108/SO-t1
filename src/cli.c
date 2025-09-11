#include <sys/wait.h>
#include "strparse.h"
#include "cli.h"

// Utiliza la system call getcwd(), que esta limitada por un buffer size.
void print_prompt()
{
	char username[] = "USERNAME"; 
	char wd_buffer[CLI_WORKDIR_BUFSIZE];
	dprintf(STDOUT_FILENO,"%s:%s $ ",username,getcwd(wd_buffer,(size_t) CLI_WORKDIR_BUFSIZE));
}

void fork_n_execp(char* program, char* args[], int fildes_dest)
{
	int fd[2];
	int pipe_status = pipe(fd);
	if (pipe_status == -1) {
		dprintf(STDERR_FILENO, "Pipe failed during command execution!\n");
		exit(-1);
	}
	int child_pid = fork();
	if (child_pid == -1) {
		dprintf(STDERR_FILENO, "Fork failed during command execution!\n");
		exit(-1);
	}

	// output of the child is sent to the parent's read end of the pipe
	if(child_pid == 0) {
		printf("pid: %i pgid: %i ppid: %i\n", getpid(), getpgid(0), getppid());
		dup2(fd[1],STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		execvp(program,args);
	}
	else {
		printf("pid: %i pgid: %i ppid: %i\n", getpid(), getpgid(0), getppid());
		close(fd[1]);
		char buf[2048]; // this should probably be #define'd
		int nbytes = sizeof(buf);
		int bytes_read;
		while ((bytes_read = read(fd[0],buf,nbytes)) > 0 ) {
			write(fildes_dest, buf, bytes_read);
		}
		close(fd[0]); // if read returned -1, then parent has finished reading
		wait(NULL);
	}
}

