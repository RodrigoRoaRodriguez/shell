#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


int main(int argc, char **argv)
{
  char mystring [100];

  if(fgets ( mystring, 100, stdin) != NULL)
    puts(mystring);

}
