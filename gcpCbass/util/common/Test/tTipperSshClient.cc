#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/TipperSshClient.h"
#include "gcp/util/common/SshTunnel.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "index", "0",  "i", "Index"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  TipperSshClient client(true);
  client.blockForever();
  return 0;
}
