#define __FILEPATH__ "util/common/Test/tComplex.cc"

#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Complex.h"
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

  Complex<float> c1(1,2), c2(2,3), prod, div;

  std::cout << c1 << std::endl;
  std::cout << c2 << std::endl;

  prod = c1*c2;
  std::cout << prod << std::endl;

  div = c1/c2;
  std::cout << div << std::endl;
  
  return 0;
}
