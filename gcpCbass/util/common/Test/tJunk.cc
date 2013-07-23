#include <iostream>
#include <sstream>
#include <cmath>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/WxServer40m.h"

#include "gcp/program/common/Program.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  {END_OF_KEYWORDS}
};

void Program::initializeUsage() {}

int Program::main()
{
  COUT("+3.5 = " << atof("+3.5"));
  COUT("+03.5 = " << atof("+03.5"));
  COUT("03.5 = " << atof("03.5"));
  COUT("-03.5 = " << atof("-03.5"));
  COUT("-3.5 = " << atof("-3.5"));
}

