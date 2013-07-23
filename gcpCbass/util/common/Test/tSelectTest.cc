#include <iostream>

#include <sys/select.h>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Connection.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/TimeOut.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

KeyTabEntry Program::keywords[] = {
  { "host",  "antps1.carma.pvt", "s", "Host"},
  { END_OF_KEYWORDS}
};

void Program::initializeUsage() {};

int Program::main()
{
  TimeOut timeOut;
  timeOut.setIntervalInSeconds(1);
  timeOut.activate(true);

  FdSet fdSet;
  fdSet.clear();

  int nready=select(fdSet.size(), fdSet.readFdSet(), NULL, NULL, timeOut.tVal());

  Connection conn;

  COUT("Is Host " << Program::getParameter("host") << " reachable: " << conn.isReachable(Program::getParameter("host")));

  return 0;
}
