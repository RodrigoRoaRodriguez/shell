#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

#define READ 0
#define WRITE 1

int main()
{
  int pipa[2];
  int pid;

  pipe(pipa);

  printf("pipa0 %d pip1 %d\n", pipa[READ], pipa[WRITE]);

  // Divide this process into two.
  pid = fork();

  if( pid == 0) //printenv process
  {
    dup2(pipa[WRITE], WRITE);
    close(pipa[WRITE]);
    close(pipa[READ]);
    execlp("printenv", "printenv", NULL);
    exit(0); // Close pipe so that results are printed

  }else{ //filter process

    // Divide this process into two.
    pid = fork();

    if( pid == 0) //printenv process
    {

      dup2(pipa[WRITE], WRITE);
      close(pipa[WRITE]);
      close(pipa[READ]);
      execlp("sort", "sort", NULL);
      exit(0); // Close pipe so that results are printed

    }else{ //filter process

      close(pipa[WRITE]);
      dup2(pipa[READ], READ);
      close(pipa[READ]);
      execlp("less","less",NULL);
      exit(0);
    }

  }
}
