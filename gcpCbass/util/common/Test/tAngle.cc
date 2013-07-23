#define __FILEPATH__ "util/common/Test/tAntNum.cc"

#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Angle.h"
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
  Angle ang1, ang2;
  Angle sum;

  ang1.setDegrees(45);
  ang2.setDegrees(55);

  sum = ang1 + ang2;

  COUT("sum = " << sum);

  return 0;
}
