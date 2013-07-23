#include <iostream>
#include <sstream>
#include <cmath>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/WxServerSA.h"

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
  WxServerSA wx(true, Ports::wxPort("cbass"));

  wx.spawn();

  wx.blockForever();

  return 0;
}

