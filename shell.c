/**********************************************************
*
*	Course: Operating Systems (ID2200)
*	Authors: Bassam Alfarhan
*					 Rodrigo Roa Rodríguez
*
***********************************************************
* This is an implementation of a mini shell in c that 
* demonstates the usage of system calls.
* It implements custom exit, cd, and checkEnv commands,
* and supports command lines of up to 80 characters.
*
* Limitations: ********************************************
* The command lines are tokenized by spaces which 
* disallows the use of string literals, for instance:
* [command "a b c"]
* will be tokenized as: 
* [command] ["a] [b] [c"]
* 
* The background indicator '&' at the end of a 
* command line have to be space separated from the 
* previous argument, otherwise not detected.
**********************************************************/

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <sys/time.h>

/* If SIGDET is defined and set to non-zero value, 
	 detection of terminated bg processes will be by sinals */
#define SIGDET 0

/* Max number of characters to read in as a command
   as specified in the task. */
#define MAX_CHARS 80

char * checkEnv_path;
char * stdout_buff;
char spawn_msg;
int num_args;
pid_t child_pid;

void sigint_handler(int);

/*
Here follows three functions that handles the timer we used.
the code inside the functions is inspired by the following discussion:
http://stackoverflow.com/questions/2150291/how-do-i-measure-a-time-interval-in-c
*/
struct timeval t1, t2;
void start_timer(void){
    gettimeofday(&t1, NULL);
}
void stop_timer(void){
    gettimeofday(&t2, NULL);
}
/* compute and return the elapsed time in millisec */
double get_time(void){
    double elapsedTime;	

    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;

    return elapsedTime;
}

/* A forker function that takes a vector of arguments
 * and tries to fork a child to execute the vector of
 * arguments in, assuming the program to be run is the 
 * first argument in the argument vector. 
 */
void forker(char **argv)
{
	int status;
	char bg = 0; /* A flag to indicate whether it is a bg or fg process */
	
	/* Check whether the last argument is a '&' 
	   if that is the case, do not send it to the 
	   child process. */
	if(0 == strcmp(argv[num_args-1],"&"))
	{
		bg = 1; /* it is a background process */
		argv[num_args-1] = NULL; /* reset the last argument */
	}else{
		start_timer(); /* otherwise if foreground process, start timer */
	}

	/* Fork for the command to be executed in.*/
	child_pid = fork();
	if ( 0 == child_pid)
	{
		/* Only run in child-process */
		execvp(argv[0], argv);
		printf("%s: command not found\n", argv[0]);
	}

	/* Check whether the process was to be run in bg
	   if that is not the case, wait until terminated
	*/
	if(!bg) { 
		/* before waiting, set sigint handler because we 
		want to catch CTRL+C for fg processes*/
		signal(SIGINT, sigint_handler); 
		waitpid(child_pid, &status, 0); 
		stop_timer();
		printf("Foreground process [%d] terminated with return value: %d\nElapsed time: %.2f msec.\n", child_pid, status, get_time());
	}else{
		spawn_msg = 1;
	}
}

/* Execute function which the command is run through
 * in order to detect integrated commands like:
 * cd, exit and checkEnv
 * if no integrated commands are detected, the command
 * is forwarded to the forker function.
*/
int execute(char **argv){
	if(0 == strcmp(argv[0], "exit")){ /* If exit, return 1. */
		return 1;
	}else if(0 == strcmp(argv[0], "checkEnv")){ 
	/* if checkEnv, run it in a child process, similar to what happens in forker() */
		int status;
		char bg = 0;

		if(0 == strcmp(argv[num_args-1],"&"))
		{
			bg = 1;
		}else{
			start_timer();
		}

		child_pid = fork();
		if ( 0 == child_pid ){
			execl(checkEnv_path, "checkEnv", argv[1], NULL);
		}

		/* Check whether the checkEnv was to be run in bg
		   if that is not the case, wait until terminated */
		if(!bg) { 
			signal(SIGINT, sigint_handler);
			waitpid(child_pid, &status, 0);
			stop_timer();
			printf("Foreground process [%d] terminated with return value: %d\nElapsed time: %.2f ms.\n", child_pid, status, get_time());
		}else{
			spawn_msg = 1;
		}
	}else if(0 == strcmp(argv[0], "cd")){ /* CD */
		/* if tilda, go to home, home in our shell is the root of the shell. */
		if(0 == strcmp(argv[1], "~")){
			if(0 != chdir(getenv("PWD"))){
				printf("shell: dude, you're in serious trouble, working directory was set to: %s\n", getenv("PWD"));
			}
		}else{ /* otherwise, construct new path by appending the directory to the current_dir_name */
			char * new_path = malloc(strlen(get_current_dir_name())+strlen(argv[1])+1);
			strcpy (new_path, get_current_dir_name());
			strcpy (new_path+strlen(get_current_dir_name()), "/");
			strcpy (new_path+strlen(get_current_dir_name())+1, argv[1]);

			/* and change the current working directory */
			if(0 != chdir(new_path)){
				printf("shell: cd: %s: No such file or directory\n", argv[1]);
			}

			free(new_path); /* ofcourse, we should free the memory. */
		}
	}else{ /* if not recognized internal command, call forker */
		forker(argv);
	}
	return 0;
}

