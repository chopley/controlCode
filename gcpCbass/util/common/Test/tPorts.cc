#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Ports.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "exp", "cbass",  "s", "Experiment name"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  std::string expName = Program::getParameter("exp");

  COUT("Listing ports for experiment: " << expName);
  COUT("Control program control port: " << Ports::controlControlPort(expName));
  COUT("Control program monitor port: " << Ports::controlMonitorPort(expName));
  COUT("Control program image port:   " << Ports::controlImMonitorPort(expName));

  return 0;
}
