#define __FILEPATH__ "antenna/control/Test/tAbsTimer.cc"

#include <iostream>

#include "carma/util/Program.h"

#include "gcp/util/common/AbsTimer.h"

using namespace std;
using namespace gcp::util;

int carma::util::Program::main()
{
  AbsTimer timer1(35);
  timer1.checkTimer();
  AbsTimer timer2(36);
  timer1.checkTimer();
  AbsTimer timer3(37);
  timer1.checkTimer();
}
