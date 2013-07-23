#define __FILEPATH__ "util/common/Test/tIoLock.cc"

#include <iostream>
#include <iomanip>
#include <cmath>

#include "gcp/util/common/Debug.h"

#include "gcp/program/common/Program.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

unsigned testIoLock()
{
  DBPRINT(true, Debug::DEBUG1, "Another message");
  return 1;
}

int Program::main()
{
  Debug::setLevel(Debug::DEBUG1);

  DBPRINT(true, Debug::DEBUG1, "A message: " << testIoLock());
}
