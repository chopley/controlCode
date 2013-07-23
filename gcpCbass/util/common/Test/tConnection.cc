#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Connection.h"
#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

KeyTabEntry Program::keywords[] = {
  { "host",  "antps1.carma.pvt", "s", "Host"},
  { END_OF_KEYWORDS}
};

void Program::initializeUsage() {};

int Program::main()
{
  Connection conn;

  COUT("Is host " << Program::getParameter("host") << " reachable: " << conn.isReachable(Program::getParameter("host")));

  return 0;
}
