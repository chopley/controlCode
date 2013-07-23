#define __FILEPATH__ "util/common/Test/tAntNum.cc"

#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Profiler.h"
#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "antenna",     "0",                        "i", "Antenna number"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  Profiler prof;

  prof.start();
  prof.stop();

  COUT("Time elapsed: " << prof.diff().getTimeInSeconds());

  return 0;
}

