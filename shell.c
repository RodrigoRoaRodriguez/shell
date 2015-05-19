#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define MAX_CHARS 80

char * checkEnv_path;
char * current_path;
int num_args;

void forker(char **argv)
{
	int status;
	int pid = fork();

	if ( pid == 0 )
	{
		/* Only run in child-process */
		execvp(argv[0], argv);
		printf("%s: command not found\n", argv[0]);
	}

	if(0 != strcmp(argv[num_args-1],"&"))
		wait(&status);
	/* waitpid(-1, &status, 0);*/
}

int execute(char **argv){
	if(0 == strcmp(argv[0], "exit")){
		/* TODO SIGQUIT, */
		return 1;
	}else if(0 == strcmp(argv[0], "checkEnv")){ 
		int status;
		int pid = fork();

		if ( pid == 0 )
			execl(checkEnv_path, checkEnv_path, argv[1]);

		wait(&status);
	}else if(0 == strcmp(argv[0], "cd")){

		if(0 == strcmp(argv[1], "~")){
			if(0 != chdir(getenv("PWD"))){
				printf("shell: dude, you're in serious trouble, home was set to: %s\n", getenv("PWD"));
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

int main(int argc, char **argv)
{
	char input [MAX_CHARS];

	char ** args;

	/* Set path to checkEnv */
	checkEnv_path = malloc(strlen(getenv("PWD"))+strlen("/checkEnv"));
	strcpy (checkEnv_path, getenv("PWD"));
	strcpy (checkEnv_path+strlen(getenv("PWD")), "/checkEnv");

/*
	current_path = malloc(strlen(getenv("PWD")));
	strcpy (current_path, getenv("PWD"));
*/

	signal(SIGTERM, SIG_IGN);

	while(123){
		printf("%s# ", get_current_dir_name());
		num_args = 0;
		if(fgets ( input, MAX_CHARS, stdin) != NULL){
			args = malloc((sizeof(char *) * 41));

			args[0] = strtok (input," \n");

			/* Count tokens*/
			while (args[num_args] != NULL)
			{
				/*printf ("%d: %s\n", num_args, args[num_args]);*/
				args[++num_args] = strtok (NULL, " \n");
			}

			if(num_args > 0 && execute(args)){
				free(args);
				break;
			}else{
				free(args);
			}
		}
	}
	free(checkEnv_path);

	kill(getpid(), SIGTERM);

	return 0;
}
