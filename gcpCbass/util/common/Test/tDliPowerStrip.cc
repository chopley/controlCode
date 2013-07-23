#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/DliPowerStrip.h"
#include "gcp/util/common/Exception.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "host",   "powercbassdaq.cm.pvt", "s", "Power strip IP"},
  { "outlet", "1", "i", "Outlet"},
  { "on",     "t", "b", "On/Off"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  DliPowerStrip strip(Program::getParameter("host"));
  
  unsigned outlet = Program::getiParameter("outlet");

#if(1)
  if(Program::getbParameter("on")) {
    if(outlet==0)
      strip.allOn();
    else
      strip.on(outlet);
  } else {
    if(outlet==0)
      strip.allOff();
    else
      strip.off(outlet);
  }

  #endif

 std::vector<DliPowerStrip::State> state = strip.queryStatus();

 for(unsigned i=0; i < DliPowerStrip::MAX_OUTLETS; i++) {
   COUT("Outlet " << (i+1) << ": " << (state[i]==DliPowerStrip::ON ? "ON" : "OFF"));
 }

  return 0;
}
