#define __FILEPATH__ "misc/emacs/incrhex.cc"

#include <stdio.h>
#include <stdlib.h>

// Emacs function to read a hex string specifying a byte address, and
// an integer number of floats, and output a hex string which is the
// addition of the two, in bytes.

int main(int argc, char* argv[])
{
  if(argc != 3) {
    fprintf(stdout, "Usage: incrhex address nfloat.\n");
    return 1;
  }

  int addr;

  sscanf(argv[1],"%x",&addr);

  int nfloat = atoi(argv[2]);

  fprintf(stdout,"0x%X", addr+nfloat*4);
  
  return 0;
}
