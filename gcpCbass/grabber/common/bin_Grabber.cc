#define __FILEPATH__ "grabber/grabber.cc"

#include <iomanip>
#include <unistd.h>
#include <math.h>
#include <cstring>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Directives.h"

#include "gcp/grabber/common/Master.h"

using namespace std;
using namespace gcp::grabber;
using namespace gcp::util;

#include "gcp/program/common/Program.h"

using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "ctlhost",      HOST,                     "s",  "Host on which the mediator is running"},
  { "imhost",       HOST,                     "s",  "Host on which the control program is running"},
  { "channel",     "1",                       "i",  "Video input channel"},
  { "simulate",    "f",                       "b",  "Simulate grabber hardware?"},
  {END_OF_KEYWORDS}
};

void Program::initializeUsage(void) {}; 

int Program::main(void) 
{
  // Just instantiate a servant

  Master grabber(Program::getParameter("ctlhost"), 
		 Program::getParameter("imhost"), 
		 Program::getbParameter("simulate"),
		 Program::getiParameter("channel"));

  return 0;
};
