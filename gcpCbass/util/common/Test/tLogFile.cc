#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/LogFile.h"
#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  {END_OF_KEYWORDS}
};

int Program::main()
{
  LogFile logFile;

  logFile.setDirectory("logTest");
  //  logFile.setDatePrefix();
  logFile.setPrefix("myFile");

  for(unsigned i=0; i < 10; i++) {
    COUT(logFile.newFileName());
    sleep(1);
  }

  return 0;
}
