/* pipetest.c - användning av pipe */
#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <unistd.h> /* definierar bland annat pipe() */
#define TRUE ( 1 ) /* den logiska konstanten TRUE */
#define MINNUM ( 999 ) /* minsta möjliga tal som genereras */
#define MAXNUM ( 99999 ) /* största möjliga tal som genereras */

pid_t child_pid; /* för child-processens PID vid fork() */

int main()
{
  int return_value; /* för returvärden från systemanrop */
  int pipe_filedesc[2]; /* för fildeskriptorer från pipe(2) */
  fprintf( stderr, "Parent (producer, pid %ld) started\n", (long int) getpid() );

  return_value = pipe( pipe_filedesc ); /* skapa en pipe */
  if( -1 == return_value ) /* avsluta programmet om pipe() misslyckas */
  { perror( "Cannot create pipe" ); exit( 1 ); }
  child_pid = fork(); /* skapa en child-process */
  if( 0 == child_pid )
  {
    /* denna kod körs endast i child-processen (consumer) */
    int tal; /* för tal som läses från parent (producer) */
    fprintf( stderr, "Child (consumer, pid %ld) started\n",
    (long int) getpid() );
    /* stäng skrivsidan av pipen -- consumer ska bara läsa */
    return_value = close( pipe_filedesc[ 1 ] );
    if( -1 == return_value )
    { perror( "Cannot close pipe" ); exit( 1 ); }
    while( 1 )
    {
      return_value = read( pipe_filedesc[ 0 ], &tal, sizeof( tal ) );
      if( -1 == return_value ) /* read() misslyckades */
      { perror( "Cannot read from pipe" ); exit( 1 ); }
      if( 0 == return_value ) /* end of file */
      { fprintf( stderr, "End of pipe, child exiting\n" ); exit( 0 ); }
      if( sizeof( tal ) == return_value )
      {
        /* dela upp talet i primfaktorer och skriv ut dem */
        int f = 2, p = 0, inc[ 11 ] = { 1, 2, 2, 4, 2, 4, 2, 4, 6, 2, 6 };
        printf( "Consumer mottog talet %d, primfaktorer ", tal );
        while( f * f <= tal )
        if( 0 != tal % f ) { f = f + inc[ p++ ]; if( p > 10 ) p = 3; }
        else { printf( "%d ", f ); tal = tal / f; };
        printf( "%d\n", tal );
      }
      else
      {
        /* Unexpected byte count -- inform user and continue */
        fprintf( stderr, "Child wanted %d bytes but read only %d bytes\n",
        sizeof( tal ), return_value );
      }
    }
  }
  else
  {
    /* denna kod körs endast i parent-processen */
    if( -1 == child_pid ) /* fork() misslyckades */
    { perror( "fork() failed" ); exit( 1 ); }
    /*
    Kommer vi hit i koden så är vi i parent-processen
    och fork() har fungerat bra
    */
    /* stäng läs-änden av pipen -- parent ska bara skriva */
    return_value = close( pipe_filedesc[ 0 ] );
    if( -1 == return_value )
    { perror( "Parent cannot close read end" ); exit( 1 ); }
    while( TRUE )
    {
      /* slumpa fram nya lagom stora tal, om och om igen */
      /* " + 0.0 " tvingar fram flyttalsberäkning i stället för heltal */
      double tmp = ( rand() + 0.0 ) / ( RAND_MAX + 0.0 );
      /* tmp har nu ett värde mellan 0 och 1 */
      int generated_number = MINNUM + tmp * ( MAXNUM - MINNUM + 0.0 );

      printf( "Producer skapade talet %d\n", generated_number );

      return_value = write( pipe_filedesc[ 1 ], &generated_number, sizeof( generated_number ) );
        if( -1 == return_value )
        { perror( "Cannot write to pipe" ); exit( 1 ); }
        sleep( 1 ); /* ta det lugnt en stund */
      }
    }
  }
