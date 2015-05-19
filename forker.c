#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>



int main(int argc, char **argv)
{
	int status;
	int pid = fork();

	if ( pid == 0 )
	{
		/* Denna kod körs endast i child-processen */
		puts("This is child");

	}
	else
	{
		
		puts("This is partent");
	/* Denna kod körs endast i parent-processen */
		if( pid == -1 )
		{
		/* ERROR */
			exit(1);
		}
		else
		{
		/* Denna kod körs i parent och endast om fork lyckades */
			puts("Successful fork");
		}
	}
	puts("This is below both.");
	wait(&status);
	
	return 0;
}