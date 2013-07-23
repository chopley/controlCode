#define __FILEPATH__ "util/common/Test/tLogStream.cc"

#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Logger.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

LOG_HANDLER_FN(logHandler) 
{
  cout << "From logger handler: " << logStr << endl;
}

LOG_HANDLER_FN(errHandler) 
{
  cerr << "From error handler: " << logStr << endl;
}

KeyTabEntry Program::keywords[] = {
  {END_OF_KEYWORDS}
};

int Program::main()
{
  LogStream logStr;
  LogStream errStr;

  errStr << "A test";
  errStr << "...And another";
  errStr.report();

  logStr << "Just a message to be logged";
  logStr.log();

  gcp::util::Logger::installLogHandler(logHandler);
  gcp::util::Logger::installErrHandler(errHandler);

  logStr.report();
  errStr.report();

  gcp::util::Logger::setPrefix("ANT0: ");

  logStr.report();
  errStr.report();

  ReportMessage("this is a test");

  return 0;
}
