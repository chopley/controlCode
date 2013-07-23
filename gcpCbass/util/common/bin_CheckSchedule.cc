#include <iostream>
#include <sstream>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Directives.h"
#include "gcp/util/common/SpecificName.h"

#include "gcp/program/common/Program.h"

#include "gcp/control/code/unix/control_src/common/controlscript.h"
#include "gcp/control/code/unix/control_src/common/genericcontrol.h"
#include "gcp/control/code/unix/control_src/common/genericscheduler.h"
#include "gcp/control/code/unix/control_src/common/navigator.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::control;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "sch",                         "", "s", "Schedule file to check"},
  { "srcdir",  GCPTOP"/control/ephem", "s", "Top level gcp directory"},
  { "srccat",            "source.cat", "s", "Source catalog to load"},
  { "scandir",  GCPTOP"/control/scan", "s", "Top level gcp directory"},
  { "scancat",             "scan.cat", "s", "Scan catalog to load"},
  { END_OF_KEYWORDS},
};

void Program::initializeUsage() 
{
  std::ostringstream os;
  os << "Usage: " << gcp::util::SpecificName::experimentName() 
     << "CheckSchedule sch=\"filename\" srcdir=\"dirname\" srccat=\"filename\" scandir=\"dirname\" scancat=\"filename\"";

  usage = os.str();
};

int Program::main()
{
  // Set the required environment variable if not already set

  setenv("GCP_DIR", GCPTOP, true);

  if(Program::isDefault("sch")) {
    COUT("You must specify the schedule to check");
    return 1;
  } else {

    ControlProg* cp = new_ControlProg(true, true);

    // Now load the source catalog

    readSourceCatalog(cp, Program::getParameter("srcdir"),  Program::getParameter("srccat"));
    readScanCatalog(cp,   Program::getParameter("scandir"), Program::getParameter("scancat"));

    // Now execute the check schedule command

    Scheduler *sch = (Scheduler* )cp_ThreadData(cp, CP_SCHEDULER);
    
    std::stringstream os;
    os << "check_schedule " << Program::getParameter("sch").c_str();
    
    COUT("");

    sch_execute_command(sch, (char*)os.str().c_str());
    
    if(cp) {
      cp = del_ControlProg(cp);
    }

    COUT("");
  }

  return 0;
}

