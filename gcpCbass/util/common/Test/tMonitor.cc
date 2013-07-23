#define __FILEPATH__ "util/common/Test/tMonitor.cc"

#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Monitor.h"
#include "gcp/util/common/MonitorDataType.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "arcdir",     "/data/gcpdaq/arc",    "s", "Archive directory"},
  { "calfile",    CALFILE, "s", "Cal file"},
  { "host",       HOST,    "s", "ACC control host"},
  { "start",      "05-may-2005",      "s", "The start UTC (dd-mmm-yy:hh:mm:ss)"},
  { "stop",       "05-may-2005:01",      "s", "The stop  UTC (dd-mmm-yy:hh:mm:ss)"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

/**.......................................................................
 * There are several possible sets of inputs.  We will treat them as
 * follows:
 *
 * 1) If a stop mjd is given, we will read from the archive until that
 * stop date.  If no start is given, we will read from the beginning
 * of the archive to the stop date.
 *
 * 2) If no - stop is given, we will read from the start date, if any,
 * then connect to the real-time controller.  If no start date is
 * given, we will simply connect to the real-time controller.
 */
int Program::main()
{
  Monitor monitor("/data/sptdaq/arc",
		  "/dev/null",
		  "23-jan-2008:0",
		  "23-jan-2008:01");
		  
  
  monitor.addRegister("array.frame.utc double");

#if 0
  Monitor monitor(Program::getParameter("arcdir"),
		  Program::getParameter("calfile"),
		  Program::getParameter("start"),
		  Program::getParameter("stop"));
  
  monitor.addRegister("antenna0.tracker.state");
  monitor.addRegister("antenna0.varactor.statusRegisterMask");

  std::vector<std::vector<std::vector<MonitorDataType> > > 
    type = monitor.readRegsAsDataTypes();

  std::cout << "Type has " << type.size() << " frames" << std::endl;
  std::cout << "Each frame contains " << type[0].size() << " registers" << std::endl;

  for(unsigned iReg=0; iReg < type[0].size(); iReg++)
    std::cout << "Register " << iReg << " comprises " << type[0][iReg].size() << " registers" << std::endl;

  for(unsigned iRow=0; iRow < type[0].size(); iRow++) {
    for(unsigned iCol=0; iCol < type[0][iRow].size(); iCol++) {
      type[0][iRow][iCol].print();
      std::cout << " ";
    }
    std::cout << std::endl;
  }
#endif

  return 0;
}
