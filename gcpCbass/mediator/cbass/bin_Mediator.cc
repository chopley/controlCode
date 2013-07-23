
#define __FILEPATH__ "mediator/specific/mediator.cc"

#include <iomanip>
#include <unistd.h>
#include <math.h>
#include <cstring>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Logger.h"
#include "gcp/util/common/SshTunnel.h"

#include "gcp/util/specific/Directives.h"

#include "gcp/mediator/specific/Master.h"

using namespace std;
using namespace gcp::mediator;
using namespace gcp::util;

#include "gcp/program/common/Program.h"
 
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "ctlhost",     HOST,       "s",  "Control host"},
  { "wxhost",      HOST,       "s",  "Weather station server host"},
  { "sim",         "f",        "b",  "Simulate data sources"},
  { "simRx",       "f",        "b",  "Simulate Receiver"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

void Program::initializeUsage() {};

int Program::main(void) 
{
  // Just instantiate the master

  Master mediator(Program::getParameter("ctlhost"),
		  Program::getParameter("wxhost"),
		  Program::getbParameter("sim"),
		  Program::getbParameter("simRx"));

  return 0;
};
