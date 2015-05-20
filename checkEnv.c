/**********************************************************
*
* Course: Operating Systems (ID2200)
* Authors: Bassam Alfarhan
*          Rodrigo Roa Rodríguez
*
***********************************************************
* This is an implementation of a command named checkEnv
* which demonstrates how pipes works in c.
* What it does is simply:
* printenv | grep <args> | sort | pager
* where pager is what is defined in the environment 
* variables, if none is defined less or more is tried
* (in order), if none of them is available an error 
* message is printed out. 
**********************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

/* File descriptors */
#define READ 0 /* STDIN */
#define WRITE 1 /* STDOUT */

/* PIPE INDEX */
#define PRINTENV_GREP 0
#define GREP_SORT 1
#define SORT_PAGER 2

/* checks the return value of a system call, 
 * if it is unsuccessful, print out the provided error 
 * message and terminate the program. */
int handle_error(int ret_value, char * error_msg){
  if(ret_value<0){
    perror(error_msg);
    exit(1);
  }
  return ret_value;
}

/* a function that prepares a pipe for input */
void pipe_in(int * pipa) {
  handle_error(close(pipa[WRITE]), "Failed to close incoming pipe for ");
  handle_error(dup2(pipa[READ], READ), "Failed to incoming duplicate fd for ");
  handle_error(close(pipa[READ]), "Failed to close incoming pipe for ");
}

/* a function that prepares a pipe for output */
void pipe_out(int * pipa) {
  handle_error(close(pipa[READ]), "Failed to close outgoing pipe for ");
  handle_error(dup2(pipa[WRITE], WRITE), "Failed to outgoing duplicate fd for ");
  handle_error(close(pipa[WRITE]), "Failed to close outgoing pipe for ");
}

int main(int argc, char **argv)
{
  int pipa[3][2];
  int status;
  char grep_vars = argc > 1; /* check whether we have args to be sent to grep */

  /* initialise the three pipes we need */
  handle_error(pipe(pipa[PRINTENV_GREP]), "Failed to create pipe");
  handle_error(pipe(pipa[GREP_SORT]), "Failed to create pipe");
  handle_error(pipe(pipa[SORT_PAGER]), "Failed to create pipe");

  /* execute printenv */
  if( 0 == handle_error(fork(), "Failed to fork for printenv") ) /* printenv process */
  {
    pipe_out(pipa[PRINTENV_GREP]);

    execlp("printenv", "printenv", NULL);

    perror("THIS MEANS THAT WE FAILED TO EXECUTE PRINTENV");
    exit(1); /* error no */
  }

  /* execute grep */
  if(grep_vars && 0 == handle_error(fork(), "Failed to fork for grep") ) /* grep process */
  {
    pipe_in(pipa[PRINTENV_GREP]);
    pipe_out(pipa[GREP_SORT]);

    execlp("grep", "grep", argv[1], NULL);

    perror("THIS MEANS THAT WE FAILED TO EXECUTE PRINTENV");
    exit(1); /* error no */
  }

  /* close special pipes */
  /* since grep is optional, we may sometimes end 
   * up with more pipes than we need, therefore, we
   * chain the pipes and close the ones in the middle, 
   * when that happens. */
  if(grep_vars){
    handle_error(close(pipa[PRINTENV_GREP][READ]), "Failed to close pipe");
    handle_error(close(pipa[PRINTENV_GREP][WRITE]), "Failed to close pipe");
  }else{
    handle_error(close(pipa[GREP_SORT][WRITE]), "Failed to close pipe");
    handle_error(close(pipa[GREP_SORT][READ]), "Failed to close pipe");
    pipa[GREP_SORT][READ] = pipa[PRINTENV_GREP][READ];
    pipa[GREP_SORT][WRITE] = pipa[PRINTENV_GREP][WRITE];
  }

  /* execute sort */
  if( 0 == handle_error(fork(), "Failed to fork for sort") ) /* sort process */
  {
    pipe_in(pipa[GREP_SORT]);
    pipe_out(pipa[SORT_PAGER]);

    execlp("sort", "sort", NULL);

    perror("THIS MEANS THAT WE FAILED TO EXECUTE SORT");
    exit(1); /* error no */
  }

  /* close pipes */
  handle_error(close(pipa[GREP_SORT][WRITE]), "Failed to close pipe");
  handle_error(close(pipa[GREP_SORT][READ]), "Failed to close pipe");

  /* execute pager */
  if( 0 == handle_error(fork(), "Failed to fork for pager") ) /* pager process */
  {
    char * pager;

    pipe_in(pipa[SORT_PAGER]);

    if((pager = getenv("PAGER")) != NULL){
      execlp(pager, pager, NULL);
    }

    execlp("less", "less", NULL);
    execlp("more", "more", NULL);

    perror("THIS MEANS THAT WE FAILED TO EXECUTE ANY PAGER");
    exit(1); /* error no */
  }

  /* close pipes */
  handle_error(close(pipa[SORT_PAGER][WRITE]), "Failed to close pipe");
  handle_error(close(pipa[SORT_PAGER][READ]), "Failed to close pipe");

  /* Wait for children */
  /* Note that the error messages reffers to the children without any guarantee. */
  /* This can be fixed by saving the childrens pids and checking for them implicitly */
  /* We decided not to, since we only needed an indication of how many children are missed */
  handle_error(wait(&status), "Failed to wait for printenv process");
  if(grep_vars) handle_error(wait(&status), "Failed to wait for grep process");
  handle_error(wait(&status), "Failed to wait for sort process");
  handle_error(wait(&status), "Failed to wait for pager process");

  return 0;
}
