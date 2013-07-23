#define __FILEPATH__ "antenna/control/Test/tSubArray.cc"

#include <iostream>

#include "carma/util/Program.h"

#include "gcp/util/common/SubArray.h"

using namespace std;
using namespace gcp::util;

int carma::util::Program::main()
{
  AntNum ant1(AntNum::ANTALL);
  AntNum ant2(AntNum::ANT0+AntNum::ANT1);

  SubArray sub1("sub1", ant1);
  sub1.printSubArrays();
  SubArray sub2("sub2", ant2);
  sub2.printSubArrays();
  SubArray sub3("sub2", ant2);
  return 0;
}
