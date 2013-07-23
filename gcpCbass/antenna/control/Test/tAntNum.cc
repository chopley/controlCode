#define __FILEPATH__ "antenna/control/Test/tAntNum.cc"

#include <iostream>

#include "carma/util/Program.h"

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;

int carma::util::Program::main()
{
  Debug::setDebug(false);

  AntNum ant1(AntNum::ANTALL);
  AntNum ant2(AntNum::ANT0+AntNum::ANT1);
  return 0;
}
