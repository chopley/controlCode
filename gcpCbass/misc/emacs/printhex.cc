#define __FILEPATH__ "misc/emacs/printhex.cc"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
  if(argc != 2) {
    fprintf(stdout, "Usage: printhex int\n");
    return 1;
  }

  int ivar = atoi(argv[1]);

  fprintf(stdout,"0x%X", ivar);
  
  return 0;
}
