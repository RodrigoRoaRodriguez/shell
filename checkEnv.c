#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>

/* STDIN_FILENO */
#define READ 0
/* STDOUT_FILENO */
#define WRITE 1

/* PIPE INDEX */
#define PRINTENV_GREP 0
#define GREP_SORT 1
#define SORT_PAGER 2


/* a bool is a byte containing either 0 or 1
   names are given to those two values as follows:
   1 is called true, 0 is called false.
   */
#define bool unsigned char
#define true 1
#define false 0

int handle_error(int ret_value, char * error_msg, int exit_code){
  if(ret_value<0){
    perror(error_msg);
    exit(exit_code);
  }
  return ret_value;
}

int main(int argc, char **argv, char **envp)
{
  int pipa[3][2];
  int status, i;
  bool grep_vars = (argc > 1 ? true : false);

  handle_error(pipe(pipa[PRINTENV_GREP]), "Failed to create pipe", 1);
  handle_error(pipe(pipa[GREP_SORT]), "Failed to create pipe", 1);
  handle_error(pipe(pipa[SORT_PAGER]), "Failed to create pipe", 1);

  /* printenv */
  if( 0 == handle_error(fork(), "Failed to fork for printenv", 1) ) /* printenv process */
  {
    handle_error(close(pipa[PRINTENV_GREP][READ]), "Failed to close pipe", 1);

    handle_error(dup2(pipa[PRINTENV_GREP][WRITE], WRITE), "Failed to duplicate fd", 1);
    handle_error(close(pipa[PRINTENV_GREP][WRITE]), "Failed to close pipe", 1);

    execlp("printenv", "printenv", NULL);
    
    perror("THIS MEANS THAT WE FAILED TO EXECUTE PRINTENV");
    exit(1); /* error no */
  }

  /* grep */
  if(grep_vars && 0 == handle_error(fork(), "Failed to fork for grep", 1) ) /* grep process */
  {
    handle_error(close(pipa[PRINTENV_GREP][WRITE]), "Failed to close pipe", 1);
    handle_error(dup2(pipa[PRINTENV_GREP][READ], READ), "Failed to duplicate fd", 1);
    handle_error(close(pipa[PRINTENV_GREP][READ]), "Failed to close pipe", 1);

    handle_error(close(pipa[GREP_SORT][READ]), "Failed to close pipe", 1);
    handle_error(dup2(pipa[GREP_SORT][WRITE], WRITE), "Failed to duplicate fd.", 1);
    handle_error(close(pipa[GREP_SORT][WRITE]), "Failed to close pipe", 1);

    /* TODO DOUBLE CHECK IF WE ARE ALLOWED TO DO SO */
    argv[0] = "grep";
    execvp("grep", argv);

    perror("THIS MEANS THAT WE FAILED TO EXECUTE PRINTENV");
    exit(1); /* error no */
  }

  /* close pipes */
  if(grep_vars){
    handle_error(close(pipa[PRINTENV_GREP][READ]), "Failed to close pipe", 1);
    handle_error(close(pipa[PRINTENV_GREP][WRITE]), "Failed to close pipe", 1);
  }else{
    handle_error(close(pipa[GREP_SORT][WRITE]), "Failed to close pipe", 1);
    handle_error(close(pipa[GREP_SORT][READ]), "Failed to close pipe", 1);
    pipa[GREP_SORT][READ] = pipa[PRINTENV_GREP][READ];
    pipa[GREP_SORT][WRITE] = pipa[PRINTENV_GREP][WRITE];
  }

  /* sort */
  if( 0 == handle_error(fork(), "Failed to fork for sort", 1) ) /* sort process */
  {
    handle_error(close(pipa[GREP_SORT][WRITE]), "Failed to close pipe", 1);
    handle_error(dup2(pipa[GREP_SORT][READ], READ), "Failed to duplicate fd", 1);
    handle_error(close(pipa[GREP_SORT][READ]), "Failed to close pipe", 1);

    handle_error(close(pipa[SORT_PAGER][READ]), "Failed to close pipe", 1);
    handle_error(dup2(pipa[SORT_PAGER][WRITE], WRITE), "Failed to duplicate fd.", 1);
    handle_error(close(pipa[SORT_PAGER][WRITE]), "Failed to close pipe", 1);
    
    execlp("sort", "sort", NULL);

    perror("THIS MEANS THAT WE FAILED TO EXECUTE SORT");
    exit(1); /* error no */
  }

  /* close pipes */
  handle_error(close(pipa[GREP_SORT][WRITE]), "Failed to close pipe", 1);
  handle_error(close(pipa[GREP_SORT][READ]), "Failed to close pipe", 1);

  /* pager */
  if( 0 == handle_error(fork(), "Failed to fork for pager", 1) ) /* pager process */
  {
    char * pager;
    
    handle_error(close(pipa[SORT_PAGER][WRITE]), "Failed to close pipe", 1);
    handle_error(dup2(pipa[SORT_PAGER][READ], READ), "Failed to duplicate fd", 1);
    handle_error(close(pipa[SORT_PAGER][READ]), "Failed to close pipe", 1);
    
    if((pager = getenv("PAGER")) != NULL){
      execlp(pager, pager, NULL);
    }

    execlp("less", "less", NULL);
    execlp("more", "more", NULL);

    perror("THIS MEANS THAT WE FAILED TO EXECUTE ANY PAGER");
    exit(1); /* error no */
  } 

  /* close pipes */
  handle_error(close(pipa[SORT_PAGER][WRITE]), "Failed to close pipe", 1);
  handle_error(close(pipa[SORT_PAGER][READ]), "Failed to close pipe", 1);

  /* Wait for childs */
  handle_error(wait(&status), "Failed to wait for printenv process", 1);
  if(grep_vars) handle_error(wait(&status), "Failed to wait for grep process", 1);
  handle_error(wait(&status), "Failed to wait for sort process", 1);
  handle_error(wait(&status), "Failed to wait for pager process", 1);
}
