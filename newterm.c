
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
	pid_t a, w_pid;
	int status;
	a = fork();
	if( a == 0 ){
		//this is the child process
		printf( "Running terminal...\n");
		char** newterm = (char *[]){"gnome-terminal", "-e", "./ush", NULL };
		execvp( "gnome-terminal", newterm );
	}else{
		//this is the parent process
		//printf("hi\n");
		do {
		    w_pid = waitpid(a, &status, WUNTRACED);
		}while ( !WIFEXITED(status) && !WIFSIGNALED(status) );
	}
	return 0;
}