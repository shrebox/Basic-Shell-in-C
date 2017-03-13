
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define DELIM " \n"
#define PIPEDELIM "|\n"
#define OUTPUTDELIM ">\n"
#define INPUTDELIM "<\n"
#define BOTHDELIM "<>\n"

char** history;
int hist_index = 0;

char* trim(char *s) {
  while (isspace((unsigned char) *s)) s++;
  if (*s) {
    char *p = s;
    while (*p) p++;
    while (isspace((unsigned char) *(--p)));
    p[1] = '\0';
  }
  return s;
}

// builtin functions

void addtohistory( char* s ){
	history[hist_index] = (char*)malloc( 50 * sizeof(char));
	strcpy( history[hist_index ] , s );
	//printf("HELLOOOO\n");
	hist_index++; 
}

int myhistory(){
	int i;
	for( i = 0; i < hist_index - 1; i++ ){
		printf("%d %s", i, history[i]);
	}
	return 1;
}

int myclear(){
	printf("\033[2J"); //will only work with unix terminals
	return 1;
}

int myhelp(){
	printf("Welcome to my shell!\n");
	printf("These are the builtin commands it can execute:\n");
	printf("cd\nhistory\nclear\n");
	printf("\nIt can also handle  piping, redirecting stdin and stdout (using > and <), \nkill, Cntrl+C without exiting, erroneous commands, return (enter), \nand help which will list all the built-in commands implemented by my shell.\n");
	printf("\nType quit to exit the shell anytime!\n");
	return 1;
}

int mycd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "Missing expected argument to \"cd\"\n");
  } 
  else {
    if (chdir(args[1]) != 0) {
      perror("Some unknown error");
    }
  }
  return 1;
}

//launcher

int runcmd( char **args ){
	pid_t pid, w_pid;
	int status;

	pid = fork();
	if( pid == 0 ){
		//this is the child process
		//printf( "hello\n");
		if( execvp( args[0], args ) == -1 ){
			perror( "Unknown command!");
		}
		exit( EXIT_FAILURE );
	}else if( pid < 0 ){
		fprintf(stderr, "Could not fork\n");
	}
	else{
		//this is the parent process
		//printf("hi\n");
		do {
		      w_pid = waitpid(pid, &status, WUNTRACED);
		}while ( !WIFEXITED(status) && !WIFSIGNALED(status) );
	}
}

char** splitstring( char* s, char* delim ){
	char **s_parsed = ( char**)malloc( sizeof( char* ) * 50 );
	char *temp;
	temp = strtok( s, delim ); 
	int i = 0;
	while( temp!= NULL)
	{
		s_parsed[i] = temp;
		i++;
		//printf( "%s\n", s_parsed[i]);
		temp = strtok( NULL, delim );
	}
	s_parsed[i] = NULL;
	return s_parsed;
}

int singlecmd( char* s ){
	//char **s_parsed = ( char**)malloc( sizeof( char* ) * 50 );
	char ** s_parsed = splitstring( s, DELIM );
	//printf( "%s\n", s );
	//parse command - exit if quit
	if( !strcmp( s_parsed[0], "quit")){
		exit(EXIT_SUCCESS);
	} 
	else if( !strcmp( s_parsed[0], "help")) {
		int t1 = myhelp();
	}
	else if( !strcmp( s_parsed[0], "cd")) {
		int t2 = mycd( s_parsed );
	}
	else if( !strcmp( s_parsed[0], "clear")) {
		int t3 = myclear();
	}
	else if( !strcmp( s_parsed[0], "history")) {
		int t4 = myhistory();
	}
	else{
		runcmd( s_parsed );
	}
}

void pipehandler( char* s, int num_cmds ){
	int fd[100][2],cid1,length,status;
	char* command[100];
	int pipe_count = num_cmds - 1;
	int i, j;

	/*
	fd[][2] is array of file descriptors. fd[i][0] is input 
	of i+1 command and fd[i][1] is output of ith command. 
	So output of i goes to input of i+1 using fd[i] pipe.
	*/

	char** cmd = splitstring(s, PIPEDELIM);  //split the strings by "\n" and "|""
	if(pipe_count)
	    {
	        for(i = 0;i < pipe_count;i++){
	            pipe(fd[i]);  // Creates a pipe with file descriptors for each array element
	        }

	        for(i = 0;i <= pipe_count;i++){
	            cid1 = fork();  //fork a new process
	            if(!cid1){
	            	if(i!=0){  //parent process
	                    dup2(fd[i-1][0],0);
	                }
	                //child process
	                if(i!=pipe_count){  //if this is not the last process in the pipe
	                    dup2(fd[i][1],1);
	                }

	                for(j = 0;j < pipe_count;j++){   
	                        close(fd[j][0]);
	                        close(fd[j][1]);
	                }
	                char** present_cmd = splitstring( cmd[i], DELIM );  //if the present cmd is also composite, split it 
	                execvp( present_cmd[0], present_cmd );
	                exit(0);
	            }


	        }
	        for(i = 0;i < pipe_count;i++)
	        {
	            close(fd[i][0]);
	            close(fd[i][1]);
	        }
	        waitpid(cid1,&status,0);
	    }

}

