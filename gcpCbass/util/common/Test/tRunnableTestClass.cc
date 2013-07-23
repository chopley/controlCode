#define __FILEPATH__ "util/common/Test/tRunnableTestClass.cc"

#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/RunnableTestClass.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));

  RunnableTestClass test2(true);

  test2.blockForever();

  std::cout << "And now exiting..." << std::endl;
}
