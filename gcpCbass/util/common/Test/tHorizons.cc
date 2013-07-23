#define __FILEPATH__ "util/common/Test/tAntNum.cc"

#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/HorizonsCommunicator.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "antenna",     "0",                        "i", "Antenna number"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  HorizonsCommunicator comm;

  comm.spawn();

  comm.registerEphemeris("mercury", "mercury.ephem");

  comm.blockForever();

  return 0;
}
