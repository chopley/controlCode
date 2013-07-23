#define __FILEPATH__ "util/common/Test/tRegParser.cc"

#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/RegDescription.h"
#include "gcp/util/common/RegParser.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "reg",   "",  "s", "Register name to parse"},
  { "debug", "t", "b", "Debug"},
  { END_OF_KEYWORDS}
};

void doIt(std::string reg, bool debug);

int Program::main()
{
  while(true) {
    //    CTOUT("ABOUT to do it");
    doIt(Program::getParameter("reg"), Program::getbParameter("debug"));
  }

  return 0;
}

void doIt(std::string reg, bool debug)
{
  std::string regSpec(reg);
  static gcp::util::RegParser parser(false);
  gcp::util::RegDescription regDesc = parser.inputReg(regSpec);    

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 10;

  nanosleep(&ts, NULL);
}
