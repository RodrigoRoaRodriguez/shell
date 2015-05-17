#include <stdio.h>

int main(){
  int tkn;
  while((tkn = getchar()) != EOF)
    putchar(tkn == 'X' ? '#': tkn);
}