void signalHandler(){
	//To handle ctrl + c
	signal(SIGINT, signalHandler);
	//printf("\n Cannot be terminated using Ctrl+C :D \n");
	fflush(stdout);
}

void redirecthandler( char* s, int redirection ){
	//
	int fd;
	pid_t pid;
	if( redirection == 1 ){
		char **s_parsed = splitstring( s, OUTPUTDELIM );
		//printf( "Input cmd : %s Output file : %s\n", s_parsed[0], s_parsed[1]);
		
		if( s_parsed[1] == NULL ) {
			printf("Not enough output arguments!\n");
			return;
		}
		char* outputfile = trim(s_parsed[1]);
		//printf("%s\n", outputfile );
		//printf("HELEOEEOELEOELOL\n");
		char** actualcmd = splitstring( s_parsed[0], DELIM );
		//printf( "%s %s\n", actualcmd[0], actualcmd[1]);
		pid = fork();
		if( pid == 0 ) //child
		{
			fd = open( outputfile, O_CREAT | O_TRUNC | O_WRONLY , 0777 );
			// S_IRUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP| S_IROTH | S_IWOTH| S_IXOTH ); 
			dup2( fd, STDOUT_FILENO );
			close( fd );
			execvp( actualcmd[0], actualcmd );
		}
		else{
			waitpid(pid,NULL,0);
			//close( fd );
		}
	}
	else if( redirection == 2 ){
		char **s_parsed = splitstring( s, INPUTDELIM );
		//printf( "Input cmd : %s Output file : %s\n", s_parsed[0], s_parsed[1]);
		if( s_parsed[1] == NULL ) {
			printf("Not enough input arguments!\n");
			return;
		}
		char* inputfile = trim(s_parsed[1]);
		//printf("%s\n", inputfile );
		//printf("HELEOEEOELEOELOL\n");
		char** actualcmd = splitstring( s_parsed[0], DELIM );
		//printf( "%s %s\n", actualcmd[0], actualcmd[1]);
		pid = fork();
		if( pid == 0 ) //child
		{
			fd = open( inputfile, O_RDONLY , 0600 );
			// S_IRUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP| S_IROTH | S_IWOTH| S_IXOTH ); 
			dup2( fd, STDIN_FILENO );
			close( fd );
			execvp( actualcmd[0], actualcmd );
		}
		else{
			waitpid(pid,NULL,0);
			//close( fd );
		}
	}
	else{  // both input and output redirection
		int fd1, fd2;
		char **s_parsed = splitstring( s, BOTHDELIM );
		if( s_parsed[1] == NULL ) {
			printf("Not enough output arguments!\n");
			return;
		}
		char* inputfile = trim(s_parsed[1]);
		char* outputfile = trim(s_parsed[2]);
		//printf( "CMD: %s Input cmd : %s Output file : %s\n", s_parsed[0], inputfile, outputfile );
		//printf("HELEOEEOELEOELOL\n");
		char** actualcmd = splitstring( s_parsed[0], DELIM );
		//printf( "%s %s\n", actualcmd[0], actualcmd[1]);
		pid = fork();
		if( pid == 0 ) //child
		{
			fd1 = open( inputfile, O_RDONLY , 0600 );
			dup2( fd1, STDIN_FILENO );
			close( fd1 );

			fd2 = open( outputfile, O_CREAT | O_TRUNC | O_WRONLY , 0777 );
			// S_IRUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP| S_IROTH | S_IWOTH| S_IXOTH ); 
			dup2( fd2, STDOUT_FILENO );
			close( fd2 );
			execvp( actualcmd[0], actualcmd );
		}
		else{
			waitpid(pid,NULL,0);
			//close( fd );
		}
	}
}

void start(){
	while(1){
		//char* s = (char*)malloc( sizeof( char) * 1000 );
		// print prompt string
		printf("-->");
		//read command from stdin
		//scanf( "%[^\n]%*c", s );
		char *s = NULL;
		ssize_t bufsize = 0; // have getline allocate a buffer for us
		getline( &s, &bufsize, stdin );
		if( s[0] != '\n' ){
			//printf("%s\n", s );
			//printf("Hello\n");
			addtohistory( s );
			int num_cmds = 0, l = 0;
			int redirection = 0; //no '>' or '<' character
			//redirection '>' 1
			//redirection '<' 2

			while ( s[l] != '\n'){
				if ( s[l] == '|' ){
					num_cmds++;
				}
				if( s[l] == '>' && redirection == 0) // only output redirection
					redirection = 1;
				else if( s[l] == '>' && redirection == 2 )//both
					redirection = 3;
				if( s[l] == '<' ) //only input redirection
					redirection = 2;
				
				l++;
			}
			num_cmds++;  // 1 more than the num of pipes

			//printf("%d\n", redirection );
			if( num_cmds == 1 && redirection == 0 ){
				singlecmd( s );
			}
			else{
				if( redirection == 0 )
					pipehandler( s, num_cmds );
				else
					redirecthandler( s, redirection );
			}
		}	
	}
}


int main()
{
	history = ( char**)malloc( sizeof( char* ) * 100 );
	signal(SIGINT, signalHandler);
	start();
	return EXIT_SUCCESS;
}