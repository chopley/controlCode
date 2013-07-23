#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/SshTunnel.h"
#include "gcp/util/common/CoProc.h"

#include "gcp/mediator/specific/TipperSshClient.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::mediator;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "index", "0",  "i", "Index"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  gcp::mediator::TipperSshClient client(0);

  sleep(10);

  return 0;
}
