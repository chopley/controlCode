#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Ports.h"

#include <string.h>

using namespace std;

using namespace gcp::util;

unsigned Ports::base(bool stable)
{
  if(stable)
    return 5000;
  else
    return 6000;
}

unsigned Ports::tipperPort(std::string exp)
{
  return base(true) + expOffset(exp) + TIPPER_SERVER_PORT_BASE;
}

unsigned Ports::wxPort(std::string exp)
{
  return base(true) + expOffset(exp) + WX_SERVER_PORT_BASE;
}

unsigned Ports::controlControlPort(bool stable)
{
  return base(stable) + expOffset() + CP_CONTROL_PORT_BASE;
}

unsigned Ports::controlControlPort(std::string exp)
{
  return base() + expOffset(exp) + CP_CONTROL_PORT_BASE;
}

unsigned Ports::controlMonitorPort(bool stable)
{
  return base(stable) + expOffset() + CP_MONITOR_PORT_BASE;
}

unsigned Ports::controlMonitorPort(std::string exp)
{
  return base() + expOffset(exp) + CP_MONITOR_PORT_BASE;
}

unsigned Ports::controlImMonitorPort(bool stable)
{
  return base(stable) + expOffset() + CP_IM_MONITOR_PORT_BASE;
}

unsigned Ports::controlImMonitorPort(std::string exp)
{
  return base() + expOffset(exp) + CP_IM_MONITOR_PORT_BASE;
}

unsigned Ports::expOffset()
{
  std::string exp(SPECIFIC);
  return expOffset(exp);
}

unsigned Ports::expOffset(std::string exp)
{
  if(strcmp(exp.c_str(),        "gcp")==0)
    return 0;
  else if(strcmp(exp.c_str(), "bicep")==0)
    return 20;
  else if(strcmp(exp.c_str(),   "spt")==0)
    return 40;
  else if(strcmp(exp.c_str(), "quiet")==0)
    return 60;
  else if(strcmp(exp.c_str(), "cbass")==0)
    return 80;
  else
    return 100;
}
