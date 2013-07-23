#define __FILEPATH__ "util/common/command.cc"

#include <iomanip>
#include <unistd.h>
#include <math.h>
#include <cstring>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#include "gcp/util/common/Control.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Directives.h"

#include "gcp/program/common/Program.h"

using namespace gcp::util;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "host", HOST, "s", "ACC control host"},
  { "log",   "f", "b", "Log messages from the control system? (t/f)"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

void Program::initializeUsage() {};
 
int Program::main(void) 
{
  Control control(Program::getParameter("host"),
		  Program::getbParameter("log"));
}
