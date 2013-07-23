#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/SshTunnel.h"
#include "gcp/util/common/Ports.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "gateway",        "(none)", "s", "The gateway computer, if any"},
  { "host",           "",       "s", "The host computer"},
  { "localisstable",  "t",      "b", "The local stable ports should be forwarded"},
  { "remoteisstable", "t",      "b", "Local ports should be forwarded to the stable ports on the remote side"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  std::string host, gateway;

  bool localStableSpecified, remoteStableSpecified;

  unsigned short localControlPort,   remoteControlPort;
  unsigned short localMonitorPort,   remoteMonitorPort;
  unsigned short localImMonitorPort, remoteImMonitorPort;

  // Force the user to specify the remote host

  if(!Program::hasValue("host"))
    ThrowError("You must specify the host");

  host = Program::getParameter("host"); 

  // Check if the gateway was specified.  If not, just default to the
  // host

  if(Program::hasValue("gateway")) 
    gateway = host;
  else
    gateway = Program::getParameter("gateway"); 

  // Check if the local version was specified

  localStableSpecified  = Program::hasValue("localisstable"); 

  // Check if the remote version was specified

  remoteStableSpecified = Program::hasValue("remoteisstable"); 

  if(localStableSpecified) {
    localControlPort   = Ports::controlControlPort(Program::getbParameter("localisstable"));
    localMonitorPort   = Ports::controlMonitorPort(Program::getbParameter("localisstable"));
    localImMonitorPort = Ports::controlImMonitorPort(Program::getbParameter("localisstable"));
  } else {
    localControlPort   = Ports::controlControlPort();
    localMonitorPort   = Ports::controlMonitorPort();
    localImMonitorPort = Ports::controlImMonitorPort();
  }

  if(remoteStableSpecified) {
    COUT("Getting remote parameters for: " << Program::getbParameter("remoteisstable"));
    remoteControlPort   = Ports::controlControlPort(Program::getbParameter("remoteisstable"));
    remoteMonitorPort   = Ports::controlMonitorPort(Program::getbParameter("remoteisstable"));
    remoteImMonitorPort = Ports::controlImMonitorPort(Program::getbParameter("remoteisstable"));
  } else {
    remoteControlPort   = Ports::controlControlPort();
    remoteMonitorPort   = Ports::controlMonitorPort();
    remoteImMonitorPort = Ports::controlImMonitorPort();
  }

  // Now instantiate the tunnels

  SshTunnel control(  gateway, host, localControlPort,   remoteControlPort);
  SshTunnel monitor(  gateway, host, localMonitorPort,   remoteMonitorPort);
  SshTunnel imMonitor(gateway, host, localImMonitorPort, remoteImMonitorPort);

  // block forever

  select(1, NULL, NULL, NULL, NULL);
 
  return 0;
}
