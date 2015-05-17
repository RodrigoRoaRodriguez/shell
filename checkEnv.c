#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

#define READ 0
#define WRITE 1

int main()
{
  pid_t pidPrintenv, pidFilter;
  int pipa[2], status;

  pipe(pipa);

  printf("pipa0 %d pip1 %d\n", pipa[READ], pipa[WRITE]);
  if((pidPrintenv = fork())== 0) //printenv process
  {
    dup2(pipa[WRITE], WRITE);
    close(pipa[WRITE]);
    execlp("printenv", "printenv", NULL);
    exit(0); // Close pipe so that results are printed
  }
  if((pidFilter = fork())==0)//filter process
  {
    dup2(pipa[READ], READ);
    close(pipa[WRITE]);
    close(pipa[READ]);
    execlp("./filter","./filter",NULL);

  }
  wait(&status);
  wait(&status);
}
