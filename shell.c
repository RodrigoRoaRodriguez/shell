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

#define MAX_CHARS 80

char * checkEnv_path;
char spawn_msg;
int num_args;
int child_pid;

void cleanup(int);



/*
Here follows three functions that handles the timer we used.
the code inside the functions is borrowed from:
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

void forker(char **argv)
{
	int status;
	char bg = 0;
	
	/* Check whether the last argument is a & 
	   if that is the case, do not send it to the 
	   process to be forked. */
	if(0 == strcmp(argv[num_args-1],"&"))
	{
		bg = 1;
		argv[num_args-1] = NULL;
	}

	start_timer();
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
		signal(SIGINT, cleanup);
		waitpid(child_pid, &status, 0); 
		stop_timer();
		printf("[%d] terminated with return value: %d after %.2f ms of execution.\n", child_pid, status, get_time());
	}else{
		spawn_msg = 1;
	}

	/* waitpid(-1, &status, 0);*/
}

int execute(char **argv){
	if(0 == strcmp(argv[0], "exit")){
		/* TODO SIGQUIT, */
		return 1;
	}else if(0 == strcmp(argv[0], "checkEnv")){
		int status;
		start_timer();
		child_pid = fork();

		if ( 0 == child_pid ){
			execl(checkEnv_path, checkEnv_path, argv[1]);
		}

		/* Check whether the checkEnv was to be run in bg
		   if that is not the case, wait until terminated */
		if(0 != strcmp(argv[num_args-1],"&")) { 
			waitpid(child_pid, &status, 0); 
			stop_timer();
			printf("[%d] terminated with return value: %d after %.2f ms of execution.\n", child_pid, status, get_time());
		}else{
			spawn_msg = 1;
		}

	}else if(0 == strcmp(argv[0], "cd")){

		if(0 == strcmp(argv[1], "~")){
			if(0 != chdir(getenv("PWD"))){
				printf("shell: dude, you're in serious trouble, working directory was set to: %s\n", getenv("PWD"));
			}
		}else{
			char * new_path = malloc(strlen(get_current_dir_name())+strlen(argv[1])+1);
			strcpy (new_path, get_current_dir_name());
			strcpy (new_path+strlen(get_current_dir_name()), "/");
			strcpy (new_path+strlen(get_current_dir_name())+1, argv[1]);

			if(0 != chdir(new_path)){
				printf("shell: cd: %s: No such file or directory\n", argv[1]);
			}

			free(new_path);
		}
	}else{
		forker(argv);
	}
	return 0;
}

void handler(int dummy){
	int status;
	wait(&status);
	printf("\nA child just terminated with return value: %d\n", status);
}

/*
 * cleanup: to be executed by parent when user types CTRL-C.
 */
void cleanup(int dummy)
{
	  printf("\nPARENT %d got SIGINT. Kill child %d\n\n", getpid(), child_pid);
	  kill(child_pid, SIGKILL);                   /* Kill child process */
}

#if 0==SIGDET
void polling_procedure(){
	int status, pid;
	do{
		pid = waitpid(0, &status, WNOHANG);
		if(pid>0) printf("[%d] terminated with return value: %d\n", pid, status);
	}while(pid>0);
}
#endif

int main(int argc, char **argv)
{
	char input [MAX_CHARS];
	char ** args;
	spawn_msg = 0;

	/* Set path to checkEnv */
	checkEnv_path = malloc(strlen(getenv("PWD"))+strlen("/checkEnv"));
	strcpy (checkEnv_path, getenv("PWD"));
	strcpy (checkEnv_path+strlen(getenv("PWD")), "/checkEnv");

	/*signal(SIGCHLD, handler);*/

	while(1){
		signal(SIGINT, SIG_IGN);
		/* if we have spawned any bg processes, inform user about them. */
		if(spawn_msg){
			printf("CHILD with PID = %d spawned\n", child_pid);
			spawn_msg = 0;
		}

		#if 0==SIGDET
		polling_procedure();
		#endif
	
		printf("%s# ", get_current_dir_name());
		num_args = 0;
		args = malloc((sizeof(char *) * 41));
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
		}
	}
	free(args);
	free(checkEnv_path);


	signal(SIGCHLD, SIG_IGN);
	kill(0, SIGKILL); /* Do not be nice. We want no zombies*/
	/* kill(0, SIGTERM); */

	return 0;
}

/*
#if SIGDET
printf("HEJ\n");
#endif
*/