/*
 * sigint_handler: to be executed by parent on bg children when user types CTRL-C.
 */
void sigint_handler(int dummy)
{
	  printf("\nPARENT [%d] got SIGINT. Kill child [%d]\n", getpid(), child_pid);
	  kill(child_pid, SIGKILL);                   /* Kill child process */
}

#if SIGDET
void sigdet_procedure(int dummy){
	int status;
	pid_t pid = wait(&status);
	if(pid>0) printf("Background process [%d] terminated with return value: %d\n", pid, status);
	/* else, it means the signal is already handled. */
}
#else
void polling_procedure(void){
	int status;
	pid_t pid;
	do{ /* As long as there is children that is unwaited for: */
		pid = waitpid(0, &status, WNOHANG); /* check if there's any child which is unwaited for, -1 means theres none. */
		if(pid>0) printf("Background process [%d] terminated with return value: %d\n", pid, status);
	}while(pid>0);
}
#endif

int main(int argc, char **argv)
{
	/* allocate memory for input, arguments, buffer for stdout */
	char input [MAX_CHARS];
	char ** args;
	spawn_msg = 0;
	stdout_buff = malloc(BUFSIZ);

	/* Set the buffer for stdout to the optimal size given by BUFSIZ */
	setvbuf (stdout, stdout_buff, _IOFBF, BUFSIZ); 

	/* Set path to checkEnv */
	checkEnv_path = malloc(strlen(getenv("PWD"))+strlen("/checkEnv"));
	strcpy (checkEnv_path, getenv("PWD"));
	strcpy (checkEnv_path+strlen(getenv("PWD")), "/checkEnv");
	
	args = malloc((sizeof(char *) * (MAX_CHARS/2 + 1)));

	/* if sigdet macro is set, use termination detection by signals */
	#if SIGDET
	signal(SIGCHLD, sigdet_procedure);
	#endif

	while(1){
		/* initially, we want to ignore SIGINT, and only handle it for bg processes*/
		signal(SIGINT, SIG_IGN);
		
		/* if we have spawned any bg processes, inform user about them. */
		if(spawn_msg){
			printf("Spawned backgrond process pid: %d\n", child_pid);
			spawn_msg = 0;
		}

		/* if sigdet macro is not defined or set to 0, use termination detection by polling */
		#if 0==SIGDET
		polling_procedure();
		#endif

		/* Print current path and flush it (along with everything buffered earlier). */
		printf("%s# ", get_current_dir_name());
		fflush(stdout);

		/* this part should be self explanatory... */
		num_args = 0;
		if(fgets ( input, MAX_CHARS, stdin) != NULL){
			args[0] = strtok (input," \n");

			/* Count tokens*/
			while (args[num_args] != NULL)
			{
				/*printf ("%d: %s\n", num_args, args[num_args]);*/
				args[++num_args] = strtok (NULL, " \n");
			}

			if(num_args > 0 && execute(args)){
				break;
			}
		}else{ /* if EOF (CTRL+D) is sent, quit... */
			break;
		}
	}
	/* give back what we have taken. */
	free(args);
	free(checkEnv_path);
	free(stdout_buff);

	/* End of the world, kill all the children... */
	signal(SIGCHLD, SIG_IGN);
	kill(0, SIGKILL); /* Do not be nice. We want no zombies*/
	/* kill(0, SIGTERM); */

	return 0;
}